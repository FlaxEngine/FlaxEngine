// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Visject/Graph.h"
#include "Engine/Content/Content.h"
#include "Engine/Animations/Curve.h"
#include "Engine/Particles/Types.h"
#include "Engine/Particles/ParticlesSimulation.h"
#include "Engine/Particles/ParticlesData.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Types/BaseTypes.h"

class ParticleEffect;

// The root node type identifier
#define PARTICLE_EMITTER_ROOT_NODE_TYPE GRAPH_NODE_MAKE_TYPE(14, 1)

// The maximum amount of particle modules used per context
#define PARTICLE_EMITTER_MAX_MODULES 32

// The maximum amount of used particles attributes per graph node
#define PARTICLE_EMITTER_MAX_ATTRIBUTES_REFS_PER_NODE 4

// The maximum amount of used asset references per graph node
#define PARTICLE_EMITTER_MAX_ASSET_REFS_PER_NODE 8

template<class Base>
class ParticleEmitterGraphNode : public Base
{
public:
    /// <summary>
    /// True if node is used by the particles graph (is connected to the any module input or its a enabled module).
    /// </summary>
    bool Used = false;

    /// <summary>
    /// Flag valid for used particle nodes that need per-particle data to evaluate its value (including dependant nodes linked to input boxes). Used to skip per-particle graph evaluation if graph uses the same value for all particles (eg. is not using per-particle seed or position node).
    /// </summary>
    bool UsesParticleData = false;

    /// <summary>
    /// Flag valid for used particle nodes that result in constant data (nothing random nor particle data).
    /// </summary>
    bool IsConstant = true;

    /// <summary>
    /// The cached particle attribute indices used by the simulation graph to access particle properties.
    /// </summary>
    int32 Attributes[PARTICLE_EMITTER_MAX_ATTRIBUTES_REFS_PER_NODE];
};

void InitParticleEmitterFunctionCall(const Guid& assetId, AssetReference<Asset>& asset, bool& usesParticleData, ParticleLayout& layout);

/// <summary>
/// The Particle Emitter Graph used to simulate particles.
/// </summary>
template<class BaseType, class NodeType, class ValueType>
class ParticleEmitterGraph : public BaseType
{
public:
    typedef ValueType Value;

    enum class ModuleType
    {
        Spawn,
        Initialize,
        Update,
        Render,
    };

protected:
    // Attributes cache
    int32 _attrPosition;
    int32 _attrVelocity;
    int32 _attrRotation;
    int32 _attrAngularVelocity;
    int32 _attrAge;
    int32 _attrLifetime;
    int32 _attrSpriteSize;
    int32 _attrScale;
    int32 _attrMass;
    int32 _attrRibbonWidth;
    int32 _attrColor;
    int32 _attrRadius;

public:
    /// <summary>
    /// The Particle Emitter Graph data version number. Used to sync the Particle Emitter Graph data with the instances state. Handles graph reloads to ensure data is valid.
    /// </summary>
    uint32 Version = 0;

    /// <summary>
    /// The cached root node.
    /// </summary>
    NodeType* Root = nullptr;

    /// <summary>
    /// The particle layout.
    /// </summary>
    ParticleLayout Layout;

    /// <summary>
    /// The particle emitter capacity (max particles limit for the simulation).
    /// </summary>
    int32 Capacity;

    /// <summary>
    /// The particles simulation space.
    /// </summary>
    ParticlesSimulationSpace SimulationSpace;

    /// <summary>
    /// The particle layout attributes default values.
    /// </summary>
    Array<Variant, FixedAllocation<PARTICLE_ATTRIBUTES_MAX_COUNT>> AttributesDefaults;

public:
    /// <summary>
    /// The particles modules for Spawn context.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> SpawnModules;

    /// <summary>
    /// The particles modules for Initialize context.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> InitModules;

    /// <summary>
    /// The particles modules for Update context.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> UpdateModules;

    /// <summary>
    /// The particles modules for Render context.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> RenderModules;

    /// <summary>
    /// The particles modules for lights rendering.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> LightModules;

    /// <summary>
    /// The particles modules for sorting particles.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> SortModules;

    /// <summary>
    /// The particles modules for ribbon particles rendering.
    /// </summary>
    Array<NodeType*, FixedAllocation<PARTICLE_EMITTER_MAX_MODULES>> RibbonRenderingModules;

    bool UsesVolumetricFogRendering = false;

    virtual void InitializeNode(NodeType* node)
    {
        // Skip if already initialized
        if (node->Used)
            return;
        node->Used = true;

#define USE_ATTRIBUTE(name, valueType, slot) \
	{ \
	const StringView name(TEXT(#name)); \
	auto idx = Layout.FindAttribute(name, ParticleAttribute::ValueTypes::valueType); \
	if (idx == -1) \
		idx = Layout.AddAttribute(name, ParticleAttribute::ValueTypes::valueType); \
	static_assert(slot < ARRAY_COUNT(NodeType::Attributes), "Invalid attribute index."); \
	node->Attributes[slot] = idx; \
	}

        switch (node->Type)
        {
        // == Tools ==

        // Get Gameplay Global
        case GRAPH_NODE_MAKE_TYPE(7, 16):
        {
            node->Assets.Resize(1);
            node->Assets[0] = Content::LoadAsync<Asset>((Guid)node->Values[0]);
            break;
        }

        // === Particles ===

        // Particle Attribute
        case GRAPH_NODE_MAKE_TYPE(14, 100):
        case GRAPH_NODE_MAKE_TYPE(14, 303):
        {
            node->UsesParticleData = true;
            const StringView name(node->Values[0]);
            const ParticleAttribute::ValueTypes valueType = (ParticleAttribute::ValueTypes)node->Values[1].AsUint64;
            auto idx = Layout.FindAttribute(name, valueType);
            if (idx == -1)
                idx = Layout.AddAttribute(name, valueType);
            node->Attributes[0] = idx;
            node->Attributes[1] = (int32)valueType;
            static_assert(PARTICLE_EMITTER_MAX_ATTRIBUTES_REFS_PER_NODE >= 2, "Invalid node attributes count. Need more space for some data here.");
            break;
        }
        // Particle Position
        case GRAPH_NODE_MAKE_TYPE(14, 101):
        case GRAPH_NODE_MAKE_TYPE(14, 212):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Position, Float3, 0);
            break;
        }
        // Particle Lifetime
        case GRAPH_NODE_MAKE_TYPE(14, 102):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Lifetime, Float, 0);
            break;
        }
        // Particle Age
        case GRAPH_NODE_MAKE_TYPE(14, 103):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Age, Float, 0);
            break;
        }
        // Particle Color
        case GRAPH_NODE_MAKE_TYPE(14, 104):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Color, Float4, 0);
            break;
        }
        // Particle Velocity
        case GRAPH_NODE_MAKE_TYPE(14, 105):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Velocity, Float3, 0);
            break;
        }
        // Particle Sprite Size
        case GRAPH_NODE_MAKE_TYPE(14, 106):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(SpriteSize, Float2, 0);
            break;
        }
        // Particle Mass
        case GRAPH_NODE_MAKE_TYPE(14, 107):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Mass, Float, 0);
            break;
        }
        // Particle Rotation
        case GRAPH_NODE_MAKE_TYPE(14, 108):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Rotation, Float3, 0);
            break;
        }
        // Particle Angular Velocity
        case GRAPH_NODE_MAKE_TYPE(14, 109):
        {
            USE_ATTRIBUTE(AngularVelocity, Float3, 0);
            break;
        }
        // Particle Normalized Age
        case GRAPH_NODE_MAKE_TYPE(14, 110):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Age, Float, 0);
            USE_ATTRIBUTE(Lifetime, Float, 1);
            break;
        }
        // Particle Radius
        case GRAPH_NODE_MAKE_TYPE(14, 111):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Radius, Float, 0);
            break;
        }
        // Particle Scale
        case GRAPH_NODE_MAKE_TYPE(14, 112):
        {
            node->UsesParticleData = true;
            USE_ATTRIBUTE(Scale, Float3, 0);
            break;
        }
        // Random
        case GRAPH_NODE_MAKE_TYPE(14, 208):
        case GRAPH_NODE_MAKE_TYPE(14, 209):
        case GRAPH_NODE_MAKE_TYPE(14, 210):
        case GRAPH_NODE_MAKE_TYPE(14, 211):
        case GRAPH_NODE_MAKE_TYPE(14, 213):
        case GRAPH_NODE_MAKE_TYPE(14, 214):
        case GRAPH_NODE_MAKE_TYPE(14, 215):
        case GRAPH_NODE_MAKE_TYPE(14, 216):
            node->IsConstant = false;
            break;
        // Particle Emitter Function
        case GRAPH_NODE_MAKE_TYPE(14, 300):
            node->Assets.Resize(1);
            InitParticleEmitterFunctionCall((Guid)node->Values[0], node->Assets[0], node->UsesParticleData, Layout);
            break;
        // Particle Index
        case GRAPH_NODE_MAKE_TYPE(14, 301):
            node->UsesParticleData = true;
            break;

        // === Particle Modules ===

        // Orient Sprite
        case GRAPH_NODE_MAKE_TYPE(15, 201):
        case GRAPH_NODE_MAKE_TYPE(15, 303):
        {
            USE_ATTRIBUTE(SpriteFacingMode, Int, 0);
            if (((ParticleSpriteFacingMode)node->Values[2].AsInt) == ParticleSpriteFacingMode::CustomFacingVector ||
                ((ParticleSpriteFacingMode)node->Values[2].AsInt) == ParticleSpriteFacingMode::FixedAxis)
            {
                USE_ATTRIBUTE(SpriteFacingVector, Float3, 1);
            }
            break;
        }
        // Orient Model
        case GRAPH_NODE_MAKE_TYPE(15, 213):
        case GRAPH_NODE_MAKE_TYPE(15, 309):
        {
            USE_ATTRIBUTE(ModelFacingMode, Int, 0);
            break;
        }
        // Update Age
        case GRAPH_NODE_MAKE_TYPE(15, 300):
        {
            USE_ATTRIBUTE(Age, Float, 0);
            break;
        }
        // Gravity/Force
        case GRAPH_NODE_MAKE_TYPE(15, 301):
        case GRAPH_NODE_MAKE_TYPE(15, 304):
        {
            USE_ATTRIBUTE(Velocity, Float3, 0);
            break;
        }
        // Linear Drag
        case GRAPH_NODE_MAKE_TYPE(15, 310):
        {
            USE_ATTRIBUTE(Velocity, Float3, 0);
            USE_ATTRIBUTE(Mass, Float, 1);
            if (node->Values[3].AsBool)
            {
                USE_ATTRIBUTE(SpriteSize, Float2, 2);
            }
            break;
        }
        // Turbulence
        case GRAPH_NODE_MAKE_TYPE(15, 311):
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Velocity, Float3, 1);
            USE_ATTRIBUTE(Mass, Float, 2);
            break;
        }
        // Position (plane/box surface/box volume/cylinder/line/sphere/circle/disc/torus/Global SDF)
        case GRAPH_NODE_MAKE_TYPE(15, 202):
        case GRAPH_NODE_MAKE_TYPE(15, 203):
        case GRAPH_NODE_MAKE_TYPE(15, 204):
        case GRAPH_NODE_MAKE_TYPE(15, 205):
        case GRAPH_NODE_MAKE_TYPE(15, 206):
        case GRAPH_NODE_MAKE_TYPE(15, 207):
        case GRAPH_NODE_MAKE_TYPE(15, 208):
        case GRAPH_NODE_MAKE_TYPE(15, 209):
        case GRAPH_NODE_MAKE_TYPE(15, 210):
        case GRAPH_NODE_MAKE_TYPE(15, 211):
        case GRAPH_NODE_MAKE_TYPE(15, 215):
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            break;
        }
        // Position (depth)
        case GRAPH_NODE_MAKE_TYPE(15, 212):
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Lifetime, Float, 1);
            break;
        }
        // Position (spiral)
        case GRAPH_NODE_MAKE_TYPE(15, 214):
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Velocity, Float3, 1);
            break;
        }
        // Set Attribute
        case GRAPH_NODE_MAKE_TYPE(15, 200):
        case GRAPH_NODE_MAKE_TYPE(15, 302):
        {
            const StringView name(node->Values[2]);
            const ParticleAttribute::ValueTypes valueType = (ParticleAttribute::ValueTypes)node->Values[3].AsInt;
            auto idx = Layout.FindAttribute(name, valueType);
            if (idx == -1)
                idx = Layout.AddAttribute(name, valueType);
            node->Attributes[0] = idx;
            break;
        }
        // Set Position/Lifetime/Age/..
#define CASE_SET_PARTICLE_ATTRIBUTE(id0, id1, name, type) case GRAPH_NODE_MAKE_TYPE(15, id0): case GRAPH_NODE_MAKE_TYPE(15, id1): USE_ATTRIBUTE(name, type, 0); break
        CASE_SET_PARTICLE_ATTRIBUTE(250, 350, Position, Float3);
        CASE_SET_PARTICLE_ATTRIBUTE(251, 351, Lifetime, Float);
        CASE_SET_PARTICLE_ATTRIBUTE(252, 352, Age, Float);
        CASE_SET_PARTICLE_ATTRIBUTE(253, 353, Color, Float4);
        CASE_SET_PARTICLE_ATTRIBUTE(254, 354, Velocity, Float3);
        CASE_SET_PARTICLE_ATTRIBUTE(255, 355, SpriteSize, Float2);
        CASE_SET_PARTICLE_ATTRIBUTE(256, 356, Mass, Float);
        CASE_SET_PARTICLE_ATTRIBUTE(257, 357, Rotation, Float3);
        CASE_SET_PARTICLE_ATTRIBUTE(258, 358, AngularVelocity, Float3);
        CASE_SET_PARTICLE_ATTRIBUTE(259, 359, Scale, Float3);
        CASE_SET_PARTICLE_ATTRIBUTE(260, 360, RibbonWidth, Float);
        CASE_SET_PARTICLE_ATTRIBUTE(261, 361, RibbonTwist, Float);
        CASE_SET_PARTICLE_ATTRIBUTE(262, 362, RibbonFacingVector, Float3);
        CASE_SET_PARTICLE_ATTRIBUTE(263, 363, Radius, Float);
#undef CASE_SET_PARTICLE_ATTRIBUTE
        // Conform to Sphere
        case GRAPH_NODE_MAKE_TYPE(15, 305):
        case GRAPH_NODE_MAKE_TYPE(15, 335): // Conform to Global SDF
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Velocity, Float3, 1);
            USE_ATTRIBUTE(Mass, Float, 2);
            break;
        }
        // Kill (sphere/box)
        case GRAPH_NODE_MAKE_TYPE(15, 306):
        case GRAPH_NODE_MAKE_TYPE(15, 307):
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            break;
        }
        // Collision (plane/sphere/box/cylinder/depth/Global SDF)
        case GRAPH_NODE_MAKE_TYPE(15, 330):
        case GRAPH_NODE_MAKE_TYPE(15, 331):
        case GRAPH_NODE_MAKE_TYPE(15, 332):
        case GRAPH_NODE_MAKE_TYPE(15, 333):
        case GRAPH_NODE_MAKE_TYPE(15, 334):
        case GRAPH_NODE_MAKE_TYPE(15, 336):
        {
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Velocity, Float3, 1);
            USE_ATTRIBUTE(Age, Float, 2);
            break;
        }
        // Sprite Rendering
        case GRAPH_NODE_MAKE_TYPE(15, 400):
        {
            node->Assets.Resize(1);
            node->Assets[0] = Content::LoadAsync<Asset>((Guid)node->Values[2]);
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Rotation, Float3, 1);
            USE_ATTRIBUTE(SpriteSize, Float2, 2);
            break;
        }
        // Sort
        case GRAPH_NODE_MAKE_TYPE(15, 402):
        {
            const auto sortMode = static_cast<ParticleSortMode>(node->Values[2].AsInt);
            if (sortMode == ParticleSortMode::CustomAscending || sortMode == ParticleSortMode::CustomDescending)
            {
                const StringView name(node->Values[3]);
                auto idx = Layout.FindAttribute(name);
                if (idx == -1)
                {
                    LOG(Warning, "Particles sort module uses missing particle attribute {0}.", name.Get());
                }
                else
                {
                    switch (Layout.Attributes[idx].ValueType)
                    {
                    case ParticleAttribute::ValueTypes::Float:
                    case ParticleAttribute::ValueTypes::Int:
                    case ParticleAttribute::ValueTypes::Uint:
                        break;
                    default:
                        LOG(Warning, "Particles sort module uses invalid particle attribute {0} of type {1}. It has to be a scalar value.", name.Get(), (int32)Layout.Attributes[idx].ValueType);
                    }
                }
                node->Attributes[0] = idx;
            }
            break;
        }
        // Model Rendering
        case GRAPH_NODE_MAKE_TYPE(15, 403):
        {
            node->Assets.Resize(2);
            node->Assets[0] = Content::LoadAsync<Asset>((Guid)node->Values[2]);
            node->Assets[1] = Content::LoadAsync<Asset>((Guid)node->Values[3]);
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Rotation, Float3, 1);
            USE_ATTRIBUTE(Scale, Float3, 2);
            break;
        }
        // Ribbon Rendering
        case GRAPH_NODE_MAKE_TYPE(15, 404):
        {
            node->Assets.Resize(1);
            node->Assets[0] = Content::LoadAsync<Asset>((Guid)node->Values[2]);
            USE_ATTRIBUTE(Position, Float3, 0);
            // TODO: add support for custom sorting key - not only by age
            USE_ATTRIBUTE(Age, Float, 1);
            break;
        }
        // Volumetric Fog Rendering
        case GRAPH_NODE_MAKE_TYPE(15, 405):
        {
            node->Assets.Resize(1);
            node->Assets[0] = Content::LoadAsync<Asset>((Guid)node->Values[2]);
            USE_ATTRIBUTE(Position, Float3, 0);
            USE_ATTRIBUTE(Radius, Float, 1);
            break;
        }
        }

        // Check all input boxes
        for (int32 i = 0; i < node->Boxes.Count(); i++)
        {
            auto box = node->Boxes[i];
            for (int32 j = 0; j < box.Connections.Count(); j++)
            {
                const auto other = box.Connections[j]->template GetParent<NodeType>();
                InitializeNode(other);

                // Propagate UsesParticleData from the input boxes
                if (other->Used)
                {
                    node->UsesParticleData |= other->UsesParticleData;
                    node->IsConstant &= other->IsConstant;
                }
            }
        }

#undef USE_ATTRIBUTE
    }

public:
    // [Graph]
    void Clear() override
    {
        // Clear cached data
        Root = nullptr;
        Layout.Clear();
        SpawnModules.Clear();
        InitModules.Clear();
        UpdateModules.Clear();
        RenderModules.Clear();
        LightModules.Clear();
        SortModules.Clear();
        RibbonRenderingModules.Clear();
        UsesVolumetricFogRendering = false;

        // Base
        BaseType::Clear();
    }

    bool Load(ReadStream* stream, bool loadMeta) override
    {
        // Bump up version on reload
        Version++;

        // Base
        if (BaseType::Load(stream, loadMeta))
            return true;

        // Compute particle data layout and initialize used nodes (for only used nodes, start depth searching rom the modules)
        //Layout.AddAttribute(TEXT("Position"), ParticleAttribute::ValueTypes::Float3);
#define PROCESS_MODULES(modules) for (int32 i = 0; i < modules.Count(); i++) { modules[i]->Used = false; InitializeNode(modules[i]); }
        PROCESS_MODULES(SpawnModules);
        PROCESS_MODULES(InitModules);
        PROCESS_MODULES(UpdateModules);
        PROCESS_MODULES(RenderModules);
#undef PROCESS_MODULES
        Layout.UpdateLayout();
        AttributesDefaults.Resize(Layout.Attributes.Count());

        // Ensure that spawn modules are not using particle data (not supported)
        for (int32 i = SpawnModules.Count() - 1; i >= 0; --i)
        {
            if (SpawnModules[i]->UsesParticleData)
            {
                LOG(Warning, "Particle spawn module uses particle data as an input which is invalid. Disabling spawn module.");
                SpawnModules.RemoveAtKeepOrder(i);
            }
        }

        // Peek the root node options
        Capacity = 0;
        if (Root && Root->Values.Count() > 3)
        {
            Capacity = Root->Values[0].AsInt;
            SimulationSpace = (ParticlesSimulationSpace)Root->Values[2].AsInt;
        }

        // Cache common attributes and initialize defaults
        for (int32 i = 0; i < AttributesDefaults.Count(); i++)
            AttributesDefaults[i] = Variant::Zero;
        int32 idx;
#define SETUP_ATTRIBUTE(name, type, defaultValue) \
            idx = Layout.FindAttribute(TEXT(MACRO_TO_STR(name)), ParticleAttribute::ValueTypes::type); \
            _attr##name = idx; \
            if (idx != -1) \
                AttributesDefaults[idx] = defaultValue
        SETUP_ATTRIBUTE(Position, Float3, Variant(Float3::Zero));
        SETUP_ATTRIBUTE(Velocity, Float3, Variant(Float3::Zero));
        SETUP_ATTRIBUTE(Rotation, Float3, Variant(Float3::Zero));
        SETUP_ATTRIBUTE(AngularVelocity, Float3, Variant(Float3::Zero));
        SETUP_ATTRIBUTE(Age, Float, Variant::Zero);
        SETUP_ATTRIBUTE(Lifetime, Float, Variant(5.0f));
        SETUP_ATTRIBUTE(SpriteSize, Float2, Variant(Float2(50.0f)));
        SETUP_ATTRIBUTE(Scale, Float3, Variant(Float3::One));
        SETUP_ATTRIBUTE(Mass, Float, Variant(1.0f));
        SETUP_ATTRIBUTE(RibbonWidth, Float, Variant(10.0f));
        SETUP_ATTRIBUTE(Color, Float4, Variant(Float4(0.0f, 0.0f, 0.0f, 1.0f)));
        SETUP_ATTRIBUTE(Radius, Float, Variant(100.0f));
#undef SETUP_ATTRIBUTE

        return false;
    }

    bool onNodeLoaded(NodeType* n) override
    {
        // Root node
        if (n->Type == PARTICLE_EMITTER_ROOT_NODE_TYPE)
        {
            ASSERT(!Root);
            Root = n;
        }
        // Particle Modules (only if module is enabled)
        else if (n->GroupID == 15 && n->Values[0].AsBool)
        {
            const auto moduleType = static_cast<ModuleType>(n->Values[1].AsInt);
            switch (moduleType)
            {
            case ModuleType::Spawn:
                SpawnModules.Add(n);
                break;
            case ModuleType::Initialize:
                InitModules.Add(n);
                break;
            case ModuleType::Update:
                UpdateModules.Add(n);
                break;
            case ModuleType::Render:
                RenderModules.Add(n);
                switch (n->TypeID)
                {
                case 401:
                    LightModules.Add(n);
                    break;
                case 402:
                    if (static_cast<ParticleSortMode>(n->Values[2].AsInt) != ParticleSortMode::None)
                        SortModules.Add(n);
                    break;
                case 404:
                    RibbonRenderingModules.Add(n);
                    break;
                case 405:
                    UsesVolumetricFogRendering = true;
                    break;
                }
                break;
            }
        }

        return BaseType::onNodeLoaded(n);
    }
};
