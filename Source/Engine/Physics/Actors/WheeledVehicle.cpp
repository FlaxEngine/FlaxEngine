// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "WheeledVehicle.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Debug/DebugDraw.h"
#endif
#if !WITH_VEHICLE
#include "Engine/Core/Log.h"
#endif

WheeledVehicle::WheeledVehicle(const SpawnParams& params)
    : RigidBody(params)
{
    _useCCD = 1;
}

WheeledVehicle::DriveTypes WheeledVehicle::GetDriveType() const
{
    return _driveType;
}

void WheeledVehicle::SetDriveType(DriveTypes value)
{
    if (_driveType == value)
        return;
    _driveType = value;
    Setup();
}

const Array<WheeledVehicle::Wheel>& WheeledVehicle::GetWheels() const
{
    return _wheels;
}

WheeledVehicle::DriveControlSettings WheeledVehicle::GetDriveControl() const
{
    return _driveControl;
}

void WheeledVehicle::SetDriveControl(DriveControlSettings value)
{
    value.RiseRateAcceleration = Math::Max(value.RiseRateAcceleration, 0.01f);
    value.RiseRateBrake = Math::Max(value.RiseRateBrake, 0.01f);
    value.RiseRateHandBrake = Math::Max(value.RiseRateHandBrake, 0.01f);
    value.RiseRateSteer = Math::Max(value.RiseRateSteer, 0.01f);

    value.FallRateAcceleration = Math::Max(value.FallRateAcceleration, 0.01f);
    value.FallRateBrake = Math::Max(value.FallRateBrake, 0.01f);
    value.FallRateHandBrake = Math::Max(value.FallRateHandBrake, 0.01f);
    value.FallRateSteer = Math::Max(value.FallRateSteer, 0.01f);

    // Don't let have an invalid steer vs speed list.
    if (value.SteerVsSpeed.Count() < 1)
        value.SteerVsSpeed.Add(WheeledVehicle::SteerControl());
    else // PhysX backend requires the max of 4 values only
        while (value.SteerVsSpeed.Count() > 4)
            value.SteerVsSpeed.RemoveLast();

    // Maintain all values clamped to have an ordered speed list
    const int32 steerVsSpeedCount = value.SteerVsSpeed.Count();
    for (int32 i = 0; i < steerVsSpeedCount; i++)
    {
        // Apply only on changed value
        if (i > _driveControl.SteerVsSpeed.Count() - 1 ||
            Math::NotNearEqual(_driveControl.SteerVsSpeed[i].Speed, value.SteerVsSpeed[i].Speed) ||
             Math::NotNearEqual(_driveControl.SteerVsSpeed[i].Steer, value.SteerVsSpeed[i].Steer))
        {
            SteerControl& steerVsSpeed = value.SteerVsSpeed[i];
            steerVsSpeed.Steer = Math::Saturate(steerVsSpeed.Steer);
            steerVsSpeed.Speed = Math::Max(steerVsSpeed.Speed, 10.0f);

            // Clamp speeds to have an ordered list
            if (i >= 1)
            {
                const SteerControl& lastSteerVsSpeed = value.SteerVsSpeed[i - 1];
                const SteerControl& nextSteerVsSpeed = value.SteerVsSpeed[Math::Clamp(i + 1, 0, steerVsSpeedCount - 1)];
                const float minSpeed = lastSteerVsSpeed.Speed;
                const float maxSpeed = nextSteerVsSpeed.Speed;

                if (i + 1 < steerVsSpeedCount - 1)
                    steerVsSpeed.Speed = Math::Clamp(steerVsSpeed.Speed, minSpeed, maxSpeed);
                else
                    steerVsSpeed.Speed = Math::Max(steerVsSpeed.Speed, minSpeed);
            }
            else if (steerVsSpeedCount > 1)
            {
                const SteerControl& nextSteerVsSpeed = value.SteerVsSpeed[i + 1];
                steerVsSpeed.Speed = Math::Min(steerVsSpeed.Speed, nextSteerVsSpeed.Speed);
            }
        }
    }

    _driveControl = value;
}

void WheeledVehicle::SetWheels(const Array<Wheel>& value)
{
#if WITH_VEHICLE
    // Don't recreate whole vehicle when some wheel properties are only changed (eg. suspension)
    if (_actor && _vehicle && _wheels.Count() == value.Count() && _wheelsData.Count() == value.Count())
    {
        bool softUpdate = true;
        for (int32 wheelIndex = 0; wheelIndex < value.Count(); wheelIndex++)
        {
            auto& oldWheel = _wheels.Get()[wheelIndex];
            auto& newWheel = value.Get()[wheelIndex];
            if (Math::NotNearEqual(oldWheel.SuspensionForceOffset, newWheel.SuspensionForceOffset) ||
                oldWheel.Collider != newWheel.Collider)
            {
                softUpdate = false;
                break;
            }
        }
        if (softUpdate)
        {
            _wheels = value;
            PhysicsBackend::UpdateVehicleWheels(this);
            return;
        }
    }
#endif
    _wheels = value;
    Setup();
}

WheeledVehicle::EngineSettings WheeledVehicle::GetEngine() const
{
    return _engine;
}

void WheeledVehicle::SetEngine(const EngineSettings& value)
{
#if WITH_VEHICLE
    if (_vehicle)
        PhysicsBackend::SetVehicleEngine(_vehicle, &value);
#endif
    _engine = value;
}

WheeledVehicle::DifferentialSettings WheeledVehicle::GetDifferential() const
{
    return _differential;
}

void WheeledVehicle::SetDifferential(const DifferentialSettings& value)
{
#if WITH_VEHICLE
    if (_vehicle)
        PhysicsBackend::SetVehicleDifferential(_vehicle, &value);
#endif
    _differential = value;
}

WheeledVehicle::GearboxSettings WheeledVehicle::GetGearbox() const
{
    return _gearbox;
}

void WheeledVehicle::SetGearbox(const GearboxSettings& value)
{
#if WITH_VEHICLE
    if (_vehicle)
        PhysicsBackend::SetVehicleGearbox(_vehicle, &value);
#endif
    _gearbox = value;
}

void WheeledVehicle::SetAntiRollBars(const Array<AntiRollBar>& value)
{
    _antiRollBars = value;
#if WITH_VEHICLE
    if (_vehicle)
        PhysicsBackend::UpdateVehicleAntiRollBars(this);
#endif
}

const Array<WheeledVehicle::AntiRollBar>& WheeledVehicle::GetAntiRollBars() const
{
    return _antiRollBars;
}

void WheeledVehicle::SetThrottle(float value)
{
    _throttle = Math::Clamp(value, -1.0f, 1.0f);
}

float WheeledVehicle::GetThrottle()
{
    return _throttle;
}

void WheeledVehicle::SetSteering(float value)
{
    _steering = Math::Clamp(value, -1.0f, 1.0f);
}

void WheeledVehicle::SetBrake(float value)
{
    value = Math::Saturate(value);
    _brake = value;
    _tankLeftBrake = value;
    _tankRightBrake = value;
}

void WheeledVehicle::SetHandbrake(float value)
{
    _handBrake = Math::Saturate(value);
}

void WheeledVehicle::SetTankLeftThrottle(float value)
{
    _tankLeftThrottle = Math::Clamp(value, -1.0f, 1.0f);
}

void WheeledVehicle::SetTankRightThrottle(float value)
{
    _tankRightThrottle = Math::Clamp(value, -1.0f, 1.0f);
}

void WheeledVehicle::SetTankLeftBrake(float value)
{
    _tankLeftBrake = Math::Saturate(value);
}

void WheeledVehicle::SetTankRightBrake(float value)
{
    _tankRightBrake = Math::Saturate(value);
}

void WheeledVehicle::ClearInput()
{
    _throttle = 0;
    _steering = 0;
    _brake = 0;
    _handBrake = 0;
    _tankLeftThrottle = 0;
    _tankRightThrottle = 0;
    _tankLeftBrake = 0;
    _tankRightBrake = 0;
}

float WheeledVehicle::GetForwardSpeed() const
{
#if WITH_VEHICLE
    return _vehicle ? PhysicsBackend::GetVehicleForwardSpeed(_vehicle) : 0.0f;
#else
    return 0.0f;
#endif
}

float WheeledVehicle::GetSidewaysSpeed() const
{
#if WITH_VEHICLE
    return _vehicle ? PhysicsBackend::GetVehicleSidewaysSpeed(_vehicle) : 0.0f;
#else
    return 0.0f;
#endif
}

float WheeledVehicle::GetEngineRotationSpeed() const
{
#if WITH_VEHICLE
    return _vehicle && _driveType != DriveTypes::NoDrive ? PhysicsBackend::GetVehicleEngineRotationSpeed(_vehicle) : 0.0f;
#else
    return 0.0f;
#endif
}

int32 WheeledVehicle::GetCurrentGear() const
{
#if WITH_VEHICLE
    return _vehicle && _driveType != DriveTypes::NoDrive ? PhysicsBackend::GetVehicleCurrentGear(_vehicle) : 0;
#else
    return 0;
#endif
}

void WheeledVehicle::SetCurrentGear(int32 value)
{
#if WITH_VEHICLE
    if (_vehicle && _driveType != DriveTypes::NoDrive)
        PhysicsBackend::SetVehicleCurrentGear(_vehicle, value);
#endif
}

int32 WheeledVehicle::GetTargetGear() const
{
#if WITH_VEHICLE
    return _vehicle && _driveType != DriveTypes::NoDrive ? PhysicsBackend::GetVehicleTargetGear(_vehicle) : 0;
#else
    return 0;
#endif
}

void WheeledVehicle::SetTargetGear(int32 value)
{
#if WITH_VEHICLE
    if (_vehicle && _driveType != DriveTypes::NoDrive)
        PhysicsBackend::SetVehicleTargetGear(_vehicle, value);
#endif
}

void WheeledVehicle::GetWheelState(int32 index, WheelState& result)
{
    if (index >= 0 && index < _wheels.Count())
    {
        const auto collider = _wheels[index].Collider.Get();
        for (auto& wheelData : _wheelsData)
        {
            if (wheelData.Collider == collider)
            {
                result = wheelData.State;
                return;
            }
        }
    }
}

void WheeledVehicle::Setup()
{
#if WITH_VEHICLE
    if (!_actor || !IsDuringPlay())
        return;

    // Release previous
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    if (_vehicle)
    {
        PhysicsBackend::RemoveVehicle(scene, this);
        PhysicsBackend::DestroyVehicle(_vehicle, (int32)_driveTypeCurrent);
        _vehicle = nullptr;
    }

    // Create a new one
    _wheelsData.Clear();
    _vehicle = PhysicsBackend::CreateVehicle(this);
    if (!_vehicle)
        return;
    _driveTypeCurrent = _driveType;
    PhysicsBackend::AddVehicle(scene, this);
    PhysicsBackend::SetRigidDynamicActorSolverIterationCounts(_actor, 12, 4);
#else
    LOG(Fatal, "Vehicles are not supported.");
#endif
}

#if USE_EDITOR

void WheeledVehicle::DrawPhysicsDebug(RenderView& view)
{
    // Wheels shapes
    for (const auto& data : _wheelsData)
    {
        int32 wheelIndex = 0;
        for (; wheelIndex < _wheels.Count(); wheelIndex++)
        {
            if (_wheels[wheelIndex].Collider == data.Collider)
                break;
        }
        if (wheelIndex == _wheels.Count())
            break;
        const auto& wheel = _wheels[wheelIndex];
        if (wheel.Collider && wheel.Collider->GetParent() == this && !wheel.Collider->GetIsTrigger())
        {
            const Vector3 currentPos = wheel.Collider->GetPosition();
            const Vector3 basePos = currentPos - Vector3(0, data.State.SuspensionOffset, 0);
            const Quaternion wheelDebugOrientation = GetOrientation() * Quaternion::Euler(-data.State.RotationAngle, data.State.SteerAngle, 0) * Quaternion::Euler(90, 0, 90);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(basePos, wheel.Radius * 0.07f), Color::Blue * 0.3f, 0, true);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(currentPos, wheel.Radius * 0.08f), Color::Blue * 0.8f, 0, true);
            DEBUG_DRAW_LINE(basePos, currentPos, Color::Blue, 0, true);
            DEBUG_DRAW_WIRE_CYLINDER(currentPos, wheelDebugOrientation, wheel.Radius, wheel.Width, Color::Red * 0.8f, 0, true);
            if (!data.State.IsInAir)
            {
                DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(data.State.TireContactPoint, 5.0f), Color::Green, 0, true);
            }
        }
    }
}

void WheeledVehicle::OnDebugDrawSelected()
{
    // Wheels shapes
    for (const auto& data : _wheelsData)
    {
        int32 wheelIndex = 0;
        for (; wheelIndex < _wheels.Count(); wheelIndex++)
        {
            if (_wheels[wheelIndex].Collider == data.Collider)
                break;
        }
        if (wheelIndex == _wheels.Count())
            break;
        const auto& wheel = _wheels[wheelIndex];
        if (wheel.Collider && wheel.Collider->GetParent() == this && !wheel.Collider->GetIsTrigger())
        {
            const Vector3 currentPos = wheel.Collider->GetPosition();
            const Vector3 basePos = currentPos - Vector3(0, data.State.SuspensionOffset, 0);
            const Quaternion wheelDebugOrientation = GetOrientation() * Quaternion::Euler(-data.State.RotationAngle, data.State.SteerAngle, 0) * Quaternion::Euler(90, 0, 90);
            Transform actorPose = Transform::Identity, shapePose = Transform::Identity;
            PhysicsBackend::GetRigidActorPose(_actor, actorPose.Translation, actorPose.Orientation);
            PhysicsBackend::GetShapeLocalPose(wheel.Collider->GetPhysicsShape(), shapePose.Translation, shapePose.Orientation);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(basePos, wheel.Radius * 0.07f), Color::Blue * 0.3f, 0, false);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(currentPos, wheel.Radius * 0.08f), Color::Blue * 0.8f, 0, false);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(actorPose.LocalToWorld(shapePose.Translation), wheel.Radius * 0.11f), Color::OrangeRed * 0.8f, 0, false);
            DEBUG_DRAW_LINE(basePos, currentPos, Color::Blue, 0, false);
            DEBUG_DRAW_WIRE_CYLINDER(currentPos, wheelDebugOrientation, wheel.Radius, wheel.Width, Color::Red * 0.4f, 0, false);
            if (!data.State.SuspensionTraceStart.IsZero())
            {
                DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(data.State.SuspensionTraceStart, 5.0f), Color::AliceBlue, 0, false);
                DEBUG_DRAW_LINE(data.State.SuspensionTraceStart, data.State.SuspensionTraceEnd, data.State.IsInAir ? Color::Red : Color::Green, 0, false);
            }
            if (!data.State.IsInAir)
            {
                DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(data.State.TireContactPoint, 5.0f), Color::Green, 0, false);
            }
        }
    }

    // Anti roll bars axes
    const int32 wheelsCount = _wheels.Count();
    for (int32 i = 0; i < GetAntiRollBars().Count(); i++)
    {
        const int32 axleIndex = GetAntiRollBars()[i].Axle;
        const int32 leftWheelIndex = axleIndex * 2;
        const int32 rightWheelIndex = leftWheelIndex + 1;
        if (leftWheelIndex >= wheelsCount || rightWheelIndex >= wheelsCount)
            continue;
        if (!_wheels[leftWheelIndex].Collider || !_wheels[rightWheelIndex].Collider)
            continue;
        DEBUG_DRAW_LINE(_wheels[leftWheelIndex].Collider->GetPosition(), _wheels[rightWheelIndex].Collider->GetPosition(), Color::Yellow, 0, false);
    }

    // Center of mass
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(_transform.LocalToWorld(_centerOfMassOffset), 10.0f), Color::Blue, 0, false);

    RigidBody::OnDebugDrawSelected();
}

#endif

void WheeledVehicle::Serialize(SerializeStream& stream, const void* otherObj)
{
    RigidBody::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(WheeledVehicle);

    SERIALIZE_MEMBER(DriveType, _driveType);
    SERIALIZE_MEMBER(Wheels, _wheels);
    SERIALIZE_MEMBER(DriveControl, _driveControl);
    SERIALIZE(UseReverseAsBrake);
    SERIALIZE(UseAnalogSteering);
    SERIALIZE_MEMBER(Engine, _engine);
    SERIALIZE_MEMBER(Differential, _differential);
    SERIALIZE_MEMBER(Gearbox, _gearbox);
    SERIALIZE_MEMBER(AntiRollBars, _antiRollBars);
}

void WheeledVehicle::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    RigidBody::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(DriveType, _driveType);
    DESERIALIZE_MEMBER(Wheels, _wheels);
    DESERIALIZE_MEMBER(DriveControl, _driveControl);
    DESERIALIZE(UseReverseAsBrake);
    DESERIALIZE(UseAnalogSteering);
    DESERIALIZE_MEMBER(Engine, _engine);
    DESERIALIZE_MEMBER(Differential, _differential);
    DESERIALIZE_MEMBER(Gearbox, _gearbox);
    DESERIALIZE_MEMBER(AntiRollBars, _antiRollBars);

    // [Deprecated on 13.06.2023, expires on 13.06.2025]
    _fixInvalidForwardDir |= modifier->EngineBuild < 6341;
}

void WheeledVehicle::OnColliderChanged(Collider* c)
{
    RigidBody::OnColliderChanged(c);

    if (_useWheelsUpdates)
    {
        // Rebuild vehicle when someone adds/removed wheels
        Setup();
    }
}

void WheeledVehicle::OnActiveInTreeChanged()
{
    // Skip rebuilds from per-wheel OnColliderChanged when whole vehicle is toggled
    _useWheelsUpdates = false;
    RigidBody::OnActiveInTreeChanged();
    _useWheelsUpdates = true;

    // Perform whole rebuild when it gets activated
    if (IsActiveInHierarchy())
        Setup();
}

void WheeledVehicle::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    RigidBody::OnPhysicsSceneChanged(previous);

#if WITH_VEHICLE
    PhysicsBackend::RemoveVehicle(previous->GetPhysicsScene(), this);
    PhysicsBackend::AddVehicle(GetPhysicsScene()->GetPhysicsScene(), this);
#endif
}

void WheeledVehicle::OnTransformChanged()
{
    RigidBody::OnTransformChanged();

    // Initially vehicles were using X axis as forward which was kind of bad idea as engine uses Z as forward
    // [Deprecated on 13.06.2023, expires on 13.06.2025]
    if (_fixInvalidForwardDir)
    {
        _fixInvalidForwardDir = false;

        // Transform all vehicle children around the vehicle origin to fix the vehicle facing direction
        const Quaternion rotationDelta(0.0f, -0.7071068f, 0.0f, 0.7071068f);
        const Vector3 origin = GetPosition();
        for (Actor* child : Children)
        {
            Transform trans = child->GetTransform();
            const Vector3 pivotOffset = trans.Translation - origin;
            if (pivotOffset.IsZero())
            {
                trans.Orientation *= Quaternion::Invert(trans.Orientation) * rotationDelta * trans.Orientation;
            }
            else
            {
                Matrix transWorld, deltaWorld;
                Matrix::RotationQuaternion(trans.Orientation, transWorld);
                Matrix::RotationQuaternion(rotationDelta, deltaWorld);
                Matrix world = transWorld * Matrix::Translation(pivotOffset) * deltaWorld * Matrix::Translation(-pivotOffset);
                trans.SetRotation(world);
                trans.Translation += world.GetTranslation();
            }
            child->SetTransform(trans);
        }
    }
}

void WheeledVehicle::BeginPlay(SceneBeginData* data)
{
    RigidBody::BeginPlay(data);

#if WITH_VEHICLE
    Setup();
#endif

#if USE_EDITOR
    GetSceneRendering()->AddPhysicsDebug<WheeledVehicle, &WheeledVehicle::DrawPhysicsDebug>(this);
#endif
}

void WheeledVehicle::EndPlay()
{
#if USE_EDITOR
    GetSceneRendering()->RemovePhysicsDebug<WheeledVehicle, &WheeledVehicle::DrawPhysicsDebug>(this);
#endif

#if WITH_VEHICLE
    if (_vehicle)
    {
        // Parkway Drive
        PhysicsBackend::RemoveVehicle(GetPhysicsScene()->GetPhysicsScene(), this);
        PhysicsBackend::DestroyVehicle(_vehicle, (int32)_driveTypeCurrent);
        _vehicle = nullptr;
    }
#endif

    RigidBody::EndPlay();
}
