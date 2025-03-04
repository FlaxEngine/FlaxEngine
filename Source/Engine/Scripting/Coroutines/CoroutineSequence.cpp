// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineSequence.h"


ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable)
{
    _steps.Add(Step{ MoveTemp(runnable) });
    return this;
}

ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenWaitSeconds(const float seconds)
{
    _steps.Add(Step{ seconds });
    return this;
}

ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenWaitFrames(const int32 frames)
{
    _steps.Add(Step{ frames });
    return this;
}

ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenWaitForPoint(const CoroutineSuspendPoint point)
{
    _steps.Add(Step{ point });
    return this;
}

ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate)
{
    _steps.Add(Step{ MoveTemp(predicate) });
    return this;
}

ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenRunFunc(const Function<void()>& runnable)
{
    ScriptingObjectReference<CoroutineRunnable> runnableReference = NewObject<CoroutineRunnable>();
    runnableReference->OnRun.Bind(runnable);
    _steps.Add(Step{ MoveTemp(runnableReference) });
    return this;
}

ScriptingObjectReference<CoroutineSequence> CoroutineSequence::ThenWaitUntilFunc(const Function<void(bool&)>& predicate)
{
    ScriptingObjectReference<CoroutinePredicate> predicateReference = NewObject<CoroutinePredicate>();
    predicateReference->OnCheck.Bind(predicate);
    _steps.Add(Step{ MoveTemp(predicateReference) });
    return this;
}

// Step is a type union initialized using specific constructor argument type. Be careful not to trigger implicit type conversion.

CoroutineSequence::Step::Step(RunnableReference&& runnable)
    : _runnable{ MoveTemp(runnable) }
    , _type{ StepType::Run }
{
}

CoroutineSequence::Step::Step(const CoroutineSuspendPoint suspensionPoint)
    : _suspensionPoint{ suspensionPoint }
    , _type{ StepType::WaitSuspensionPoint }
{
}

CoroutineSequence::Step::Step(const int32 framesDelay)
    : _framesDelay{ framesDelay }
    , _type{ StepType::WaitFrames }
{
}

CoroutineSequence::Step::Step(const float secondsDelay)
    : _secondsDelay{ secondsDelay }
    , _type{ StepType::WaitSeconds }
{
}

CoroutineSequence::Step::Step(PredicateReference&& predicate)
    : _predicate{ MoveTemp(predicate) }
    , _type{ StepType::WaitUntil }
{
}


CoroutineSequence::Step::Step(const Step& other)
    : _type{ StepType::None } // Temp value
{
    ASSERT(other._type != StepType::None);

    EmplaceCopy(other);
}

CoroutineSequence::Step::Step(Step&& other) noexcept
    : _type{ StepType::None } // Temp value
{
    ASSERT(other._type != StepType::None);

    EmplaceMove(MoveTemp(other));
}

void CoroutineSequence::Step::Clear()
{
    switch (_type)
    {
        case StepType::Run:
            _runnable.~RunnableReference();
        break;

        case StepType::WaitUntil:
            _predicate.~PredicateReference();
        break;

        //TODO Add macro for going through all cases.
        case StepType::None:
        case StepType::WaitFrames:
        case StepType::WaitSeconds:
        case StepType::WaitSuspensionPoint:
        default: break;
    }

    _type = StepType::None;
}

void CoroutineSequence::Step::EmplaceCopy(const Step& other)
{
    ASSERT(_type == StepType::None);
    _type = other._type;
    switch (other._type)
    {
        case StepType::Run:
            new (&_runnable) RunnableReference(other._runnable);
        break;

        case StepType::WaitFrames:
            _framesDelay = other._framesDelay;
        break;

        case StepType::WaitSeconds:
            _secondsDelay = other._secondsDelay;
        break;

        case StepType::WaitUntil:
            new (&_predicate) PredicateReference(other._predicate);
        break;

        case StepType::WaitSuspensionPoint:
            _suspensionPoint = other._suspensionPoint;
        break;

        case StepType::None: CRASH;
        default: break;
    }
}

void CoroutineSequence::Step::EmplaceMove(Step&& other) noexcept
{
    ASSERT(_type == StepType::None);
    _type = other._type;
    switch (other._type)
    {
        case StepType::Run:
            new (&_runnable) RunnableReference(MoveTemp(other._runnable));
        break;

        case StepType::WaitFrames:
            _framesDelay = other._framesDelay;
        break;

        case StepType::WaitSeconds:
            _secondsDelay = other._secondsDelay;
        break;

        case StepType::WaitUntil:
            new (&_predicate) PredicateReference(MoveTemp(other._predicate));
        break;

        case StepType::WaitSuspensionPoint:
            _suspensionPoint = other._suspensionPoint;
        break;

        case StepType::None: CRASH;
        default: break;
    }
}

CoroutineSequence::Step::~Step()
{
    Clear();

    ASSERT(_type == StepType::None);
}

CoroutineSequence::Step& CoroutineSequence::Step::operator=(const Step& other)
{
    if (this == &other)
        return *this;

    Clear();

    EmplaceCopy(other);

    return *this;
}

CoroutineSequence::Step& CoroutineSequence::Step::operator=(Step&& other) noexcept
{
    if (this == &other)
        return *this;

    Clear();

    EmplaceMove(MoveTemp(other));

    return *this;
}
