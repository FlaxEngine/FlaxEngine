// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Particles.h"
#include "ParticleEffect.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Profiler/ProfilerCPU.h"
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

struct SpriteParticleVertex
{
    float X;
    float Y;
    float U;
    float V;
};

class SpriteParticleRenderer
{
public:

    GPUBuffer* VB = nullptr;
    GPUBuffer* IB = nullptr;
    const static int32 VertexCount = 4;
    const static int32 IndexCount = 6;

public:

    bool Init()
    {
        if (VB)
            return false;
        VB = GPUDevice::Instance->CreateBuffer(TEXT("SpriteParticleRenderer,VB"));
        IB = GPUDevice::Instance->CreateBuffer(TEXT("SpriteParticleRenderer.IB"));
        static SpriteParticleVertex vertexBuffer[] =
        {
            { -0.5f, -0.5f, 0.0f, 0.0f },
            { +0.5f, -0.5f, 1.0f, 0.0f },
            { +0.5f, +0.5f, 1.0f, 1.0f },
            { -0.5f, +0.5f, 0.0f, 1.0f },
        };
        static uint16 indexBuffer[] = { 0, 1, 2, 0, 2, 3, };
        return VB->Init(GPUBufferDescription::Vertex(sizeof(SpriteParticleVertex), VertexCount, vertexBuffer)) || IB->Init(GPUBufferDescription::Index(sizeof(uint16), IndexCount, indexBuffer));
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
bool Particles::EnableParticleBufferPooling = true;
float Particles::ParticleBufferRecycleTimeout = 10.0f;

SpriteParticleRenderer SpriteRenderer;

namespace ParticlesDrawCPU
{
    Array<uint32> SortingKeys[2];
    Array<int32> SortingIndices;
    Array<int32> SortedIndices;
    Array<float> RibbonTotalDistances;
}

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
    void Job(int32 index);
    void Execute(TaskGraph* graph) override;
    void PostExecute(TaskGraph* graph) override;
};

ParticleManagerService ParticleManagerServiceInstance;

void Particles::UpdateEffect(ParticleEffect* effect)
{
    ASSERT_LOW_LAYER(!UpdateList.Contains(effect));
    UpdateList.Add(effect);
}

void Particles::OnEffectDestroy(ParticleEffect* effect)
{
    UpdateList.Remove(effect);
#if COMPILE_WITH_GPU_PARTICLES
    GpuUpdateList.Remove(effect);
#endif
}

typedef Array<int32, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> RenderModulesIndices;

void DrawEmitterCPU(RenderContext& renderContext, ParticleBuffer* buffer, DrawCall& drawCall, DrawPass drawModes, StaticFlags staticFlags, ParticleEmitterInstance& emitterData, const RenderModulesIndices& renderModulesIndices)
{
    // Skip if CPU buffer is empty
    if (buffer->CPU.Count == 0)
        return;
    const auto context = GPUDevice::Instance->GetMainContext();
    auto emitter = buffer->Emitter;

    // Check if need to perform any particles sorting
    if (emitter->Graph.SortModules.HasItems() && renderContext.View.Pass != DrawPass::Depth)
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
#define PREPARE_CACHE(list) (ParticlesDrawCPU::list).Clear(); (ParticlesDrawCPU::list).Resize(listSize)
            PREPARE_CACHE(SortingKeys[0]);
            PREPARE_CACHE(SortingKeys[1]);
            PREPARE_CACHE(SortingIndices);
#undef PREPARE_CACHE
            uint32* sortedKeys = ParticlesDrawCPU::SortingKeys[0].Get();
            const uint32 sortKeyXor = sortMode != ParticleSortMode::CustomAscending ? MAX_uint32 : 0;
            switch (sortMode)
            {
            case ParticleSortMode::ViewDepth:
            {
                const Matrix viewProjection = renderContext.View.ViewProjection();
                byte* positionPtr = buffer->CPU.Buffer.Get() + emitter->Graph.GetPositionAttributeOffset();
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        // TODO: use SIMD
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey(Matrix::TransformPosition(viewProjection, Matrix::TransformPosition(drawCall.World, *(Vector3*)positionPtr)).W) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                else
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey(Matrix::TransformPosition(viewProjection, *(Vector3*)positionPtr).W) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                break;
            }
            case ParticleSortMode::ViewDistance:
            {
                const Vector3 viewPosition = renderContext.View.Position;
                byte* positionPtr = buffer->CPU.Buffer.Get() + emitter->Graph.GetPositionAttributeOffset();
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        // TODO: use SIMD
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey((viewPosition - Vector3::Transform(*(Vector3*)positionPtr, drawCall.World)).LengthSquared()) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                else
                {
                    for (int32 i = 0; i < buffer->CPU.Count; i++)
                    {
                        // TODO: use SIMD
                        sortedKeys[i] = RenderTools::ComputeDistanceSortKey((viewPosition - *(Vector3*)positionPtr).LengthSquared()) ^ sortKeyXor;
                        positionPtr += stride;
                    }
                }
                break;
            }
            case ParticleSortMode::CustomAscending:
            case ParticleSortMode::CustomDescending:
            {
                int32 attributeIdx = module->Attributes[0];
                if (attributeIdx == -1)
                    break;
                byte* attributePtr = buffer->CPU.Buffer.Get() + emitter->Graph.Layout.Attributes[attributeIdx].Offset;
                for (int32 i = 0; i < buffer->CPU.Count; i++)
                {
                    sortedKeys[i] = RenderTools::ComputeDistanceSortKey(*(float*)attributePtr) ^ sortKeyXor;
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
            int32* sortedIndices;
            {
                ParticlesDrawCPU::SortedIndices.Resize(listSize);
                sortedIndices = ParticlesDrawCPU::SortedIndices.Get();
                for (int i = 0; i < listSize; i++)
                    sortedIndices[i] = i;
            }

            // Sort keys with indices
            {
                Sorting::RadixSort(sortedKeys, sortedIndices, ParticlesDrawCPU::SortingKeys[1].Get(), ParticlesDrawCPU::SortingIndices.Get(), listSize);
            }

            // Upload CPU particles indices
            {
                context->UpdateBuffer(buffer->GPU.SortedIndices, sortedIndices, listSize * sizeof(int32), sortedIndicesOffset);
            }
        }
    }

    // Upload CPU particles data to GPU
    {
        context->UpdateBuffer(buffer->GPU.Buffer, buffer->CPU.Buffer.Get(), buffer->CPU.Count * buffer->Stride);
    }

    // Check if need to setup ribbon modules
    int32 ribbonModuleIndex = 0;
    int32 ribbonModulesDrawIndicesPos = 0;
    int32 ribbonModulesDrawIndicesStart[PARTICLE_EMITTER_MAX_RIBBONS];
    int32 ribbonModulesDrawIndicesCount[PARTICLE_EMITTER_MAX_RIBBONS];
    int32 ribbonModulesSegmentCount[PARTICLE_EMITTER_MAX_RIBBONS];
    if (emitter->Graph.RibbonRenderingModules.HasItems())
    {
        // Prepare ribbon data
        if (!buffer->GPU.RibbonIndexBufferDynamic)
        {
            buffer->GPU.RibbonIndexBufferDynamic = New<DynamicIndexBuffer>(0, (uint32)sizeof(uint16), TEXT("RibbonIndexBufferDynamic"));
        }
        buffer->GPU.RibbonIndexBufferDynamic->Clear();

        // Setup all ribbon modules
        auto& totalDistances = ParticlesDrawCPU::RibbonTotalDistances;
        totalDistances.Clear();
        for (int32 index = 0; index < renderModulesIndices.Count(); index++)
        {
            const int32 moduleIndex = renderModulesIndices[index];
            auto module = emitter->Graph.RenderModules[moduleIndex];
            if (module->TypeID != 404 || ribbonModuleIndex >= PARTICLE_EMITTER_MAX_RIBBONS)
                continue;
            ribbonModulesDrawIndicesStart[ribbonModuleIndex] = ribbonModulesDrawIndicesPos;
            ribbonModulesDrawIndicesCount[ribbonModuleIndex] = 0;

            // Prepare particles buffer access
            auto positionOffset = emitter->Graph.GetPositionAttributeOffset();
            if (positionOffset == -1 || buffer->CPU.Count < 2 || buffer->CPU.RibbonOrder.IsEmpty())
                break;
            int32 count = buffer->CPU.Count;
            ASSERT(buffer->CPU.RibbonOrder.Count() == emitter->Graph.RibbonRenderingModules.Count() * buffer->Capacity);
            int32* ribbonOrderData = buffer->CPU.RibbonOrder.Get() + module->RibbonOrderOffset;

            ParticleBufferCPUDataAccessor<Vector3> positionData(buffer, emitter->Graph.Layout.GetAttributeOffset(module->Attributes[0]));

            int32 indices = 0;

            float totalDistance = 0.0f;
            uint32 lastParticleIdx = ribbonOrderData[0];
            for (int32 i = 0; i < count; i++)
            {
                bool isNotLast = i != count - 1;
                uint32 idx0 = ribbonOrderData[i];
                uint32 idx1 = 0;
                Vector3 direction;
                if (isNotLast)
                {
                    idx1 = ribbonOrderData[i + 1];
                    direction = positionData[idx1] - positionData[lastParticleIdx];
                }
                else
                {
                    idx1 = ribbonOrderData[i - 1];
                    direction = positionData[lastParticleIdx] - positionData[idx1];
                }

                if (direction.LengthSquared() > 0.002f || !isNotLast)
                {
                    totalDistances.Add(totalDistance);
                    lastParticleIdx = idx1;

                    if (isNotLast)
                    {
                        auto idx = buffer->GPU.RibbonIndexBufferDynamic->Data.Count();
                        buffer->GPU.RibbonIndexBufferDynamic->Data.AddDefault(6 * sizeof(uint16));
                        auto ptr = (uint16*)(buffer->GPU.RibbonIndexBufferDynamic->Data.Get() + idx);

                        idx0 *= 2;
                        idx1 *= 2;

                        *ptr++ = idx0 + 1;
                        *ptr++ = idx1;
                        *ptr++ = idx0;

                        *ptr++ = idx0 + 1;
                        *ptr++ = idx1 + 1;
                        *ptr++ = idx1;

                        indices += 6;
                    }
                }

                totalDistance += direction.Length();
            }
            if (indices == 0)
                break;

            // Setup ribbon data
            ribbonModulesSegmentCount[ribbonModuleIndex] = totalDistances.Count();
            if (totalDistances.HasItems())
            {
                auto& ribbonSegmentDistancesBuffer = buffer->GPU.RibbonSegmentDistances[index];
                if (!ribbonSegmentDistancesBuffer)
                {
                    ribbonSegmentDistancesBuffer = GPUDevice::Instance->CreateBuffer(TEXT("RibbonSegmentDistances"));
                    ribbonSegmentDistancesBuffer->Init(GPUBufferDescription::Typed(buffer->Capacity, PixelFormat::R32_Float, false, GPUResourceUsage::Dynamic));
                }
                context->UpdateBuffer(ribbonSegmentDistancesBuffer, totalDistances.Get(), totalDistances.Count() * sizeof(float));
            }

            ribbonModulesDrawIndicesCount[index] = indices;
            ribbonModulesDrawIndicesPos += indices;

            ribbonModuleIndex++;
        }

        if (ribbonModuleIndex != 0)
        {
            // Upload data to the GPU buffer
            buffer->GPU.RibbonIndexBufferDynamic->Flush(context);
        }
    }

    // Execute all rendering modules
    ribbonModuleIndex = 0;
    for (int32 index = 0; index < renderModulesIndices.Count(); index++)
    {
        const int32 moduleIndex = renderModulesIndices[index];
        auto module = emitter->Graph.RenderModules[moduleIndex];
        drawCall.Particle.Module = module;

        switch (module->TypeID)
        {
            // Sprite Rendering
        case 400:
        {
            const auto material = (MaterialBase*)module->Assets[0].Get();
            const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
            drawCall.Material = material;

            // Submit draw call
            SpriteRenderer.SetupDrawCall(drawCall);
            drawCall.InstanceCount = buffer->CPU.Count;
            renderContext.List->AddDrawCall((DrawPass)(drawModes & moduleDrawModes), staticFlags, drawCall, false);

            break;
        }
            // Model Rendering
        case 403:
        {
            const auto model = (Model*)module->Assets[0].Get();
            const auto material = (MaterialBase*)module->Assets[1].Get();
            const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
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
                renderContext.List->AddDrawCall((DrawPass)(drawModes & moduleDrawModes), staticFlags, drawCall, false);
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
            drawCall.Material = material;

            // Node properties
            float uvTilingDistance = module->Values[3].AsFloat;
            Vector2 uvScale = module->Values[4].AsVector2();
            Vector2 uvOffset = module->Values[5].AsVector2();

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
            ribbon.SegmentDistances = ribbon.SegmentCount != 0 ? buffer->GPU.RibbonSegmentDistances[index] : nullptr;

            // TODO: invert particles rendering order if camera is closer to the ribbon end than start

            // Submit draw call
            drawCall.Geometry.IndexBuffer = buffer->GPU.RibbonIndexBufferDynamic->GetBuffer();
            drawCall.Geometry.VertexBuffers[0] = nullptr;
            drawCall.Geometry.VertexBuffers[1] = nullptr;
            drawCall.Geometry.VertexBuffers[2] = nullptr;
            drawCall.Geometry.VertexBuffersOffsets[0] = 0;
            drawCall.Geometry.VertexBuffersOffsets[1] = 0;
            drawCall.Geometry.VertexBuffersOffsets[2] = 0;
            drawCall.Draw.StartIndex = ribbonModulesDrawIndicesStart[ribbonModuleIndex];
            drawCall.Draw.IndicesCount = ribbonModulesDrawIndicesCount[ribbonModuleIndex];
            drawCall.InstanceCount = 1;
            renderContext.List->AddDrawCall((DrawPass)(drawModes & moduleDrawModes), staticFlags, drawCall, false);

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
            ParticleBufferCPUDataAccessor<Vector3> positionData(buffer, positionOffset);
            ParticleBufferCPUDataAccessor<float> radiusData(buffer, radiusOffset);
            const bool hasRadius = radiusOffset != -1;
            for (int32 i = 0; i < count; i++)
            {
                // Submit draw call
                // TODO: use instancing for volumetric fog particles (combine it with instanced circle rasterization into 3d texture)
                drawCall.Particle.VolumetricFog.Position = positionData[i];
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                    Vector3::Transform(drawCall.Particle.VolumetricFog.Position, drawCall.World, drawCall.Particle.VolumetricFog.Position);
                drawCall.Particle.VolumetricFog.Radius = hasRadius ? radiusData[i] : 100.0f;
                drawCall.Particle.VolumetricFog.ParticleIndex = i;
                renderContext.List->VolumetricFogParticles.Add(drawCall);
            }
            break;
        }
        }
    }
}

#if COMPILE_WITH_GPU_PARTICLES

PACK_STRUCT(struct GPUParticlesSortingData {
    Vector3 ViewPosition;
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
}

void DrawEmitterGPU(RenderContext& renderContext, ParticleBuffer* buffer, DrawCall& drawCall, DrawPass drawModes, StaticFlags staticFlags, ParticleEmitterInstance& emitterData, const RenderModulesIndices& renderModulesIndices)
{
    const auto context = GPUDevice::Instance->GetMainContext();
    auto emitter = buffer->Emitter;

    // Check if need to perform any particles sorting
    if (emitter->Graph.SortModules.HasItems() && renderContext.View.Pass != DrawPass::Depth)
    {
        PROFILE_GPU_CPU("Sort Particles");

        // Prepare pipeline
        if (GPUParticlesSorting == nullptr)
        {
            // TODO: preload shader if platform supports GPU particles
            GPUParticlesSorting = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GPUParticlesSorting"));
            if (GPUParticlesSorting == nullptr || GPUParticlesSorting->WaitForLoaded())
                return;
#if COMPILE_WITH_DEV_ENV
            GPUParticlesSorting.Get()->OnReloading.Bind<OnShaderReloading>();
#endif
        }
        if (!GPUParticlesSortingCB)
        {
            const auto shader = GPUParticlesSorting->GetShader();
            const StringAnsiView CS_Sort("CS_Sort");
            GPUParticlesSortingCS[0] = shader->GetCS(CS_Sort, 0);
            GPUParticlesSortingCS[1] = shader->GetCS(CS_Sort, 1);
            GPUParticlesSortingCS[2] = shader->GetCS(CS_Sort, 2);
            GPUParticlesSortingCB = shader->GetCB(0);
            ASSERT(GPUParticlesSortingCB);
        }

        // Prepare sorting data
        if (!buffer->GPU.SortedIndices)
            buffer->AllocateSortBuffer();
        ASSERT(buffer->GPU.SortingKeysBuffer);

        // Execute all sorting modules
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.SortModules.Count(); moduleIndex++)
        {
            auto module = emitter->Graph.SortModules[moduleIndex];
            const auto sortMode = static_cast<ParticleSortMode>(module->Values[2].AsInt);

            // Generate sorting keys based on sorting mode
            GPUParticlesSortingData data;
            data.ParticleCounterOffset = buffer->GPU.ParticleCounterOffset;
            data.ParticleStride = buffer->Stride;
            data.ParticleCapacity = buffer->Capacity;
            int32 permutationIndex;
            bool sortAscending;
            switch (sortMode)
            {
            case ParticleSortMode::ViewDepth:
            {
                permutationIndex = 0;
                sortAscending = false;
                data.PositionOffset = emitter->Graph.GetPositionAttributeOffset();
                const Matrix viewProjection = renderContext.View.ViewProjection();
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    Matrix matrix;
                    Matrix::Multiply(drawCall.World, viewProjection, matrix);
                    Matrix::Transpose(matrix, data.PositionTransform);
                }
                else
                {
                    Matrix::Transpose(viewProjection, data.PositionTransform);
                }
                break;
            }
            case ParticleSortMode::ViewDistance:
            {
                permutationIndex = 1;
                sortAscending = false;
                data.PositionOffset = emitter->Graph.GetPositionAttributeOffset();
                data.ViewPosition = renderContext.View.Position;
                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    Matrix::Transpose(drawCall.World, data.PositionTransform);
                }
                else
                {
                    Matrix::Transpose(Matrix::Identity, data.PositionTransform);
                }
                break;
            }
            case ParticleSortMode::CustomAscending:
            case ParticleSortMode::CustomDescending:
            {
                permutationIndex = 2;
                sortAscending = sortMode == ParticleSortMode::CustomAscending;
                int32 attributeIdx = module->Attributes[0];
                if (attributeIdx == -1)
                    break;
                data.CustomOffset = emitter->Graph.Layout.Attributes[attributeIdx].Offset;
                break;
            }
#if !BUILD_RELEASE
            default:
            CRASH;
                return;
#endif
            }
            context->UpdateCB(GPUParticlesSortingCB, &data);
            context->BindCB(0, GPUParticlesSortingCB);
            context->BindSR(0, buffer->GPU.Buffer->View());
            context->BindUA(0, buffer->GPU.SortingKeysBuffer->View());
            // TODO: optimize it by using DispatchIndirect with shared invoke args generated after particles update
            const int32 threadGroupSize = 1024;
            context->Dispatch(GPUParticlesSortingCS[permutationIndex], Math::DivideAndRoundUp(buffer->GPU.ParticlesCountMax, threadGroupSize), 1, 1);

            // Perform sorting
            BitonicSort::Instance()->Sort(context, buffer->GPU.SortingKeysBuffer, buffer->GPU.Buffer, data.ParticleCounterOffset, sortAscending, buffer->GPU.SortedIndices);
        }
    }

    // Count draw calls to perform during this emitter rendering
    int32 drawCalls = 0;
    for (int32 index = 0; index < renderModulesIndices.Count(); index++)
    {
        int32 moduleIndex = renderModulesIndices[index];
        auto module = emitter->Graph.RenderModules[moduleIndex];
        switch (module->TypeID)
        {
            // Sprite Rendering
        case 400:
        {
            drawCalls++;
            break;
        }
            // Model Rendering
        case 403:
        {
            const auto model = (Model*)module->Assets[0].Get();

            // TODO: model LOD picking for particles?
            int32 lodIndex = 0;
            ModelLOD& lod = model->LODs[lodIndex];
            drawCalls += lod.Meshes.Count();
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                Mesh& mesh = lod.Meshes[meshIndex];
                if (!mesh.IsInitialized())
                    continue;

                drawCalls++;
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
    if (drawCalls == 0)
        return;

    // Ensure to have enough space for indirect draw arguments
    const uint32 minSize = drawCalls * sizeof(GPUDrawIndexedIndirectArgs);
    if (buffer->GPU.IndirectDrawArgsBuffer->GetSize() < minSize)
    {
        buffer->GPU.IndirectDrawArgsBuffer->Init(GPUBufferDescription::Argument(minSize));
    }

    // Initialize indirect draw arguments contents (do it before drawing to reduce memory barriers amount when updating arguments buffer)
    int32 indirectDrawCallIndex = 0;
    for (int32 index = 0; index < renderModulesIndices.Count(); index++)
    {
        int32 moduleIndex = renderModulesIndices[index];
        auto module = emitter->Graph.RenderModules[moduleIndex];
        switch (module->TypeID)
        {
            // Sprite Rendering
        case 400:
        {
            GPUDrawIndexedIndirectArgs indirectArgsBufferInitData{ SpriteParticleRenderer::IndexCount, 1, 0, 0, 0 };
            const uint32 offset = indirectDrawCallIndex * sizeof(GPUDrawIndexedIndirectArgs);
            context->UpdateBuffer(buffer->GPU.IndirectDrawArgsBuffer, &indirectArgsBufferInitData, sizeof(indirectArgsBufferInitData), offset);
            const uint32 counterOffset = buffer->GPU.ParticleCounterOffset;
            context->CopyBuffer(buffer->GPU.IndirectDrawArgsBuffer, buffer->GPU.Buffer, 4, offset + 4, counterOffset);
            indirectDrawCallIndex++;
            break;
        }
            // Model Rendering
        case 403:
        {
            const auto model = (Model*)module->Assets[0].Get();

            // TODO: model LOD picking for particles?
            int32 lodIndex = 0;
            ModelLOD& lod = model->LODs[lodIndex];
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                Mesh& mesh = lod.Meshes[meshIndex];
                if (!mesh.IsInitialized())
                    continue;

                GPUDrawIndexedIndirectArgs indirectArgsBufferInitData = { (uint32)mesh.GetTriangleCount() * 3, 1, 0, 0, 0 };
                const uint32 offset = indirectDrawCallIndex * sizeof(GPUDrawIndexedIndirectArgs);
                context->UpdateBuffer(buffer->GPU.IndirectDrawArgsBuffer, &indirectArgsBufferInitData, sizeof(indirectArgsBufferInitData), offset);
                const uint32 counterOffset = buffer->GPU.ParticleCounterOffset;
                context->CopyBuffer(buffer->GPU.IndirectDrawArgsBuffer, buffer->GPU.Buffer, 4, offset + 4, counterOffset);
                indirectDrawCallIndex++;
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

    // Execute all rendering modules
    indirectDrawCallIndex = 0;
    for (int32 index = 0; index < renderModulesIndices.Count(); index++)
    {
        int32 moduleIndex = renderModulesIndices[index];
        auto module = emitter->Graph.RenderModules[moduleIndex];
        drawCall.Particle.Module = module;

        switch (module->TypeID)
        {
            // Sprite Rendering
        case 400:
        {
            const auto material = (MaterialBase*)module->Assets[0].Get();
            const auto moduleDrawModes = module->Values.Count() > 3 ? (DrawPass)module->Values[3].AsInt : DrawPass::Default;
            drawCall.Material = material;

            // Submit draw call
            SpriteRenderer.SetupDrawCall(drawCall);
            drawCall.InstanceCount = 0;
            drawCall.Draw.IndirectArgsBuffer = buffer->GPU.IndirectDrawArgsBuffer;
            drawCall.Draw.IndirectArgsOffset = indirectDrawCallIndex * sizeof(GPUDrawIndexedIndirectArgs);
            renderContext.List->AddDrawCall((DrawPass)(drawModes & moduleDrawModes), staticFlags, drawCall, false);
            indirectDrawCallIndex++;

            break;
        }
            // Model Rendering
        case 403:
        {
            const auto model = (Model*)module->Assets[0].Get();
            const auto material = (MaterialBase*)module->Assets[1].Get();
            const auto moduleDrawModes = module->Values.Count() > 4 ? (DrawPass)module->Values[4].AsInt : DrawPass::Default;
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

                // Execute draw call
                mesh.GetDrawCallGeometry(drawCall);
                drawCall.InstanceCount = 0;
                drawCall.Draw.IndirectArgsBuffer = buffer->GPU.IndirectDrawArgsBuffer;
                drawCall.Draw.IndirectArgsOffset = indirectDrawCallIndex * sizeof(GPUDrawIndexedIndirectArgs);
                renderContext.List->AddDrawCall((DrawPass)(drawModes & moduleDrawModes), staticFlags, drawCall, false);
                indirectDrawCallIndex++;
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

#endif

void Particles::DrawParticles(RenderContext& renderContext, ParticleEffect* effect)
{
    // Setup
    auto& view = renderContext.View;
    const auto drawModes = static_cast<DrawPass>(view.Pass & effect->DrawModes);
    if (drawModes == DrawPass::None || SpriteRenderer.Init())
        return;
    Matrix world;
    effect->GetWorld(&world);
    const auto staticFlags = effect->GetStaticFlags();

    // Draw lights
    for (int32 emitterIndex = 0; emitterIndex < effect->Instance.Emitters.Count(); emitterIndex++)
    {
        auto& emitterData = effect->Instance.Emitters[emitterIndex];
        const auto buffer = emitterData.Buffer;
        if (!buffer || (buffer->Mode == ParticlesSimulationMode::CPU && buffer->CPU.Count == 0))
            continue;

        buffer->Emitter->GraphExecutorCPU.Draw(buffer->Emitter, effect, emitterData, renderContext, world);
    }

    // Setup a draw call common data
    DrawCall drawCall;
    drawCall.PerInstanceRandom = effect->GetPerInstanceRandom();
    drawCall.ObjectPosition = world.GetTranslation();

    // Draw all emitters
    for (int32 emitterIndex = 0; emitterIndex < effect->Instance.Emitters.Count(); emitterIndex++)
    {
        auto& emitterData = effect->Instance.Emitters[emitterIndex];
        const auto buffer = emitterData.Buffer;
        if (!buffer)
            continue;
        auto emitter = buffer->Emitter;

        drawCall.World = emitter->SimulationSpace == ParticlesSimulationSpace::World ? Matrix::Identity : world;
        drawCall.WorldDeterminantSign = Math::FloatSelect(drawCall.World.RotDeterminant(), 1, -1);
        drawCall.Particle.Particles = buffer;

        // Check if need to render any module
        RenderModulesIndices renderModulesIndices;
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
        {
            auto module = emitter->Graph.RenderModules[moduleIndex];

            switch (module->TypeID)
            {
                // Sprite Rendering
            case 400:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 3 ? module->Values[3].AsInt : (int32)DrawPass::Default;
                if (!material ||
                    !material->IsReady() ||
                    !material->IsParticle() ||
                    (view.Pass & material->GetDrawModes() & moduleDrawModes) == 0
                )
                    break;
                renderModulesIndices.Add(moduleIndex);
                break;
            }
                // Model Rendering
            case 403:
            {
                const auto model = (Model*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 4 ? module->Values[4].AsInt : (int32)DrawPass::Default;
                if (!model ||
                    !model->IsLoaded() ||
                    !model->CanBeRendered())
                    break;
                const auto material = (MaterialBase*)module->Assets[1].Get();
                if (!material ||
                    !material->IsReady() ||
                    !material->IsParticle() ||
                    (view.Pass & material->GetDrawModes() & moduleDrawModes) == 0
                )
                    break;
                renderModulesIndices.Add(moduleIndex);
                break;
            }
                // Ribbon Rendering
            case 404:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                const auto moduleDrawModes = module->Values.Count() > 6 ? module->Values[6].AsInt : (int32)DrawPass::Default;
                if (!material ||
                    !material->IsReady() ||
                    !material->IsParticle() ||
                    (view.Pass & material->GetDrawModes() & moduleDrawModes) == 0
                )
                    break;
                renderModulesIndices.Add(moduleIndex);
                break;
            }
                // Volumetric Fog Rendering
            case 405:
            {
                const auto material = (MaterialBase*)module->Assets[0].Get();
                if (!material ||
                    !material->IsReady() ||
                    material->GetInfo().Domain != MaterialDomain::VolumeParticle ||
                    (view.Flags & ViewFlags::Fog) == 0
                )
                    break;
                renderModulesIndices.Add(moduleIndex);
                break;
            }
            }
        }
        if (renderModulesIndices.IsEmpty())
            continue;

        // Draw
        switch (buffer->Mode)
        {
        case ParticlesSimulationMode::CPU:
            DrawEmitterCPU(renderContext, buffer, drawCall, drawModes, staticFlags, emitterData, renderModulesIndices);
            break;
#if COMPILE_WITH_GPU_PARTICLES
        case ParticlesSimulationMode::GPU:
            DrawEmitterGPU(renderContext, buffer, drawCall, drawModes, staticFlags, emitterData, renderModulesIndices);
            break;
#endif
        }
    }
}

#if COMPILE_WITH_GPU_PARTICLES

void UpdateGPU(RenderTask* task, GPUContext* context)
{
    ScopeLock lock(GpuUpdateListLocker);
    if (GpuUpdateList.IsEmpty())
        return;

    PROFILE_GPU("GPU Particles");

    for (ParticleEffect* effect : GpuUpdateList)
    {
        auto& instance = effect->Instance;
        const auto particleSystem = effect->ParticleSystem.Get();
        if (!particleSystem || !particleSystem->IsLoaded())
            continue;

        // Update all emitter tracks
        for (int32 j = 0; j < particleSystem->Tracks.Count(); j++)
        {
            const auto& track = particleSystem->Tracks[j];
            if (track.Type != ParticleSystem::Track::Types::Emitter || track.Disabled)
                continue;
            const uint32 emitterIndex = track.AsEmitter.Index;
            auto emitter = particleSystem->Emitters[emitterIndex].Get();
            auto& data = instance.Emitters[emitterIndex];
            if (!emitter || !emitter->IsLoaded() || !data.Buffer || emitter->SimulationMode != ParticlesSimulationMode::GPU)
                continue;
            ASSERT(emitter->Capacity != 0 && emitter->Graph.Layout.Size != 0);

            // TODO: use async context for particles to update them on compute during GBuffer rendering
            emitter->GPU.Execute(context, emitter, effect, emitterIndex, data);
        }
    }
    GpuUpdateList.Clear();

    context->ResetSR();
    context->ResetUA();
    context->FlushState();
}

#endif

ParticleBuffer* Particles::AcquireParticleBuffer(ParticleEmitter* emitter)
{
    ParticleBuffer* result = nullptr;
    ASSERT(emitter && emitter->IsLoaded());

    if (emitter->EnablePooling && EnableParticleBufferPooling)
    {
        PoolLocker.Lock();
        const auto entries = Pool.TryGet(emitter);
        if (entries)
        {
            while (entries->HasItems())
            {
                // Reuse buffer
                result = entries->Last().Buffer;
                entries->RemoveLast();

                // Remove old buffers
                if (result->Version != emitter->Graph.Version)
                {
                    Delete(result);
                    result = nullptr;
                    if (entries->IsEmpty())
                    {
                        Pool.Remove(emitter);
                        break;
                    }
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
    ParticlesDrawCPU::SortingKeys[0].SetCapacity(0);
    ParticlesDrawCPU::SortingKeys[1].SetCapacity(0);
    ParticlesDrawCPU::SortingIndices.SetCapacity(0);
    ParticlesDrawCPU::SortedIndices.SetCapacity(0);
    ParticlesDrawCPU::RibbonTotalDistances.SetCapacity(0);

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
    if (dt <= 1.0f / 240.0f)
        return;
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
        ASSERT(emitter->Capacity != 0 && emitter->Graph.Layout.Size != 0);

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
    PROFILE_CPU_NAMED("Particles.PostExecute");

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
