// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Threading/ThreadPool.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Platform.h"
#if USE_EDITOR
#if !COMPILE_WITH_DEBUG_DRAW
#define COMPILE_WITH_DEBUG_DRAW 1
#define COMPILE_WITH_DEBUG_DRAW_HACK
#endif
#include "Engine/Debug/DebugDraw.h"
#ifdef COMPILE_WITH_DEBUG_DRAW_HACK
#undef COMPILE_WITH_DEBUG_DRAW_HACK
#undef COMPILE_WITH_DEBUG_DRAW
#define COMPILE_WITH_DEBUG_DRAW 0
#endif
#endif

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
#if USE_EDITOR
            // Flush any single-frame shapes to prevent memory leaking (eg. via terrain collision debug during scene drawing with PhysicsColliders or PhysicsDebug flag)
            DebugDraw::UpdateContext(nullptr, 0.0f);
#endif
            Engine::OnDraw();
            Platform::Sleep(20);
        }
        return false;
    }
};
