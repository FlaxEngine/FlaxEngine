// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Threading/ThreadPool.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Async DoDragDrop helper (used for rendering frames during main thread stall).
/// </summary>
class DoDragDropJob : public ThreadPoolTask
{
public:
    int64 ExitFlag = 0;

    // [ThreadPoolTask]
    bool Run() override
    {
        Scripting::GetScriptsDomain()->Dispatch();
        while (Platform::AtomicRead(&ExitFlag) == 0)
        {
            Engine::OnDraw();
            Platform::Sleep(20);
        }
        return false;
    }
};
