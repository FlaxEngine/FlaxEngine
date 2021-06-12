// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Scripting.h"
#include "ScriptingType.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Scripting/InternalCalls.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Threading/Threading.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-gc.h>

namespace ProfilerInternal
{
#if COMPILE_WITH_PROFILER
    Array<int32> ManagedEventsGPU;
#endif

    void BeginEvent(MonoString* nameObj)
    {
#if COMPILE_WITH_PROFILER
        ProfilerCPU::BeginEvent((const Char*)mono_string_chars(nameObj));
#endif
    }

    void EndEvent()
    {
#if COMPILE_WITH_PROFILER
        ProfilerCPU::EndEvent();
#endif
    }

    void BeginEventGPU(MonoString* nameObj)
    {
#if COMPILE_WITH_PROFILER
        const auto index = ProfilerGPU::BeginEvent((const Char*)mono_string_chars(nameObj));
        ManagedEventsGPU.Push(index);
#endif
    }

    void EndEventGPU()
    {
#if COMPILE_WITH_PROFILER
        const auto index = ManagedEventsGPU.Pop();
        ProfilerGPU::EndEvent(index);
#endif
    }
}

class ScriptingInternal
{
public:
    static bool HasGameModulesLoaded()
    {
        return Scripting::HasGameModulesLoaded();
    }

    static bool IsTypeFromGameScripts(MonoReflectionType* type)
    {
        return Scripting::IsTypeFromGameScripts(Scripting::FindClass(MUtils::GetClass(type)));
    }

    static void FlushRemovedObjects()
    {
        ASSERT(IsInMainThread());
        ObjectsRemovalService::Flush();
    }

    static void InitRuntime()
    {
        // Scripting API
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::HasGameModulesLoaded", &HasGameModulesLoaded);
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::IsTypeFromGameScripts", &IsTypeFromGameScripts);
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::FlushRemovedObjects", &FlushRemovedObjects);

        // Profiler API
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEvent", &ProfilerInternal::BeginEvent);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEvent", &ProfilerInternal::EndEvent);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEventGPU", &ProfilerInternal::BeginEventGPU);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEventGPU", &ProfilerInternal::EndEventGPU);
    }
};

IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(Scripting, FlaxEngine, "FlaxEngine.Scripting", nullptr, nullptr);
