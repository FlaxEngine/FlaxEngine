// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Object.h"
#include "Engine/Core/Delegate.h"

/// <summary>
/// Interface for runnable objects for multi-threading purposes.
/// </summary>
class IRunnable : public Object
{
public:

    // Virtual destructor
    virtual ~IRunnable()
    {
    }

    // Initializes the runnable object
    // @return True if initialization was successful, otherwise false
    virtual bool Init()
    {
        return true;
    }

    // Runs the runnable object.
    // @return The exit code
    virtual int32 Run() = 0;

    // Stops the runnable object. Called when thread is being terminated
    virtual void Stop()
    {
    }

    // Exits the runnable object
    virtual void Exit()
    {
    }

    // Called when thread ends work (via Kill or normally)
    // @param wasKilled True if thread has been killed
    virtual void AfterWork(bool wasKilled)
    {
    }
};

/// <summary>
/// Simple runnable object for single function bind
/// </summary>
class SimpleRunnable : public IRunnable
{
private:

    bool _autoDelete;

public:

    /// <summary>
    /// Working function
    /// </summary>
    Function<int32()> OnWork;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="autoDelete">True if delete itself after work.</param>
    SimpleRunnable(bool autoDelete)
        : _autoDelete(autoDelete)
    {
    }

public:

    // [IRunnable]
    String ToString() const override
    {
        return TEXT("SimpleRunnable");
    }

    int32 Run() override
    {
        int32 result = -1;
        if (OnWork.IsBinded())
            result = OnWork();
        return result;
    }

    void AfterWork(bool wasKilled) override
    {
        if (_autoDelete)
            Delete(this);
    }
};
