// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ParticleEmitterGraph.CPU.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Particles/ParticleEffect.h"
#include "Engine/Engine/Time.h"

namespace
{
    bool SortRibbonParticles(const int32& a, const int32& b, ParticleBufferCPUDataAccessor<float>* data)
    {
        return data->Get(a) < data->Get(b);
    }
}

void ParticleEmitterGraphCPU::CreateDefault()
{
    // Create node
    const auto rootNode = &Nodes.AddOne();
    rootNode->ID = 1;
    rootNode->Type = PARTICLE_EMITTER_ROOT_NODE_TYPE;
    rootNode->Values.Resize(6);
    rootNode->Values[0] = 1000; // Capacity
    rootNode->Values[1] = (int32)ParticlesSimulationMode::Default; // Simulation Mode
    rootNode->Values[2] = (int32)ParticlesSimulationSpace::Local; // Simulation Space
    rootNode->Values[3] = true; // Enable Pooling
    rootNode->Values[4] = Variant(BoundingBox(Vector3(-1000.0f), Vector3(1000.0f))); // Custom Bounds
    rootNode->Values[5] = true; // Use Auto Bounds

    // Mark as root
    Root = rootNode;
}

bool ParticleEmitterGraphCPU::Load(ReadStream* stream, bool loadMeta)
{
    // Base
    if (ParticleEmitterGraph::Load(stream, loadMeta))
        return true;

    // Assign the offset in the sorted indices buffer to the rendering modules
    uint32 lastSortModuleSortedIndicesOffset = 0xFFFFFFFF;
    uint32 sortedIndicesOffset = 0;
    for (int32 i = 0; i < RenderModules.Count(); i++)
    {
        const auto module = RenderModules[i];
        module->SortedIndicesOffset = lastSortModuleSortedIndicesOffset;

        if (module->TypeID == 402 && (ParticleSortMode)module->Values[2].AsInt != ParticleSortMode::None)
        {
            // Allocate sorted indices buffer space for sorting modules
            lastSortModuleSortedIndicesOffset = sortedIndicesOffset;
            module->SortedIndicesOffset = sortedIndicesOffset;
            sortedIndicesOffset += Capacity * sizeof(int32);
        }
    }

    // Assign ribbon modules offset in the sorted ribbon particles indices buffer
    int32 ribbonOrderOffset = 0;
    for (int32 i = 0; i < RibbonRenderingModules.Count(); i++)
    {
        const auto module = RibbonRenderingModules[i];
        module->Ribbon.RibbonOrderOffset = ribbonOrderOffset;
        ribbonOrderOffset += Capacity;
    }

    // Initialize default particle data
    _defaultParticleData.Resize(Layout.Size);
    for (int32 i = 0; i < Layout.Attributes.Count(); i++)
    {
        const auto& attr = Layout.Attributes[i];
        switch (attr.ValueType)
        {
#define SETUP_ATTR(type, valueType, getter) \
        case ParticleAttribute::ValueTypes::valueType: \
            *(type*)(_defaultParticleData.Get() + attr.Offset) = AttributesDefaults[i].getter; \
            break
        SETUP_ATTR(float, Float, AsFloat);
        SETUP_ATTR(Vector2, Vector2, AsVector2());
        SETUP_ATTR(Vector3, Vector3, AsVector3());
        SETUP_ATTR(Vector4, Vector4, AsVector4());
        SETUP_ATTR(int32, Int, AsInt);
        SETUP_ATTR(uint32, Uint, AsUint);
#undef SETUP_ATTR
        default: ;
        }
    }

    return false;
}

void ParticleEmitterGraphCPU::InitializeNode(Node* node)
{
    // Skip if already initialized
    if (node->Used)
        return;

    // Base
    ParticleEmitterGraph::InitializeNode(node);

    switch (node->Type)
    {
        // Position (spiral)
    case GRAPH_NODE_MAKE_TYPE(15, 214):
    {
        node->SpiralModuleProgress = 0.0f;
        break;
    }
    }
}

ParticleEmitterGraphCPUExecutor::ParticleEmitterGraphCPUExecutor(ParticleEmitterGraphCPU& graph)
    : _graph(graph)
{
    _perGroupProcessCall[5] = (ProcessBoxHandler)&ParticleEmitterGraphCPUExecutor::ProcessGroupTextures;
    _perGroupProcessCall[6] = (ProcessBoxHandler)&ParticleEmitterGraphCPUExecutor::ProcessGroupParameters;
    _perGroupProcessCall[7] = (ProcessBoxHandler)&ParticleEmitterGraphCPUExecutor::ProcessGroupTools;
    _perGroupProcessCall[14] = (ProcessBoxHandler)&ParticleEmitterGraphCPUExecutor::ProcessGroupParticles;
    _perGroupProcessCall[16] = (ProcessBoxHandler)&ParticleEmitterGraphCPUExecutor::ProcessGroupFunction;
}

ParticleEmitterGraphCPUExecutor::~ParticleEmitterGraphCPUExecutor()
{
    _functions.ClearDelete();
}

bool ParticleEmitterGraphCPUExecutor::ComputeBounds(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, BoundingBox& result)
{
    // CPU particles bounds calculation
    if (emitter->SimulationMode == ParticlesSimulationMode::CPU &&
        emitter->UseAutoBounds &&
        data.Version == _graph.Version &&
        data.Buffer &&
        data.Buffer->CPU.Count != 0 &&
        _graph._attrPosition != -1)
    {
        const int32 count = data.Buffer->CPU.Count;
        byte* bufferPtr = data.Buffer->CPU.Buffer.Get();
        auto layout = data.Buffer->Layout;
        const int32 stride = data.Buffer->Stride;

        // Build sphere bounds out of all living particles positions
        byte* positionPtr = bufferPtr + layout->Attributes[_graph._attrPosition].Offset;
#if 0
        BoundingSphere sphere(*((Vector3*)positionPtr), 0.0f);
        for (int32 particleIndex = 0; particleIndex < count; particleIndex++)
        {
            BoundingSphere::Merge(sphere, *((Vector3*)positionPtr), &sphere);
            positionPtr += stride;
        }
#endif
#if 0
        BoundingSphere sphere;
        {
            // Find the center of all points
            Vector3 center = Vector3::Zero;
            for (int32 i = 0; i < count; i++)
            {
                Vector3::Add(*((Vector3*)positionPtr), center, &center);
                positionPtr += stride;
            }
            center /= static_cast<float>(count);

            // Find the radius of the sphere
            float radius = 0.0f;
            positionPtr = bufferPtr + layout->Attributes[_attrPosition].Offset;
            for (int32 i = 0; i < count; i++)
            {
                // We are doing a relative distance comparison to find the maximum distance from the center of our sphere
                const float distance = Vector3::DistanceSquared(center, *(Vector3*)positionPtr);
                positionPtr += stride;

                if (distance > radius)
                    radius = distance;
            }
            radius = Math::Sqrt(radius);

            // Construct the sphere
            sphere.Center = center;
            sphere.Radius = radius;
        }
#endif
#if 1
        BoundingSphere sphere;
        {
            BoundingBox box = BoundingBox::Empty;
            for (int32 particleIndex = 0; particleIndex < count; particleIndex++)
            {
                Vector3 position = *(Vector3*)positionPtr;
#if ENABLE_ASSERTION
                if (!position.IsNanOrInfinity())
#endif
                {
                    Vector3::Min(box.Minimum, position, box.Minimum);
                    Vector3::Max(box.Maximum, position, box.Maximum);
                }
                positionPtr += stride;
            }
            BoundingSphere::FromBox(box, sphere);
#if ENABLE_ASSERTION
            if (isnan(sphere.Radius) || isinf(sphere.Radius) || sphere.Center.IsNanOrInfinity())
            {
                // Handle issues with data
                sphere.Center = _graph.SimulationSpace == ParticlesSimulationSpace::Local ? Vector3::Zero : effect->GetPosition();
                sphere.Radius = 1000000000.0f;
            }
#endif
        }
#endif
        ASSERT(!isnan(sphere.Radius) && !isinf(sphere.Radius) && !sphere.Center.IsNanOrInfinity());

        // Expand sphere based on the render modules rules (sprite or mesh size)
        for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.RenderModules.Count(); moduleIndex++)
        {
            const auto module = emitter->Graph.RenderModules[moduleIndex];
            switch (module->TypeID)
            {
                // Sprite Rendering
            case 400:
            {
                if (_graph._attrSpriteSize != -1)
                {
                    // Find the maximum local bounds of the particle sprite
                    Vector2 maxSpriteSize = Vector2::Zero;
                    byte* spriteSize = bufferPtr + layout->Attributes[_graph._attrSpriteSize].Offset;
                    for (int32 i = 0; i < count; i++)
                    {
                        Vector2::Max(*((Vector2*)spriteSize), maxSpriteSize, maxSpriteSize);
                        spriteSize += stride;
                    }
                    ASSERT(!maxSpriteSize.IsNanOrInfinity());

                    // Enlarge the emitter bounds sphere
                    sphere.Radius += maxSpriteSize.MaxValue();
                }
                break;
            }
                // Light Rendering
            case 401:
            {
                // Prepare graph data
                _graphStack.Clear();
                _graphStack.Push((Graph*)&_graph);
                _data = &data;
                _emitter = emitter;
                _effect = effect;
                _deltaTime = 0.0f;
                _viewTask = PARTICLE_EMITTER_GET_VIEW_TASK(effect);
                _callStack.Clear();

                // Find the maximum radius of the particle light
                float maxRadius = 0.0f;
                for (int32 particleIndex = 0; particleIndex < count; particleIndex++)
                {
                    _particleIndex = particleIndex;
                    const float radius = (float)GetValue(module->GetBox(1), 3);
                    if (radius > maxRadius)
                        maxRadius = radius;
                }
                ASSERT(!isnan(maxRadius) && !isinf(maxRadius));

                // Enlarge the emitter bounds sphere
                sphere.Radius += maxRadius;

                break;
            }
                // Model Rendering
            case 403:
            {
                const auto modelAsset = (Model*)module->Assets[0].Get();
                if (!modelAsset ||
                    !modelAsset->IsLoaded() ||
                    !modelAsset->CanBeRendered())
                    break;

                if (_graph._attrScale != -1)
                {
                    // Find the maximum local bounds of the particle model
                    Vector3 maxScale = Vector3::Zero;
                    byte* scale = bufferPtr + layout->Attributes[_graph._attrScale].Offset;
                    for (int32 i = 0; i < count; i++)
                    {
                        Vector3::Max(*((Vector3*)scale), maxScale, maxScale);
                        scale += stride;
                    }

                    // Enlarge the emitter bounds sphere
                    BoundingBox box = modelAsset->GetBox();
                    BoundingSphere bounds;
                    BoundingSphere::FromBox(box, bounds);
                    sphere.Radius += maxScale.MaxValue() * bounds.Radius;
                }
                break;
            }
                // Ribbon Rendering
            case 404:
            {
                if (_graph._attrRibbonWidth != -1)
                {
                    // Find the maximum ribbon width of the particle
                    float maxRibbonWidth = 0.0f;
                    byte* ribbonWidth = bufferPtr + layout->Attributes[_graph._attrRibbonWidth].Offset;
                    for (int32 i = 0; i < count; i++)
                    {
                        maxRibbonWidth = Math::Max(*((float*)ribbonWidth), maxRibbonWidth);
                        ribbonWidth += stride;
                    }
                    ASSERT(!isnan(maxRibbonWidth) && !isinf(maxRibbonWidth));

                    // Enlarge the emitter bounds sphere
                    sphere.Radius += maxRibbonWidth * 0.5f;
                }
                break;
            }
            }
        }

        // Convert sphere into bounding box
        BoundingBox::FromSphere(sphere, result);
        return true;
    }

    if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
    {
        result = emitter->CustomBounds;
    }
    else
    {
        Matrix world;
        effect->GetLocalToWorldMatrix(world);
        BoundingBox::Transform(emitter->CustomBounds, world, result);
    }
    return true;
}

void ParticleEmitterGraphCPUExecutor::Draw(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, RenderContext& renderContext, Matrix& transform)
{
    if (!emitter->IsUsingLights || _graph._attrPosition == -1)
        return;

    // Prepare particles buffer access
    auto buffer = data.Buffer;
    byte* positionPtr = buffer->CPU.Buffer.Get() + buffer->Layout->Attributes[_graph._attrPosition].Offset;
    const int32 count = buffer->CPU.Count;
    const int32 stride = buffer->Stride;

    // Prepare graph data
    _graphStack.Clear();
    _graphStack.Push((Graph*)&_graph);
    _data = &data;
    _emitter = emitter;
    _effect = effect;
    _deltaTime = 0.0f;
    _viewTask = PARTICLE_EMITTER_GET_VIEW_TASK(effect);
    _callStack.Clear();

    // Draw lights
    for (int32 moduleIndex = 0; moduleIndex < emitter->Graph.LightModules.Count(); moduleIndex++)
    {
        const auto module = emitter->Graph.LightModules[moduleIndex];
        ASSERT(module->TypeID == 401);

        RendererPointLightData lightData;
        lightData.MinRoughness = 0.04f;
        lightData.ShadowsDistance = 2000.0f;
        lightData.ShadowsStrength = 1.0f;
        lightData.Direction = Vector3::Forward;
        lightData.ShadowsFadeDistance = 50.0f;
        lightData.ShadowsNormalOffsetScale = 10.0f;
        lightData.ShadowsDepthBias = 0.5f;
        lightData.ShadowsSharpness = 1.0f;
        lightData.UseInverseSquaredFalloff = false;
        lightData.VolumetricScatteringIntensity = 1.0f;
        lightData.CastVolumetricShadow = false;
        lightData.RenderedVolumetricFog = 0;
        lightData.ShadowsMode = ShadowsCastingMode::None;
        lightData.SourceRadius = 0.0f;
        lightData.SourceLength = 0.0f;
        lightData.IESTexture = nullptr;

        for (int32 particleIndex = 0; particleIndex < count; particleIndex++)
        {
            _particleIndex = particleIndex;

            const Vector4 color = (Vector4)GetValue(module->GetBox(0), 2);
            const float radius = (float)GetValue(module->GetBox(1), 3);
            const float fallOffExponent = (float)GetValue(module->GetBox(2), 4);

            lightData.Position = *(Vector3*)positionPtr;
            lightData.Color = Vector3(color) * color.W;
            lightData.Radius = radius;
            lightData.FallOffExponent = fallOffExponent;

            if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                Vector3::Transform(lightData.Position, transform, lightData.Position);

            renderContext.List->PointLights.Add(lightData);

            positionPtr += stride;
        }
    }
}

void ParticleEmitterGraphCPUExecutor::Update(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt, bool canSpawn)
{
    // Prepare data
    _graphStack.Clear();
    _graphStack.Push((Graph*)&_graph);
    _data = &data;
    _emitter = emitter;
    _effect = effect;
    _particleIndex = 0;
    _deltaTime = dt;
    _viewTask = PARTICLE_EMITTER_GET_VIEW_TASK(effect);
    _callStack.Clear();
    auto& cpu = data.Buffer->CPU;

    // Update particles
    if (cpu.Count > 0)
    {
        for (int32 i = 0; i < _graph.UpdateModules.Count(); i++)
        {
            ProcessModule(_graph.UpdateModules[i], 0, cpu.Count);
        }
    }

    // Dead particles removal
    if (_graph._attrAge != -1 && _graph._attrLifetime != -1)
    {
        byte* agePtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrAge].Offset;
        byte* lifetimePtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrLifetime].Offset;
        for (int32 particleIndex = 0; particleIndex < cpu.Count; particleIndex++)
        {
            if (*(float*)agePtr >= *(float*)lifetimePtr)
            {
                cpu.Count--;
                Platform::MemoryCopy(_data->Buffer->GetParticleCPU(particleIndex), _data->Buffer->GetParticleCPU(cpu.Count), _data->Buffer->Stride);
                particleIndex--;
            }
            else
            {
                agePtr += _data->Buffer->Stride;
                lifetimePtr += _data->Buffer->Stride;
            }
        }
    }

#if BUILD_DEBUG && 0
    // Debug validation for NANs in data
    if (_graph._attrPosition != -1)
    {
        byte* positionPtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrPosition].Offset;
        for (int32 particleIndex = 0; particleIndex < cpu.Count; particleIndex++)
        {
            Vector3 pos = *((Vector3*)positionPtr);
            ASSERT(!pos.IsNanOrInfinity());
            positionPtr += _data->Buffer->Stride;
        }
    }
#endif

    // Euler integration
    if (_graph._attrPosition != -1 && _graph._attrVelocity != -1)
    {
        byte* positionPtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrPosition].Offset;
        byte* velocityPtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrVelocity].Offset;
        for (int32 particleIndex = 0; particleIndex < cpu.Count; particleIndex++)
        {
            *((Vector3*)positionPtr) += *((Vector3*)velocityPtr) * _deltaTime;
            positionPtr += _data->Buffer->Stride;
            velocityPtr += _data->Buffer->Stride;
        }
    }

    // Angular Euler Integration
    if (_graph._attrRotation != -1 && _graph._attrAngularVelocity != -1)
    {
        byte* rotationPtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrRotation].Offset;
        byte* angularVelocityPtr = cpu.Buffer.Get() + _data->Buffer->Layout->Attributes[_graph._attrAngularVelocity].Offset;
        for (int32 particleIndex = 0; particleIndex < cpu.Count; particleIndex++)
        {
            *((Vector3*)rotationPtr) += *((Vector3*)angularVelocityPtr) * _deltaTime;
            rotationPtr += _data->Buffer->Stride;
            angularVelocityPtr += _data->Buffer->Stride;
        }
    }

    // Spawn particles
    int32 spawnCount = 0;
    if (canSpawn)
    {
        for (int32 i = 0; i < _graph.SpawnModules.Count(); i++)
        {
            spawnCount += ProcessSpawnModule(i);
        }

        const int32 countBefore = data.Buffer->CPU.Count;
        const int32 countAfter = Math::Min(data.Buffer->CPU.Count + spawnCount, data.Buffer->Capacity);
        spawnCount = countAfter - countBefore;
        if (spawnCount != 0)
        {
            // Spawn particles
            data.Buffer->CPU.Count = countAfter;

            // Initialize particles data
            //Platform::MemoryClear(data.Buffer->GetParticleCPU(countBefore), spawnCount * data.Buffer->Stride);
            for (int32 i = 0; i < spawnCount; i++)
                Platform::MemoryCopy(data.Buffer->GetParticleCPU(countBefore + i), _graph._defaultParticleData.Get(), data.Buffer->Stride);

            // Initialize particles
            for (int32 i = 0; i < _graph.InitModules.Count(); i++)
            {
                ProcessModule(_graph.InitModules[i], countBefore, countAfter);
            }
        }
    }

    if (_graph.RibbonRenderingModules.HasItems())
    {
        // Sort ribbon particles
        if (cpu.RibbonOrder.IsEmpty())
        {
            cpu.RibbonOrder.Resize(_graph.RibbonRenderingModules.Count() * _data->Buffer->Capacity);
        }
        ASSERT(cpu.RibbonOrder.Count() == _graph.RibbonRenderingModules.Count() * _data->Buffer->Capacity);
        for (int32 i = 0; i < _graph.RibbonRenderingModules.Count(); i++)
        {
            const auto module = _graph.RibbonRenderingModules[i];

            ParticleBufferCPUDataAccessor<float> sortKeyData(_data->Buffer, emitter->Graph.Layout.GetAttributeOffset(module->Attributes[1]));

            int32* ribbonOrderData = cpu.RibbonOrder.Get() + module->Ribbon.RibbonOrderOffset;

            for (int32 j = 0; j < cpu.Count; j++)
            {
                ribbonOrderData[j] = j;
            }

            if (sortKeyData.IsValid())
            {
                Sorting::SortArray(ribbonOrderData, cpu.Count, SortRibbonParticles, &sortKeyData);
            }
        }
    }

    // Cleanup
    _data = nullptr;
}

int32 ParticleEmitterGraphCPUExecutor::UpdateSpawn(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt)
{
    // Prepare data
    _graphStack.Clear();
    _graphStack.Push((Graph*)&_graph);
    _data = &data;
    _emitter = emitter;
    _effect = effect;
    _particleIndex = 0;
    _deltaTime = dt;
    _viewTask = PARTICLE_EMITTER_GET_VIEW_TASK(effect);
    _callStack.Clear();

    // Spawn particles
    int32 spawnCount = 0;
    for (int32 i = 0; i < _graph.SpawnModules.Count(); i++)
    {
        spawnCount += ProcessSpawnModule(i);
    }

    // Cleanup
    _data = nullptr;

    return spawnCount;
}

VisjectExecutor::Value ParticleEmitterGraphCPUExecutor::eatBox(Node* caller, Box* box)
{
    // Check if graph is looped or is too deep
    if (_callStack.Count() >= PARTICLE_EMITTER_MAX_CALL_STACK)
    {
        OnError(caller, box, TEXT("Graph is looped or too deep!"));
        return Value::Zero;
    }
#if !BUILD_RELEASE
    if (box == nullptr)
    {
        OnError(caller, box, TEXT("Null graph box!"));
        return Value::Zero;
    }
#endif

    // Add to the calling stack
    _callStack.Add(caller);

    // Call per group custom processing event
    Value value;
    const auto parentNode = box->GetParent<Node>();
    const ProcessBoxHandler func = _perGroupProcessCall[parentNode->GroupID];
    (this->*func)(box, parentNode, value);

    // Remove from the calling stack
    _callStack.RemoveLast();

    return value;
}

VisjectExecutor::Graph* ParticleEmitterGraphCPUExecutor::GetCurrentGraph() const
{
    return _graphStack.Peek();
}
