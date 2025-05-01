// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Delegate.h"

class IRunnable;

/// <summary>
/// Base class for thread objects.
/// </summary>
/// <remarks>Ensure to call Kill or Join before deleting thread object.</remarks>
class FLAXENGINE_API ThreadBase : public Object, public NonCopyable
{
public:

    /// <summary>
    /// The custom event called before thread execution just after startup. Can be used to setup per-thread state or data. Argument is: thread handle.
    /// </summary>
    static Delegate<Thread*> ThreadStarting;

    /// <summary>
    /// The custom event called after thread execution just before exit. Can be used to cleanup per-thread state or data. Arguments are: thread handle and exit code.
    /// </summary>
    static Delegate<Thread*, int32> ThreadExiting;

protected:

    IRunnable* _runnable;
#if BUILD_DEBUG
    String _runnableName;
#endif
    ThreadPriority _priority;
    String _name;
    uint64 _id;
    bool _isRunning;
    bool _callAfterWork;

    ThreadBase(IRunnable* runnable, const String& name, ThreadPriority priority);

public:

    /// <summary>
    /// Gets priority level of the thread.
    /// </summary>
    FORCE_INLINE ThreadPriority GetPriority() const
    {
        return _priority;
    }

    /// <summary>
    /// Sets priority level of the thread
    /// </summary>
    /// <param name="priority">The thread priority to change to.</param>
    void SetPriority(ThreadPriority priority);

    /// <summary>
    /// Gets thread ID
    /// </summary>
    FORCE_INLINE uint64 GetID() const
    {
        return _id;
    }

    /// <summary>
    /// Gets thread running state.
    /// </summary>
    FORCE_INLINE bool IsRunning() const
    {
        return _isRunning;
    }

    /// <summary>
    /// Gets name of the thread.
    /// </summary>
    FORCE_INLINE const String& GetName() const
    {
        return _name;
    }

public:

    /// <summary>
    /// Aborts the thread execution by force.
    /// </summary>
    /// <param name="waitForJoin">True if should wait for thread end, otherwise false.</param>
    void Kill(bool waitForJoin = false);

    /// <summary>
    /// Stops the current thread execution and waits for the thread execution end.
    /// </summary>
    virtual void Join() = 0;

protected:

    int32 Run();

    virtual void ClearHandleInternal() = 0;
    virtual void SetPriorityInternal(ThreadPriority priority) = 0;
    virtual void KillInternal(bool waitForJoin) = 0;

public:

    // [Object]
    String ToString() const override
    {
        return _name;
    }
};
