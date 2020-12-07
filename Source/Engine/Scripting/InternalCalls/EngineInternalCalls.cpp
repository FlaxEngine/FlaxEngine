// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Engine/Platform/FileSystem.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Animations/Graph/AnimGraph.h"
#include "Engine/Scripting/InternalCalls.h"

namespace UtilsInternal
{
    void MemoryCopy(void* source, void* destination, int32 length)
    {
        Platform::MemoryCopy(destination, source, length);
    }
}

void registerFlaxEngineInternalCalls()
{
    DebugLog::RegisterInternalCalls();
    AnimGraphExecutor::initRuntime();
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCopy", &UtilsInternal::MemoryCopy);
}
