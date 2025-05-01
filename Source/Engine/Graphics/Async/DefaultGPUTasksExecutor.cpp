// Copyright (c) Wojciech Figat. All rights reserved.

#include "DefaultGPUTasksExecutor.h"
#include "GPUTasksContext.h"
#include "GPUTask.h"
#include "GPUTasksManager.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Profiler/ProfilerCPU.h"

DefaultGPUTasksExecutor::DefaultGPUTasksExecutor()
    : _context(nullptr)
{
}

String DefaultGPUTasksExecutor::ToString() const
{
    return TEXT("Default GPU Async Executor");
}

void DefaultGPUTasksExecutor::FrameBegin()
{
    PROFILE_CPU();

    // Ensure to have valid async context
    if (_context == nullptr)
        _context = createContext();

    _context->OnFrameBegin();

    // Default implementation performs async operations on start of the frame which is synchronized with a rendering thread
    GPUTask* buffer[32];
    const int32 count = GPUDevice::Instance->GetTasksManager()->RequestWork(buffer, 32);
    for (int32 i = 0; i < count; i++)
    {
        _context->Run(buffer[i]);
    }
}

void DefaultGPUTasksExecutor::FrameEnd()
{
    PROFILE_CPU();
    ASSERT(_context != nullptr);
    _context->OnFrameEnd();
}
