// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Task.h"

class ThreadPool;

/// <summary>
/// General purpose task executed using Thread Pool.
/// </summary>
/// <seealso cref="Task" />
class ThreadPoolTask : public Task
{
    friend ThreadPool;

protected:

    /// <summary>
    /// Initializes a new instance of the <see cref="ThreadPoolTask"/> class.
    /// </summary>
    ThreadPoolTask()
        : Task()
    {
    }

public:

    // [Task]
    String ToString() const override;

protected:

    // [Task]
    void Enqueue() override;
};

/// <summary>
/// General purpose task executing custom action using Thread Pool.
/// </summary>
/// <seealso cref="ThreadPoolTask" />
/// <seealso cref="Task" />
class ThreadPoolActionTask : public ThreadPoolTask
{
protected:

    Function<void()> _action1;
    Function<bool()> _action2;
    Object* _target;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    ThreadPoolActionTask(const Function<void()>& action, Object* target = nullptr)
        : ThreadPoolTask()
        , _action1(action)
        , _target(target)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    ThreadPoolActionTask(Function<void()>::Signature action, Object* target = nullptr)
        : ThreadPoolTask()
        , _action1(action)
        , _target(target)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    ThreadPoolActionTask(const Function<bool()>& action, Object* target = nullptr)
        : ThreadPoolTask()
        , _action2(action)
        , _target(target)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    ThreadPoolActionTask(Function<bool()>::Signature action, Object* target = nullptr)
        : ThreadPoolTask()
        , _action2(action)
        , _target(target)
    {
    }

public:

    // [ThreadPoolTask]
    bool HasReference(Object* obj) const override
    {
        return obj == _target;
    }

protected:

    // [ThreadPoolTask]
    bool Run() override
    {
        if (_action1.IsBinded())
        {
            _action1();
            return false;
        }

        if (_action2.IsBinded())
        {
            return _action2();
        }

        return false;
    }
};
