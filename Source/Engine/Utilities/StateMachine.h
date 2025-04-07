// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"

class StateMachine;

/// <summary>
/// State machine state
/// </summary>
class FLAXENGINE_API State
{
    friend StateMachine;

protected:
    StateMachine* _parent;

protected:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="parent">Parent state machine</param>
    State(StateMachine* parent);

    /// <summary>
    /// Destructor
    /// </summary>
    ~State();

public:
    /// <summary>
    /// State's activity
    /// </summary>
    /// <returns>True if state is active, otherwise false</returns>
    bool IsActive() const;

    /// <summary>
    /// Checks if can enter to that state
    /// </summary>
    /// <returns>True if can enter to that state, otherwise false</returns>
    virtual bool CanEnter() const
    {
        return true;
    }

    /// <summary>
    /// Checks if can exit from that state
    /// </summary>
    /// <param name="nextState">Next state to enter after exit from the current state</param>
    /// <returns>True if can exit from that state, otherwise false</returns>
    virtual bool CanExit(State* nextState) const
    {
        return true;
    }

protected:
    virtual void enterState()
    {
    }

    virtual void exitState()
    {
    }
};

/// <summary>
/// State machine logic pattern
/// </summary>
class FLAXENGINE_API StateMachine
{
    friend State;

protected:
    State* _currentState;
    Array<State*> _states;

public:
    /// <summary>
    /// Init
    /// </summary>
    StateMachine();

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~StateMachine();

public:
    /// <summary>
    /// Gets current state
    /// </summary>
    /// <returns>Current state</returns>
    virtual State* CurrentState()
    {
        return _currentState;
    }

    /// <summary>
    /// Gets readonly states array
    /// </summary>
    /// <returns>Array with states</returns>
    const Array<State*>* GetStates() const
    {
        return &_states;
    }

public:
    /// <summary>
    /// Go to state
    /// </summary>
    /// <param name="stateIndex">Target state index</param>
    void GoToState(int32 stateIndex)
    {
        GoToState(_states[stateIndex]);
    }

    /// <summary>
    /// Go to state
    /// </summary>
    /// <param name="state">Target state</param>
    virtual void GoToState(State* state);

protected:
    virtual void switchState(State* nextState);
};
