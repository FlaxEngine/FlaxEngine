// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "WheeledVehicle.h"
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
#include <ThirdParty/PhysX/vehicle/PxVehicleDrive4W.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleUtilSetup.h>
#include <ThirdParty/PhysX/PxFiltering.h>
#endif

#if WITH_VEHICLE
extern void InitVehicleSDK();
extern Array<WheeledVehicle*> WheelVehicles;
#endif

WheeledVehicle::WheeledVehicle(const SpawnParams& params)
    : RigidBody(params)
{
    _useCCD = 1;
}

const Array<WheeledVehicle::Wheel>& WheeledVehicle::GetWheels() const
{
    return _wheels;
}

void WheeledVehicle::SetWheels(const Array<Wheel>& value)
{
    _wheels = value;
    if (IsDuringPlay())
    {
        Setup();
    }
}

WheeledVehicle::GearboxSettings WheeledVehicle::GetGearbox() const
{
    return _gearbox;
}

void WheeledVehicle::SetGearbox(const GearboxSettings& value)
{
#if WITH_VEHICLE
    auto& drive = (PxVehicleDrive4W*&)_drive;
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
    auto& drive = (const PxVehicleDrive4W*&)_drive;
    return drive ? drive->computeForwardSpeed() : 0.0f;
#else
    return 0.0f;
#endif
}

float WheeledVehicle::GetSidewaysSpeed() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive4W*&)_drive;
    return drive ? drive->computeSidewaysSpeed() : 0.0f;
#else
    return 0.0f;
#endif
}

float WheeledVehicle::GetEngineRotationSpeed() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive4W*&)_drive;
    return drive ? RadPerSToRpm(drive->mDriveDynData.getEngineRotationSpeed()) : 0.0f;
#else
    return 0.0f;
#endif
}

int32 WheeledVehicle::GetCurrentGear() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive4W*&)_drive;
    return drive ? (int32)drive->mDriveDynData.getCurrentGear() - 1 : 0;
#else
    return 0;
#endif
}

void WheeledVehicle::SetCurrentGear(int32 value)
{
#if WITH_VEHICLE
    auto& drive = (PxVehicleDrive4W*&)_drive;
    if (drive)
    {
        drive->mDriveDynData.forceGearChange((PxU32)(value + 1));
    }
#endif
}

int32 WheeledVehicle::GetTargetGear() const
{
#if WITH_VEHICLE
    auto& drive = (const PxVehicleDrive4W*&)_drive;
    return drive ? (int32)drive->mDriveDynData.getTargetGear() - 1 : 0;
#else
    return 0;
#endif
}

void WheeledVehicle::SetTargetGear(int32 value)
{
#if WITH_VEHICLE
    auto& drive = (PxVehicleDrive4W*&)_drive;
    if (drive)
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
    if (!_actor)
        return;
    auto& drive = (PxVehicleDrive4W*&)_drive;

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
        wheels.Add(&wheel);
    }
    if (wheels.IsEmpty())
    {
        // No wheel, no car
        // No woman, no cry
        if (drive)
        {
            WheelVehicles.Remove(this);
            drive->free();
            drive = nullptr;
        }
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
    PxVehicleComputeSprungMasses(wheels.Count(), offsets, centerOfMassOffset.p, _actor->getMass(), 1, sprungMasses);
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(wheels.Count());
    for (int32 i = 0; i < wheels.Count(); i++)
    {
        Wheel& wheel = *wheels[i];

        auto& data = _wheelsData[i];
        data.Collider = wheel.Collider;
        data.LocalOrientation = wheel.Collider->GetLocalOrientation();

        PxVehicleSuspensionData suspensionData;
        // TODO: expose as wheel settings
        const float suspensionFrequency = 7.0f;
        const float suspensionDampingRatio = 1.0f;
        suspensionData.mMaxCompression = 10.0f;
        suspensionData.mMaxDroop = 10.0f;
        suspensionData.mSprungMass = sprungMasses[i];
        suspensionData.mSpringStrength = Math::Square(suspensionFrequency) * suspensionData.mSprungMass;
        suspensionData.mSpringDamperRate = suspensionDampingRatio * 2.0f * Math::Sqrt(suspensionData.mSpringStrength * suspensionData.mSprungMass);

        PxVehicleTireData tire;
        tire.mType = 0;

        PxVehicleWheelData wheelData;
        wheelData.mMass = wheel.Mass;
        wheelData.mRadius = wheel.Radius;
        wheelData.mWidth = wheel.Width;
        wheelData.mMOI = 0.5f * wheelData.mMass * Math::Square(wheelData.mRadius);
        wheelData.mDampingRate = M2ToCm2(0.25f);
        switch (wheel.Type)
        {
        case WheelType::FrontLeft:
        case WheelType::FrontRight:
            // Enable steering for the front wheels only
            // TODO: expose as settings
            wheelData.mMaxSteer = PI * 0.3333f;
            wheelData.mMaxSteer = PI * 0.3333f;
            break;
        case WheelType::RearLeft:
        case WheelType::ReadRight:
            // Enable the handbrake for the rear wheels only
            // TODO: expose as settings
            wheelData.mMaxHandBrakeTorque = M2ToCm2(4000.0f);
            wheelData.mMaxHandBrakeTorque = M2ToCm2(4000.0f);
            break;
        }

        PxVec3 centreOffset = centerOfMassOffset.transformInv(offsets[i]);
        float suspensionForceOffset = 0.0f;
        PxVec3 forceAppPointOffset(centreOffset.x, centreOffset.y, centreOffset.z + suspensionForceOffset);

        wheelsSimData->setTireData(i, tire);
        wheelsSimData->setWheelData(i, wheelData);
        wheelsSimData->setSuspensionData(i, suspensionData);
        wheelsSimData->setSuspTravelDirection(i, PxVec3(0, -1, 0));
        wheelsSimData->setWheelCentreOffset(i, centreOffset);
        wheelsSimData->setSuspForceAppPointOffset(i, forceAppPointOffset);
        wheelsSimData->setTireForceAppPointOffset(i, forceAppPointOffset);

        PxShape* wheelShape = wheel.Collider->GetPxShape();
        wheelsSimData->setWheelShapeMapping(i, shapes.Find(wheelShape));

        // Setup Vehicle ID inside word3 for suspension raycasts to ignore self
        PxFilterData filter = wheelShape->getQueryFilterData();
        filter.word3 = _id.D + 1;
        wheelShape->setQueryFilterData(filter);
        wheelShape->setSimulationFilterData(filter);
        wheelsSimData->setSceneQueryFilterData(i, filter);
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

    // Initialize vehicle drive simulation data
    PxVehicleDriveSimData4W driveSimData;
    {
        // Differential
        PxVehicleDifferential4WData diff;
        // TODO: expose Differential options
        diff.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
        driveSimData.setDiffData(diff);

        // Engine
        PxVehicleEngineData engine;
        // TODO: expose Engine options
        engine.mMOI = M2ToCm2(1.0f);
        engine.mPeakTorque = M2ToCm2(500.0f);
        engine.mMaxOmega = RpmToRadPerS(6000.0f);
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
        // TODO: expose Clutch options
        clutch.mStrength = M2ToCm2(10.0f);
        driveSimData.setClutchData(clutch);

        // Ackermann steer accuracy
        PxVehicleAckermannGeometryData ackermann;
        ackermann.mAxleSeparation = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x);
        ackermann.mFrontWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).z - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z);
        ackermann.mRearWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).z - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z);
        driveSimData.setAckermannGeometryData(ackermann);
    }

    // Create vehicle drive
    drive = PxVehicleDrive4W::allocate(wheels.Count());
    _actor->setSolverIterationCounts(12, 4);
    drive->setup(CPhysX, _actor, *wheelsSimData, driveSimData, wheels.Count() - 4);
    WheelVehicles.Add(this);

    // Initialize
    drive->setToRestState();
    drive->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    drive->mDriveDynData.setUseAutoGears(_gearbox.AutoGear);

    wheelsSimData->free();
#else
    LOG(Fatal, "PhysX Vehicle SDK is not supported.");
#endif
}

#if USE_EDITOR

void WheeledVehicle::DrawPhysicsDebug(RenderView& view)
{
    // Wheels shapes
    for (auto& wheel : _wheels)
    {
        if (wheel.Collider && wheel.Collider->GetParent() == this && !wheel.Collider->GetIsTrigger())
        {
            DEBUG_DRAW_WIRE_CYLINDER(wheel.Collider->GetPosition(), wheel.Collider->GetOrientation(), wheel.Radius, wheel.Width, Color::Red * 0.8f, 0, true);
        }
    }
}

void WheeledVehicle::OnDebugDrawSelected()
{
    // Wheels shapes
    for (auto& wheel : _wheels)
    {
        if (wheel.Collider && wheel.Collider->GetParent() == this && !wheel.Collider->GetIsTrigger())
        {
            DEBUG_DRAW_WIRE_CYLINDER(wheel.Collider->GetPosition(), wheel.Collider->GetOrientation(), wheel.Radius, wheel.Width, Color::Red * 0.4f, 0, false);
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

    SERIALIZE_MEMBER(Wheels, _wheels);
    SERIALIZE(UseReverseAsBrake);
    SERIALIZE_MEMBER(Gearbox, _gearbox);
}

void WheeledVehicle::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    RigidBody::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Wheels, _wheels);
    DESERIALIZE(UseReverseAsBrake);
    DESERIALIZE_MEMBER(Gearbox, _gearbox);
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
    auto& drive = (PxVehicleDrive4W*&)_drive;
    if (drive)
    {
        // Parkway Drive
        WheelVehicles.Remove(this);
        drive->free();
        drive = nullptr;
    }
#endif

    RigidBody::EndPlay();
}
