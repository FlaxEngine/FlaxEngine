// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../ParticleEmitterGraph.h"
#include "Engine/Particles/ParticlesSimulation.h"
#include "Engine/Particles/ParticlesData.h"
#include "Engine/Visject/VisjectGraph.h"

class ParticleEffect;
class ParticleEmitterGraphCPU;
class ParticleEmitterGraphCPUBase;
class ParticleEmitterGraphCPUNode;
class ParticleEmitterGraphCPUExecutor;

// The root node type identifier
#define PARTICLE_EMITTER_ROOT_NODE_TYPE GRAPH_NODE_MAKE_TYPE(14, 1)

// The maximum amount of particle modules used per context
#define PARTICLE_EMITTER_MAX_MODULES 32

// The maximum amount of used particles attributes per graph node
#define PARTICLE_EMITTER_MAX_ATTRIBUTES_REFS_PER_NODE 4

// The maximum amount of used asset references per graph node
#define PARTICLE_EMITTER_MAX_ASSET_REFS_PER_NODE 8

#define PARTICLE_EMITTER_MAX_CALL_STACK 100

class ParticleEmitterGraphCPUBox : public VisjectGraphBox
{
public:

    ParticleEmitterGraphCPUBox()
    {
    }

    ParticleEmitterGraphCPUBox(void* parent, byte id, VariantType type)
        : VisjectGraphBox(parent, id, type)
    {
    }
};

class ParticleEmitterGraphCPUNode : public ParticleEmitterGraphNode<VisjectGraphNode<ParticleEmitterGraphCPUBox>>
{
public:

    /// <summary>
    /// The sorted indices buffer offset used by the rendering modules to point the sorted indices buffer start to use for rendering.
    /// </summary>
    uint32 SortedIndicesOffset;

    union
    {
        /// <summary>
        /// The spiral position module progress value.
        /// </summary>
        float SpiralModuleProgress;

        struct
        {
            int32 RibbonOrderOffset;
        } Ribbon;
    };

public:

    /// <summary>
    /// True if this node uses the per-particle data resolve instead of optimized whole-collection fetch.
    /// </summary>
    /// <returns>True if use per particle data resolve, otherwise can optimize resolve pass.</returns>
    FORCE_INLINE bool UsePerParticleDataResolve() const
    {
        return UsesParticleData || !IsConstant;
    }
};

/// <summary>
/// The Particle Emitter Graph used to simulate CPU particles.
/// </summary>
class ParticleEmitterGraphCPU : public ParticleEmitterGraph<VisjectGraph<ParticleEmitterGraphCPUNode, ParticleEmitterGraphCPUBox, ParticleSystemParameter>, ParticleEmitterGraphCPUNode, Variant>
{
    friend ParticleEmitterGraphCPUExecutor;
private:

    Array<byte> _defaultParticleData;

public:

    /// <summary>
    /// Creates the default surface graph (the main root node) for the particle emitter. Ensure to dispose the previous graph data before.
    /// </summary>
    void CreateDefault();

public:

    /// <summary>
    /// Determines whenever this emitter uses lights rendering.
    /// </summary>
    /// <returns>True if emitter uses lights rendering, otherwise false.</returns>
    FORCE_INLINE bool UsesLightRendering() const
    {
        return LightModules.HasItems();
    }

    /// <summary>
    /// Gets the position attribute offset from the particle data layout start (in bytes).
    /// </summary>
    /// <returns>The offset in bytes.</returns>
    FORCE_INLINE int32 GetPositionAttributeOffset() const
    {
        return _attrPosition != -1 ? Layout.Attributes[_attrPosition].Offset : -1;
    }

    /// <summary>
    /// Gets the age attribute offset from the particle data layout start (in bytes).
    /// </summary>
    /// <returns>The offset in bytes.</returns>
    FORCE_INLINE int32 GetAgeAttributeOffset() const
    {
        return _attrAge != -1 ? Layout.Attributes[_attrAge].Offset : -1;
    }

public:

    // [ParticleEmitterGraph]
    bool Load(ReadStream* stream, bool loadMeta) override;

    bool onNodeLoaded(Node* n) override
    {
        ParticleEmitterGraph::onNodeLoaded(n);
        return VisjectGraph::onNodeLoaded(n);
    }

protected:

    // [ParticleEmitterGraph]
    void InitializeNode(Node* node) override;
};

/// <summary>
/// The Particle Emitter Graph simulation on a CPU.
/// </summary>
class ParticleEmitterGraphCPUExecutor : public VisjectExecutor
{
private:

    // Runtime
    ParticleEmitterGraphCPU& _graph;
    float _deltaTime;
    uint32 _particleIndex;
    ParticleEmitterInstance* _data;
    ParticleEmitter* _emitter;
    ParticleEffect* _effect;
    class SceneRenderTask* _viewTask;
    Array<Node*, FixedAllocation<PARTICLE_EMITTER_MAX_CALL_STACK>> _callStack;
    Array<Graph*, FixedAllocation<32>> _graphStack;
    Dictionary<Node*, ParticleEmitterGraphCPU*> _functions;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ParticleEmitterGraphCPUExecutor"/> class.
    /// </summary>
    /// <param name="graph">The graph to execute.</param>
    explicit ParticleEmitterGraphCPUExecutor(ParticleEmitterGraphCPU& graph);

    /// <summary>
    /// Finalizes an instance of the <see cref="ParticleEmitterGraphCPUExecutor"/> class.
    /// </summary>
    ~ParticleEmitterGraphCPUExecutor();

    /// <summary>
    /// Computes the local bounds of the particle emitter instance.
    /// </summary>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    /// <param name="result">The result bounding box.</param>
    /// <returns>True if has valid bounds, otherwise false if failed to calculate it (eg. GPU emitter or not synced or no particles.</returns>
    bool ComputeBounds(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, BoundingBox& result);

    /// <summary>
    /// Draw the particles (eg. lights).
    /// </summary>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="transform">The effect transform matrix.</param>
    void Draw(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, RenderContext& renderContext, Matrix& transform);

    /// <summary>
    /// Updates the particles simulation (the CPU simulation).
    /// </summary>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    /// <param name="dt">The delta time (in seconds).</param>
    /// <param name="canSpawn">True if can spawn new particles, otherwise will just perform an update.</param>
    void Update(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt, bool canSpawn);

    /// <summary>
    /// Updates the particles spawning logic (the non-CPU simulation that needs to spawn particles) and returns the amount of particles to add to the simulation.
    /// </summary>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    /// <param name="dt">The simulation delta time (in seconds).</param>
    /// <returns>The particles to spawn count</returns>
    int32 UpdateSpawn(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt);

private:

    Value eatBox(Node* caller, Box* box) override;
    Graph* GetCurrentGraph() const override;

    void ProcessGroupParameters(Box* box, Node* node, Value& value);
    void ProcessGroupTextures(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupParticles(Box* box, Node* nodeBase, Value& value);
    void ProcessGroupFunction(Box* box, Node* node, Value& value);

    int32 ProcessSpawnModule(int32 index);
    void ProcessModule(ParticleEmitterGraphCPUNode* node, int32 particlesStart, int32 particlesEnd);

    Value TryGetValue(Box* box, int32 defaultValueBoxIndex, const Value& defaultValue)
    {
        const auto parentNode = box->GetParent<Node>();
        if (box->HasConnection())
            return eatBox(parentNode, box->FirstConnection());
        if (parentNode->Values.Count() > defaultValueBoxIndex)
            return parentNode->Values[defaultValueBoxIndex];
        return defaultValue;
    }

    Value GetValue(Box* box, int32 defaultValueBoxIndex)
    {
        const auto parentNode = box->GetParent<Node>();
        if (box->HasConnection())
            return eatBox(parentNode, box->FirstConnection());
        return parentNode->Values[defaultValueBoxIndex];
    }

    FORCE_INLINE Value TryGetValue(Box* box, const Value& defaultValue)
    {
        return box && box->HasConnection() ? eatBox(box->GetParent<Node>(), box->FirstConnection()) : defaultValue;
    }
};
