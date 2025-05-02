// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_EMPTY_PHYSICS

#include "Engine/Core/Log.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/CollisionCooking.h"
#include "Engine/Physics/Actors/IPhysicsActor.h"
#include "Engine/Physics/Joints/Limits.h"
#include "Engine/Physics/Joints/DistanceJoint.h"
#include "Engine/Physics/Joints/HingeJoint.h"
#include "Engine/Physics/Joints/SliderJoint.h"
#include "Engine/Physics/Joints/SphericalJoint.h"
#include "Engine/Physics/Joints/D6Joint.h"
#include "Engine/Physics/Colliders/Collider.h"

#define DUMY_HANDLE ((void*)1)

void* PhysicalMaterial::GetPhysicsMaterial()
{
    return DUMY_HANDLE;
}

void PhysicalMaterial::UpdatePhysicsMaterial()
{
}

#if COMPILE_WITH_PHYSICS_COOKING

bool CollisionCooking::CookConvexMesh(CookingInput& input, BytesContainer& output)
{
    LOG(Error, "No physics.");
    return true;
}

bool CollisionCooking::CookTriangleMesh(CookingInput& input, BytesContainer& output)
{
    LOG(Error, "No physics.");
    return true;
}

bool CollisionCooking::CookHeightField(int32 cols, int32 rows, const PhysicsBackend::HeightFieldSample* data, WriteStream& stream)
{
    LOG(Error, "No physics.");
    return true;
}

#endif

bool PhysicsBackend::Init()
{
    LOG(Info, "No physics.");
    return false;
}

void PhysicsBackend::Shutdown()
{
}

void PhysicsBackend::ApplySettings(const PhysicsSettings& settings)
{
}

void* PhysicsBackend::CreateScene(const PhysicsSettings& settings)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::DestroyScene(void* scene)
{
}

void PhysicsBackend::StartSimulateScene(void* scene, float dt)
{
}

void PhysicsBackend::EndSimulateScene(void* scene)
{
}

Vector3 PhysicsBackend::GetSceneGravity(void* scene)
{
    return Vector3::Zero;
}

void PhysicsBackend::SetSceneGravity(void* scene, const Vector3& value)
{
}

bool PhysicsBackend::GetSceneEnableCCD(void* scene)
{
    return false;
}

void PhysicsBackend::SetSceneEnableCCD(void* scene, bool value)
{
}

float PhysicsBackend::GetSceneBounceThresholdVelocity(void* scene)
{
    return 0.0f;
}

void PhysicsBackend::SetSceneBounceThresholdVelocity(void* scene, float value)
{
}

void PhysicsBackend::SetSceneOrigin(void* scene, const Vector3& oldOrigin, const Vector3& newOrigin)
{
}

void PhysicsBackend::AddSceneActor(void* scene, void* actor)
{
}

void PhysicsBackend::RemoveSceneActor(void* scene, void* actor, bool immediately)
{
}

void PhysicsBackend::AddSceneActorAction(void* scene, void* actor, ActionType action)
{
}

#if COMPILE_WITH_PROFILER

void PhysicsBackend::GetSceneStatistics(void* scene, PhysicsStatistics& result)
{
}

#endif

bool PhysicsBackend::RayCast(void* scene, const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::RayCast(void* scene, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::RayCastAll(void* scene, const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::BoxCastAll(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::SphereCast(void* scene, const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::SphereCast(void* scene, const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::SphereCastAll(void* scene, const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CapsuleCast(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CapsuleCast(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CapsuleCastAll(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::ConvexCastAll(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CheckBox(void* scene, const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CheckSphere(void* scene, const Vector3& center, const float radius, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CheckCapsule(void* scene, const Vector3& center, const float radius, const float height, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::CheckConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapBox(void* scene, const Vector3& center, const Vector3& halfExtents, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapSphere(void* scene, const Vector3& center, const float radius, Array<Collider*>& results, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapCapsule(void* scene, const Vector3& center, const float radius, const float height, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapBox(void* scene, const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapSphere(void* scene, const Vector3& center, const float radius, Array<PhysicsColliderActor*>& results, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapCapsule(void* scene, const Vector3& center, const float radius, const float height, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

bool PhysicsBackend::OverlapConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return false;
}

PhysicsBackend::ActorFlags PhysicsBackend::GetActorFlags(void* actor)
{
    return ActorFlags::None;
}

void PhysicsBackend::SetActorFlags(void* actor, ActorFlags value)
{
}

void PhysicsBackend::GetActorBounds(void* actor, BoundingBox& bounds)
{
    bounds = BoundingBox::Empty;
}

int32 PhysicsBackend::GetRigidActorShapesCount(void* actor)
{
    return 0;
}

void* PhysicsBackend::CreateRigidDynamicActor(IPhysicsActor* actor, const Vector3& position, const Quaternion& orientation, void* scene)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateRigidStaticActor(IPhysicsActor* actor, const Vector3& position, const Quaternion& orientation, void* scene)
{
    return DUMY_HANDLE;
}

PhysicsBackend::RigidDynamicFlags PhysicsBackend::GetRigidDynamicActorFlags(void* actor)
{
    return RigidDynamicFlags::None;
}

void PhysicsBackend::SetRigidDynamicActorFlags(void* actor, RigidDynamicFlags value)
{
}

void PhysicsBackend::GetRigidActorPose(void* actor, Vector3& position, Quaternion& orientation)
{
    position = Vector3::Zero;
    orientation = Quaternion::Identity;
}

void PhysicsBackend::SetRigidActorPose(void* actor, const Vector3& position, const Quaternion& orientation, bool kinematic, bool wakeUp)
{
}

void PhysicsBackend::SetRigidDynamicActorLinearDamping(void* actor, float value)
{
}

void PhysicsBackend::SetRigidDynamicActorAngularDamping(void* actor, float value)
{
}

void PhysicsBackend::SetRigidDynamicActorMaxAngularVelocity(void* actor, float value)
{
}

void PhysicsBackend::SetRigidDynamicActorConstraints(void* actor, RigidbodyConstraints value)
{
}

Vector3 PhysicsBackend::GetRigidDynamicActorLinearVelocity(void* actor)
{
    return Vector3::Zero;
}

void PhysicsBackend::SetRigidDynamicActorLinearVelocity(void* actor, const Vector3& value, bool wakeUp)
{
}

Vector3 PhysicsBackend::GetRigidDynamicActorAngularVelocity(void* actor)
{
    return Vector3::Zero;
}

void PhysicsBackend::SetRigidDynamicActorAngularVelocity(void* actor, const Vector3& value, bool wakeUp)
{
}

Vector3 PhysicsBackend::GetRigidDynamicActorCenterOfMass(void* actor)
{
    return Vector3::Zero;
}

void PhysicsBackend::SetRigidDynamicActorCenterOfMassOffset(void* actor, const Float3& value)
{
}

bool PhysicsBackend::GetRigidDynamicActorIsSleeping(void* actor)
{
    return false;
}

void PhysicsBackend::RigidDynamicActorSleep(void* actor)
{
}

void PhysicsBackend::RigidDynamicActorWakeUp(void* actor)
{
}

float PhysicsBackend::GetRigidDynamicActorSleepThreshold(void* actor)
{
    return 0.0f;
}

void PhysicsBackend::SetRigidDynamicActorSleepThreshold(void* actor, float value)
{
}

float PhysicsBackend::GetRigidDynamicActorMaxDepenetrationVelocity(void* actor)
{
    return 0.0f;
}

void PhysicsBackend::SetRigidDynamicActorMaxDepenetrationVelocity(void* actor, float value)
{
}

void PhysicsBackend::SetRigidDynamicActorSolverIterationCounts(void* actor, int32 minPositionIters, int32 minVelocityIters)
{
}

void PhysicsBackend::UpdateRigidDynamicActorMass(void* actor, float& mass, float massScale, bool autoCalculate)
{
}

void PhysicsBackend::AddRigidDynamicActorForce(void* actor, const Vector3& force, ForceMode mode)
{
}

void PhysicsBackend::AddRigidDynamicActorForceAtPosition(void* actor, const Vector3& force, const Vector3& position, ForceMode mode)
{
}

void PhysicsBackend::AddRigidDynamicActorTorque(void* actor, const Vector3& torque, ForceMode mode)
{
}

void* PhysicsBackend::CreateShape(PhysicsColliderActor* collider, const CollisionShape& geometry, Span<JsonAsset*> materials, bool enabled, bool trigger)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::SetShapeState(void* shape, bool enabled, bool trigger)
{
}

void PhysicsBackend::SetShapeFilterMask(void* shape, uint32 mask0, uint32 mask1)
{
}

void* PhysicsBackend::GetShapeActor(void* shape)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::GetShapePose(void* shape, Vector3& position, Quaternion& orientation)
{
}

CollisionShape::Types PhysicsBackend::GetShapeType(void* shape)
{
    return CollisionShape::Types::Box;
}

void PhysicsBackend::GetShapeLocalPose(void* shape, Vector3& position, Quaternion& orientation)
{
}

void PhysicsBackend::SetShapeLocalPose(void* shape, const Vector3& position, const Quaternion& orientation)
{
}

void PhysicsBackend::SetShapeContactOffset(void* shape, float value)
{
}

void PhysicsBackend::SetShapeMaterials(void* shape, Span<JsonAsset*> materials)
{
}

void PhysicsBackend::SetShapeGeometry(void* shape, const CollisionShape& geometry)
{
}

void PhysicsBackend::AttachShape(void* shape, void* actor)
{
}

void PhysicsBackend::DetachShape(void* shape, void* actor)
{
}

bool PhysicsBackend::ComputeShapesPenetration(void* shapeA, void* shapeB, const Vector3& positionA, const Quaternion& orientationA, const Vector3& positionB, const Quaternion& orientationB, Vector3& direction, float& distance)
{
    direction = Vector3::Forward;
    return false;
}

float PhysicsBackend::ComputeShapeSqrDistanceToPoint(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& point, Vector3* closestPoint)
{
    return 0.0f;
}

bool PhysicsBackend::RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance)
{
    return false;
}

bool PhysicsBackend::RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance)
{
    return false;
}

void PhysicsBackend::SetJointFlags(void* joint, JointFlags value)
{
}

void PhysicsBackend::SetJointActors(void* joint, void* actors0, void* actor1)
{
}

void PhysicsBackend::SetJointActorPose(void* joint, const Vector3& position, const Quaternion& orientation, uint8 index)
{
}

void PhysicsBackend::SetJointBreakForce(void* joint, float force, float torque)
{
}

void PhysicsBackend::GetJointForce(void* joint, Vector3& linear, Vector3& angular)
{
    linear = Vector3::Zero;
    angular = Vector3::Zero;
}

void* PhysicsBackend::CreateFixedJoint(const PhysicsJointDesc& desc)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateDistanceJoint(const PhysicsJointDesc& desc)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateHingeJoint(const PhysicsJointDesc& desc)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateSliderJoint(const PhysicsJointDesc& desc)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateSphericalJoint(const PhysicsJointDesc& desc)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateD6Joint(const PhysicsJointDesc& desc)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::SetDistanceJointFlags(void* joint, DistanceJointFlag flags)
{
}

void PhysicsBackend::SetDistanceJointMinDistance(void* joint, float value)
{
}

void PhysicsBackend::SetDistanceJointMaxDistance(void* joint, float value)
{
}

void PhysicsBackend::SetDistanceJointTolerance(void* joint, float value)
{
}

void PhysicsBackend::SetDistanceJointSpring(void* joint, const SpringParameters& value)
{
}

float PhysicsBackend::GetDistanceJointDistance(void* joint)
{
    return 0.0f;
}

void PhysicsBackend::SetHingeJointFlags(void* joint, HingeJointFlag value, bool driveFreeSpin)
{
}

void PhysicsBackend::SetHingeJointLimit(void* joint, const LimitAngularRange& value)
{
}

void PhysicsBackend::SetHingeJointDrive(void* joint, const HingeJointDrive& value)
{
}

float PhysicsBackend::GetHingeJointAngle(void* joint)
{
    return 0.0f;
}

float PhysicsBackend::GetHingeJointVelocity(void* joint)
{
    return 0.0f;
}

void PhysicsBackend::SetSliderJointFlags(void* joint, SliderJointFlag value)
{
}

void PhysicsBackend::SetSliderJointLimit(void* joint, const LimitLinearRange& value)
{
}

float PhysicsBackend::GetSliderJointPosition(void* joint)
{
    return 0.0f;
}

float PhysicsBackend::GetSliderJointVelocity(void* joint)
{
    return 0.0f;
}

void PhysicsBackend::SetSphericalJointFlags(void* joint, SphericalJointFlag value)
{
}

void PhysicsBackend::SetSphericalJointLimit(void* joint, const LimitConeRange& value)
{
}

void PhysicsBackend::SetD6JointMotion(void* joint, D6JointAxis axis, D6JointMotion value)
{
}

void PhysicsBackend::SetD6JointDrive(void* joint, const D6JointDriveType index, const D6JointDrive& value)
{
}

void PhysicsBackend::SetD6JointLimitLinear(void* joint, const LimitLinear& value)
{
}

void PhysicsBackend::SetD6JointLimitTwist(void* joint, const LimitAngularRange& value)
{
}

void PhysicsBackend::SetD6JointLimitSwing(void* joint, const LimitConeRange& value)
{
}

Vector3 PhysicsBackend::GetD6JointDrivePosition(void* joint)
{
    return Vector3::Zero;
}

void PhysicsBackend::SetD6JointDrivePosition(void* joint, const Vector3& value)
{
}

Quaternion PhysicsBackend::GetD6JointDriveRotation(void* joint)
{
    return Quaternion::Identity;
}

void PhysicsBackend::SetD6JointDriveRotation(void* joint, const Quaternion& value)
{
}

void PhysicsBackend::GetD6JointDriveVelocity(void* joint, Vector3& linear, Vector3& angular)
{
    linear = Vector3::Zero;
    angular = Vector3::Zero;
}

void PhysicsBackend::SetD6JointDriveVelocity(void* joint, const Vector3& linear, const Vector3& angular)
{
}

float PhysicsBackend::GetD6JointTwist(void* joint)
{
    return 0.0f;
}

float PhysicsBackend::GetD6JointSwingY(void* joint)
{
    return 0.0f;
}

float PhysicsBackend::GetD6JointSwingZ(void* joint)
{
    return 0.0f;
}

void* PhysicsBackend::CreateController(void* scene, IPhysicsActor* actor, PhysicsColliderActor* collider, float contactOffset, const Vector3& position, float slopeLimit, int32 nonWalkableMode, JsonAsset* material, float radius, float height, float stepOffset, void*& shape)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::GetControllerRigidDynamicActor(void* controller)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::SetControllerSize(void* controller, float radius, float height)
{
}

void PhysicsBackend::SetControllerSlopeLimit(void* controller, float value)
{
}

void PhysicsBackend::SetControllerNonWalkableMode(void* controller, int32 value)
{
}

void PhysicsBackend::SetControllerStepOffset(void* controller, float value)
{
}

Vector3 PhysicsBackend::GetControllerUpDirection(void* controller)
{
    return Vector3::Up;
}

void PhysicsBackend::SetControllerUpDirection(void* controller, const Vector3& value)
{
}

Vector3 PhysicsBackend::GetControllerPosition(void* controller)
{
    return Vector3::Zero;
}

void PhysicsBackend::SetControllerPosition(void* controller, const Vector3& value)
{
}

int32 PhysicsBackend::MoveController(void* controller, void* shape, const Vector3& displacement, float minMoveDistance, float deltaTime)
{
    return 0;
}

#if WITH_VEHICLE

void* PhysicsBackend::CreateVehicle(WheeledVehicle* actor)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::DestroyVehicle(void* vehicle, int32 driveType)
{
}

void PhysicsBackend::UpdateVehicleWheels(WheeledVehicle* actor)
{
}

void PhysicsBackend::SetVehicleEngine(void* vehicle, const void* value)
{
}

void PhysicsBackend::SetVehicleDifferential(void* vehicle, const void* value)
{
}

void PhysicsBackend::SetVehicleGearbox(void* vehicle, const void* value)
{
}

int32 PhysicsBackend::GetVehicleTargetGear(void* vehicle)
{
    return 0;
}

void PhysicsBackend::SetVehicleTargetGear(void* vehicle, int32 value)
{
}

int32 PhysicsBackend::GetVehicleCurrentGear(void* vehicle)
{
    return 0;
}

void PhysicsBackend::SetVehicleCurrentGear(void* vehicle, int32 value)
{
}

float PhysicsBackend::GetVehicleForwardSpeed(void* vehicle)
{
    return 0.0f;
}

float PhysicsBackend::GetVehicleSidewaysSpeed(void* vehicle)
{
    return 0.0f;
}

float PhysicsBackend::GetVehicleEngineRotationSpeed(void* vehicle)
{
    return 0.0f;
}

void PhysicsBackend::AddVehicle(void* scene, WheeledVehicle* actor)
{
}

void PhysicsBackend::RemoveVehicle(void* scene, WheeledVehicle* actor)
{
}

#endif

void* PhysicsBackend::CreateConvexMesh(byte* data, int32 dataSize, BoundingBox& localBounds)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateTriangleMesh(byte* data, int32 dataSize, BoundingBox& localBounds)
{
    return DUMY_HANDLE;
}

void* PhysicsBackend::CreateHeightField(byte* data, int32 dataSize)
{
    return DUMY_HANDLE;
}

void PhysicsBackend::GetConvexMeshTriangles(void* contextMesh, Array<Float3, HeapAllocation>& vertexBuffer, Array<int, HeapAllocation>& indexBuffer)
{
}

void PhysicsBackend::GetTriangleMeshTriangles(void* triangleMesh, Array<Float3>& vertexBuffer, Array<int32, HeapAllocation>& indexBuffer)
{
}

const uint32* PhysicsBackend::GetTriangleMeshRemap(void* triangleMesh, uint32& count)
{
    count = 0;
    return nullptr;
}

void PhysicsBackend::GetHeightFieldSize(void* heightField, int32& rows, int32& columns)
{
    rows = 0;
    columns = 0;
}

float PhysicsBackend::GetHeightFieldHeight(void* heightField, int32 x, int32 z)
{
    return 0.0f;
}

PhysicsBackend::HeightFieldSample PhysicsBackend::GetHeightFieldSample(void* heightField, int32 x, int32 z)
{
    return HeightFieldSample();
}

bool PhysicsBackend::ModifyHeightField(void* heightField, int32 startCol, int32 startRow, int32 cols, int32 rows, const HeightFieldSample* data)
{
    return true;
}

void PhysicsBackend::FlushRequests()
{
}

void PhysicsBackend::FlushRequests(void* scene)
{
}

void PhysicsBackend::DestroyActor(void* actor)
{
}

void PhysicsBackend::DestroyShape(void* shape)
{
}

void PhysicsBackend::DestroyJoint(void* joint)
{
}

void PhysicsBackend::DestroyController(void* controller)
{
}

void PhysicsBackend::DestroyMaterial(void* material)
{
}

void PhysicsBackend::DestroyObject(void* object)
{
}

void PhysicsBackend::RemoveCollider(PhysicsColliderActor* collider)
{
}

void PhysicsBackend::RemoveJoint(Joint* joint)
{
}

#endif
