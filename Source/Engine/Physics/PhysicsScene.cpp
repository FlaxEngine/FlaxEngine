#include "Physics.h"
#include "PhysicsScene.h"
#include "PhysicsSettings.h"
#include "PhysicsStepper.h"
#include "Utilities.h"

#include "Actors/IPhysicsActor.h"

#include "Engine/Core/Log.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include <ThirdParty/PhysX/PxPhysicsAPI.h>

#if WITH_VEHICLE
#include "Actors/WheeledVehicle.h"
#include <ThirdParty/PhysX/vehicle/PxVehicleUpdate.h>
#endif

// Temporary memory size used by the PhysX during the simulation. Must be multiply of 4kB and 16bit aligned.
#define SCRATCH_BLOCK_SIZE (1024 * 128)

PxFilterFlags FilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
        return PxFilterFlag::eDEFAULT;
    }

    // Send events for the kinematic actors but don't solve the contact
    if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1))
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
        return PxFilterFlag::eSUPPRESS;
    }

    // Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
    if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
    {
        pairFlags |= PxPairFlag::eSOLVE_CONTACT;
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        pairFlags |= PxPairFlag::ePOST_SOLVER_VELOCITY;
        pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
        return PxFilterFlag::eDEFAULT;
    }

    // Ignore pair (no collisions nor events)
    return PxFilterFlag::eKILL;
}

enum class ActionType
{
    Sleep,
};

struct ActionData
{
    ActionType Type;
    PxActor* Actor;
};

#if WITH_VEHICLE

static PxQueryHitType::Enum WheelRaycastPreFilter(PxFilterData filterData0, PxFilterData filterData1, const void* constantBlock, PxU32 constantBlockSize, PxHitFlags& queryFlags)
{
    // Hardcoded id for vehicle shapes masking
    if (filterData0.word3 == filterData1.word3)
    {
        return PxQueryHitType::eNONE;
    }

    // Collide for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
    if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
    {
        return PxQueryHitType::eBLOCK;
    }

    return PxQueryHitType::eNONE;
}

#endif

class PhysicsScenePhysX
{
    friend PhysicsScene;

private:
    PxScene* Scene;
    PxCpuDispatcher* CpuDispatcher;
    PxControllerManager* ControllerManager;
    PxSimulationFilterShader PhysXDefaultFilterShader = PxDefaultSimulationFilterShader;

    Array<PxActor*> NewActors;
    Array<PxActor*> DeadActors;
    Array<PxMaterial*> DeadMaterials;
    Array<PhysicsColliderActor*> DeadColliders;
    Array<Joint*> DeadJoints;
    Array<ActionData> Actions;
    Array<PxBase*> DeadObjects;

#if WITH_VEHICLE
    Array<PxVehicleWheels*> WheelVehiclesCache;
    Array<PxRaycastQueryResult> WheelQueryResults;
    Array<PxRaycastHit> WheelHitResults;
    Array<PxWheelQueryResult> WheelVehiclesResultsPerWheel;
    Array<PxVehicleWheelQueryResult> WheelVehiclesResultsPerVehicle;
    PxBatchQuery* WheelRaycastBatchQuery = nullptr;
    PxVehicleDrivableSurfaceToTireFrictionPairs* WheelTireFrictions = nullptr;
    Array<WheeledVehicle*> WheelVehicles;
#endif
};

PhysicsScene::PhysicsScene(const String& name, const PhysicsSettings& settings)
    : PersistentScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
{
#define CHECK_INIT(value, msg) if(!value) { LOG(Error, msg); return; }
    
    mName = name;
    mPhysxImpl = New<PhysicsScenePhysX>();

    // Create scene description
    PxSceneDesc sceneDesc(CPhysX->getTolerancesScale());
    sceneDesc.gravity = C2P(settings.DefaultGravity);
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    if (!settings.DisableCCD)
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    if (settings.EnableAdaptiveForce)
        sceneDesc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    sceneDesc.simulationEventCallback = &mEventsCallback;
    sceneDesc.filterShader = FilterShader;
    sceneDesc.bounceThresholdVelocity = settings.BounceThresholdVelocity;
    if (sceneDesc.cpuDispatcher == nullptr)
    {
        mPhysxImpl->CpuDispatcher = PxDefaultCpuDispatcherCreate(Math::Clamp<uint32>(Platform::GetCPUInfo().ProcessorCoreCount - 1, 1, 4));
        CHECK_INIT(mPhysxImpl->CpuDispatcher, "PxDefaultCpuDispatcherCreate failed!");
        sceneDesc.cpuDispatcher = mPhysxImpl->CpuDispatcher;
    }
    if (sceneDesc.filterShader == nullptr)
    {
        sceneDesc.filterShader = mPhysxImpl->PhysXDefaultFilterShader;
    }

    // Create scene
    mPhysxImpl->Scene = CPhysX->createScene(sceneDesc);
    CHECK_INIT(mPhysxImpl->Scene, "createScene failed!");
#if WITH_PVD
    auto pvdClient = PhysicsScene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES | PxPvdSceneFlag::eTRANSMIT_CONTACTS);
    }
    else
    {
        LOG(Info, "Missing PVD client scene.");
    }
#endif

    // Init characters controller
    mPhysxImpl->ControllerManager = PxCreateControllerManager(*mPhysxImpl->Scene);
}

PhysicsScene::~PhysicsScene()
{
#if WITH_VEHICLE
    RELEASE_PHYSX(mPhysxImpl->WheelRaycastBatchQuery);
    RELEASE_PHYSX(mPhysxImpl->WheelTireFrictions);
    mPhysxImpl->WheelQueryResults.Resize(0);
    mPhysxImpl->WheelHitResults.Resize(0);
    mPhysxImpl->WheelVehiclesResultsPerWheel.Resize(0);
    mPhysxImpl->WheelVehiclesResultsPerVehicle.Resize(0);
#endif
    
    RELEASE_PHYSX(mPhysxImpl->ControllerManager);
    SAFE_DELETE(mPhysxImpl->CpuDispatcher);
    SAFE_DELETE(mStepper);
    Allocator::Free(mScratchMemory);

    mScratchMemory = nullptr;
    mPhysxImpl->Scene->release();

    SAFE_DELETE(mPhysxImpl);
}

String PhysicsScene::GetName() const
{
    return mName;
}

PxScene* PhysicsScene::GetScene()
{
    return mPhysxImpl->Scene;
}

bool PhysicsScene::GetAutoSimulation()
{
    return mAutoSimulation;
}

void PhysicsScene::SetAutoSimulation(bool value)
{
    mAutoSimulation = value;
}

void PhysicsScene::SetGravity(const Vector3& value)
{
    if(mPhysxImpl->Scene)
    {
        mPhysxImpl->Scene->setGravity(C2P(value));
    }
}

Vector3 PhysicsScene::GetGravity()
{
    return mPhysxImpl->Scene ? P2C(mPhysxImpl->Scene->getGravity()) : Vector3::Zero;
}

bool PhysicsScene::GetEnableCCD()
{
    return mPhysxImpl->Scene ? (mPhysxImpl->Scene->getFlags() & PxSceneFlag::eENABLE_CCD) == PxSceneFlag::eENABLE_CCD : !PhysicsSettings::Get()->DisableCCD;
}

void PhysicsScene::SetEnableCCD(const bool value)
{
    if (mPhysxImpl->Scene)
        mPhysxImpl->Scene->setFlag(PxSceneFlag::eENABLE_CCD, value);
}

float PhysicsScene::GetBounceThresholdVelocity()
{
    return mPhysxImpl->Scene ? mPhysxImpl->Scene->getBounceThresholdVelocity() : PhysicsSettings::Get()->BounceThresholdVelocity;
}

void PhysicsScene::SetBounceThresholdVelocity(const float value)
{
    if (mPhysxImpl->Scene)
        mPhysxImpl->Scene->setBounceThresholdVelocity(value);
}

void PhysicsScene::Simulate(float dt)
{
    ASSERT(IsInMainThread() && !mIsDuringSimulation);
    ASSERT(CPhysX);
    const auto& settings = *PhysicsSettings::Get();

    // Flush the old/new objects and the other requests before the simulation
    FlushRequests();

    // Clamp delta
    dt = Math::Clamp(dt, 0.0f, settings.MaxDeltaTime);

    // Prepare util objects
    if (mScratchMemory == nullptr)
    {
        mScratchMemory = Allocator::Allocate(SCRATCH_BLOCK_SIZE, 16);
    }
    if (mStepper == nullptr)
    {
        mStepper = New<FixedStepper>();
    }
    if (settings.EnableSubstepping)
    {
        // Use substeps
        mStepper->Setup(settings.SubstepDeltaTime, settings.MaxSubsteps);
    }
    else
    {
        // Use single step
        mStepper->Setup(dt);
    }

    // Start simulation (may not be fired due to too small delta time)
    mIsDuringSimulation = true;
    if (mStepper->advance(mPhysxImpl->Scene, dt, mScratchMemory, SCRATCH_BLOCK_SIZE) == false)
        return;
    mEventsCallback.Clear();
    mLastDeltaTime = dt;

    // TODO: move this call after rendering done
    mStepper->renderDone();
}

bool PhysicsScene::IsDuringSimulation()
{
    return mIsDuringSimulation;
}

void PhysicsScene::CollectResults()
{
    if (!mIsDuringSimulation)
        return;
    ASSERT(IsInMainThread());
    ASSERT(CPhysX && mStepper);

    {
        PROFILE_CPU_NAMED("Physics.Fetch");

        // Gather results (with waiting for the end)
        mStepper->wait(mPhysxImpl->Scene);
    }

#if WITH_VEHICLE
    if (mPhysxImpl->WheelVehicles.HasItems())
    {
        PROFILE_CPU_NAMED("Physics.Vehicles");

        // Update vehicles steering
        mPhysxImpl->WheelVehiclesCache.Clear();
        mPhysxImpl->WheelVehiclesCache.EnsureCapacity(mPhysxImpl->WheelVehicles.Count());
        int32 wheelsCount = 0;
        for (auto wheelVehicle : mPhysxImpl->WheelVehicles)
        {
            if (!wheelVehicle->IsActiveInHierarchy())
                continue;
            auto drive = (PxVehicleWheels*)wheelVehicle->_drive;
            ASSERT(drive);
            mPhysxImpl->WheelVehiclesCache.Add(drive);
            wheelsCount += drive->mWheelsSimData.getNbWheels();

            float throttle = wheelVehicle->_throttle;
            float brake = wheelVehicle->_brake;
            if (wheelVehicle->UseReverseAsBrake)
            {
                const float invalidDirectionThreshold = 80.0f;
                const float breakThreshold = 8.0f;
                const float forwardSpeed = wheelVehicle->GetForwardSpeed();

                // Automatic gear change when changing driving direction
                if (Math::Abs(forwardSpeed) < invalidDirectionThreshold)
                {
                    if (throttle < -ZeroTolerance && wheelVehicle->GetCurrentGear() >= 0 && wheelVehicle->GetTargetGear() >= 0)
                    {
                        wheelVehicle->SetCurrentGear(-1);
                    }
                    else if (throttle > ZeroTolerance && wheelVehicle->GetCurrentGear() <= 0 && wheelVehicle->GetTargetGear() <= 0)
                    {
                        wheelVehicle->SetCurrentGear(1);
                    }
                }

                // Automatic break when changing driving direction
                if (throttle > 0.0f)
                {
                    if (forwardSpeed < -invalidDirectionThreshold)
                    {
                        brake = 1.0f;
                    }
                }
                else if (throttle < 0.0f)
                {
                    if (forwardSpeed > invalidDirectionThreshold)
                    {
                        brake = 1.0f;
                    }
                }
                else
                {
                    if (forwardSpeed < breakThreshold && forwardSpeed > -breakThreshold)
                    {
                        brake = 1.0f;
                    }
                }

                // Block throttle if user is changing driving direction
                if ((throttle > 0.0f && wheelVehicle->GetTargetGear() < 0) || (throttle < 0.0f && wheelVehicle->GetTargetGear() > 0))
                {
                    throttle = 0.0f;
                }

                throttle = Math::Abs(throttle);
            }
            else
            {
                throttle = Math::Max(throttle, 0.0f);
            }
            // @formatter:off
            // Reference: PhysX SDK docs
            // TODO: expose input control smoothing data
            static constexpr PxVehiclePadSmoothingData padSmoothing =
            {
	            {
		            6.0f,  // rise rate eANALOG_INPUT_ACCEL
		            6.0f,  // rise rate eANALOG_INPUT_BRAKE
		            12.0f, // rise rate eANALOG_INPUT_HANDBRAKE
		            2.5f,  // rise rate eANALOG_INPUT_STEER_LEFT
		            2.5f,  // rise rate eANALOG_INPUT_STEER_RIGHT
	            },
	            {
		            10.0f, // fall rate eANALOG_INPUT_ACCEL
		            10.0f, // fall rate eANALOG_INPUT_BRAKE
		            12.0f, // fall rate eANALOG_INPUT_HANDBRAKE
		            5.0f,  // fall rate eANALOG_INPUT_STEER_LEFT
		            5.0f,  // fall rate eANALOG_INPUT_STEER_RIGHT
	            }
            };
            PxVehicleKeySmoothingData keySmoothing =
            {
                {
                    3.0f,  // rise rate eANALOG_INPUT_ACCEL
                    3.0f,  // rise rate eANALOG_INPUT_BRAKE
                    10.0f, // rise rate eANALOG_INPUT_HANDBRAKE
                    2.5f,  // rise rate eANALOG_INPUT_STEER_LEFT
                    2.5f,  // rise rate eANALOG_INPUT_STEER_RIGHT
                },
                {
                    5.0f,  // fall rate eANALOG_INPUT__ACCEL
                    5.0f,  // fall rate eANALOG_INPUT__BRAKE
                    10.0f, // fall rate eANALOG_INPUT__HANDBRAKE
                    5.0f,  // fall rate eANALOG_INPUT_STEER_LEFT
                    5.0f   // fall rate eANALOG_INPUT_STEER_RIGHT
                }
            };
            // Reference: PhysX SDK docs
            // TODO: expose steer vs forward curve into per-vehicle (up to 8 points, values clamped into 0/1 range)
            static constexpr PxF32 steerVsForwardSpeedData[] =
            {
	            0.0f,		1.0f,
	            20.0f,		0.9f,
	            65.0f,		0.8f,
	            120.0f,		0.7f,
	            PX_MAX_F32, PX_MAX_F32,
	            PX_MAX_F32, PX_MAX_F32,
	            PX_MAX_F32, PX_MAX_F32,
	            PX_MAX_F32, PX_MAX_F32,
            };
            const PxFixedSizeLookupTable<8> steerVsForwardSpeed(steerVsForwardSpeedData, 4);
            // @formatter:on
            if (wheelVehicle->UseAnalogSteering)
            {
                switch (wheelVehicle->_driveTypeCurrent)
                {
                case WheeledVehicle::DriveTypes::Drive4W:
                {
                    PxVehicleDrive4WRawInputData rawInputData;
                    rawInputData.setAnalogAccel(throttle);
                    rawInputData.setAnalogBrake(brake);
                    rawInputData.setAnalogSteer(wheelVehicle->_steering);
                    rawInputData.setAnalogHandbrake(wheelVehicle->_handBrake);
                    PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, steerVsForwardSpeed, rawInputData, mLastDeltaTime, false, *(PxVehicleDrive4W*)drive);
                    break;
                }
                case WheeledVehicle::DriveTypes::DriveNW:
                {
                    PxVehicleDriveNWRawInputData rawInputData;
                    rawInputData.setAnalogAccel(throttle);
                    rawInputData.setAnalogBrake(brake);
                    rawInputData.setAnalogSteer(wheelVehicle->_steering);
                    rawInputData.setAnalogHandbrake(wheelVehicle->_handBrake);
                    PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, steerVsForwardSpeed, rawInputData, mLastDeltaTime, false, *(PxVehicleDriveNW*)drive);
                    break;
                }
                }
            }
            else
            {
                const float deadZone = 0.1f;
                switch (wheelVehicle->_driveTypeCurrent)
                {
                case WheeledVehicle::DriveTypes::Drive4W:
                {
                    PxVehicleDrive4WRawInputData rawInputData;
                    rawInputData.setDigitalAccel(throttle > deadZone);
                    rawInputData.setDigitalBrake(brake > deadZone);
                    rawInputData.setDigitalSteerLeft(wheelVehicle->_steering < -deadZone);
                    rawInputData.setDigitalSteerRight(wheelVehicle->_steering > deadZone);
                    rawInputData.setDigitalHandbrake(wheelVehicle->_handBrake > deadZone);
                    PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(keySmoothing, steerVsForwardSpeed, rawInputData, mLastDeltaTime, false, *(PxVehicleDrive4W*)drive);
                    break;
                }
                case WheeledVehicle::DriveTypes::DriveNW:
                {
                    PxVehicleDriveNWRawInputData rawInputData;
                    rawInputData.setDigitalAccel(throttle > deadZone);
                    rawInputData.setDigitalBrake(brake > deadZone);
                    rawInputData.setDigitalSteerLeft(wheelVehicle->_steering < -deadZone);
                    rawInputData.setDigitalSteerRight(wheelVehicle->_steering > deadZone);
                    rawInputData.setDigitalHandbrake(wheelVehicle->_handBrake > deadZone);
                    PxVehicleDriveNWSmoothDigitalRawInputsAndSetAnalogInputs(keySmoothing, steerVsForwardSpeed, rawInputData, mLastDeltaTime, false, *(PxVehicleDriveNW*)drive);
                    break;
                }
                }
            }
        }

        // Update batches queries cache
        if (wheelsCount > mPhysxImpl->WheelQueryResults.Count())
        {
            if (mPhysxImpl->WheelRaycastBatchQuery)
                mPhysxImpl->WheelRaycastBatchQuery->release();
            mPhysxImpl->WheelQueryResults.Resize(wheelsCount, false);
            mPhysxImpl->WheelHitResults.Resize(wheelsCount, false);
            PxBatchQueryDesc desc(wheelsCount, 0, 0);
            desc.queryMemory.userRaycastResultBuffer = mPhysxImpl->WheelQueryResults.Get();
            desc.queryMemory.userRaycastTouchBuffer = mPhysxImpl->WheelHitResults.Get();
            desc.queryMemory.raycastTouchBufferSize = wheelsCount;
            desc.preFilterShader = WheelRaycastPreFilter;
            mPhysxImpl->WheelRaycastBatchQuery = mPhysxImpl->Scene->createBatchQuery(desc);
        }

        // TODO: expose vehicle tires configuration
        if (!mPhysxImpl->WheelTireFrictions)
        {
            PxVehicleDrivableSurfaceType surfaceTypes[1];
            surfaceTypes[0].mType = 0;
            const PxMaterial* surfaceMaterials[1];
            surfaceMaterials[0] = Physics::GetDefaultMaterial();
            mPhysxImpl->WheelTireFrictions = PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(1, 1);
            mPhysxImpl->WheelTireFrictions->setup(1, 1, surfaceMaterials, surfaceTypes);
            mPhysxImpl->WheelTireFrictions->setTypePairFriction(0, 0, 5.0f);
        }

        // Setup cache for wheel states
        mPhysxImpl->WheelVehiclesResultsPerVehicle.Resize(mPhysxImpl->WheelVehiclesCache.Count(), false);
        mPhysxImpl->WheelVehiclesResultsPerWheel.Resize(wheelsCount, false);
        wheelsCount = 0;
        for (int32 i = 0, ii = 0; i < mPhysxImpl->WheelVehicles.Count(); i++)
        {
            auto wheelVehicle = mPhysxImpl->WheelVehicles[i];
            if (!wheelVehicle->IsActiveInHierarchy())
                continue;
            auto drive = (PxVehicleWheels*)mPhysxImpl->WheelVehicles[ii]->_drive;
            auto& perVehicle = mPhysxImpl->WheelVehiclesResultsPerVehicle[ii];
            ii++;
            perVehicle.nbWheelQueryResults = drive->mWheelsSimData.getNbWheels();
            perVehicle.wheelQueryResults = mPhysxImpl->WheelVehiclesResultsPerWheel.Get() + wheelsCount;
            wheelsCount += perVehicle.nbWheelQueryResults;
        }

        // Update vehicles
        if (mPhysxImpl->WheelVehiclesCache.Count() != 0)
        {
            PxVehicleSuspensionRaycasts(mPhysxImpl->WheelRaycastBatchQuery, mPhysxImpl->WheelVehiclesCache.Count(), mPhysxImpl->WheelVehiclesCache.Get(), mPhysxImpl->WheelQueryResults.Count(), mPhysxImpl->WheelQueryResults.Get());
            PxVehicleUpdates(mLastDeltaTime, mPhysxImpl->Scene->getGravity(), *mPhysxImpl->WheelTireFrictions, mPhysxImpl->WheelVehiclesCache.Count(), mPhysxImpl->WheelVehiclesCache.Get(), mPhysxImpl->WheelVehiclesResultsPerVehicle.Get());
        }

        // Synchronize state
        for (int32 i = 0, ii = 0; i < mPhysxImpl->WheelVehicles.Count(); i++)
        {
            auto wheelVehicle = mPhysxImpl->WheelVehicles[i];
            if (!wheelVehicle->IsActiveInHierarchy())
                continue;
            auto drive = mPhysxImpl->WheelVehiclesCache[ii];
            auto& perVehicle = mPhysxImpl->WheelVehiclesResultsPerVehicle[ii];
            ii++;
#if PHYSX_VEHICLE_DEBUG_TELEMETRY
            LOG(Info, "Vehicle[{}] Gear={}, RPM={}", ii, wheelVehicle->GetCurrentGear(), (int32)wheelVehicle->GetEngineRotationSpeed());
#endif

            // Update wheels
            for (int32 j = 0; j < wheelVehicle->_wheelsData.Count(); j++)
            {
                auto& wheelData = wheelVehicle->_wheelsData[j];
                auto& perWheel = perVehicle.wheelQueryResults[j];
#if PHYSX_VEHICLE_DEBUG_TELEMETRY
                LOG(Info, "Vehicle[{}] Wheel[{}] longitudinalSlip={}, lateralSlip={}, suspSpringForce={}", ii, j, Utilities::RoundTo2DecimalPlaces(perWheel.longitudinalSlip), Utilities::RoundTo2DecimalPlaces(perWheel.lateralSlip), (int32)perWheel.suspSpringForce);
#endif

                auto& state = wheelData.State;
                state.IsInAir = perWheel.isInAir;
                state.TireContactCollider = perWheel.tireContactShape ? static_cast<PhysicsColliderActor*>(perWheel.tireContactShape->userData) : nullptr;
                state.TireContactPoint = P2C(perWheel.tireContactPoint);
                state.TireContactNormal = P2C(perWheel.tireContactNormal);
                state.TireFriction = perWheel.tireFriction;
                state.SteerAngle = RadiansToDegrees * perWheel.steerAngle;
                state.RotationAngle = -RadiansToDegrees * drive->mWheelsDynData.getWheelRotationAngle(j);
                state.SuspensionOffset = perWheel.suspJounce;
#if USE_EDITOR
                state.SuspensionTraceStart = P2C(perWheel.suspLineStart);
                state.SuspensionTraceEnd = P2C(perWheel.suspLineStart + perWheel.suspLineDir * perWheel.suspLineLength);
#endif

                if (!wheelData.Collider)
                    continue;
                auto shape = wheelData.Collider->GetPxShape();

                // Update wheel collider transformation
                auto localPose = shape->getLocalPose();
                Transform t = wheelData.Collider->GetLocalTransform();
                t.Orientation = Quaternion::Euler(0, state.SteerAngle, state.RotationAngle) * wheelData.LocalOrientation;
                t.Translation = P2C(localPose.p) / wheelVehicle->GetScale() - t.Orientation * wheelData.Collider->GetCenter();
                wheelData.Collider->SetLocalTransform(t);
            }
        }
    }
#endif

    {
        PROFILE_CPU_NAMED("Physics.FlushActiveTransforms");

        // Gather change info
        PxU32 activeActorsCount;
        PxActor** activeActors = mPhysxImpl->Scene->getActiveActors(activeActorsCount);
        if (activeActorsCount > 0)
        {
            // Update changed transformations
            // TODO: use jobs system if amount if huge
            for (uint32 i = 0; i < activeActorsCount; i++)
            {
                const auto pxActor = (PxRigidActor*)*activeActors++;
                auto actor = dynamic_cast<IPhysicsActor*>((Actor*)pxActor->userData);
                ASSERT(actor);
                actor->OnActiveTransformChanged(pxActor->getGlobalPose());
            }
        }
    }

    {
        PROFILE_CPU_NAMED("Physics.SendEvents");

        mEventsCallback.CollectResults();
        mEventsCallback.SendTriggerEvents();
        mEventsCallback.SendCollisionEvents();
        mEventsCallback.SendJointEvents();
    }

    // End
    mIsDuringSimulation = false;
}

void PhysicsScene::FlushRequests()
{
    ASSERT(!IsDuringSimulation());
    ASSERT(CPhysX);

    PROFILE_CPU();

    mFlushLocker.Lock();

    // Note: this does not handle case when actor is removed and added to the scene at the same time

    if (mPhysxImpl->NewActors.HasItems())
    {
        GetScene()->addActors(mPhysxImpl->NewActors.Get(), mPhysxImpl->NewActors.Count());
        mPhysxImpl->NewActors.Clear();
    }

    for (int32 i = 0; i < mPhysxImpl->Actions.Count(); i++)
    {
        const auto action = mPhysxImpl->Actions[i];
        switch (action.Type)
        {
        case ActionType::Sleep:
            static_cast<PxRigidDynamic*>(action.Actor)->putToSleep();
            break;
        }
    }
    mPhysxImpl->Actions.Clear();

    if (mPhysxImpl->DeadActors.HasItems())
    {
        GetScene()->removeActors(mPhysxImpl->DeadActors.Get(), mPhysxImpl->DeadActors.Count(), true);
        for (int32 i = 0; i < mPhysxImpl->DeadActors.Count(); i++)
        {
            mPhysxImpl->DeadActors[i]->release();
        }
        mPhysxImpl->DeadActors.Clear();
    }

    if (mPhysxImpl->DeadColliders.HasItems())
    {
        for (int32 i = 0; i < mPhysxImpl->DeadColliders.Count(); i++)
        {
            mEventsCallback.OnColliderRemoved(mPhysxImpl->DeadColliders[i]);
        }
        mPhysxImpl->DeadColliders.Clear();
    }

    if (mPhysxImpl->DeadJoints.HasItems())
    {
        for (int32 i = 0; i < mPhysxImpl->DeadJoints.Count(); i++)
        {
            mEventsCallback.OnJointRemoved(mPhysxImpl->DeadJoints[i]);
        }
        mPhysxImpl->DeadJoints.Clear();
    }

    for (int32 i = 0; i < mPhysxImpl->DeadMaterials.Count(); i++)
    {
        auto material = mPhysxImpl->DeadMaterials[i];

        // Unlink ref to flax object
        material->userData = nullptr;

        material->release();
    }
    mPhysxImpl->DeadMaterials.Clear();

    for (int32 i = 0; i < mPhysxImpl->DeadObjects.Count(); i++)
    {
        mPhysxImpl->DeadObjects[i]->release();
    }
    mPhysxImpl->DeadObjects.Clear();

    mFlushLocker.Unlock();
}

void PhysicsScene::RemoveMaterial(PxMaterial* material)
{
    ASSERT(material);

    mFlushLocker.Lock();
    mPhysxImpl->DeadMaterials.Add(material);
    mFlushLocker.Unlock();
}

void PhysicsScene::RemoveObject(PxBase* obj)
{
    ASSERT(obj);

    mFlushLocker.Lock();
    mPhysxImpl->DeadObjects.Add(obj);
    mFlushLocker.Unlock();
}

void PhysicsScene::AddActor(PxActor* actor)
{
    ASSERT(actor);

    mFlushLocker.Lock();
    if (IsInMainThread())
    {
        GetScene()->addActor(*actor);
    }
    else
    {
        mPhysxImpl->NewActors.Add(actor);
    }
    mFlushLocker.Unlock();
}

void PhysicsScene::AddActor(PxRigidDynamic* actor, bool putToSleep)
{
    ASSERT(actor);

    mFlushLocker.Lock();
    if (IsInMainThread())
    {
        GetScene()->addActor(*actor);
        if (putToSleep)
            actor->putToSleep();
    }
    else
    {
        mPhysxImpl->NewActors.Add(actor);
        if (putToSleep)
            mPhysxImpl->Actions.Add({ ActionType::Sleep, actor });
    }
    mFlushLocker.Unlock();
}

void PhysicsScene::UnlinkActor(PxActor* actor)
{
    ASSERT(IsInMainThread())
    ASSERT(actor);

    GetScene()->removeActor(*actor);
}

void PhysicsScene::RemoveActor(PxActor* actor)
{
    ASSERT(actor);

    // Unlink ref to flax object
    actor->userData = nullptr;

    mFlushLocker.Lock();
    mPhysxImpl->DeadActors.Add(actor);
    mFlushLocker.Unlock();
}

void PhysicsScene::RemoveCollider(PhysicsColliderActor* collider)
{
    ASSERT(collider);

    mFlushLocker.Lock();
    mPhysxImpl->DeadColliders.Add(collider);
    mFlushLocker.Unlock();
}

void PhysicsScene::RemoveJoint(Joint* joint)
{
    ASSERT(joint);

    mFlushLocker.Lock();
    mPhysxImpl->DeadJoints.Add(joint);
    mFlushLocker.Unlock();
}

PxControllerManager* PhysicsScene::GetControllerManager()
{
    return mPhysxImpl->ControllerManager;
}

#if WITH_VEHICLE
void PhysicsScene::AddWheeledVehicle(WheeledVehicle* vehicle)
{
    mPhysxImpl->WheelVehicles.Add(vehicle);
}

void PhysicsScene::RemoveWheeledVehicle(WheeledVehicle* vehicle)
{
    mPhysxImpl->WheelVehicles.Remove(vehicle);
}
#endif
