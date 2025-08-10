// Copyright (c) Wojciech Figat. All rights reserved.

#include "Particles.h"
#include "ParticleEffect.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUPass.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Threading/TaskGraph.h"
#if COMPILE_WITH_GPU_PARTICLES
#include "Engine/Threading/Threading.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Profiler/ProfilerGPU.h"
#include "Engine/Renderer/Utils/BitonicSort.h"
#endif
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

PACK_STRUCT(struct SpriteParticleVertex
    {
    float X;
    float Y;
    float U;
    float V;
    });

class SpriteParticleRenderer
{
public:
    volatile int64 Ready = 0;
    GPUBuffer* VB = nullptr;
    GPUBuffer* IB = nullptr;
    const static int32 VertexCount = 4;
    const static int32 IndexCount = 6;

public:
    bool Init()
    {
        if (Platform::AtomicRead(&Ready))
            return false;
        ScopeLock lock(RenderContext::GPULocker);
        if (Platform::AtomicRead(&Ready))
            return false;
        VB = GPUDevice::Instance->CreateBuffer(TEXT("SpriteParticleRenderer.VB"));
        IB = GPUDevice::Instance->CreateBuffer(TEXT("SpriteParticleRenderer.IB"));
        SpriteParticleVertex vertexBuffer[] =
        {
            { -0.5f, -0.5f, 0.0f, 0.0f },
            { +0.5f, -0.5f, 1.0f, 0.0f },
            { +0.5f, +0.5f, 1.0f, 1.0f },
            { -0.5f, +0.5f, 0.0f, 1.0f },
        };
        uint16 indexBuffer[] = { 0, 1, 2, 0, 2, 3, };
        auto layout = GPUVertexLayout::Get({
            { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32_Float },
            { VertexElement::Types::TexCoord, 0, 0, 0, PixelFormat::R32G32_Float },
        });
        bool result = VB->Init(GPUBufferDescription::Vertex(layout, sizeof(SpriteParticleVertex), VertexCount, vertexBuffer)) ||
                      IB->Init(GPUBufferDescription::Index(sizeof(uint16), IndexCount, indexBuffer));
        Platform::AtomicStore(&Ready, 1);
        return result;
    }

    void Dispose()
    {
        SAFE_DELETE_GPU_RESOURCE(VB);
        SAFE_DELETE_GPU_RESOURCE(IB);
    }

    void SetupDrawCall(DrawCall& drawCall) const
    {
        drawCall.Geometry.IndexBuffer = IB;
        drawCall.Geometry.VertexBuffers[0] = VB;
        drawCall.Geometry.VertexBuffers[1] = nullptr;
        drawCall.Geometry.VertexBuffers[2] = nullptr;
        drawCall.Geometry.VertexBuffersOffsets[0] = 0;
        drawCall.Geometry.VertexBuffersOffsets[1] = 0;
        drawCall.Geometry.VertexBuffersOffsets[2] = 0;
        drawCall.Draw.StartIndex = 0;
        drawCall.Draw.IndicesCount = IndexCount;
    }
};

PACK_STRUCT(struct RibbonParticleVertex {
    uint32 Order;
    uint32 ParticleIndex;
    uint32 PrevParticleIndex;
    float Distance;
    // TODO: pack into half/uint16 data

    static GPUVertexLayout* GetLayout()
    {
        return GPUVertexLayout::Get({
            { VertexElement::Types::TexCoord0, 0, 0, 0, PixelFormat::R32_UInt },
            { VertexElement::Types::TexCoord1, 0, 0, 0, PixelFormat::R32_UInt },
            { VertexElement::Types::TexCoord2, 0, 0, 0, PixelFormat::R32_UInt },
            { VertexElement::Types::TexCoord3, 0, 0, 0, PixelFormat::R32_Float },
        });
    }
    });

struct EmitterCache
{
    double LastTimeUsed;
    ParticleBuffer* Buffer;
};

namespace ParticleManagerImpl
{
    CriticalSection PoolLocker;
    Dictionary<ParticleEmitter*, Array<EmitterCache>> Pool;
    Array<ParticleEffect*> UpdateList;
#if COMPILE_WITH_GPU_PARTICLES
    CriticalSection GpuUpdateListLocker;
    Array<ParticleEffect*> GpuUpdateList;
    RenderTask* GpuRenderTask = nullptr;
#endif
}

using namespace ParticleManagerImpl;

TaskGraphSystem* Particles::System = nullptr;
ConcurrentSystemLocker Particles::SystemLocker;
bool Particles::EnableParticleBufferPooling = true;
float Particles::ParticleBufferRecycleTimeout = 10.0f;

SpriteParticleRenderer SpriteRenderer;

class ParticleManagerService : public EngineService
{
public:
    ParticleManagerService()
        : EngineService(TEXT("Particle Manager"), 65)
    {
    }

    bool Init() override;
    void Dispose() override;
};

class ParticlesSystem : public TaskGraphSystem
{
public:
    float DeltaTime, UnscaledDeltaTime, Time, UnscaledTime;
    bool Active;

    void Job(int32 index);
    void Execute(TaskGraph* graph) override;
    void PostExecute(TaskGraph* graph) override;
};

ParticleManagerService ParticleManagerServiceInstance;

void Particles::UpdateEffect(ParticleEffect* effect)
{
    PROFILE_MEM(Particles);
    UpdateList.Add(effect);
}

void Particles::OnEffectDestroy(ParticleEffect* effect)
{
    UpdateList.Remove(effect);
#if COMPILE_WITH_GPU_PARTICLES
    GpuUpdateList.Remove(effect);
#endif
}

bool EmitterUseSorting(RenderContextBatch& renderContextBatch, ParticleBuffer* buffer, DrawPass drawModes, const BoundingSphere& bounds)
{
    const RenderView& mainView = renderContextBatch.GetMainContext().View;
    drawModes &= mainView.Pass;
    return buffer->Emitter->Graph.SortModules.HasItems() && EnumHasAnyFlags(drawModes, DrawPass::Forward) && (mainView.IsCullingDisabled || mainView.CullingFrustum.Intersects(bounds));
}

void DrawEmitterCPU(RenderContextBatch& renderContextBatch, ParticleBuffer* buffer, DrawCall& drawCall, DrawPass drawModes, StaticFlags staticFlags, const BoundingSphere& bounds, uint32 renderModulesIndices, int8 sortOrder)
{
    // Skip if CPU buffer is empty
    if (buffer->CPU.Count == 0)
        return;
    const auto context = GPUDevice::Instance->GetMainContext();
    auto emitter = buffer->Emitter;

    // Check if need to perform any particles sorting
    if (EmitterUseSorting(renderContextBatch, buffer, drawModes, bounds) && (buffer->CPU.Count != 0 || buffer->GPU.SortedIndices))
    {
        // Prepare sorting data
        if (!buffer->GPU.SortedIndices)
            buffer->AllocateSortBuffer();

        // Execute all sorting modules
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.SortModules.Count(); moduleIndex++)
        {
            auto module = emitter->Graph.SortModules[moduleIndex];
            const int32 sortedIndicesOffset = module->SortedIndicesOffset;
            const auto sortMode = static_cast<ParticleSortMode>(module->Values[2].AsInt);
            const int32 stride = buffer->Stride;
            const int32 listSize = buffer->CPU.Count;
            const int32 indicesByteSize = listSize * buffer->GPU.SortedIndices->GetStride();
            Array<uint32, RendererAllocation> sortingKeysList[4];
            Array<byte, RendererAllocation> sortingIndicesList[2];
            uint32* sortingKeys[2];
            void* sortingIndices[2];
            if (listSize < 500)
            {
                // Use fast stack allocator from RenderList
                auto& memory = renderContextBatch.GetMainContext().List->Memory;
                sortingKeys[0] = memory.Allocate<uint32>(listSize);
                sortingKeys[1] = memory.Allocate<uint32>(listSize);
                sortingIndices[0] = memory.Allocate(indicesByteSize, GPU_SHADER_DATA_ALIGNMENT);
                sortingIndices[1] = memory.Allocate(indicesByteSize, GPU_SHADER_DATA_ALIGNMENT);
            }
            else
            {
                // Use shared pooled memory from RendererAllocation
                sortingKeysList[0].Resize(listSize);
                sortingKeysList[1].Resize(listSize);
                sortingIndicesList[0].Resize(indicesByteSize);
                sortingIndicesList[1].Resize(indicesByteSize);
                sortingKeys[0] = sortingKeysList[0].Get();
                sortingKeys[1] = sortingKeysList[1].Get();
                sortingIndices[0] = sortingIndicesList[0].Get();
                sortingIndices[1] = sortingIndicesList[1].Get();
            }
            uint32* sortedKeys = sortingKeys[0];
            const uint32 sortKeyXor = sortMode != ParticleSortMode::CustomAscending ? MAX_uint32 : 0;
            switch (sortMode)
            {
            case ParticleSortMode::ViewDepth:
            {
                const int32 positionOffset = emitter->Graph.GetPositionAttributeOffset();
                if (positionOffset == -1)
                    break;
                const Matrix viewProjection = renderContextBatch.GetMainContext().View.ViewProjection();
                const byte* positionPtr = buffer->CPU.Buffer.Get() + positionOffset;
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        // TODO: use SIMD
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey(Matrix::TransformPosition(viewProjection, Matrix::TransformPosition(drawCall.World, *(const Float3*)positionPtr)).W) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                else
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey(Matrix::TransformPosition(viewProjection, *(const Float3*)positionPtr).W) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                break;
            }
            case ParticleSortMode::ViewDistance:
            {
                const int32 positionOffset = emitter->Graph.GetPositionAttributeOffset();
                if (positionOffset == -1)
                    break;
                const Float3 viewPosition = renderContextBatch.GetMainContext().View.Position;
                const byte* positionPtr = buffer->CPU.Buffer.Get() + positionOffset;
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        // TODO: use SIMD
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey((viewPosition - Float3::Transform(*(const Float3*)positionPtr, drawCall.World)).LengthSquared()) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                else
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        // TODO: use SIMD
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey((viewPosition - *(const Float3*)positionPtr).LengthSquared()) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                break;
            }
            case ParticleSortMode::CustomAscending:
            case ParticleSortMode::CustomDescending:
            {
                const int32 attributeIdx = module->Attributes[0];
                if (attributeIdx == -1)
                    break;
                const int32 attributeOffset = emitter->Graph.Layout.Attributes[attributeIdx].Offset;
                if (attributeOffset == -1)
                    break;
                const byte* attributePtr = buffer->CPU.Buffer.Get() + attributeOffset;
                for (int32 i = 0; i < buffer->CPU.Count; i++)
                {
                    sortedKeys[i] = RenderTools::ComputeDistanceSortKey(*(const float*)attributePtr) ^ sortKeyXor;
                    attributePtr += stride;
                }
                break;
            }
#if !BUILD_RELEASE
            default:
                CRASH;
#endif
            }

            // Generate sorting indices
            void* sortedIndices = sortingIndices[0];
            switch (buffer->GPU.SortedIndices->GetFormat())
            {
            case PixelFormat::R16_UInt:
                for (int32 i = 0; i < listSize; i++)
                    ((uint16*)sortedIndices)[i] = i;
                break;
            case PixelFormat::R32_UInt:
                for (int32 i = 0; i < listSize; i++)
                    ((uint32*)sortedIndices)[i] = i;
                break;
            }

            // Sort keys with indices
            switch (buffer->GPU.SortedIndices->GetFormat())
            {
            case PixelFormat::R16_UInt:
            {
                uint16* sortedIndicesTyped = (uint16*)sortedIndices;
                Sorting::RadixSort(sortedKeys, sortedIndicesTyped, sortingKeys[1], (uint16*)sortingIndices[1], listSize);
                sortedIndices = sortedIndicesTyped;
                break;
            }
            case PixelFormat::R32_UInt:
            {
                uint32* sortedIndicesTyped = (uint32*)sortedIndices;
                Sorting::RadixSort(sortedKeys, sortedIndicesTyped, sortingKeys[1], (uint32*)sortingIndices[1], listSize);
                sortedIndices = sortedIndicesTyped;
                break;
            }
            }

            // Upload CPU particles indices
            {
                RenderContext::GPULocker.Lock();
                context->UpdateBuffer(buffer->GPU.SortedIndices, sortedIndices, indicesByteSize, sortedIndicesOffset);
                RenderContext::GPULocker.Unlock();
            }
        }
    }

    // Upload CPU particles data to GPU
    {
        RenderContext::GPULocker.Lock();
        context->UpdateBuffer(buffer->GPU.Buffer, buffer->CPU.Buffer.Get(), buffer->CPU.Count * buffer->Stride);
        RenderContext::GPULocker.Unlock();
    }

    // Check if need to setup ribbon modules
    int32 ribbonModuleIndex = 0;
    int32 ribbonModulesDrawIndicesPos = 0;
    int32 ribbonModulesDrawIndicesStart[PARTICLE_EMITTER_MAX_RIBBONS] = {};
    int32 ribbonModulesDrawIndicesCount[PARTICLE_EMITTER_MAX_RIBBONS] = {};
    int32 ribbonModulesSegmentCount[PARTICLE_EMITTER_MAX_RIBBONS] = {};
    if (emitter->Graph.RibbonRenderingModules.HasItems())
    {
        // Prepare ribbon data
        if (!buffer->GPU.RibbonIndexBufferDynamic)
            buffer->GPU.RibbonIndexBufferDynamic = New<DynamicIndexBuffer>(0, (uint32)sizeof(uint16), TEXT("RibbonIndexBufferDynamic"));
        else
            buffer->GPU.RibbonIndexBufferDynamic->Clear();
        if (!buffer->GPU.RibbonVertexBufferDynamic)
            buffer->GPU.RibbonVertexBufferDynamic = New<DynamicVertexBuffer>(0, (uint32)sizeof(RibbonParticleVertex), TEXT("RibbonVertexBufferDynamic"), RibbonParticleVertex::GetLayout());
        else
            buffer->GPU.RibbonVertexBufferDynamic->Clear();
        auto& indexBuffer = buffer->GPU.RibbonIndexBufferDynamic->Data;
        auto& vertexBuffer = buffer->GPU.RibbonVertexBufferDynamic->Data;

        // Setup all ribbon modules
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
        {
            if ((renderModulesIndices & (1u << moduleIndex)) == 0)
                continue;
            auto module = emitter->Graph.RenderModules[moduleIndex];
            if (module->TypeID != 404 || ribbonModuleIndex >= PARTICLE_EMITTER_MAX_RIBBONS)
                continue;
            ribbonModulesDrawIndicesStart[ribbonModuleIndex] = ribbonModulesDrawIndicesPos;
            ribbonModulesDrawIndicesCount[ribbonModuleIndex] = 0;

            // Prepare particles buffer access
            auto positionOffset = emitter->Graph.GetPositionAttributeOffset();
            if (positionOffset == -1 || buffer->CPU.Count < 2 || buffer->CPU.RibbonOrder.IsEmpty())
                break;
            uint32 count = buffer->CPU.Count;
            ASSERT(buffer->CPU.RibbonOrder.Count() == emitter->Graph.RibbonRenderingModules.Count() * buffer->Capacity);
            int32* ribbonOrderData = buffer->CPU.RibbonOrder.Get() + module->RibbonOrderOffset;
            ParticleBufferCPUDataAccessor<Float3> positionData(buffer, emitter->Graph.Layout.GetAttributeOffset(module->Attributes[0]));

            // Write ribbon indices/vertices
            int32 indices = 0, segmentCount = 0;
            float totalDistance = 0.0f;
            int32 firstVertexIndex = vertexBuffer.Count();
            uint32 idxPrev = ribbonOrderData[0], vertexPrev = 0;
            {
                uint32 idxThis = ribbonOrderData[0];

                // 2 vertices
                {
                    vertexBuffer.AddUninitialized(2 * sizeof(RibbonParticleVertex));
                    auto ptr = (RibbonParticleVertex*)(vertexBuffer.Get() + firstVertexIndex);

                    RibbonParticleVertex v = { 0, idxThis, idxThis, totalDistance };

                    *ptr++ = v;
                    *ptr++ = v;
                }

                idxPrev = idxThis;
            }
            for (uint32 i = 1; i < count; i++)
            {
                uint32 idxThis = ribbonOrderData[i];
                Float3 direction = positionData[idxThis] - positionData[idxPrev];
                const float distance = direction.Length();
                if (distance > 0.002f)
                {
                    totalDistance += distance;

                    // 2 vertices
                    {
                        auto idx = vertexBuffer.Count();
                        vertexBuffer.AddUninitialized(2 * sizeof(RibbonParticleVertex));
                        auto ptr = (RibbonParticleVertex*)(vertexBuffer.Get() + idx);

                        // TODO: this could be optimized by manually fetching per-particle data in vertex shader (2x less data to send and fetch)
                        RibbonParticleVertex v = { i, idxThis, idxPrev, totalDistance };

                        *ptr++ = v;
                        *ptr++ = v;
                    }

                    // 2 triangles
                    {
                        auto idx = indexBuffer.Count();
                        indexBuffer.AddUninitialized(6 * sizeof(uint16));
                        auto ptr = (uint16*)(indexBuffer.Get() + idx);

                        uint32 i0 = vertexPrev;
                        uint32 i1 = vertexPrev + 2;

                        *ptr++ = i0;
                        *ptr++ = i0 + 1;
                        *ptr++ = i1;

                        *ptr++ = i0 + 1;
                        *ptr++ = i1 + 1;
                        *ptr++ = i1;

                        indices += 6;
                    }

                    idxPrev = idxThis;
                    segmentCount++;
                    vertexPrev += 2;
                }
            }
            if (segmentCount == 0)
                continue;
            {
                // Fix first particle vertex data to have proper direction
                auto ptr0 = (RibbonParticleVertex*)(vertexBuffer.Get() + firstVertexIndex);
                auto ptr1 = ptr0 + 1;
                auto ptr2 = ptr1 + 1;
                ptr0->PrevParticleIndex = ptr1->PrevParticleIndex = ptr2->ParticleIndex;
            }

            // Setup ribbon data
            ribbonModulesSegmentCount[ribbonModuleIndex] = segmentCount;
            ribbonModulesDrawIndicesCount[ribbonModuleIndex] = indices;
            ribbonModulesDrawIndicesPos += indices;

            ribbonModuleIndex++;
        }

        if (ribbonModuleIndex != 0)
        {
            // Upload data to the GPU buffer
            RenderContext::GPULocker.Lock();
            buffer->GPU.RibbonIndexBufferDynamic->Flush(context);
            buffer->GPU.RibbonVertexBufferDynamic->Flush(context);
            RenderContext::GPULocker.Unlock();
        }
    }

    // Execute all rendering modules
    ribbonModuleIndex = 0;
    for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
    {
        if ((renderModulesIndices & (1u << moduleIndex)) == 0)
            continue;
        auto module = emitter->Graph.RenderModules[moduleIndex];
        drawCall.Particle.Module = module;

        switch (module->TypeID)
        {
        // Sprite Rendering
        case 400:
        {
            const auto material = (MaterialBase*)module->Assets[0].Get();
            const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
            auto dp = drawModes & moduleDrawModes & material->GetDrawModes();
            if (dp == DrawPass::None || SpriteRenderer.Init())
                break;
            drawCall.Material = material;

            // Submit draw call
            SpriteRenderer.SetupDrawCall(drawCall);
            drawCall.InstanceCount = buffer->CPU.Count;
            renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, dp, staticFlags, ShadowsCastingMode::DynamicOnly, bounds, drawCall, false, sortOrder);

            break;
        }
        // Model Rendering
        case 403:
        {
            const auto model = (Model*)module->Assets[0].Get();
            const auto material = (MaterialBase*)module->Assets[1].Get();
            const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
            auto dp = drawModes & moduleDrawModes & material->GetDrawModes();
            if (dp == DrawPass::None)
                break;
            drawCall.Material = material;

            // TODO: model LOD picking for particles?
            int32 lodIndex = 0;
            ModelLOD& lod = model->LODs[lodIndex];
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                Mesh& mesh = lod.Meshes[meshIndex];
                if (!mesh.IsInitialized())
                    continue;
                // TODO: include mesh entry transformation, visibility and shadows mode?

                // Submit draw call
                mesh.GetDrawCallGeometry(drawCall);
                drawCall.InstanceCount = buffer->CPU.Count;
                renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, dp, staticFlags, ShadowsCastingMode::DynamicOnly, bounds, drawCall, false, sortOrder);
            }

            break;
        }
        // Ribbon Rendering
        case 404:
        {
            if (ribbonModulesDrawIndicesCount[ribbonModuleIndex] == 0)
                break;
            const auto material = (MaterialBase*)module->Assets[0].Get();
            const auto moduleDrawModes = module->Values.Count() > 6 ? (DrawPass)module->Values[6].AsInt : DrawPass::Default;
            auto dp = drawModes & moduleDrawModes & material->GetDrawModes();
            if (dp == DrawPass::None)
                break;
            drawCall.Material = material;

            // Node properties
            float uvTilingDistance = module->Values[3].AsFloat;
            Float2 uvScale = module->Values[4].AsFloat2();
            Float2 uvOffset = module->Values[5].AsFloat2();

            ParticleBufferCPUDataAccessor<float> sortKeyData(buffer, emitter->Graph.Layout.GetAttributeOffset(module->Attributes[1]));
            int32* ribbonOrderData = buffer->CPU.RibbonOrder.Get() + module->RibbonOrderOffset;
            int32 count = buffer->CPU.Count;

            // Setup ribbon data
            auto& ribbon = drawCall.Particle.Ribbon;
            ribbon.UVTilingDistance = uvTilingDistance;
            ribbon.SegmentCount = ribbonModulesSegmentCount[ribbonModuleIndex];
            ribbon.UVScaleX = uvScale.X;
            ribbon.UVScaleY = uvScale.Y;
            ribbon.UVOffsetX = uvOffset.X;
            ribbon.UVOffsetY = uvOffset.Y;
            if (ribbon.SegmentCount != 0 && Math::IsZero(uvTilingDistance) && sortKeyData.IsValid())
            {
                float firstSortValue = sortKeyData[ribbonOrderData[0]];
                float lastSortValue = sortKeyData[ribbonOrderData[count - 1]];

                float sortUScale = lastSortValue - firstSortValue;
                float sortUOffset = firstSortValue;

                ribbon.UVScaleX *= sortUScale;
                ribbon.UVOffsetX += sortUOffset * uvScale.X;
            }

            // TODO: invert particles rendering order if camera is closer to the ribbon end than start

            // Submit draw call
            drawCall.Geometry.IndexBuffer = buffer->GPU.RibbonIndexBufferDynamic->GetBuffer();
            drawCall.Geometry.VertexBuffers[0] = buffer->GPU.RibbonVertexBufferDynamic->GetBuffer();
            drawCall.Geometry.VertexBuffers[1] = nullptr;
            drawCall.Geometry.VertexBuffers[2] = nullptr;
            drawCall.Geometry.VertexBuffersOffsets[0] = 0;
            drawCall.Geometry.VertexBuffersOffsets[1] = 0;
            drawCall.Geometry.VertexBuffersOffsets[2] = 0;
            drawCall.Draw.StartIndex = ribbonModulesDrawIndicesStart[ribbonModuleIndex];
            drawCall.Draw.IndicesCount = ribbonModulesDrawIndicesCount[ribbonModuleIndex];
            drawCall.InstanceCount = 1;
            renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, dp, staticFlags, ShadowsCastingMode::DynamicOnly, bounds, drawCall, false, sortOrder);

            ribbonModuleIndex++;

            break;
        }
        // Volumetric Fog Rendering
        case 405:
        {
            const auto material = (MaterialBase*)module->Assets[0].Get();
            drawCall.Material = material;
            drawCall.InstanceCount = 1;

            auto positionOffset = emitter->Graph.Layout.GetAttributeOffset(module->Attributes[0]);
            int32 count = buffer->CPU.Count;
            if (positionOffset == -1 || count < 0)
                break;
            auto radiusOffset = emitter->Graph.Layout.GetAttributeOffset(module->Attributes[1]);
            ParticleBufferCPUDataAccessor<Float3> positionData(buffer, positionOffset);
            ParticleBufferCPUDataAccessor<float> radiusData(buffer, radiusOffset);
            const bool hasRadius = radiusOffset != -1;
            for (int32 i = 0; i < count; i++)
            {
                // Submit draw call
                // TODO: use instancing for volumetric fog particles (combine it with instanced circle rasterization into 3d texture)
                drawCall.Particle.VolumetricFog.Position = positionData[i];
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                    Float3::Transform(drawCall.Particle.VolumetricFog.Position, drawCall.World, drawCall.Particle.VolumetricFog.Position);
                drawCall.Particle.VolumetricFog.Radius = hasRadius ? radiusData[i] : 100.0f;
                drawCall.Particle.VolumetricFog.ParticleIndex = i;
                renderContextBatch.GetMainContext().List->VolumetricFogParticles.Add(drawCall);
            }
            break;
        }
        }
    }
}

#if COMPILE_WITH_GPU_PARTICLES

GPU_CB_STRUCT(GPUParticlesSortingData {
    Float3 ViewPosition;
    uint32 ParticleCounterOffset;
    uint32 ParticleStride;
    uint32 ParticleCapacity;
    uint32 PositionOffset;
    uint32 CustomOffset;
    Matrix PositionTransform;
    });

AssetReference<Shader> GPUParticlesSorting;
GPUConstantBuffer* GPUParticlesSortingCB;
GPUShaderProgramCS* GPUParticlesSortingCS[3];

// GPU emitters drawing is batched for efficiency
struct GPUEmitterDraw
{
    ParticleBuffer* Buffer;
    DrawCall DrawCall;
    DrawPass DrawModes;
    StaticFlags StaticFlags;
    BoundingSphere Bounds;
    uint32 RenderModulesIndices;
    uint32 IndirectArgsSize;
    int8 SortOrder;
    bool Sorting;
};
Array<GPUEmitterDraw> GPUEmitterDraws;
GPUBuffer* GPUIndirectArgsBuffer = nullptr;

#if COMPILE_WITH_DEV_ENV

void OnShaderReloading(Asset* obj)
{
    GPUParticlesSortingCB = nullptr;
    Platform::MemoryClear(GPUParticlesSortingCS, sizeof(GPUParticlesSortingCS));
}

#endif

void CleanupGPUParticlesSorting()
{
    GPUParticlesSorting = nullptr;
    GPUEmitterDraws.Resize(0);
    SAFE_DELETE_GPU_RESOURCE(GPUIndirectArgsBuffer);
}

void DrawEmittersGPU(RenderContextBatch& renderContextBatch)
{
    PROFILE_GPU_CPU_NAMED("DrawEmittersGPU");
    ConcurrentSystemLocker::ReadScope systemScope(Particles::SystemLocker);
    GPUContext* context = GPUDevice::Instance->GetMainContext();

    // Count draws and sorting passes needed for resources allocation
    uint32 indirectArgsSize = 0;
    bool sorting = false;
    for (const GPUEmitterDraw& draw : GPUEmitterDraws)
    {
        indirectArgsSize += draw.IndirectArgsSize;
        sorting |= draw.Sorting;
    }

    // Prepare pipeline
    if (sorting && GPUParticlesSorting == nullptr)
    {
        // TODO: preload shader if platform supports GPU particles (eg. inside ParticleEmitter::load if it's GPU sim with any sort module)
        GPUParticlesSorting = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GPUParticlesSorting"));
#if COMPILE_WITH_DEV_ENV
        if (GPUParticlesSorting)
            GPUParticlesSorting.Get()->OnReloading.Bind<OnShaderReloading>();
#endif
    }
    if (GPUParticlesSorting == nullptr || !GPUParticlesSorting->IsLoaded())
    {
        // Skip sorting until shader is ready
        sorting = false;
    }
    else if (!GPUParticlesSortingCB)
    {
        const auto shader = GPUParticlesSorting->GetShader();
        const StringAnsiView CS_Sort("CS_Sort");
        GPUParticlesSortingCS[0] = shader->GetCS(CS_Sort, 0);
        GPUParticlesSortingCS[1] = shader->GetCS(CS_Sort, 1);
        GPUParticlesSortingCS[2] = shader->GetCS(CS_Sort, 2);
        GPUParticlesSortingCB = shader->GetCB(0);
        ASSERT_LOW_LAYER(GPUParticlesSortingCB);
    }
    const uint32 indirectArgsCapacity = Math::RoundUpToPowerOf2(indirectArgsSize);
    if (GPUIndirectArgsBuffer == nullptr)
        GPUIndirectArgsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("ParticleIndirectDrawArgsBuffer"));
    if (GPUIndirectArgsBuffer->GetSize() < indirectArgsCapacity)
        GPUIndirectArgsBuffer->Init(GPUBufferDescription::Argument(indirectArgsCapacity));

    // Build indirect arguments
    uint32 indirectArgsOffset = 0;
    {
        PROFILE_GPU_CPU_NAMED("Init Indirect Args");

        GPUMemoryPass pass(context);
        pass.Transition(GPUIndirectArgsBuffer, GPUResourceAccess::CopyWrite);
        for (GPUEmitterDraw& draw : GPUEmitterDraws)
            pass.Transition(draw.Buffer->GPU.Buffer, GPUResourceAccess::CopyRead);

        // Init default arguments
        byte* indirectArgsMemory = (byte*)renderContextBatch.GetMainContext().List->Memory.Allocate(indirectArgsSize, GPU_SHADER_DATA_ALIGNMENT);
        for (GPUEmitterDraw& draw : GPUEmitterDraws)
        {
            ParticleEmitter* emitter = draw.Buffer->Emitter;
            for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
            {
                if ((draw.RenderModulesIndices & (1u << moduleIndex)) == 0)
                    continue;
                auto module = emitter->Graph.RenderModules.Get()[moduleIndex];
                switch (module->TypeID)
                {
                // Sprite Rendering
                case 400:
                {
                    const auto material = (MaterialBase*)module->Assets[0].Get();
                    const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
                    auto dp = draw.DrawModes & moduleDrawModes & material->GetDrawModes();
                    if (dp == DrawPass::None || SpriteRenderer.Init())
                        break;

                    // Draw sprite for each particle
                    GPUDrawIndexedIndirectArgs args = { SpriteParticleRenderer::IndexCount, 1, 0, 0, 0 };
                    Platform::MemoryCopy(indirectArgsMemory + indirectArgsOffset, &args, sizeof(args));
                    indirectArgsOffset += sizeof(args);
                    break;
                }
                // Model Rendering
                case 403:
                {
                    const auto model = (Model*)module->Assets[0].Get();
                    const auto material = (MaterialBase*)module->Assets[1].Get();
                    const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
                    auto dp = draw.DrawModes & moduleDrawModes & material->GetDrawModes();
                    if (dp == DrawPass::None)
                        break;
                    // TODO: model LOD picking for particles?
                    int32 lodIndex = 0;
                    ModelLOD& lod = model->LODs[lodIndex];
                    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                    {
                        Mesh& mesh = lod.Meshes[meshIndex];
                        if (!mesh.IsInitialized())
                            continue;

                        // Draw mesh for each particle
                        GPUDrawIndexedIndirectArgs args = { (uint32)mesh.GetTriangleCount() * 3, 1, 0, 0, 0 };
                        Platform::MemoryCopy(indirectArgsMemory + indirectArgsOffset, &args, sizeof(args));
                        indirectArgsOffset += sizeof(args);
                    }
                    break;
                }
                }
            }
        }

        // Upload default arguments
        context->UpdateBuffer(GPUIndirectArgsBuffer, indirectArgsMemory, indirectArgsOffset);

        // Wait for whole buffer write end before submitting buffer copies
        pass.MemoryBarrier();

        // Copy particle counts into draw commands
        indirectArgsOffset = 0;
        for (GPUEmitterDraw& draw : GPUEmitterDraws)
        {
            ParticleEmitter* emitter = draw.Buffer->Emitter;
            for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
            {
                if ((draw.RenderModulesIndices & (1u << moduleIndex)) == 0)
                    continue;
                auto module = emitter->Graph.RenderModules.Get()[moduleIndex];
                switch (module->TypeID)
                {
                // Sprite Rendering
                case 400:
                {
                    const auto material = (MaterialBase*)module->Assets[0].Get();
                    const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
                    auto dp = draw.DrawModes & moduleDrawModes & material->GetDrawModes();
                    if (dp == DrawPass::None || SpriteRenderer.Init())
                        break;

                    // Draw sprite for each particle
                    context->CopyBuffer(GPUIndirectArgsBuffer, draw.Buffer->GPU.Buffer, 4, indirectArgsOffset + 4, draw.Buffer->GPU.ParticleCounterOffset);
                    indirectArgsOffset += sizeof(GPUDrawIndexedIndirectArgs);
                    break;
                }
                // Model Rendering
                case 403:
                {
                    const auto model = (Model*)module->Assets[0].Get();
                    const auto material = (MaterialBase*)module->Assets[1].Get();
                    const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
                    auto dp = draw.DrawModes & moduleDrawModes & material->GetDrawModes();
                    if (dp == DrawPass::None)
                        break;
                    // TODO: model LOD picking for particles?
                    int32 lodIndex = 0;
                    ModelLOD& lod = model->LODs[lodIndex];
                    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                    {
                        Mesh& mesh = lod.Meshes[meshIndex];
                        if (!mesh.IsInitialized())
                            continue;

                        // Draw mesh for each particle
                        context->CopyBuffer(GPUIndirectArgsBuffer, draw.Buffer->GPU.Buffer, 4, indirectArgsOffset + 4, draw.Buffer->GPU.ParticleCounterOffset);
                        indirectArgsOffset += sizeof(GPUDrawIndexedIndirectArgs);
                    }
                    break;
                }
                }
            }
        }
    }
    indirectArgsOffset = 0;

    // Sort particles
    if (sorting)
    {
        PROFILE_GPU_CPU_NAMED("Sort Particles");
        context->BindCB(0, GPUParticlesSortingCB);

        // Generate sort keys for each particle
        {
            PROFILE_GPU("Gen Sort Keys");

            GPUComputePass pass(context);
            for (const GPUEmitterDraw& draw : GPUEmitterDraws)
            {
                if (draw.Sorting)
                {
                    pass.Transition(draw.Buffer->GPU.Buffer, GPUResourceAccess::ShaderReadCompute);
                    pass.Transition(draw.Buffer->GPU.SortedIndices, GPUResourceAccess::UnorderedAccess);
                    pass.Transition(draw.Buffer->GPU.SortingKeys, GPUResourceAccess::UnorderedAccess);
                }
            }

            for (const GPUEmitterDraw& draw : GPUEmitterDraws)
            {
                if (!draw.Sorting)
                    continue;
                ASSERT(draw.Buffer->GPU.SortingKeys);
                ParticleEmitter* emitter = draw.Buffer->Emitter;
                for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.SortModules.Count(); moduleIndex++)
                {
                    auto module = emitter->Graph.SortModules[moduleIndex];
                    // TODO: add support for module->SortedIndicesOffset (multiple sort modules)
                    const auto sortMode = (ParticleSortMode)module->Values[2].AsInt;
                    GPUParticlesSortingData data;
                    data.ParticleCounterOffset = draw.Buffer->GPU.ParticleCounterOffset;
                    data.ParticleStride = draw.Buffer->Stride;
                    data.ParticleCapacity = draw.Buffer->Capacity;
                    int32 permutationIndex;
                    switch (sortMode)
                    {
                    case ParticleSortMode::ViewDepth:
                    {
                        permutationIndex = 0;
                        data.PositionOffset = emitter->Graph.GetPositionAttributeOffset();
                        const Matrix viewProjection = renderContextBatch.GetMainContext().View.ViewProjection();
                        if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                            Matrix::Transpose(draw.DrawCall.World * viewProjection, data.PositionTransform);
                        else
                            Matrix::Transpose(viewProjection, data.PositionTransform);
                        break;
                    }
                    case ParticleSortMode::ViewDistance:
                    {
                        permutationIndex = 1;
                        data.PositionOffset = emitter->Graph.GetPositionAttributeOffset();
                        data.ViewPosition = renderContextBatch.GetMainContext().View.Position;
                        if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                            Matrix::Transpose(draw.DrawCall.World, data.PositionTransform);
                        else
                            Matrix::Transpose(Matrix::Identity, data.PositionTransform);
                        break;
                    }
                    case ParticleSortMode::CustomAscending:
                    case ParticleSortMode::CustomDescending:
                    {
                        permutationIndex = 2;
                        int32 attributeIdx = module->Attributes[0];
                        if (attributeIdx == -1)
                            break;
                        data.CustomOffset = emitter->Graph.Layout.Attributes[attributeIdx].Offset;
                        break;
                    }
                    }
                    context->UpdateCB(GPUParticlesSortingCB, &data);
                    context->BindSR(0, draw.Buffer->GPU.Buffer->View());
                    context->BindUA(0, draw.Buffer->GPU.SortedIndices->View());
                    context->BindUA(1, draw.Buffer->GPU.SortingKeys->View());
                    const int32 threadGroupSize = 1024;
                    context->Dispatch(GPUParticlesSortingCS[permutationIndex], Math::DivideAndRoundUp(draw.Buffer->GPU.ParticlesCountMax, threadGroupSize), 1, 1);
                }
            }
            context->ResetUA();
        }

        // Run sorting
        constexpr int32 inplaceSortSizeLimit = 2048;
        {
            // Small emitters can be sorted in-place with a single independent dispatch (simultaneously)
            GPUComputePass pass(context);
            for (const GPUEmitterDraw& draw : GPUEmitterDraws)
            {
                if (!draw.Sorting || draw.Buffer->GPU.ParticlesCountMax > inplaceSortSizeLimit)
                    continue;
                ParticleEmitter* emitter = draw.Buffer->Emitter;
                for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.SortModules.Count(); moduleIndex++)
                {
                    auto module = emitter->Graph.SortModules[moduleIndex];
                    // TODO: add support for module->SortedIndicesOffset (multiple sort modules)
                    const auto sortMode = (ParticleSortMode)module->Values[2].AsInt;
                    bool sortAscending = sortMode == ParticleSortMode::CustomAscending;
                    BitonicSort::Instance()->Sort(context, draw.Buffer->GPU.SortedIndices, draw.Buffer->GPU.SortingKeys, draw.Buffer->GPU.Buffer, draw.Buffer->GPU.ParticleCounterOffset, sortAscending, draw.Buffer->GPU.ParticlesCountMax);
                }
            }
        }
        for (const GPUEmitterDraw& draw : GPUEmitterDraws)
        {
            if (!draw.Sorting || draw.Buffer->GPU.ParticlesCountMax <= inplaceSortSizeLimit)
                continue;
            ParticleEmitter* emitter = draw.Buffer->Emitter;
            for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.SortModules.Count(); moduleIndex++)
            {
                auto module = emitter->Graph.SortModules[moduleIndex];
                // TODO: add support for module->SortedIndicesOffset (multiple sort modules)
                const auto sortMode = (ParticleSortMode)module->Values[2].AsInt;
                bool sortAscending = sortMode == ParticleSortMode::CustomAscending;
                BitonicSort::Instance()->Sort(context, draw.Buffer->GPU.SortedIndices, draw.Buffer->GPU.SortingKeys, draw.Buffer->GPU.Buffer, draw.Buffer->GPU.ParticleCounterOffset, sortAscending, draw.Buffer->GPU.ParticlesCountMax);
                // TODO: use args buffer from GPUIndirectArgsBuffer instead of internal from BitonicSort to get rid of UAV barrier (all sorting in parallel)
            }
        }
    }

    // TODO: transition here SortedIndices into ShaderReadNonPixel and Buffer into ShaderReadGraphics to reduce barriers during particles rendering

    // Submit draw calls
    for (GPUEmitterDraw& draw : GPUEmitterDraws)
    {
        // Execute all rendering modules using indirect draw arguments
        ParticleEmitter* emitter = draw.Buffer->Emitter;
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
        {
            if ((draw.RenderModulesIndices & (1u << moduleIndex)) == 0)
                continue;
            auto module = emitter->Graph.RenderModules.Get()[moduleIndex];
            draw.DrawCall.Particle.Module = module;
            switch (module->TypeID)
            {
                // Sprite Rendering
            case 400:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
                auto dp = draw.DrawModes & moduleDrawModes & material->GetDrawModes();
                if (dp == DrawPass::None || SpriteRenderer.Init())
                    break;
                draw.DrawCall.Material = material;

                // Submit draw call
                SpriteRenderer.SetupDrawCall(draw.DrawCall);
                draw.DrawCall.InstanceCount = 0;
                draw.DrawCall.Draw.IndirectArgsBuffer = GPUIndirectArgsBuffer;
                draw.DrawCall.Draw.IndirectArgsOffset = indirectArgsOffset;
                renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, dp, draw.StaticFlags, ShadowsCastingMode::DynamicOnly, draw.Bounds, draw.DrawCall, false, draw.SortOrder);
                indirectArgsOffset += sizeof(GPUDrawIndexedIndirectArgs);
                break;
            }
            // Model Rendering
            case 403:
            {
                const auto model = (Model*)module->Assets[0].Get();
                const auto material = (MaterialBase*)module->Assets[1].Get();
                const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
                auto dp = draw.DrawModes & moduleDrawModes & material->GetDrawModes();
                if (dp == DrawPass::None)
                    break;
                draw.DrawCall.Material = material;

                // TODO: model LOD picking for particles?
                int32 lodIndex = 0;
                ModelLOD& lod = model->LODs[lodIndex];
                for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                {
                    Mesh& mesh = lod.Meshes[meshIndex];
                    if (!mesh.IsInitialized())
                        continue;
                    // TODO: include mesh entry transformation, visibility and shadows mode?

                    // Execute draw call
                    mesh.GetDrawCallGeometry(draw.DrawCall);
                    draw.DrawCall.InstanceCount = 0;
                    draw.DrawCall.Draw.IndirectArgsBuffer = GPUIndirectArgsBuffer;
                    draw.DrawCall.Draw.IndirectArgsOffset = indirectArgsOffset;
                    renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, dp, draw.StaticFlags, ShadowsCastingMode::DynamicOnly, draw.Bounds, draw.DrawCall, false, draw.SortOrder);
                    indirectArgsOffset += sizeof(GPUDrawIndexedIndirectArgs);
                }
                break;
            }
            // Ribbon Rendering
            case 404:
            {
                // Not supported
                break;
            }
            // Volumetric Fog Rendering
            case 405:
            {
                // Not supported
                break;
            }
            }
        }
    }

    GPUEmitterDraws.Clear();
}

void DrawEmitterGPU(RenderContextBatch& renderContextBatch, ParticleBuffer* buffer, DrawCall& drawCall, DrawPass drawModes, StaticFlags staticFlags, const BoundingSphere& bounds, uint32 renderModulesIndices, int8 sortOrder)
{
    // Setup drawing data
    uint32 indirectArgsSize = 0;
    ParticleEmitter* emitter = buffer->Emitter;
    for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
    {
        if ((renderModulesIndices & (1u << moduleIndex)) == 0)
            continue;
        auto module = emitter->Graph.RenderModules.Get()[moduleIndex];
        switch (module->TypeID)
        {
            // Sprite Rendering
        case 400:
            indirectArgsSize += sizeof(GPUDrawIndexedIndirectArgs);
            break;
        // Model Rendering
        case 403:
        {
            const auto model = (Model*)module->Assets[0].Get();
            // TODO: model LOD picking for particles?
            int32 lodIndex = 0;
            ModelLOD& lod = model->LODs[lodIndex];
            indirectArgsSize += sizeof(GPUDrawIndexedIndirectArgs) * lod.Meshes.Count();
            break;
        }
        }
    }
    if (indirectArgsSize == 0)
        return;
    bool sorting = EmitterUseSorting(renderContextBatch, buffer, drawModes, bounds) && (buffer->GPU.ParticlesCountMax != 0 || buffer->GPU.SortedIndices);
    if (sorting && !buffer->GPU.SortedIndices)
        buffer->AllocateSortBuffer();

    // When rendering in async, delay GPU particles drawing to be in sync by moving drawing into delayed callback post scene drawing to use GPUContext safely
    // Also, batch rendering all GPU emitters together for more efficient usage of GPU memory barriers and indirect arguments buffers allocation
    RenderContext::GPULocker.Lock();
    if (GPUEmitterDraws.Count() == 0)
    {
        // The first emitter schedules the drawing of all batched draws
        renderContextBatch.GetMainContext().List->AddDelayedDraw([](RenderContextBatch& renderContextBatch, int32 contextIndex)
        {
            DrawEmittersGPU(renderContextBatch);
        });
    }
    GPUEmitterDraws.Add({ buffer, drawCall, drawModes, staticFlags, bounds, renderModulesIndices, indirectArgsSize, sortOrder, sorting });
    RenderContext::GPULocker.Unlock();
}

#endif

void Particles::DrawParticles(RenderContextBatch& renderContextBatch, ParticleEffect* effect)
{
    PROFILE_CPU();
    PROFILE_MEM(Particles);

    // Drawing assumes that all views within a batch have the same Origin
    const Vector3& viewOrigin = renderContextBatch.GetMainContext().View.Origin;
    BoundingSphere bounds = effect->GetSphere();
    bounds.Center -= viewOrigin;

    // Cull particles against all views
    uint64 viewsMask = 0;
    ASSERT_LOW_LAYER(renderContextBatch.Contexts.Count() <= 64);
    DrawPass viewsDrawModes = DrawPass::None;
    for (int32 i = 0; i < renderContextBatch.Contexts.Count(); i++)
    {
        const RenderView& view = renderContextBatch.Contexts.Get()[i].View;
        const bool visible = (view.Pass & effect->DrawModes) != DrawPass::None && (view.IsCullingDisabled || view.CullingFrustum.Intersects(bounds));
        if (visible)
        {
            viewsMask |= 1ull << (uint64)i;
            viewsDrawModes |= view.Pass;
        }
    }
    if (viewsMask == 0)
        return;
    viewsDrawModes &= effect->DrawModes;

    // Setup
    ConcurrentSystemLocker::ReadScope systemScope(SystemLocker);
    Matrix worlds[2];
    Matrix::Translation(-viewOrigin, worlds[0]); // World
    renderContextBatch.GetMainContext().View.GetWorldMatrix(effect->GetTransform(), worlds[1]); // Local
    float worldDeterminantSigns[2];
    worldDeterminantSigns[0] = Math::FloatSelect(worlds[0].RotDeterminant(), 1, -1);
    worldDeterminantSigns[1] = Math::FloatSelect(worlds[1].RotDeterminant(), 1, -1);
    const StaticFlags staticFlags = effect->GetStaticFlags();
    const int8 sortOrder = effect->SortOrder;

    // Draw lights (only to into the main view)
    if ((viewsMask & 1) == 1 && renderContextBatch.GetMainContext().View.Pass != DrawPass::Depth)
    {
        for (int32 emitterIndex = 0; emitterIndex < effect->Instance.Emitters.Count(); emitterIndex++)
        {
            auto& emitterData = effect->Instance.Emitters[emitterIndex];
            const auto buffer = emitterData.Buffer;
            if (!buffer || (buffer->Mode == ParticlesSimulationMode::CPU && buffer->CPU.Count == 0))
                continue;
            auto emitter = buffer->Emitter;
            if (!emitter || !emitter->IsLoaded())
                continue;

            buffer->Emitter->GraphExecutorCPU.Draw(buffer->Emitter, effect, emitterData, renderContextBatch.GetMainContext(), worlds[(int32)emitter->SimulationSpace]);
        }
    }

    // Setup a draw call common data
    DrawCall drawCall;
    drawCall.PerInstanceRandom = effect->GetPerInstanceRandom();
    drawCall.ObjectPosition = bounds.Center;
    drawCall.ObjectRadius = (float)bounds.Radius;

    // Draw all emitters
    for (int32 emitterIndex = 0; emitterIndex < effect->Instance.Emitters.Count(); emitterIndex++)
    {
        auto& emitterData = effect->Instance.Emitters[emitterIndex];
        const auto buffer = emitterData.Buffer;
        if (!buffer)
            continue;
        auto emitter = buffer->Emitter;
        if (!emitter || !emitter->IsLoaded())
            continue;

        drawCall.World = worlds[(int32)emitter->SimulationSpace];
        drawCall.WorldDeterminantSign = worldDeterminantSigns[(int32)emitter->SimulationSpace];
        drawCall.Particle.Particles = buffer;

        // Check if need to render any module
        uint32 renderModulesIndices = 0;
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count() && moduleIndex < 32; moduleIndex++)
        {
            auto module = emitter->Graph.RenderModules[moduleIndex];

            switch (module->TypeID)
            {
            // Sprite Rendering
            case 400:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
                if (!material ||
                    !material->IsReady() ||
                    !material->IsParticle() ||
                    (viewsDrawModes & material->GetDrawModes() & moduleDrawModes) == DrawPass::None
                )
                    break;
                renderModulesIndices |= 1u << moduleIndex;
                break;
            }
            // Model Rendering
            case 403:
            {
                const auto model = (Model*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
                if (!model ||
                    !model->IsLoaded() ||
                    !model->CanBeRendered())
                    break;
                const auto material = (MaterialBase*)module->Assets[1].Get();
                if (!material ||
                    !material->IsReady() ||
                    !material->IsParticle() ||
                    (viewsDrawModes & material->GetDrawModes() & moduleDrawModes) == DrawPass::None
                )
                    break;
                renderModulesIndices |= 1u << moduleIndex;
                break;
            }
            // Ribbon Rendering
            case 404:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 6 ? (DrawPass)module->Values[6].AsInt : DrawPass::Default;
                if (!material ||
                    !material->IsReady() ||
                    !material->IsParticle() ||
                    (viewsDrawModes & material->GetDrawModes() & moduleDrawModes) == DrawPass::None
                )
                    break;
                renderModulesIndices |= 1u << moduleIndex;
                break;
            }
            // Volumetric Fog Rendering
            case 405:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                if (!material ||
                    !material->IsReady() ||
                    material->GetInfo().Domain != MaterialDomain::VolumeParticle ||
                    (renderContextBatch.GetMainContext().View.Flags & ViewFlags::Fog) == ViewFlags::None ||
                    (viewsMask & 1) == 0
                )
                    break;
                renderModulesIndices |= 1u << moduleIndex;
                break;
            }
            }
        }
        if (renderModulesIndices == 0)
            continue;

        // Draw
        switch (buffer->Mode)
        {
        case ParticlesSimulationMode::CPU:
            DrawEmitterCPU(renderContextBatch, buffer, drawCall, viewsDrawModes, staticFlags, bounds, renderModulesIndices, sortOrder);
            break;
#if COMPILE_WITH_GPU_PARTICLES
        case ParticlesSimulationMode::GPU:
            DrawEmitterGPU(renderContextBatch, buffer, drawCall, viewsDrawModes, staticFlags, bounds, renderModulesIndices, sortOrder);
            break;
#endif
        }
    }
}

#if USE_EDITOR

void Particles::DebugDraw(ParticleEffect* effect)
{
    PROFILE_CPU_NAMED("Particles.DrawDebug");
    ConcurrentSystemLocker::ReadScope systemScope(SystemLocker);

    // Draw all emitters
    for (auto& emitterData : effect->Instance.Emitters)
    {
        const auto buffer = emitterData.Buffer;
        if (!buffer)
            continue;
        auto emitter = buffer->Emitter;
        if (!emitter || !emitter->IsLoaded())
            continue;
        emitter->GraphExecutorCPU.DrawDebug(emitter, effect, emitterData);
    }
}

#endif

#if COMPILE_WITH_GPU_PARTICLES

void UpdateGPU(RenderTask* task, GPUContext* context)
{
    ScopeLock lock(GpuUpdateListLocker);
    if (GpuUpdateList.IsEmpty())
        return;
    PROFILE_CPU_NAMED("GPUParticles");
    PROFILE_GPU("GPU Particles");
    PROFILE_MEM(Particles);
    ConcurrentSystemLocker::ReadScope systemScope(Particles::SystemLocker);

    // Collect valid emitter tracks to update
    struct GPUSim
    {
        ParticleEffect* Effect;
        ParticleEmitter* Emitter;
        int32 EmitterIndex;
        ParticleEmitterInstance& Data;
    };
    Array<GPUSim, RendererAllocation> sims;
    sims.EnsureCapacity(Math::AlignUp(GpuUpdateList.Count(), 64)); // Preallocate with some slack
    for (ParticleEffect* effect : GpuUpdateList)
    {
        auto& instance = effect->Instance;
        const auto particleSystem = effect->ParticleSystem.Get();
        if (!particleSystem || !particleSystem->IsLoaded())
            continue;

        for (int32 j = 0; j < particleSystem->Tracks.Count(); j++)
        {
            const auto& track = particleSystem->Tracks[j];
            if (track.Type != ParticleSystem::Track::Types::Emitter || track.Disabled)
                continue;
            const int32 emitterIndex = track.AsEmitter.Index;
            ParticleEmitter* emitter = particleSystem->Emitters[emitterIndex].Get();
            if (!emitter || !emitter->IsLoaded() || emitter->SimulationMode != ParticlesSimulationMode::GPU || instance.Emitters.Count() <= emitterIndex)
                continue;
            ParticleEmitterInstance& data = instance.Emitters[emitterIndex];
            if (!data.Buffer || !emitter->GPU.CanSim(emitter, data))
                continue;
            ASSERT(emitter->Capacity != 0 && emitter->Graph.Layout.Size != 0);
            sims.Add({ effect, emitter, emitterIndex, data });
        }
    }
    GpuUpdateList.Clear();

    // Pre-pass with buffers setup
    {
        PROFILE_CPU_NAMED("PreSim");

        GPUMemoryPass pass(context);
        for (GPUSim& sim : sims)
        {
            if (sim.Data.Buffer->GPU.PendingClear)
                pass.Transition(sim.Data.Buffer->GPU.Buffer, GPUResourceAccess::CopyWrite);
            pass.Transition(sim.Data.Buffer->GPU.BufferSecondary, GPUResourceAccess::CopyWrite);
        }

        for (GPUSim& sim : sims)
        {
            sim.Emitter->GPU.PreSim(context, sim.Emitter, sim.Effect, sim.EmitterIndex, sim.Data);
        }
    }

    // Pre-pass with buffers setup
    {
        PROFILE_GPU_CPU_NAMED("Sim");

        GPUComputePass pass(context);
        for (GPUSim& sim : sims)
        {
            pass.Transition(sim.Data.Buffer->GPU.Buffer, GPUResourceAccess::ShaderReadCompute);
            pass.Transition(sim.Data.Buffer->GPU.BufferSecondary, GPUResourceAccess::UnorderedAccess);
        }

        for (GPUSim& sim : sims)
        {
            sim.Emitter->GPU.Sim(context, sim.Emitter, sim.Effect, sim.EmitterIndex, sim.Data);
        }
    }

    // Post-pass with buffers setup
    {
        PROFILE_CPU_NAMED("PostSim");

        GPUMemoryPass pass(context);
        for (GPUSim& sim : sims)
        {
            if (sim.Data.CustomData.HasItems())
            {
                pass.Transition(sim.Data.Buffer->GPU.BufferSecondary, GPUResourceAccess::CopyRead);
                pass.Transition(sim.Data.Buffer->GPU.Buffer, GPUResourceAccess::CopyWrite);
            }
        }

        for (GPUSim& sim : sims)
        {
            sim.Emitter->GPU.PostSim(context, sim.Emitter, sim.Effect, sim.EmitterIndex, sim.Data);
        }
    }

    context->ResetSR();
    context->ResetUA();
    context->FlushState();
}

#endif

ParticleBuffer* Particles::AcquireParticleBuffer(ParticleEmitter* emitter)
{
    PROFILE_CPU();
    PROFILE_MEM(Particles);
    ParticleBuffer* result = nullptr;
    ASSERT(emitter && emitter->IsLoaded());

    if (emitter->EnablePooling && EnableParticleBufferPooling)
    {
        PoolLocker.Lock();
        const auto entries = Pool.TryGet(emitter);
        if (entries)
        {
            while (entries->HasItems() && !result)
            {
                // Reuse buffer
                result = entries->Last().Buffer;
                entries->RemoveLast();

                // Remove old buffers
                if (result->Version != emitter->Graph.Version)
                {
                    Delete(result);
                    result = nullptr;
                }
            }
        }
        PoolLocker.Unlock();
    }

    if (!result)
    {
        // Create new buffer
        result = New<ParticleBuffer>();
        if (result->Init(emitter))
        {
            LOG(Error, "Failed to create particle buffer for emitter {0}", emitter->ToString());
            Delete(result);
            return nullptr;
        }
    }
    else
    {
        // Prepare buffer
        result->Clear();
    }

    return result;
}

void Particles::RecycleParticleBuffer(ParticleBuffer* buffer)
{
    PROFILE_CPU();
    PROFILE_MEM(Particles);
    if (buffer->Emitter->EnablePooling && EnableParticleBufferPooling)
    {
        // Return to pool
        EmitterCache c;
        c.LastTimeUsed = Platform::GetTimeSeconds();
        c.Buffer = buffer;

        PoolLocker.Lock();
        Pool[buffer->Emitter].Add(c);
        PoolLocker.Unlock();
    }
    else
    {
        // Destroy
        Delete(buffer);
    }
}

void Particles::OnEmitterUnload(ParticleEmitter* emitter)
{
    PROFILE_CPU();
    PoolLocker.Lock();
    const auto entries = Pool.TryGet(emitter);
    if (entries)
    {
        for (int32 i = 0; i < entries->Count(); i++)
        {
            Delete(entries->At(i).Buffer);
        }
        entries->Clear();
        Pool.Remove(emitter);
    }
    PoolLocker.Unlock();

#if COMPILE_WITH_GPU_PARTICLES
    GpuUpdateListLocker.Lock();
    for (int32 i = GpuUpdateList.Count() - 1; i >= 0; i--)
    {
        if (GpuUpdateList[i]->Instance.ContainsEmitter(emitter))
            GpuUpdateList.RemoveAt(i);
    }
    GpuUpdateListLocker.Unlock();
#endif
}

bool ParticleManagerService::Init()
{
    PROFILE_MEM(Particles);
    Particles::System = New<ParticlesSystem>();
    Particles::System->Order = 10000;
    Engine::UpdateGraph->AddSystem(Particles::System);
    return false;
}

void ParticleManagerService::Dispose()
{
    UpdateList.Clear();
#if COMPILE_WITH_GPU_PARTICLES
    GpuUpdateList.Clear();
    if (GpuRenderTask)
    {
        ScopeLock lock(RenderTask::TasksLocker);
        RenderTask::Tasks.Remove(GpuRenderTask);
        Delete(GpuRenderTask);
        GpuRenderTask = nullptr;
    }
    CleanupGPUParticlesSorting();
#endif

    PoolLocker.Lock();
    for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
    {
        auto& entries = i->Value;
        for (int32 j = 0; j < entries.Count(); j++)
        {
            Delete(entries[j].Buffer);
        }
        entries.Clear();
    }
    Pool.Clear();
    PoolLocker.Unlock();

    SpriteRenderer.Dispose();
    SAFE_DELETE(Particles::System);
}

void ParticlesSystem::Job(int32 index)
{
    PROFILE_CPU_NAMED("Particles.Job");
    PROFILE_MEM(Particles);
    auto effect = UpdateList[index];
    auto& instance = effect->Instance;
    const auto particleSystem = effect->ParticleSystem.Get();
    if (!particleSystem || !particleSystem->IsLoaded())
        return;
    bool anyEmitterNotReady = false;
    for (int32 j = 0; j < particleSystem->Tracks.Count(); j++)
    {
        const auto& track = particleSystem->Tracks[j];
        if (track.Type != ParticleSystem::Track::Types::Emitter || track.Disabled)
            continue;
        auto emitter = particleSystem->Emitters[track.AsEmitter.Index].Get();
        if (!emitter || !emitter->IsLoaded())
        {
            anyEmitterNotReady = true;
            break;
        }
    }
    if (anyEmitterNotReady)
        return;
#if COMPILE_WITH_PROFILER && TRACY_ENABLE
    const StringView particleSystemName(particleSystem->GetPath());
    ZoneName(*particleSystemName, particleSystemName.Length());
#endif

    // Prepare instance data
    instance.Sync(particleSystem);

    bool updateBounds = false;
    bool updateGpu = false;

    // Simulation delta time can be based on a time since last update or the current delta time
    bool useTimeScale = effect->UseTimeScale;
#if USE_EDITOR
    if (!Editor::IsPlayMode)
        useTimeScale = false;
#endif
    float dt = useTimeScale ? DeltaTime : UnscaledDeltaTime;
    float t = useTimeScale ? Time : UnscaledTime;
    const float lastUpdateTime = instance.LastUpdateTime;
    if (lastUpdateTime > 0 && t > lastUpdateTime)
    {
        dt = t - lastUpdateTime;
    }
    else if (lastUpdateTime < 0)
    {
        // Update bounds after first system update
        updateBounds = true;
    }
    // TODO: if using fixed timestep quantize the dt and accumulate remaining part for the next update?
    //if (dt <= 1.0f / 240.0f)
    //    return;
    dt *= effect->SimulationSpeed;
    instance.Time += dt;
    const float fps = particleSystem->FramesPerSecond;
    const float duration = (float)particleSystem->DurationFrames / fps;
    if (instance.Time > duration)
    {
        if (effect->IsLooping)
        {
            // Loop
            // TODO: accumulate (duration - instance.Time) into next update dt
            instance.Time = 0;
            for (int32 j = 0; j < instance.Emitters.Count(); j++)
            {
                auto& e = instance.Emitters[j];
                e.Time = 0;
                for (auto& s : e.SpawnModulesData)
                {
                    s.NextSpawnTime = 0.0f;
                }
            }
        }
        else
        {
            // End
            instance.Time = duration;
            for (auto& emitterInstance : instance.Emitters)
            {
                if (emitterInstance.Buffer)
                {
                    Particles::RecycleParticleBuffer(emitterInstance.Buffer);
                    emitterInstance.Buffer = nullptr;
                }
            }
            // Stop playing effect.
            effect->Stop();
            return;
        }
    }
    instance.LastUpdateTime = t;

    // Update all emitter tracks
    for (int32 j = 0; j < particleSystem->Tracks.Count(); j++)
    {
        const auto& track = particleSystem->Tracks[j];
        if (track.Type != ParticleSystem::Track::Types::Emitter || track.Disabled)
            continue;
        auto emitter = particleSystem->Emitters[track.AsEmitter.Index].Get();
        auto& data = instance.Emitters[track.AsEmitter.Index];
        ASSERT(emitter && emitter->IsLoaded());
        if (emitter->Capacity == 0 || emitter->Graph.Layout.Size == 0)
            continue;
        PROFILE_CPU_ASSET(emitter);

        // Calculate new time position
        const float startTime = (float)track.AsEmitter.StartFrame / fps;
        const float durationTime = (float)track.AsEmitter.DurationFrames / fps;
        const bool canSpawn = startTime <= instance.Time && instance.Time <= startTime + durationTime;

        // Update instance data
        data.Sync(effect->Instance, particleSystem, track.AsEmitter.Index);
        if (!data.Buffer)
        {
            data.Buffer = Particles::AcquireParticleBuffer(emitter);
        }
        data.Time += dt;

        // Update particles simulation
        switch (emitter->SimulationMode)
        {
        case ParticlesSimulationMode::CPU:
            emitter->GraphExecutorCPU.Update(emitter, effect, data, dt, canSpawn);
            updateBounds |= emitter->UseAutoBounds;
            break;
#if COMPILE_WITH_GPU_PARTICLES
        case ParticlesSimulationMode::GPU:
            emitter->GPU.Update(emitter, effect, data, dt, canSpawn);
            updateGpu = true;
            break;
#endif
        default:
            break;
        }
    }

    // Update bounds if any of the emitters uses auto-bounds
    if (updateBounds)
    {
        effect->UpdateBounds();
    }

#if COMPILE_WITH_GPU_PARTICLES
    // Register for GPU update
    if (updateGpu)
    {
        ScopeLock lock(GpuUpdateListLocker);
        GpuUpdateList.Add(effect);
    }
#endif
}

void ParticlesSystem::Execute(TaskGraph* graph)
{
    if (UpdateList.Count() == 0)
        return;
    Active = true;

    // Ensure no particle assets can be reloaded/modified during async update
    Particles::SystemLocker.Begin(false);

    // Setup data for async update
    const auto& tickData = Time::Update;
    DeltaTime = tickData.DeltaTime.GetTotalSeconds();
    UnscaledDeltaTime = tickData.UnscaledDeltaTime.GetTotalSeconds();
    Time = tickData.Time.GetTotalSeconds();
    UnscaledTime = tickData.UnscaledTime.GetTotalSeconds();

    // Schedule work to update all particles in async
    Function<void(int32)> job;
    job.Bind<ParticlesSystem, &ParticlesSystem::Job>(this);
    graph->DispatchJob(job, UpdateList.Count());
}

void ParticlesSystem::PostExecute(TaskGraph* graph)
{
    if (!Active)
        return;
    PROFILE_CPU_NAMED("Particles.PostExecute");
    PROFILE_MEM(Particles);

    // Cleanup
    Particles::SystemLocker.End(false);
    Active = false;
    UpdateList.Clear();

#if COMPILE_WITH_GPU_PARTICLES
    // Create GPU render task if missing but required
    if (GpuUpdateList.HasItems() && !GpuRenderTask)
    {
        GpuRenderTask = New<RenderTask>();
        GpuRenderTask->Order = -10000000;
        GpuRenderTask->Render.Bind(UpdateGPU);
        ScopeLock lock(RenderTask::TasksLocker);
        RenderTask::Tasks.Add(GpuRenderTask);
    }
    else if (GpuRenderTask)
    {
        ScopeLock lock(RenderTask::TasksLocker);
        GpuRenderTask->Enabled = GpuUpdateList.HasItems();
    }
#endif

    // Recycle buffers
    const auto timeSeconds = Platform::GetTimeSeconds();
    PoolLocker.Lock();
    for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
    {
        auto& entries = i->Value;
        for (int32 j = 0; j < entries.Count(); j++)
        {
            auto& e = entries[j];
            if (timeSeconds - e.LastTimeUsed >= Particles::ParticleBufferRecycleTimeout)
            {
                Delete(e.Buffer);
                entries.RemoveAt(j--);
            }
        }

        if (entries.IsEmpty())
            Pool.Remove(i);
    }
    PoolLocker.Unlock();
}
