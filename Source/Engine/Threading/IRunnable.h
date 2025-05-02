// Copyright (c) Wojciech Figat. All rights reserved.

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

    /// <summary>
    /// Initializes the runnable object.
    /// </summary>
    /// <returns>True if initialization was successful, otherwise false.</returns>
    virtual bool Init()
    {
        return true;
    }

    /// <summary>
    /// Executes the runnable object.
    /// </summary>
    /// <returns>The exit code. Non-zero on error.</returns>
    virtual int32 Run() = 0;

    /// <summary>
    /// Stops the runnable object. Called when thread is being terminated
    /// </summary>
    virtual void Stop()
    {
    }

    /// <summary>
    /// Exits the runnable object
    /// </summary>
    virtual void Exit()
    {
    }

    /// <summary>
    /// Called when thread ends work (via Kill or normally).
    /// </summary>
    /// <param name="wasKilled">True if thread has been killed.</param>
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
