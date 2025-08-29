// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Level/Actors/ModelInstanceActor.h"
#include "IPhysicsDebug.h"

// Used internally to validate cloth data against invalid nan/inf values
#define USE_CLOTH_SANITY_CHECKS 0

/// <summary>
/// Physical simulation actor for cloth objects made of vertices that are simulated as cloth particles with physical properties, forces, and constraints to affect cloth behavior. 
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Cloth\"), ActorToolbox(\"Physics\")") class FLAXENGINE_API Cloth : public Actor
#if USE_EDITOR
    , public IPhysicsDebug
#endif
{
    DECLARE_SCENE_OBJECT(Cloth);

    /// <summary>
    /// Cloth response to forces settings.
    /// </summary>
    API_STRUCT() struct ForceSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(ForceSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Scale multiplier applied to the gravity of cloth particles (scales the global gravity force).
        /// </summary>
        API_FIELD() float GravityScale = 1.0f;

        /// <summary>
        /// Damping of cloth particle velocity. 0: velocity is unaffected. 1: velocity is zeroed.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float Damping = 0.4f;

        /// <summary>
        /// Portion of velocity applied to cloth particles. 0: cloth particles are unaffected. 1: damped global cloth particle velocity.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float LinearDrag = 0.2f;

        /// <summary>
        /// Portion of angular velocity applied to turning cloth particles. 0: cloth particles are unaffected. 1: damped global cloth particle angular velocity.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float AngularDrag = 0.2f;

        /// <summary>
        /// Portion of linear acceleration applied to cloth particles. 0: cloth particles are unaffected. 1: physically correct linear acceleration.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float LinearInertia = 1.0f;

        /// <summary>
        /// Portion of angular acceleration applied to turning cloth particles. 0: cloth particles are unaffected. 1: physically correct angular acceleration.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float AngularInertia = 1.0f;

        /// <summary>
        /// Portion of angular velocity applied to turning cloth particles. 0: cloth particles are unaffected. 1: physically correct angular velocity.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float CentrifugalInertia = 1.0f;

        /// <summary>
        /// Defines how much drag air applies to the cloth particles. Set to 0 to disable wind.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float AirDragCoefficient = 0.02f;

        /// <summary>
        /// Defines how much lift air applies to the cloth particles. Set to 0 to disable wind.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float AirLiftCoefficient = 0.02f;

        /// <summary>
        /// Defines fluid density of air used for drag and lift calculations.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float AirDensity = 1.0f;
    };

    /// <summary>
    /// Cloth response to collisions settings.
    /// </summary>
    API_STRUCT() struct CollisionSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(CollisionSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Controls the amount of friction between cloth particles and colliders. 0: friction disabled.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float Friction = 0.1f;

        /// <summary>
        /// Controls how quickly cloth particle mass is increased during collisions. 0: mass scale disabled.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float MassScale = 0.0f;

        /// <summary>
        /// Enables collisions with scene geometry (both dynamic and static). Disable this to improve performance of cloth that doesn't need to collide.
        /// </summary>
        API_FIELD() bool SceneCollisions = true;

        /// <summary>
        /// Enables Continuous Collision Detection (CCD) that improves collision by computing the time of impact between cloth particles and colliders. The increase in quality can impact performance.
        /// </summary>
        API_FIELD() bool ContinuousCollisionDetection = false;

        /// <summary>
        /// Additional thickness of the simulated cloth to prevent intersections with nearby colliders.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float CollisionThickness = 1.0f;

        /// <summary>
        /// The minimum distance that the colliding cloth particles must maintain from each other in meters. 0: self collision disabled.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float SelfCollisionDistance = 0.0f;

        /// <summary>
        /// Stiffness for the self collision constraints. 0: self collision disabled.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float SelfCollisionStiffness = 0.2f;
    };

    /// <summary>
    /// Cloth simulation settings.
    /// </summary>
    API_STRUCT() struct SimulationSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(SimulationSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Target cloth solver iterations per second. The executed number of iterations per second may vary dependent on many performance factors. However, at least one iteration per frame is solved regardless of the value set.
        /// </summary>
        API_FIELD() float SolverFrequency = 200.0f;

        /// <summary>
        /// The maximum distance from the camera at which to run cloth simulation. Used to improve performance and skip updating too far clothes. The physics system might reduce the update rate for clothes far enough (eg. half this distance). 0 to disable any culling.
        /// </summary>
        API_FIELD() float CullDistance = 5000.0f;

        /// <summary>
        /// If true, the cloth will be updated even when an actor cannot be seen by any camera. Otherwise, the cloth simulation will stop running when the actor is off-screen.
        /// </summary>
        API_FIELD() bool UpdateWhenOffscreen = false;

        /// <summary>
        /// The maximum distance cloth particles can move from the original location (within local-space of the actor). Scaled by painted per-particle value (0-1) to restrict movement of certain particles.
        /// </summary>
        API_FIELD() float MaxParticleDistance = 1000.0f;

        /// <summary>
        /// Enables automatic normal vectors computing for the cloth mesh, otherwise original mesh normals will be used.
        /// </summary>
        API_FIELD() bool ComputeNormals = true;

        /// <summary>
        /// Wind velocity vector (direction and magnitude) in world coordinates. A greater magnitude applies a stronger wind force. Ensure that Air Drag and Air Lift coefficients are non-zero in order to apply wind force.
        /// </summary>
        API_FIELD() Vector3 WindVelocity = Vector3::Zero;
    };

    /// <summary>
    /// Cloth's fabric settings (material's stiffness and compression response) for a single axis.
    /// </summary>
    API_STRUCT() struct FabricAxisSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(FabricAxisSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Stiffness value for stretch and compression constraints. 0: disables it.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float Stiffness = 1.0f;

        /// <summary>
        /// Scale value for stretch and compression constraints. 0: no stretch and compression constraints applied. 1: fully apply stretch and compression constraints.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float StiffnessMultiplier = 1.0f;

        /// <summary>
        /// Compression limit for constraints.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float CompressionLimit = 1.0f;

        /// <summary>
        /// Stretch limit for constraints.
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float StretchLimit = 1.0f;
    };

    /// <summary>
    /// Cloth's fabric settings (material's stiffness and compression response).
    /// </summary>
    API_STRUCT() struct FabricSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(FabricStiffnessSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Vertical constraints for stretching or compression (along the gravity).
        /// </summary>
        API_FIELD() FabricAxisSettings Vertical;

        /// <summary>
        /// Horizontal constraints for stretching or compression (perpendicular to the gravity).
        /// </summary>
        API_FIELD() FabricAxisSettings Horizontal;

        /// <summary>
        /// Bending constraints for out-of-plane bending in angle-based formulation.
        /// </summary>
        API_FIELD() FabricAxisSettings Bending;

        /// <summary>
        /// Shearing constraints for plane shearing along (typically) diagonal edges.
        /// </summary>
        API_FIELD() FabricAxisSettings Shearing;
    };

private:
    void* _cloth = nullptr;
    Real _lastMinDstSqr = MAX_Real;
    uint32 _frameCounter = 0;
    int32 _sceneRenderingKey = -1;
    ForceSettings _forceSettings;
    CollisionSettings _collisionSettings;
    SimulationSettings _simulationSettings;
    FabricSettings _fabricSettings;
    Vector3 _cachedPosition = Vector3::Zero;
    ModelInstanceActor::MeshReference _mesh;
    MeshDeformation* _meshDeformation = nullptr;
    Array<float> _paint;

public:
    /// <summary>
    /// Gets the native physics backend object.
    /// </summary>
    void* GetPhysicsCloth() const;

    /// <summary>
    /// Gets the mesh to use for the cloth simulation (single mesh from specific LOD). Always from the parent static or animated model actor.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(0), EditorDisplay(\"Cloth\")")
    ModelInstanceActor::MeshReference GetMesh() const;

    /// <summary>
    /// Sets the mesh to use for the cloth simulation (single mesh from specific LOD). Always from the parent static or animated model actor.
    /// </summary>
    API_PROPERTY() void SetMesh(const ModelInstanceActor::MeshReference& value);

    /// <summary>
    /// Gets the cloth response to forces settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), EditorDisplay(\"Cloth\")")
    FORCE_INLINE ForceSettings GetForce() const
    {
        return _forceSettings;
    }

    /// <summary>
    /// Sets the cloth response to forces settings.
    /// </summary>
    API_PROPERTY() void SetForce(const ForceSettings& value);

    /// <summary>
    /// Gets the cloth response to collisions settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(20), EditorDisplay(\"Cloth\")")
    FORCE_INLINE CollisionSettings GetCollision() const
    {
        return _collisionSettings;
    }

    /// <summary>
    /// Sets the cloth response to collisions settings.
    /// </summary>
    API_PROPERTY() void SetCollision(const CollisionSettings& value);

    /// <summary>
    /// Gets the cloth simulation settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(30), EditorDisplay(\"Cloth\")")
    FORCE_INLINE SimulationSettings GetSimulation() const
    {
        return _simulationSettings;
    }

    /// <summary>
    /// Sets the cloth simulation settings.
    /// </summary>
    API_PROPERTY() void SetSimulation(const SimulationSettings& value);

    /// <summary>
    /// Gets the cloth's fabric settings (material's stiffness and compression response).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), EditorDisplay(\"Cloth\")")
    FORCE_INLINE FabricSettings GetFabric() const
    {
        return _fabricSettings;
    }

    /// <summary>
    /// Sets the cloth's fabric settings (material's stiffness and compression response).
    /// </summary>
    API_PROPERTY() void SetFabric(const FabricSettings& value);

public:
    /// <summary>
    /// Recreates the cloth by removing current instance data and creating a new physical cloth object.
    /// </summary>
    API_FUNCTION() void Rebuild();

    /// <summary>
    /// Sets the inertia derived from transform change to zero (once). It will reset any cloth object movement effects as it was teleported.
    /// </summary>
    API_FUNCTION() void ClearInteria();

    /// <summary>
    /// Gets the cloth particles data with per-particle XYZ position (in local cloth-space).
    /// </summary>
    API_FUNCTION() Array<Float3> GetParticles() const;

    /// <summary>
    /// Sets the cloth particles data with per-particle XYZ position (in local cloth-space). The size of the input data had to match the cloth size.
    /// </summary>
    API_FUNCTION() void SetParticles(Span<const Float3> value);

    /// <summary>
    /// Gets the cloth particles paint data with per-particle max distance (normalized 0-1, 0 makes particle immovable). Returned value is empty if cloth was not initialized or doesn't use paint feature.
    /// </summary>
    API_FUNCTION() Span<float> GetPaint() const;

    /// <summary>
    /// Sets the cloth particles paint data with per-particle max distance (normalized 0-1, 0 makes particle immovable). The size of the input data had to match the cloth size. Set to empty to remove paint.
    /// </summary>
    API_FUNCTION() void SetPaint(Span<const float> value);

    bool OnPreUpdate();
    void OnPostUpdate();

private:
#if USE_EDITOR
    API_FIELD(Internal) bool DebugDrawDepthTest = true;
#endif

public:
    // [Actor]
    void Draw(RenderContext& renderContext) override;
    void Draw(RenderContextBatch& renderContextBatch) override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnDeleteObject() override;
    void OnParentChanged() override;
    void OnTransformChanged() override;
    void OnPhysicsSceneChanged(PhysicsScene* previous) override;

private:
    ImplementPhysicsDebug;
    bool CreateCloth();
    void DestroyCloth();
    void CalculateInvMasses(Array<float>& invMasses);
    void RunClothDeformer(const MeshBase* mesh, struct MeshDeformationData& deformation);
};
