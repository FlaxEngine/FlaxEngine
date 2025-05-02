// Copyright (c) Wojciech Figat. All rights reserved.

#include "StateMachine.h"

State::State(StateMachine* parent)
    : _parent(parent)
{
    _parent->_states.Add(this);
}

State::~State()
{
    if (_parent)
        _parent->_states.Remove(this);
}

bool State::IsActive() const
{
    return _parent->_currentState == this;
}

StateMachine::StateMachine()
    : _currentState(nullptr)
{
}

StateMachine::~StateMachine()
{
    for (int32 i = 0; i < _states.Count(); i++)
    {
        _states[i]->_parent = nullptr;
    }
}

void StateMachine::GoToState(State* state)
{
    // Prevent from entering the same state
    if (state == _currentState)
        return;

    // Check if cannot leave current state
    if (_currentState && !_currentState->CanExit(state))
        return;

    // Check if cannot enter new state
    if (state && !state->CanEnter())
        return;

    // Change state
    switchState(state);
}

void StateMachine::switchState(State* nextState)
{
    if (_currentState)
        _currentState->exitState();

    _currentState = nextState;

    if (_currentState)
        _currentState->enterState();
}
