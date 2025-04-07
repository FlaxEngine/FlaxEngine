// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PHYSX

#include "Types.h"
#include <ThirdParty/PhysX/task/PxTask.h>
#include <ThirdParty/PhysX/foundation/PxSimpleTypes.h>
#include <ThirdParty/PhysX/foundation/PxSync.h>

class MultiThreadStepper;

class PhysicsStepper
{
public:
    PhysicsStepper()
    {
    }

    virtual ~PhysicsStepper()
    {
    }

public:
    virtual bool advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize) = 0;
    virtual void wait(PxScene* scene) = 0;
    virtual void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) = 0;

    virtual void setSubStepper(const PxReal stepSize, const PxU32 maxSteps)
    {
    }

    virtual void renderDone()
    {
    }
};

class StepperTask : public PxLightCpuTask
{
public:
    void setStepper(MultiThreadStepper* stepper)
    {
        mStepper = stepper;
    }

    MultiThreadStepper* getStepper()
    {
        return mStepper;
    }

    const MultiThreadStepper* getStepper() const
    {
        return mStepper;
    }

public:
    // [PxLightCpuTask]
    const char* getName() const override
    {
        return "Stepper Task";
    }

    void run() override;

protected:
    MultiThreadStepper* mStepper = nullptr;
};

/// <summary>
/// Physics simulation task used by the stepper.
/// </summary>
/// <seealso cref="StepperTask" />
class StepperTaskSimulate : public StepperTask
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="StepperTaskSimulate"/> class.
    /// </summary>
    StepperTaskSimulate()
    {
    }

public:
    // [StepperTask]
    void run() override;
};

class MultiThreadStepper : public PhysicsStepper
{
public:
    MultiThreadStepper()
        : mFirstCompletionPending(false)
        , mScene(nullptr)
        , mSync(nullptr)
        , mCurrentSubStep(0)
        , mNbSubSteps(0)
    {
        mCompletion0.setStepper(this);
        mCompletion1.setStepper(this);
        mSimulateTask.setStepper(this);
    }

    ~MultiThreadStepper();

    bool advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize) override;
    virtual void substepDone(StepperTask* ownerTask);
    void renderDone() override;

    // if mNbSubSteps is 0 then the sync will never 
    // be set so waiting would cause a deadlock
    void wait(PxScene* scene) override
    {
        if (mNbSubSteps && mSync)
            mSync->wait();
    }

    virtual void shutdown();

    virtual void reset() = 0;
    void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) override = 0;
    virtual void simulate(PxBaseTask* ownerTask);

    PxReal getSubStepSize() const
    {
        return mSubStepSize;
    }

protected:
    void substep(StepperTask& completionTask);

    // we need two completion tasks because when multistepping we can't submit completion0 from the
    // substepDone function which is running inside completion0
    bool mFirstCompletionPending;
    StepperTaskSimulate mSimulateTask;
    StepperTask mCompletion0, mCompletion1;
    PxScene* mScene;
    PxSync* mSync;

    PxU32 mCurrentSubStep;
    PxU32 mNbSubSteps;
    PxReal mSubStepSize;
    void* mScratchBlock;
    PxU32 mScratchBlockSize;
};

// The way this should be called is:
// bool stepped = advance(dt)
//
// ... reads from the scene graph for rendering
//
// if(stepped) renderDone()
//
// ... anything that doesn't need access to the the physics scene
//
// if(stepped) sFixedStepper.wait()
//
// Note that per-substep callbacks to the sample need to be issued out of here, 
// between fetchResults and simulate

class FixedStepper : public MultiThreadStepper
{
protected:
    PxReal mAccumulator;
    PxReal mFixedSubStepSize;
    PxU32 mMaxSubSteps;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="FixedStepper"/> class.
    /// </summary>
    FixedStepper()
        : MultiThreadStepper()
        , mAccumulator(0)
        , mFixedSubStepSize(0.013f)
        , mMaxSubSteps(1)
    {
    }

public:
    /// <summary>
    /// Setups the specified step size and the maximum amount of them.
    /// </summary>
    /// <param name="stepSize">Size of the step (in seconds).</param>
    /// <param name="maxSubsetps">The maximum amount of subsetps.</param>
    void Setup(const float stepSize, const uint32 maxSubsetps = 1)
    {
        mFixedSubStepSize = stepSize;
        mMaxSubSteps = maxSubsetps;
    }

public:
    // [MultiThreadStepper]
    void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) override;

    void reset() override
    {
        mAccumulator = 0.0f;
    }

    void setSubStepper(const PxReal stepSize, const PxU32 maxSteps) override
    {
        mFixedSubStepSize = stepSize;
        mMaxSubSteps = maxSteps;
    }
};

#endif
