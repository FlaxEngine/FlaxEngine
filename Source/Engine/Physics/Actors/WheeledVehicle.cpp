// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "WheeledVehicle.h"
#include "Engine/Core/Log.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Debug/DebugDraw.h"
#endif
#if WITH_VEHICLE
#include <ThirdParty/PhysX/vehicle/PxVehicleSDK.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleNoDrive.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDrive4W.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDriveNW.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleUtilSetup.h>
#include <ThirdParty/PhysX/PxFiltering.h>
#endif

#if WITH_VEHICLE
extern void InitVehicleSDK();
extern Array<WheeledVehicle*> WheelVehicles;
#endif

namespace
{
    void FreeDrive(WheeledVehicle::DriveTypes driveType, PxVehicleWheels* drive)
    {
        switch (driveType)
        {
        case WheeledVehicle::DriveTypes::Drive4W:
            ((PxVehicleDrive4W*)drive)->free();
            break;
        case WheeledVehicle::DriveTypes::DriveNW:
            ((PxVehicleDriveNW*)drive)->free();
            break;
        case WheeledVehicle::DriveTypes::NoDrive:
            ((PxVehicleNoDrive*)drive)->free();
            break;
        }
    }
}

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

void WheeledVehicle::SetWheels(const Array<Wheel>& value)
{
    _wheels = value;
    Setup();
}

WheeledVehicle::EngineSettings WheeledVehicle::GetEngine() const
{
    return _engine;
}

void WheeledVehicle::SetEngine(const EngineSettings& value)
{
    _engine = value;
}

WheeledVehicle::DifferentialSettings WheeledVehicle::GetDifferential() const
{
    return _differential;
}

void WheeledVehicle::SetDifferential(const DifferentialSettings& value)
{
    _differential = value;
}

WheeledVehicle::GearboxSettings WheeledVehicle::GetGearbox() const
{
    return _gearbox;
}

void WheeledVehicle::SetGearbox(const GearboxSettings& value)
{
#if WITH_VEHICLE
    auto& drive = (PxVehicleDrive*&)_drive;
    if (drive)
    {
        drive->mDriveDynData.setUseAutoGears(value.AutoGear);
        drive->mDriveDynData.setAutoBoxSwitchTime(Math::Max(value.SwitchTime, 0.0f));
    }
#endif
    _gearbox = value;
}

void WheeledVehicle::SetThrottle(float value)
{
    _throttle = Math::Clamp(value, -1.0f, 1.0f);
}

void WheeledVehicle::SetSteering(float value)
{
    _steering = Math::Clamp(value, -1.0f, 1.0f);
}

void WheeledVehicle::SetBrake(float value)
{
    _brake = Math::Saturate(value);
}

void WheeledVehicle::SetHandbrake(float value)
{
    _handBrake = Math::Saturate(value);
}

void WheeledVehicle::ClearInput()
{
    _throttle = 0;
    _steering = 0;
    _brake = 0;
    _handBrake = 0;
}

float WheeledVehicle::GetForwardSpeed() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleWheels*&)_drive;
    return drive ? drive->computeForwardSpeed() : 0.0f;
#else
    return 0.0f;
#endif
}

float WheeledVehicle::GetSidewaysSpeed() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleWheels*&)_drive;
    return drive ? drive->computeSidewaysSpeed() : 0.0f;
#else
    return 0.0f;
#endif
}

float WheeledVehicle::GetEngineRotationSpeed() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive*&)_drive;
    return drive && _driveType != DriveTypes::NoDrive ? RadPerSToRpm(drive->mDriveDynData.getEngineRotationSpeed()) : 0.0f;
#else
    return 0.0f;
#endif
}

int32 WheeledVehicle::GetCurrentGear() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive*&)_drive;
    return drive && _driveType != DriveTypes::NoDrive ? (int32)drive->mDriveDynData.getCurrentGear() - 1 : 0;
#else
    return 0;
#endif
}

void WheeledVehicle::SetCurrentGear(int32 value)
{
#if WITH_VEHICLE
    auto& drive = (PxVehicleDrive*&)_drive;
    if (drive && _driveType != DriveTypes::NoDrive)
    {
        drive->mDriveDynData.forceGearChange((PxU32)(value + 1));
    }
#endif
}

int32 WheeledVehicle::GetTargetGear() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive*&)_drive;
    return drive && _driveType != DriveTypes::NoDrive ? (int32)drive->mDriveDynData.getTargetGear() - 1 : 0;
#else
    return 0;
#endif
}

void WheeledVehicle::SetTargetGear(int32 value)
{
#if WITH_VEHICLE
    auto& drive = (PxVehicleDrive*&)_drive;
    if (drive && _driveType != DriveTypes::NoDrive)
    {
        drive->mDriveDynData.startGearChange((PxU32)(value + 1));
    }
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
    auto& drive = (PxVehicleWheels*&)_drive;

    // Release previous
    if (drive)
    {
        WheelVehicles.Remove(this);
        FreeDrive(_driveTypeCurrent, drive);
        drive = nullptr;
    }

    // Get wheels
    Array<Wheel*, FixedAllocation<PX_MAX_NB_WHEELS>> wheels;
    _wheelsData.Clear();
    for (auto& wheel : _wheels)
    {
        if (!wheel.Collider)
        {
            LOG(Warning, "Missing wheel collider in vehicle {0}", ToString());
            continue;
        }
        if (wheel.Collider->GetParent() != this)
        {
            LOG(Warning, "Invalid wheel collider {1} in vehicle {0} attached to {2} (wheels needs to be added as children to vehicle)", ToString(), wheel.Collider->ToString(), wheel.Collider->GetParent() ? wheel.Collider->GetParent()->ToString() : String::Empty);
            continue;
        }
        if (wheel.Collider->GetIsTrigger())
        {
            LOG(Warning, "Invalid wheel collider {1} in vehicle {0} cannot be a trigger", ToString(), wheel.Collider->ToString());
            continue;
        }
        if (wheel.Collider->IsDuringPlay())
        {
            wheels.Add(&wheel);
        }
    }
    if (wheels.IsEmpty())
    {
        // No wheel, no car
        // No woman, no cry
        return;
    }
    _wheelsData.Resize(wheels.Count());

    InitVehicleSDK();

    // Get linked shapes for the wheels mapping
    Array<PxShape*, InlinedAllocation<8>> shapes;
    shapes.Resize(_actor->getNbShapes());
    _actor->getShapes(shapes.Get(), shapes.Count(), 0);
    const PxTransform centerOfMassOffset = _actor->getCMassLocalPose();

    // Initialize wheels simulation data
    PxVec3 offsets[PX_MAX_NB_WHEELS];
    for (int32 i = 0; i < wheels.Count(); i++)
    {
        Wheel& wheel = *wheels[i];
        offsets[i] = C2P(wheel.Collider->GetLocalPosition());
    }
    PxF32 sprungMasses[PX_MAX_NB_WHEELS];
    const float mass = _actor->getMass();
    PxVehicleComputeSprungMasses(wheels.Count(), offsets, centerOfMassOffset.p, mass, 1, sprungMasses);
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(wheels.Count());
    for (int32 i = 0; i < wheels.Count(); i++)
    {
        Wheel& wheel = *wheels[i];

        auto& data = _wheelsData[i];
        data.Collider = wheel.Collider;
        data.LocalOrientation = wheel.Collider->GetLocalOrientation();

        PxVehicleSuspensionData suspensionData;
        const float suspensionFrequency = 7.0f;
        suspensionData.mMaxCompression = wheel.SuspensionMaxRaise;
        suspensionData.mMaxDroop = wheel.SuspensionMaxDrop;
        suspensionData.mSprungMass = sprungMasses[i];
        suspensionData.mSpringStrength = Math::Square(suspensionFrequency) * suspensionData.mSprungMass;
        suspensionData.mSpringDamperRate = wheel.SuspensionDampingRate * 2.0f * Math::Sqrt(suspensionData.mSpringStrength * suspensionData.mSprungMass);

        PxVehicleTireData tire;
        tire.mType = 0;

        PxVehicleWheelData wheelData;
        wheelData.mMass = wheel.Mass;
        wheelData.mRadius = wheel.Radius;
        wheelData.mWidth = wheel.Width;
        wheelData.mMOI = 0.5f * wheelData.mMass * Math::Square(wheelData.mRadius);
        wheelData.mDampingRate = M2ToCm2(wheel.DampingRate);
        wheelData.mMaxSteer = wheel.MaxSteerAngle * DegreesToRadians;
        wheelData.mMaxBrakeTorque = M2ToCm2(wheel.MaxBrakeTorque);
        wheelData.mMaxHandBrakeTorque = M2ToCm2(wheel.MaxHandBrakeTorque);

        PxVec3 centreOffset = centerOfMassOffset.transformInv(offsets[i]);
        PxVec3 forceAppPointOffset(centreOffset.z, wheel.SuspensionForceOffset, centreOffset.z);

        wheelsSimData->setTireData(i, tire);
        wheelsSimData->setWheelData(i, wheelData);
        wheelsSimData->setSuspensionData(i, suspensionData);
        wheelsSimData->setSuspTravelDirection(i, centerOfMassOffset.rotate(PxVec3(0.0f, -1.0f, 0.0f)));
        wheelsSimData->setWheelCentreOffset(i, centreOffset);
        wheelsSimData->setSuspForceAppPointOffset(i, forceAppPointOffset);
        wheelsSimData->setTireForceAppPointOffset(i, forceAppPointOffset);
        wheelsSimData->setSubStepCount(4.0f * 100.0f, 3, 1);
        wheelsSimData->setMinLongSlipDenominator(4.0f * 100.0f);

        PxShape* wheelShape = wheel.Collider->GetPxShape();
        if (wheel.Collider->IsActiveInHierarchy())
        {
            wheelsSimData->setWheelShapeMapping(i, shapes.Find(wheelShape));

            // Setup Vehicle ID inside word3 for suspension raycasts to ignore self
            PxFilterData filter = wheelShape->getQueryFilterData();
            filter.word3 = _id.D + 1;
            wheelShape->setQueryFilterData(filter);
            wheelShape->setSimulationFilterData(filter);
            wheelsSimData->setSceneQueryFilterData(i, filter);

            // Remove wheels from the simulation (suspension force hold the vehicle)
            wheelShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        }
        else
        {
            wheelsSimData->setWheelShapeMapping(i, -1);
            wheelsSimData->disableWheel(i);
        }
    }
    for (auto child : Children)
    {
        auto collider = Cast<Collider>(child);
        if (collider && collider->GetAttachedRigidBody() == this)
        {
            bool isWheel = false;
            for (auto& w : wheels)
            {
                if (w->Collider == collider)
                {
                    isWheel = true;
                    break;
                }
            }
            if (!isWheel)
            {
                // Setup Vehicle ID inside word3 for suspension raycasts to ignore self
                PxShape* shape = collider->GetPxShape();
                PxFilterData filter = shape->getQueryFilterData();
                filter.word3 = _id.D + 1;
                shape->setQueryFilterData(filter);
                shape->setSimulationFilterData(filter);
            }
        }
    }

    // Initialize vehicle drive
    _driveTypeCurrent = _driveType;
    switch (_driveType)
    {
    case DriveTypes::Drive4W:
    {
        PxVehicleDriveSimData4W driveSimData;

        // Differential
        PxVehicleDifferential4WData diff;
        diff.mType = (PxVehicleDifferential4WData::Enum)_differential.Type;
        diff.mFrontRearSplit = _differential.FrontRearSplit;
        diff.mFrontLeftRightSplit = _differential.FrontLeftRightSplit;
        diff.mRearLeftRightSplit = _differential.RearLeftRightSplit;
        diff.mCentreBias = _differential.CentreBias;
        diff.mFrontBias = _differential.FrontBias;
        diff.mRearBias = _differential.RearBias;
        driveSimData.setDiffData(diff);

        // Engine
        PxVehicleEngineData engine;
        engine.mMOI = M2ToCm2(_engine.MOI);
        engine.mPeakTorque = M2ToCm2(_engine.MaxTorque);
        engine.mMaxOmega = RpmToRadPerS(_engine.MaxRotationSpeed);
        engine.mDampingRateFullThrottle = M2ToCm2(0.15f);
        engine.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(2.0f);
        engine.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(0.35f);
        driveSimData.setEngineData(engine);

        // Gears
        PxVehicleGearsData gears;
        gears.mSwitchTime = Math::Max(_gearbox.SwitchTime, 0.0f);
        driveSimData.setGearsData(gears);

        // Auto Box
        PxVehicleAutoBoxData autoBox;
        driveSimData.setAutoBoxData(autoBox);

        // Clutch
        PxVehicleClutchData clutch;
        clutch.mStrength = M2ToCm2(_gearbox.ClutchStrength);
        driveSimData.setClutchData(clutch);

        // Ackermann steer accuracy
        PxVehicleAckermannGeometryData ackermann;
        ackermann.mAxleSeparation = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x);
        ackermann.mFrontWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).z - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z);
        ackermann.mRearWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).z - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z);
        driveSimData.setAckermannGeometryData(ackermann);

        // Create vehicle drive
        auto drive4W = PxVehicleDrive4W::allocate(wheels.Count());
        drive4W->setup(CPhysX, _actor, *wheelsSimData, driveSimData, Math::Max(wheels.Count() - 4, 0));
        drive4W->setToRestState();
        drive4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        drive4W->mDriveDynData.setUseAutoGears(_gearbox.AutoGear);
        drive = drive4W;
        break;
    }
    case DriveTypes::DriveNW:
    {
        PxVehicleDriveSimDataNW driveSimData;

        // Differential
        PxVehicleDifferentialNWData diff;
        for (int32 i = 0; i < wheels.Count(); i++)
            diff.setDrivenWheel(i, true);
        driveSimData.setDiffData(diff);

        // Engine
        PxVehicleEngineData engine;
        engine.mMOI = M2ToCm2(_engine.MOI);
        engine.mPeakTorque = M2ToCm2(_engine.MaxTorque);
        engine.mMaxOmega = RpmToRadPerS(_engine.MaxRotationSpeed);
        engine.mDampingRateFullThrottle = M2ToCm2(0.15f);
        engine.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(2.0f);
        engine.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(0.35f);
        driveSimData.setEngineData(engine);

        // Gears
        PxVehicleGearsData gears;
        gears.mSwitchTime = Math::Max(_gearbox.SwitchTime, 0.0f);
        driveSimData.setGearsData(gears);

        // Auto Box
        PxVehicleAutoBoxData autoBox;
        driveSimData.setAutoBoxData(autoBox);

        // Clutch
        PxVehicleClutchData clutch;
        clutch.mStrength = M2ToCm2(_gearbox.ClutchStrength);
        driveSimData.setClutchData(clutch);

        // Create vehicle drive
        auto driveNW = PxVehicleDriveNW::allocate(wheels.Count());
        driveNW->setup(CPhysX, _actor, *wheelsSimData, driveSimData, wheels.Count());
        driveNW->setToRestState();
        driveNW->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        driveNW->mDriveDynData.setUseAutoGears(_gearbox.AutoGear);
        drive = driveNW;
        break;
    }
    case DriveTypes::NoDrive:
    {
        // Create vehicle drive
        auto driveNo = PxVehicleNoDrive::allocate(wheels.Count());
        driveNo->setup(CPhysX, _actor, *wheelsSimData);
        driveNo->setToRestState();
        drive = driveNo;
        break;
    }
    default:
    CRASH;
    }

    WheelVehicles.Add(this);
    wheelsSimData->free();
    _actor->setSolverIterationCounts(12, 4);

#else
    LOG(Fatal, "PhysX Vehicle SDK is not supported.");
#endif
}

#if USE_EDITOR

void WheeledVehicle::DrawPhysicsDebug(RenderView& view)
{
    // Wheels shapes
    for (auto& data : _wheelsData)
    {
        int32 wheelIndex = 0;
        for (; wheelIndex < _wheels.Count(); wheelIndex++)
        {
            if (_wheels[wheelIndex].Collider == data.Collider)
                break;
        }
        if (wheelIndex == _wheels.Count())
            break;
        auto& wheel = _wheels[wheelIndex];
        if (wheel.Collider && wheel.Collider->GetParent() == this && !wheel.Collider->GetIsTrigger())
        {
            const Vector3 currentPos = wheel.Collider->GetPosition();
            const Vector3 basePos = currentPos - Vector3(0, data.State.SuspensionOffset, 0);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(basePos, wheel.Radius * 0.07f), Color::Blue * 0.3f, 0, true);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(currentPos, wheel.Radius * 0.08f), Color::Blue * 0.8f, 0, true);
            DEBUG_DRAW_LINE(basePos, currentPos, Color::Blue, 0, true);
            DEBUG_DRAW_WIRE_CYLINDER(currentPos, wheel.Collider->GetOrientation(), wheel.Radius, wheel.Width, Color::Red * 0.8f, 0, true);
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
    for (auto& data : _wheelsData)
    {
        int32 wheelIndex = 0;
        for (; wheelIndex < _wheels.Count(); wheelIndex++)
        {
            if (_wheels[wheelIndex].Collider == data.Collider)
                break;
        }
        if (wheelIndex == _wheels.Count())
            break;
        auto& wheel = _wheels[wheelIndex];
        if (wheel.Collider && wheel.Collider->GetParent() == this && !wheel.Collider->GetIsTrigger())
        {
            const Vector3 currentPos = wheel.Collider->GetPosition();
            const Vector3 basePos = currentPos - Vector3(0, data.State.SuspensionOffset, 0);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(basePos, wheel.Radius * 0.07f), Color::Blue * 0.3f, 0, false);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(currentPos, wheel.Radius * 0.08f), Color::Blue * 0.8f, 0, false);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(P2C(_actor->getGlobalPose().transform(wheel.Collider->GetPxShape()->getLocalPose()).p), wheel.Radius * 0.11f), Color::OrangeRed * 0.8f, 0, false);
            DEBUG_DRAW_LINE(basePos, currentPos, Color::Blue, 0, false);
            DEBUG_DRAW_WIRE_CYLINDER(currentPos, wheel.Collider->GetOrientation(), wheel.Radius, wheel.Width, Color::Red * 0.4f, 0, false);
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
    SERIALIZE(UseReverseAsBrake);
    SERIALIZE(UseAnalogSteering);
    SERIALIZE_MEMBER(Engine, _engine);
    SERIALIZE_MEMBER(Differential, _differential);
    SERIALIZE_MEMBER(Gearbox, _gearbox);
}

void WheeledVehicle::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    RigidBody::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(DriveType, _driveType);
    DESERIALIZE_MEMBER(Wheels, _wheels);
    DESERIALIZE(UseReverseAsBrake);
    DESERIALIZE(UseAnalogSteering);
    DESERIALIZE_MEMBER(Engine, _engine);
    DESERIALIZE_MEMBER(Differential, _differential);
    DESERIALIZE_MEMBER(Gearbox, _gearbox);
}

void WheeledVehicle::OnColliderChanged(Collider* c)
{
    RigidBody::OnColliderChanged(c);

    // Rebuild vehicle when someone adds/removed wheels
    Setup();
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
    auto& drive = (PxVehicleWheels*&)_drive;
    if (drive)
    {
        // Parkway Drive
        WheelVehicles.Remove(this);
        FreeDrive(_driveTypeCurrent, drive);
        drive = nullptr;
    }
#endif

    RigidBody::EndPlay();
}
