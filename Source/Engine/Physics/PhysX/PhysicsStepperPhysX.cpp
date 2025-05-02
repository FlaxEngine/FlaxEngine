// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PHYSX

#include "PhysicsStepperPhysX.h"
#include "PhysicsBackendPhysX.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/PhysX/foundation/PxMath.h>
#include <ThirdParty/PhysX/PxSceneLock.h>

void StepperTask::run()
{
    mStepper->substepDone(this);
    release();
}

void StepperTaskSimulate::run()
{
    mStepper->simulate(mCont);
    // -> OnSubstepStart
}

MultiThreadStepper::~MultiThreadStepper()
{
    if (mSync)
        Delete(mSync);
}

bool MultiThreadStepper::advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize)
{
    mScratchBlock = scratchBlock;
    mScratchBlockSize = scratchBlockSize;

    if (!mSync)
        mSync = New<PxSync>();

    substepStrategy(dt, mNbSubSteps, mSubStepSize);

    if (mNbSubSteps == 0)
        return false;

    mScene = scene;

    mSync->reset();

    mCurrentSubStep = 1;

    mCompletion0.setContinuation(*mScene->getTaskManager(), nullptr);

    // Take the first substep
    substep(mCompletion0);
    mFirstCompletionPending = true;

    return true;
}

void MultiThreadStepper::substepDone(StepperTask* ownerTask)
{
    // -> OnSubstepPreFetchResult

    {
#ifndef PX_PROFILE
		PxSceneWriteLock writeLock(*mScene);
#endif
        mScene->fetchResults(true);
    }

    // -> OnSubstep
    PhysicsBackendPhysX::SimulationStepDone(mScene, mSubStepSize);

    if (mCurrentSubStep >= mNbSubSteps)
    {
        mSync->set();
    }
    else
    {
        StepperTask& s = ownerTask == &mCompletion0 ? mCompletion1 : mCompletion0;
        s.setContinuation(*mScene->getTaskManager(), nullptr);
        mCurrentSubStep++;

        substep(s);

        // After the first substep, completions run freely
        s.removeReference();
    }
}

void MultiThreadStepper::renderDone()
{
    if (mFirstCompletionPending)
    {
        mCompletion0.removeReference();
        mFirstCompletionPending = false;
    }
}

void MultiThreadStepper::shutdown()
{
    SAFE_DELETE(mSync);
}

void MultiThreadStepper::simulate(PxBaseTask* ownerTask)
{
    PROFILE_CPU_NAMED("Physics.Simulate");

    PxSceneWriteLock writeLock(*mScene);

    mScene->simulate(mSubStepSize, ownerTask, mScratchBlock, mScratchBlockSize);
}

void MultiThreadStepper::substep(StepperTask& completionTask)
{
    // Setup any tasks that should run in parallel to simulate()

    // -> OnSubstepSetup

    // step
    {
        mSimulateTask.setContinuation(&completionTask);
        mSimulateTask.removeReference();
    }
    // parallel sample tasks are started in mSolveTask (after solve was called which acquires a write lock).
}

void FixedStepper::substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize)
{
    if (mAccumulator > mFixedSubStepSize)
        mAccumulator = 0.0f;

    // Don't step less than the step size, just accumulate
    mAccumulator += stepSize;
    if (mAccumulator < mFixedSubStepSize)
    {
        substepCount = 0;
        return;
    }

    substepSize = mFixedSubStepSize;
    substepCount = PxMin(PxU32(mAccumulator / mFixedSubStepSize), mMaxSubSteps);

    mAccumulator -= PxReal(substepCount) * substepSize;
}

#endif
