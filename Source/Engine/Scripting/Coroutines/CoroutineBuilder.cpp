// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineBuilder.h"


ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable)
{
    _steps.Add(Step{ MoveTemp(runnable) });
    return this;
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitSeconds(const float seconds)
{
    _steps.Add(Step{ seconds });
    return this;
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitFrames(const int32 frames)
{
    _steps.Add(Step{ frames });
    return this;
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitForPoint(const CoroutineSuspendPoint point)
{
    _steps.Add(Step{ point });
    return this;
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate)
{
    _steps.Add(Step{ MoveTemp(predicate) });
    return this;
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenRunFunc(const Function<void()>& runnable)
{
    ScriptingObjectReference<CoroutineRunnable> runnableReference = NewObject<CoroutineRunnable>();
    runnableReference->OnRun.Bind(runnable);
    _steps.Add(Step{ MoveTemp(runnableReference) });
    return this;
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitUntilFunc(const Function<void(bool&)>& predicate)
{
    ScriptingObjectReference<CoroutinePredicate> predicateReference = NewObject<CoroutinePredicate>();
    predicateReference->OnCheck.Bind(predicate);
    _steps.Add(Step{ MoveTemp(predicateReference) });
    return this;
}

// Step is a type union initialized using specific constructor argument type. Be careful not to trigger implicit type conversion.

CoroutineBuilder::Step::Step(RunnableReference&& runnable)
    : _runnable{ MoveTemp(runnable) }
    , _type{ StepType::Run }
{
}

CoroutineBuilder::Step::Step(const CoroutineSuspendPoint suspensionPoint)
    : _suspensionPoint{ suspensionPoint }
    , _type{ StepType::WaitSuspensionPoint }
{
}

CoroutineBuilder::Step::Step(const int32 framesDelay)
    : _framesDelay{ framesDelay }
    , _type{ StepType::WaitFrames }
{
}

CoroutineBuilder::Step::Step(const float secondsDelay)
    : _secondsDelay{ secondsDelay }
    , _type{ StepType::WaitSeconds }
{
}

CoroutineBuilder::Step::Step(PredicateReference&& predicate)
    : _predicate{ MoveTemp(predicate) }
    , _type{ StepType::WaitUntil }
{
}


CoroutineBuilder::Step::Step(const Step& other)
    : _type{ StepType::None } // Temp value
{
    ASSERT(other._type != StepType::None);

    EmplaceCopy(other);
}

CoroutineBuilder::Step::Step(Step&& other) noexcept
    : _type{ StepType::None } // Temp value
{
    ASSERT(other._type != StepType::None);

    EmplaceMove(MoveTemp(other));
}

void CoroutineBuilder::Step::Clear()
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

void CoroutineBuilder::Step::EmplaceCopy(const Step& other)
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
        default:             break;
    }
}

void CoroutineBuilder::Step::EmplaceMove(Step&& other) noexcept
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
        default:             break;
    }
}

CoroutineBuilder::Step::~Step()
{
    Clear();

    ASSERT(_type == StepType::None);
}

auto CoroutineBuilder::Step::operator=(const Step& other) -> Step&
{
    if (this == &other)
        return *this;

    Clear();

    EmplaceCopy(other);

    return *this;
}

auto CoroutineBuilder::Step::operator=(Step&& other) noexcept -> Step&
{
    if (this == &other)
        return *this;

    Clear();

    EmplaceMove(MoveTemp(other));

    return *this;
}


CoroutineBuilder::StepType CoroutineBuilder::Step::GetType() const
{
    return _type;
}

auto CoroutineBuilder::Step::GetRunnable() const -> const RunnableReference&
{
    ASSERT(_type == StepType::Run);
    return _runnable;
}

auto CoroutineBuilder::Step::GetPredicate() const -> const PredicateReference&
{
    ASSERT(_type == StepType::WaitUntil);
    return _predicate;
}

auto CoroutineBuilder::Step::GetFramesDelay() const -> int32
{
    ASSERT(_type == StepType::WaitFrames);
    return _framesDelay;
}

auto CoroutineBuilder::Step::GetSecondsDelay() const -> float
{
    ASSERT(_type == StepType::WaitSeconds);
    return _secondsDelay;
}

auto CoroutineBuilder::Step::GetSuspensionPoint() const -> CoroutineSuspendPoint
{
    ASSERT(_type == StepType::WaitSuspensionPoint);
    return _suspensionPoint;
}
