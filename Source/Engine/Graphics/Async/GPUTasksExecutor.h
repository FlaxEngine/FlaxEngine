// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Core/Collections/Array.h"
#include "GPUTasksContext.h"

class GPUTask;

/// <summary>
/// Describes object responsible for GPU jobs execution scheduling.
/// </summary>
class GPUTasksExecutor : public Object
{
protected:
    Array<GPUTasksContext*> _contextList;

public:
    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~GPUTasksExecutor();

public:
    /// <summary>
    /// Sync point event called on begin of the frame
    /// </summary>
    virtual void FrameBegin() = 0;

    /// <summary>
    /// Sync point event called on end of the frame
    /// </summary>
    virtual void FrameEnd() = 0;

public:
    /// <summary>
    /// Gets the context list.
    /// </summary>
    FORCE_INLINE const Array<GPUTasksContext*>* GetContextList() const
    {
        return &_contextList;
    }

protected:
    GPUTasksContext* createContext();
};
