// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Physics.h"
#include "PhysicsSettings.h"
#include "Engine/Core/Types/Span.h"

struct HingeJointDrive;
struct SpringParameters;
struct LimitLinearRange;
struct LimitAngularRange;
struct LimitConeRange;
struct LimitLinear;
struct D6JointDrive;
enum class HingeJointFlag;
enum class DistanceJointFlag;
enum class SliderJointFlag;
enum class SphericalJointFlag;
enum class D6JointAxis;
enum class D6JointMotion;
enum class D6JointDriveType;
class IPhysicsActor;
class PhysicalMaterial;
class JsonAsset;
class JsonAsset;

struct PhysicsJointDesc
{
    Joint* Joint;
    void* Actor0;
    void* Actor1;
    Quaternion Rot0;
    Quaternion Rot1;
    Vector3 Pos0;
    Vector3 Pos1;
};

struct PhysicsClothDesc
{
    class Cloth* Actor;
    void* VerticesData;
    void* IndicesData;
    float* InvMassesData;
    float* MaxDistancesData;
    uint32 VerticesCount;
    uint32 VerticesStride;
    uint32 IndicesCount;
    uint32 IndicesStride;
    uint32 InvMassesStride;
    uint32 MaxDistancesStride;
};

/// <summary>
/// Interface for the physical simulation backend implementation.
/// </summary>
class FLAXENGINE_API PhysicsBackend
{
public:
    enum class ActorFlags
    {
        None = 0,
        NoGravity = 1 << 0,
        NoSimulation = 1 << 1,
    };

    enum class RigidDynamicFlags
    {
        None = 0,
        Kinematic = 1 << 0,
        CCD = 1 << 1,
    };

    enum class JointFlags
    {
        None = 0,
        Collision = 1 << 0,
    };

    enum class ActionType
    {
        Sleep,
    };

    struct HeightFieldSample
    {
        int16 Height;
        uint8 MaterialIndex0;
        uint8 MaterialIndex1;
    };

    enum class HeightFieldMaterial
    {
        Hole = 127,
    };

public:
    // General
    static bool Init();
    static void Shutdown();
    static void ApplySettings(const PhysicsSettings& settings);

    // Scene
    static void* CreateScene(const PhysicsSettings& settings);
    static void DestroyScene(void* scene);
    static void StartSimulateScene(void* scene, float dt);
    static void EndSimulateScene(void* scene);
    static Vector3 GetSceneGravity(void* scene);
    static void SetSceneGravity(void* scene, const Vector3& value);
    static bool GetSceneEnableCCD(void* scene);
    static void SetSceneEnableCCD(void* scene, bool value);
    static float GetSceneBounceThresholdVelocity(void* scene);
    static void SetSceneBounceThresholdVelocity(void* scene, float value);
    static void SetSceneOrigin(void* scene, const Vector3& oldOrigin, const Vector3& newOrigin);
    static void AddSceneActor(void* scene, void* actor);
    static void RemoveSceneActor(void* scene, void* actor, bool immediately = false);
    static void AddSceneActorAction(void* scene, void* actor, ActionType action);
#if COMPILE_WITH_PROFILER
    static void GetSceneStatistics(void* scene, PhysicsStatistics& result);
#endif

    // Scene Queries
    static bool RayCast(void* scene, const Vector3& origin, const Vector3& direction, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool RayCast(void* scene, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool RayCastAll(void* scene, const Vector3& origin, const Vector3& direction, Array<RayCastHit, HeapAllocation>& results, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool BoxCastAll(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool SphereCast(void* scene, const Vector3& center, float radius, const Vector3& direction, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool SphereCast(void* scene, const Vector3& center, float radius, const Vector3& direction, RayCastHit& hitInfo, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool SphereCastAll(void* scene, const Vector3& center, float radius, const Vector3& direction, Array<RayCastHit, HeapAllocation>& results, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool CapsuleCast(void* scene, const Vector3& center, float radius, float height, const Vector3& direction, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool CapsuleCast(void* scene, const Vector3& center, float radius, float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool CapsuleCastAll(void* scene, const Vector3& center, float radius, float height, const Vector3& direction, Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool ConvexCastAll(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, Array<RayCastHit, HeapAllocation>& results, const Quaternion& rotation, float maxDistance, uint32 layerMask, bool hitTriggers);
    static bool CheckBox(void* scene, const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers);
    static bool CheckSphere(void* scene, const Vector3& center, float radius, uint32 layerMask, bool hitTriggers);
    static bool CheckCapsule(void* scene, const Vector3& center, float radius, float height, const Quaternion& rotation, uint32 layerMask, bool hitTriggers);
    static bool CheckConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation, uint32 layerMask, bool hitTriggers);
    static bool OverlapBox(void* scene, const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers);
    static bool OverlapSphere(void* scene, const Vector3& center, float radius, Array<PhysicsColliderActor*, HeapAllocation>& results, uint32 layerMask, bool hitTriggers);
    static bool OverlapCapsule(void* scene, const Vector3& center, float radius, float height, Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers);
    static bool OverlapConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<PhysicsColliderActor*, HeapAllocation>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers);

    // Actors
    static ActorFlags GetActorFlags(void* actor);
    static void SetActorFlags(void* actor, ActorFlags value);
    static void GetActorBounds(void* actor, BoundingBox& bounds);
    static int32 GetRigidActorShapesCount(void* actor);
    static void* CreateRigidDynamicActor(IPhysicsActor* actor, const Vector3& position, const Quaternion& orientation, void* scene);
    static void* CreateRigidStaticActor(IPhysicsActor* actor, const Vector3& position, const Quaternion& orientation, void* scene);
    static RigidDynamicFlags GetRigidDynamicActorFlags(void* actor);
    static void SetRigidDynamicActorFlags(void* actor, RigidDynamicFlags value);
    static void GetRigidActorPose(void* actor, Vector3& position, Quaternion& orientation);
    static void SetRigidActorPose(void* actor, const Vector3& position, const Quaternion& orientation, bool kinematic = false, bool wakeUp = false);
    static void SetRigidDynamicActorLinearDamping(void* actor, float value);
    static void SetRigidDynamicActorAngularDamping(void* actor, float value);
    static void SetRigidDynamicActorMaxAngularVelocity(void* actor, float value);
    static void SetRigidDynamicActorConstraints(void* actor, RigidbodyConstraints value);
    static Vector3 GetRigidDynamicActorLinearVelocity(void* actor);
    static void SetRigidDynamicActorLinearVelocity(void* actor, const Vector3& value, bool wakeUp);
    static Vector3 GetRigidDynamicActorAngularVelocity(void* actor);
    static void SetRigidDynamicActorAngularVelocity(void* actor, const Vector3& value, bool wakeUp);
    static Vector3 GetRigidDynamicActorCenterOfMass(void* actor);
    static void SetRigidDynamicActorCenterOfMassOffset(void* actor, const Float3& value);
    static bool GetRigidDynamicActorIsSleeping(void* actor);
    static void RigidDynamicActorSleep(void* actor);
    static void RigidDynamicActorWakeUp(void* actor);
    static float GetRigidDynamicActorSleepThreshold(void* actor);
    static void SetRigidDynamicActorSleepThreshold(void* actor, float value);
    static float GetRigidDynamicActorMaxDepenetrationVelocity(void* actor);
    static void SetRigidDynamicActorMaxDepenetrationVelocity(void* actor, float value);
    static void SetRigidDynamicActorSolverIterationCounts(void* actor, int32 minPositionIters, int32 minVelocityIters);
    static void UpdateRigidDynamicActorMass(void* actor, float& mass, float massScale, bool autoCalculate);
    static void AddRigidDynamicActorForce(void* actor, const Vector3& force, ForceMode mode);
    static void AddRigidDynamicActorForceAtPosition(void* actor, const Vector3& force, const Vector3& position, ForceMode mode);
    static void AddRigidDynamicActorTorque(void* actor, const Vector3& torque, ForceMode mode);

    // Shapes
    static void* CreateShape(PhysicsColliderActor* collider, const CollisionShape& geometry, Span<JsonAsset*> materials, bool enabled, bool trigger);
    static void SetShapeState(void* shape, bool enabled, bool trigger);
    static void SetShapeFilterMask(void* shape, uint32 mask0, uint32 mask1);
    static void* GetShapeActor(void* shape);
    static void GetShapePose(void* shape, Vector3& position, Quaternion& orientation);
    static CollisionShape::Types GetShapeType(void* shape);
    static void GetShapeLocalPose(void* shape, Vector3& position, Quaternion& orientation);
    static void SetShapeLocalPose(void* shape, const Vector3& position, const Quaternion& orientation);
    static void SetShapeContactOffset(void* shape, float value);
    static void SetShapeMaterials(void* shape, Span<JsonAsset*> materials);
    static void SetShapeGeometry(void* shape, const CollisionShape& geometry);
    static void AttachShape(void* shape, void* actor);
    static void DetachShape(void* shape, void* actor);
    static bool ComputeShapesPenetration(void* shapeA, void* shapeB, const Vector3& positionA, const Quaternion& orientationA, const Vector3& positionB, const Quaternion& orientationB, Vector3& direction, float& distance);
    static float ComputeShapeSqrDistanceToPoint(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& point, Vector3* closestPoint = nullptr);
    static bool RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance);
    static bool RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance);

    // Joints
    static void SetJointFlags(void* joint, JointFlags value);
    static void SetJointActors(void* joint, void* actors0, void* actor1);
    static void SetJointActorPose(void* joint, const Vector3& position, const Quaternion& orientation, uint8 index);
    static void SetJointBreakForce(void* joint, float force, float torque);
    static void GetJointForce(void* joint, Vector3& linear, Vector3& angular);
    static void* CreateFixedJoint(const PhysicsJointDesc& desc);
    static void* CreateDistanceJoint(const PhysicsJointDesc& desc);
    static void* CreateHingeJoint(const PhysicsJointDesc& desc);
    static void* CreateSliderJoint(const PhysicsJointDesc& desc);
    static void* CreateSphericalJoint(const PhysicsJointDesc& desc);
    static void* CreateD6Joint(const PhysicsJointDesc& desc);
    static void SetDistanceJointFlags(void* joint, DistanceJointFlag flags);
    static void SetDistanceJointMinDistance(void* joint, float value);
    static void SetDistanceJointMaxDistance(void* joint, float value);
    static void SetDistanceJointTolerance(void* joint, float value);
    static void SetDistanceJointSpring(void* joint, const SpringParameters& value);
    static float GetDistanceJointDistance(void* joint);
    static void SetHingeJointFlags(void* joint, HingeJointFlag value, bool driveFreeSpin);
    static void SetHingeJointLimit(void* joint, const LimitAngularRange& value);
    static void SetHingeJointDrive(void* joint, const HingeJointDrive& value);
    static float GetHingeJointAngle(void* joint);
    static float GetHingeJointVelocity(void* joint);
    static void SetSliderJointFlags(void* joint, SliderJointFlag value);
    static void SetSliderJointLimit(void* joint, const LimitLinearRange& value);
    static float GetSliderJointPosition(void* joint);
    static float GetSliderJointVelocity(void* joint);
    static void SetSphericalJointFlags(void* joint, SphericalJointFlag value);
    static void SetSphericalJointLimit(void* joint, const LimitConeRange& value);
    static void SetD6JointMotion(void* joint, D6JointAxis axis, D6JointMotion value);
    static void SetD6JointDrive(void* joint, const D6JointDriveType index, const D6JointDrive& value);
    static void SetD6JointLimitLinear(void* joint, const LimitLinear& value);
    static void SetD6JointLimitTwist(void* joint, const LimitAngularRange& value);
    static void SetD6JointLimitSwing(void* joint, const LimitConeRange& value);
    static Vector3 GetD6JointDrivePosition(void* joint);
    static void SetD6JointDrivePosition(void* joint, const Vector3& value);
    static Quaternion GetD6JointDriveRotation(void* joint);
    static void SetD6JointDriveRotation(void* joint, const Quaternion& value);
    static void GetD6JointDriveVelocity(void* joint, Vector3& linear, Vector3& angular);
    static void SetD6JointDriveVelocity(void* joint, const Vector3& linear, const Vector3& angular);
    static float GetD6JointTwist(void* joint);
    static float GetD6JointSwingY(void* joint);
    static float GetD6JointSwingZ(void* joint);

    // Character Controllers
    static void* CreateController(void* scene, IPhysicsActor* actor, PhysicsColliderActor* collider, float contactOffset, const Vector3& position, float slopeLimit, int32 nonWalkableMode, JsonAsset* material, float radius, float height, float stepOffset, void*& shape);
    static void* GetControllerRigidDynamicActor(void* controller);
    static void SetControllerSize(void* controller, float radius, float height);
    static void SetControllerSlopeLimit(void* controller, float value);
    static void SetControllerNonWalkableMode(void* controller, int32 value);
    static void SetControllerStepOffset(void* controller, float value);
    static Vector3 GetControllerUpDirection(void* controller);
    static void SetControllerUpDirection(void* controller, const Vector3& value);
    static Vector3 GetControllerPosition(void* controller);
    static void SetControllerPosition(void* controller, const Vector3& value);
    static int32 MoveController(void* controller, void* shape, const Vector3& displacement, float minMoveDistance, float deltaTime);

#if WITH_VEHICLE
    // Vehicles
    static void* CreateVehicle(class WheeledVehicle* actor);
    static void DestroyVehicle(void* vehicle, int32 driveType);
    static void UpdateVehicleWheels(WheeledVehicle* actor);
    static void UpdateVehicleAntiRollBars(WheeledVehicle* actor);
    static void SetVehicleEngine(void* vehicle, const void* value);
    static void SetVehicleDifferential(void* vehicle, const void* value);
    static void SetVehicleGearbox(void* vehicle, const void* value);
    static int32 GetVehicleTargetGear(void* vehicle);
    static void SetVehicleTargetGear(void* vehicle, int32 value);
    static int32 GetVehicleCurrentGear(void* vehicle);
    static void SetVehicleCurrentGear(void* vehicle, int32 value);
    static float GetVehicleForwardSpeed(void* vehicle);
    static float GetVehicleSidewaysSpeed(void* vehicle);
    static float GetVehicleEngineRotationSpeed(void* vehicle);
    static void AddVehicle(void* scene, WheeledVehicle* actor);
    static void RemoveVehicle(void* scene, WheeledVehicle* actor);
#endif
    
#if WITH_CLOTH
    // Cloth
    static void* CreateCloth(const PhysicsClothDesc& desc);
    static void DestroyCloth(void* cloth);
    static void SetClothForceSettings(void* cloth, const void* settingsPtr);
    static void SetClothCollisionSettings(void* cloth, const void* settingsPtr);
    static void SetClothSimulationSettings(void* cloth, const void* settingsPtr);
    static void SetClothFabricSettings(void* cloth, const void* settingsPtr);
    static void SetClothTransform(void* cloth, const Transform& transform, bool teleport);
    static void ClearClothInertia(void* cloth);
    static void LockClothParticles(void* cloth);
    static void UnlockClothParticles(void* cloth);
    static Span<const Float4> GetClothParticles(void* cloth);
    static void SetClothParticles(void* cloth, Span<const Float4> value, Span<const Float3> positions, Span<const float> invMasses);
    static void SetClothPaint(void* cloth, Span<const float> value);
    static void AddCloth(void* scene, void* cloth);
    static void RemoveCloth(void* scene, void* cloth);
#endif

    // Resources
    static void* CreateConvexMesh(byte* data, int32 dataSize, BoundingBox& localBounds);
    static void* CreateTriangleMesh(byte* data, int32 dataSize, BoundingBox& localBounds);
    static void* CreateHeightField(byte* data, int32 dataSize);
    static void GetConvexMeshTriangles(void* contextMesh, Array<Float3, HeapAllocation>& vertexBuffer, Array<int32, HeapAllocation>& indexBuffer);
    static void GetTriangleMeshTriangles(void* triangleMesh, Array<Float3, HeapAllocation>& vertexBuffer, Array<int32, HeapAllocation>& indexBuffer);
    static const uint32* GetTriangleMeshRemap(void* triangleMesh, uint32& count);
    static void GetHeightFieldSize(void* heightField, int32& rows, int32& columns);
    static float GetHeightFieldHeight(void* heightField, int32 x, int32 z);
    static HeightFieldSample GetHeightFieldSample(void* heightField, int32 x, int32 z);
    static bool ModifyHeightField(void* heightField, int32 startCol, int32 startRow, int32 cols, int32 rows, const HeightFieldSample* data);
    static void FlushRequests();
    static void FlushRequests(void* scene);
    static void DestroyActor(void* actor);
    static void DestroyShape(void* shape);
    static void DestroyJoint(void* joint);
    static void DestroyController(void* controller);
    static void DestroyMaterial(void* material);
    static void DestroyObject(void* object);
    static void RemoveCollider(PhysicsColliderActor* collider);
    static void RemoveJoint(Joint* joint);

public:
    // Utilities
    FORCE_INLINE static void SetActorFlag(void* actor, ActorFlags flag, bool value)
    {
        auto flags = GetActorFlags(actor);
        flags = (ActorFlags)(((uint32)flags & ~(uint32)flag) | (value ? (uint32)flag : 0));
        SetActorFlags(actor, flags);
    }

    FORCE_INLINE static void SetRigidDynamicActorFlag(void* actor, RigidDynamicFlags flag, bool value)
    {
        auto flags = GetRigidDynamicActorFlags(actor);
        flags = (RigidDynamicFlags)(((uint32)flags & ~(uint32)flag) | (value ? (uint32)flag : 0));
        SetRigidDynamicActorFlags(actor, flags);
    }
    FORCE_INLINE static void* CreateShape(PhysicsColliderActor* collider, const CollisionShape& geometry, JsonAsset* material, bool enabled, bool trigger)
    {
        return CreateShape(collider, geometry, Span<JsonAsset*>(&material, 1), enabled, trigger);
    }
    FORCE_INLINE static void SetShapeMaterial(void* shape, JsonAsset* material)
    {
        SetShapeMaterials(shape, Span<JsonAsset*>(&material, 1));
    }
};

DECLARE_ENUM_OPERATORS(PhysicsBackend::ActorFlags);
DECLARE_ENUM_OPERATORS(PhysicsBackend::RigidDynamicFlags);
DECLARE_ENUM_OPERATORS(PhysicsBackend::JointFlags);
