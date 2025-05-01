// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUTasksExecutor.h"

/// <summary>
/// Default implementation for GPU async job execution
/// </summary>
class DefaultGPUTasksExecutor : public GPUTasksExecutor
{
protected:
    GPUTasksContext* _context;

public:
    /// <summary>
    /// Init
    /// </summary>
    DefaultGPUTasksExecutor();

public:
    // [GPUTasksExecutor]
    String ToString() const override;
    void FrameBegin() override;
    void FrameEnd() override;
};
