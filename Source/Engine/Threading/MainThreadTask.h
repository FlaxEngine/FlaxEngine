// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Task.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Threading/Threading.h"

// Invokes a target method on a main thread (using task or directly if already on main thread)
// Example: INVOKE_ON_MAIN_THREAD(Collector, Collector::SyncData, this);
#define INVOKE_ON_MAIN_THREAD(targetType, targetMethod, targetObject) \
    if (IsInMainThread()) \
	{ \
		targetObject->targetMethod(); \
	} else { \
		Function<void()> action; \
		action.Bind<targetType, &targetMethod>(targetObject); \
		Task::StartNew(New<MainThreadActionTask>(action))->Wait(); \
	}

/// <summary>
/// General purpose task executed on Main Thread in the beginning of the next frame.
/// </summary>
/// <seealso cref="Task" />
class FLAXENGINE_API MainThreadTask : public Task
{
    friend class Engine;
private:
    static void RunAll(float dt);

public:

    /// <summary>
    /// The initial time delay (in seconds) before task execution. Use 0 to skip this feature.
    /// </summary>
    float InitialDelay = 0.0f;

public:

    // [Task]
    String ToString() const override;

protected:

    // [Task]
    void Enqueue() override;
};

/// <summary>
/// General purpose task executing custom action using Main Thread in the beginning of the next frame.
/// </summary>
/// <seealso cref="MainThreadTask" />
/// <seealso cref="Task" />
class FLAXENGINE_API MainThreadActionTask : public MainThreadTask
{
protected:

    Function<void()> _action1;
    Function<bool()> _action2;
    Object* _target;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    MainThreadActionTask(const Function<void()>& action, Object* target = nullptr)
        : MainThreadTask()
        , _action1(action)
        , _target(target)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    MainThreadActionTask(Function<void()>::Signature action, Object* target = nullptr)
        : MainThreadTask()
        , _action1(action)
        , _target(target)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    MainThreadActionTask(const Function<bool()>& action, Object* target = nullptr)
        : MainThreadTask()
        , _action2(action)
        , _target(target)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The target object.</param>
    MainThreadActionTask(Function<bool()>::Signature action, Object* target = nullptr)
        : MainThreadTask()
        , _action2(action)
        , _target(target)
    {
    }

protected:

    // [MainThreadTask]
    bool Run() override;

public:

    // [MainThreadTask]
    bool HasReference(Object* obj) const override;
};
