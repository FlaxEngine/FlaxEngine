// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

/// <summary>
/// Current GPU particles emitter shader version.
/// </summary>
#define PARTICLE_GPU_GRAPH_VERSION 11

#if COMPILE_WITH_PARTICLE_GPU_GRAPH

#include "../ParticleEmitterGraph.h"
#include "Engine/Visject/ShaderGraph.h"
#include "Engine/Visject/ShaderGraphValue.h"

typedef ShaderGraphBox ParticleEmitterGraphGPUBox;

class ParticleEmitterGraphGPUNode : public ParticleEmitterGraphNode<ShaderGraphNode<>>
{
public:
    ParticleEmitterGraphGPUNode()
        : ParticleEmitterGraphNode<ShaderGraphNode<>>()
    {
    }

    /// <summary>
    /// The asset references. Linked resources such as Animation assets are referenced in graph data as ID. We need to keep valid refs to them at runtime to keep data in memory.
    /// </summary>
    Array<AssetReference<Asset>> Assets;
};

/// <summary>
/// The Particle Emitter Graph used to generate shader for GPU particles simulation.
/// </summary>
/// <seealso cref="ParticleEmitterGraph" />
class ParticleEmitterGraphGPU : public ParticleEmitterGraph<ShaderGraph<ParticleEmitterGraphGPUNode, ParticleEmitterGraphGPUBox, ParticleSystemParameter>, ParticleEmitterGraphGPUNode, ShaderGraphValue>
{
public:
    /// <summary>
    /// Clears all the cached values.
    /// </summary>
    void ClearCache();

    bool onNodeLoaded(Node* n) override
    {
        ParticleEmitterGraph::onNodeLoaded(n);
        return ShaderGraph::onNodeLoaded(n);
    }
};

/// <summary>
/// The GPU shader source generator tool.
/// </summary>
class ParticleEmitterGPUGenerator : public ShaderGenerator
{
private:
    enum class ParticleContextType
    {
        Initialize,
        Update,
    };

    enum class AccessMode
    {
        None = 0,
        Read = 1,
        Write = 2,
        ReadWrite = Read | Write,
    };

    struct AttributeCache
    {
        Value Variable;
        AccessMode Access = AccessMode::None;
    };

    int32 _customDataSize;
    bool _contextUsesKill;
    Array<AttributeCache> _attributeValues;
    ParticleContextType _contextType;
    Array<ParticleEmitterGraphGPU*, InlinedAllocation<16>> _graphs;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ParticleEmitterGPUGenerator"/> class.
    /// </summary>
    ParticleEmitterGPUGenerator();

    /// <summary>
    /// Finalizes an instance of the <see cref="ParticleEmitterGPUGenerator"/> class.
    /// </summary>
    ~ParticleEmitterGPUGenerator();

    /// <summary>
    /// Gets the root graph.
    /// </summary>
    FORCE_INLINE ParticleEmitterGraphGPU* GetRootGraph() const
    {
        return _graphs.First();
    }

    /// <summary>
    /// Adds a new graph to the generator data (will be deleted after usage).
    /// </summary>
    /// <param name="graph">The graph to add.</param>
    void AddGraph(ParticleEmitterGraphGPU* graph);

    /// <summary>
    /// Generates GPU particles simulation source code (first graph should be the base one).
    /// </summary>
    /// <param name="source">The output source code.</param>
    /// <param name="parametersData">The output material parameters data.</param>
    /// <param name="customDataSize">The output custom data size (in bytes) required by the nodes to store the additional global state for the simulation in the particles buffer on a GPU.</param>
    /// <returns>True if cannot generate code, otherwise false.</returns>
    bool Generate(WriteStream& source, BytesContainer& parametersData, int32& customDataSize);

private:
    void clearCache();

    void ProcessModule(Node* node);

    void ProcessGroupParameters(Box* box, Node* node, Value& value);
    void ProcessGroupTextures(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupParticles(Box* box, Node* node, Value& value);
    void ProcessGroupFunction(Box* box, Node* node, Value& value);

    Parameter* findGraphParam(const Guid& id);
    bool sampleSceneTexture(Node* caller, Box* box, const SerializedMaterialParam& texture, Value& result);
    bool loadTexture(Node* caller, Box* box, const SerializedMaterialParam& texture, Value& result);
    void sampleSceneDepth(Node* caller, Value& value, Box* box);
    void linearizeSceneDepth(Node* caller, const Value& depth, Value& value);

    Value AccessParticleAttribute(Node* caller, const StringView& name, ParticleAttribute::ValueTypes valueType, AccessMode mode);
    Value AccessParticleAttribute(Node* caller, int32 index, AccessMode mode);
    void WriteParticleAttributesWrites();
    void PrepareGraph(ParticleEmitterGraphGPU* graph);
    void UseKill();
    void WriteReturnOnKill();

    Value GetValue(Box* box, int32 defaultValueBoxIndex)
    {
        const auto parentNode = box->GetParent<Node>();
        if (box->HasConnection())
            return eatBox(parentNode, box->FirstConnection());
        return Value(parentNode->Values[defaultValueBoxIndex]);
    }

    Value GetValue(Box* box)
    {
        return box->HasConnection() ? eatBox(box->GetParent<Node>(), box->FirstConnection()) : Value::Zero;
    }

    bool IsLocalSimulationSpace() const
    {
        return GetRootGraph()->SimulationSpace == ParticlesSimulationSpace::Local;
    }

    bool IsWorldSimulationSpace() const
    {
        return GetRootGraph()->SimulationSpace == ParticlesSimulationSpace::World;
    }
};

#endif
