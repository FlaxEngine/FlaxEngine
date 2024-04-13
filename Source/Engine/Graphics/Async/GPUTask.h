// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/Task.h"
#include "Engine/Platform/Platform.h"
#include "GPUTasksContext.h"

class GPUResource;

/// <summary>
/// Describes GPU work object.
/// </summary>
class GPUTask : public Task
{
    friend GPUTasksContext;

public:
    /// <summary>
    /// Describes GPU work type
    /// </summary>
    DECLARE_ENUM_4(Type, Custom, CopyResource, UploadTexture, UploadBuffer);

    /// <summary>
    /// Describes GPU work result value
    /// </summary>
    DECLARE_ENUM_4(Result, Ok, Failed, MissingResources, MissingData);

private:
    /// <summary>
    /// Task type
    /// </summary>
    Type _type;

    /// <summary>
    /// Synchronization point when async task has been done
    /// </summary>
    GPUSyncPoint _syncPoint;

    /// <summary>
    /// The context that performed this task, it's should synchronize it.
    /// </summary>
    GPUTasksContext* _context;

protected:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTask"/> class.
    /// </summary>
    /// <param name="type">The type.</param>
    GPUTask(const Type type)
        : _type(type)
        , _syncPoint(0)
        , _context(nullptr)
    {
    }

public:
    /// <summary>
    /// Gets a task type.
    /// </summary>
    /// <returns>The type.</returns>
    FORCE_INLINE Type GetType() const
    {
        return _type;
    }

    /// <summary>
    /// Gets work finish synchronization point
    /// </summary>
    /// <returns>Finish task sync point</returns>
    FORCE_INLINE GPUSyncPoint GetSyncPoint() const
    {
        return _syncPoint;
    }

public:
    /// <summary>
    /// Checks if operation is syncing
    /// </summary>
    /// <returns>True if operation is syncing, otherwise false</returns>
    FORCE_INLINE bool IsSyncing() const
    {
        return IsRunning() && _syncPoint != 0;
    }

public:
    /// <summary>
    /// Executes this task.
    /// </summary>
    /// <param name="context">The context.</param>
    void Execute(GPUTasksContext* context);

    /// <summary>
    /// Action fired when asynchronous operation has been synchronized with a GPU
    /// </summary>
    void Sync()
    {
        if (_context != nullptr)
        {
            ASSERT(IsSyncing());

            // Sync and finish
            _context = nullptr;
            OnSync();
            OnFinish();
        }
    }

    /// <summary>
    /// Cancels the task results synchronization.
    /// </summary>
    void CancelSync()
    {
        ASSERT(IsSyncing());

        // Rollback state and cancel
        _context = nullptr;
        SetState(TaskState::Queued);
        Cancel();
    }

protected:
    virtual Result run(GPUTasksContext* context) = 0;

    virtual void OnSync()
    {
    }

public:
    // [Task]
    String ToString() const override;

protected:
    // [Task]
    void Enqueue() override;

    bool Run() override
    {
        return true;
    }

    void OnCancel() override
    {
        // Check if task is waiting for sync (very likely situation)
        if (IsSyncing())
        {
            // Task has been performed but is waiting for a CPU/GPU sync so we have to cancel that
            ASSERT(_context != nullptr);
            _context->OnCancelSync(this);
            _context = nullptr;
            SetState(TaskState::Canceled);
        }
        else
        {
            // Maybe we could also handle cancel event during running but not yet syncing
            ASSERT(!IsRunning());
        }

        // Base
        Task::OnCancel();
    }
};
