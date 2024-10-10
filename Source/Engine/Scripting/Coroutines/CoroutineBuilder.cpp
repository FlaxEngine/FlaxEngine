// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineBuilder.h"


ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable)
{
    return this; //TODO(mtszkarbowiak) Implement running a coroutine.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitSeconds(const float seconds)
{
    return this; //TODO(mtszkarbowiak) Implement waiting for seconds.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitFrames(int32 frames)
{
    return this; //TODO(mtszkarbowiak) Implement waiting for frames.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate)
{
    return this; //TODO(mtszkarbowiak) Implement waiting until predicate.
}


CoroutineBuilder::Step::Step()
    : _type{ StepType::None }
{
}

CoroutineBuilder::Step::Step(RunnableReference runnable)
    : _type{ StepType::Run }
{
    _runnable = MoveTemp(runnable);
}

CoroutineBuilder::Step::Step(const int32 framesDelay)
    : _type{ StepType::WaitFrames }
{
    _framesDelay = framesDelay;
}

CoroutineBuilder::Step::Step(const float secondsDelay)
    : _type{ StepType::WaitSeconds }
{
    _secondsDelay = secondsDelay;
}

CoroutineBuilder::Step::Step(PredicateReference predicate)
    : _type{ StepType::WaitUntil }
{
    _predicate = MoveTemp(predicate);
}


CoroutineBuilder::Step::Step(const Step& other)
    : _type{ other._type }
{
    ASSERT(_type != StepType::None);

    switch (_type)
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

        case StepType::None: CRASH;
        default:             break;
    }
}

CoroutineBuilder::Step::Step(Step&& other) noexcept
    : _type{ other._type }
{
    ASSERT(_type != StepType::None);

    switch (_type)
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

        case StepType::None: CRASH;
        default:             break;
    }
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
        default: break;
    }

    _type = StepType::None;
}

CoroutineBuilder::Step::~Step()
{
    Clear();

    ASSERT(_type == StepType::None);
}

auto CoroutineBuilder::Step::operator=(const Step& other) -> Step&
{
    if (this == &other)
    {
        return *this;
    }

    Clear();

    _type = other._type;

    switch (_type)
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

        case StepType::None: CRASH;
        default:             break;
    }

    return *this;
}

auto CoroutineBuilder::Step::operator=(Step&& other) noexcept -> Step&
{
    if (this == &other)
    {
        return *this;
    }

    Clear();

    _type = other._type;

    switch (_type)
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

        case StepType::None: CRASH;
        default:             break;
    }

    return *this;
}
