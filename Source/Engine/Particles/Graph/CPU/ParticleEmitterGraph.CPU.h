// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../ParticleEmitterGraph.h"
#include "Engine/Particles/ParticlesSimulation.h"
#include "Engine/Particles/ParticlesData.h"
#include "Engine/Visject/VisjectGraph.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Threading/ThreadLocal.h"

struct RenderContext;
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
        int32 CustomDataOffset;
        int32 RibbonOrderOffset;
    };

    /// <summary>
    /// True if this node uses the per-particle data resolve instead of optimized whole-collection fetch.
    /// </summary>
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
    typedef ParticleEmitterGraph<VisjectGraph<ParticleEmitterGraphCPUNode, ParticleEmitterGraphCPUBox, ParticleSystemParameter>, ParticleEmitterGraphCPUNode, Variant> Base;
private:
    struct NodeState
    {
        union
        {
            int32 SpiralProgress;
        };
    };

    Array<byte> _defaultParticleData;

public:
    // Size of the custom pre-node data buffer used for state tracking (eg. position on spiral arc progression).
    int32 CustomDataSize = 0;

    /// <summary>
    /// Creates the default surface graph (the main root node) for the particle emitter. Ensure to dispose the previous graph data before.
    /// </summary>
    void CreateDefault();

    /// <summary>
    /// Gets the position attribute offset from the particle data layout start (in bytes).
    /// </summary>
    FORCE_INLINE int32 GetPositionAttributeOffset() const
    {
        return _attrPosition != -1 ? Layout.Attributes[_attrPosition].Offset : -1;
    }

    /// <summary>
    /// Gets the age attribute offset from the particle data layout start (in bytes).
    /// </summary>
    FORCE_INLINE int32 GetAgeAttributeOffset() const
    {
        return _attrAge != -1 ? Layout.Attributes[_attrAge].Offset : -1;
    }

public:
    // [ParticleEmitterGraph]
    bool Load(ReadStream* stream, bool loadMeta) override;
    void InitializeNode(Node* node) override;
};

/// <summary>
/// The CPU particles emitter graph evaluation context.
/// </summary>
struct ParticleEmitterGraphCPUContext
{
    float DeltaTime;
    uint32 ParticleIndex;
    ParticleEmitterInstance* Data;
    ParticleEmitter* Emitter;
    ParticleEffect* Effect;
    class SceneRenderTask* ViewTask;
    Array<ParticleEmitterGraphCPU*, FixedAllocation<32>> GraphStack;
    Dictionary<VisjectExecutor::Node*, ParticleEmitterGraphCPU*> Functions;
    byte AttributesRemappingTable[PARTICLE_ATTRIBUTES_MAX_COUNT]; // Maps node attribute indices to the current particle layout (used to support accessing particle data from function graph which has different layout).
    int32 CallStackSize = 0;
    VisjectExecutor::Node* CallStack[PARTICLE_EMITTER_MAX_CALL_STACK];
};

/// <summary>
/// The Particle Emitter Graph simulation on a CPU.
/// </summary>
class ParticleEmitterGraphCPUExecutor : public VisjectExecutor
{
private:
    ParticleEmitterGraphCPU& _graph;

    // Per-thread context to allow async execution
    static ThreadLocal<ParticleEmitterGraphCPUContext*> Context;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ParticleEmitterGraphCPUExecutor"/> class.
    /// </summary>
    /// <param name="graph">The graph to execute.</param>
    explicit ParticleEmitterGraphCPUExecutor(ParticleEmitterGraphCPU& graph);

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

#if USE_EDITOR
    /// <summary>
    /// Draws the particles debug shapes.
    /// </summary>
    /// <param name="emitter">The owning emitter.</param>
    /// <param name="effect">The instance effect.</param>
    /// <param name="data">The instance data.</param>
    void DrawDebug(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data);
#endif

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
    void Init(ParticleEmitter* emitter, ParticleEffect* effect, ParticleEmitterInstance& data, float dt = 0.0f);
    Value eatBox(Node* caller, Box* box) override;
    Graph* GetCurrentGraph() const override;

    void ProcessGroupParameters(Box* box, Node* node, Value& value);
    void ProcessGroupTextures(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupParticles(Box* box, Node* nodeBase, Value& value);
    void ProcessGroupFunction(Box* box, Node* node, Value& value);

    int32 ProcessSpawnModule(int32 index);
    void ProcessModule(ParticleEmitterGraphCPUNode* node, int32 particlesStart, int32 particlesEnd);
#if USE_EDITOR
    void DebugDrawModule(ParticleEmitterGraphCPUNode* node, const Transform& transform);
#endif

    FORCE_INLINE Value GetValue(Box* box, int32 defaultValueBoxIndex)
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
