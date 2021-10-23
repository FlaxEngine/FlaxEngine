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
#if TRACY_ENABLE && !PROFILE_CPU_USE_TRANSIENT_DATA
#include "Engine/Core/Collections/ChunkedArray.h"
#endif
#include "Engine/Core/Types/Pair.h"
#include "Engine/Threading/Threading.h"
#if USE_MONO
#include <ThirdParty/mono-2.0/mono/metadata/mono-gc.h>
#endif

#if USE_MONO

namespace ProfilerInternal
{
#if COMPILE_WITH_PROFILER
    Array<int32, InlinedAllocation<32>> ManagedEventsGPU;
#if TRACY_ENABLE && !PROFILE_CPU_USE_TRANSIENT_DATA
    CriticalSection ManagedSourceLocationsLocker;

    struct Location
    {
        String Name;
        StringAnsi NameAnsi;
        tracy::SourceLocationData SrcLocation;
    };

    ChunkedArray<Location, 256> ManagedSourceLocations;
#endif
#endif

    void BeginEvent(MonoString* nameObj)
    {
#if COMPILE_WITH_PROFILER
        const StringView name((const Char*)mono_string_chars(nameObj), mono_string_length(nameObj));
        ProfilerCPU::BeginEvent(*name);
#if TRACY_ENABLE
#if PROFILE_CPU_USE_TRANSIENT_DATA
        tracy::ScopedZone::Begin(__LINE__, __FILE__, strlen( __FILE__ ), __FUNCTION__, strlen( __FUNCTION__ ), name.Get(), name.Length() );
#else
        ScopeLock lock(ManagedSourceLocationsLocker);
        tracy::SourceLocationData* srcLoc = nullptr;
        for (auto e = ManagedSourceLocations.Begin(); e.IsNotEnd(); ++e)
        {
            if (name == e->Name)
            {
                srcLoc = &e->SrcLocation;
                break;
            }
        }
        if (!srcLoc)
        {
            auto& e = ManagedSourceLocations.AddOne();
            e.Name = name;
            e.NameAnsi = name.Get();
            srcLoc = &e.SrcLocation;
            srcLoc->name = e.NameAnsi.Get();
            srcLoc->function = nullptr;
            srcLoc->file = nullptr;
            srcLoc->line = 0;
            srcLoc->color = 0;
        }
        //static constexpr tracy::SourceLocationData tracySrcLoc{ nullptr, __FUNCTION__, __FILE__, (uint32_t)__LINE__, 0 };
        tracy::ScopedZone::Begin(srcLoc);
#endif
#endif
#endif
    }

    void EndEvent()
    {
#if COMPILE_WITH_PROFILER
#if TRACY_ENABLE
        tracy::ScopedZone::End();
#endif
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

#endif

class ScriptingInternal
{
public:
#if USE_MONO
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
#endif

    static void InitRuntime()
    {
#if USE_MONO
        // Scripting API
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::HasGameModulesLoaded", &HasGameModulesLoaded);
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::IsTypeFromGameScripts", &IsTypeFromGameScripts);
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::FlushRemovedObjects", &FlushRemovedObjects);

        // Profiler API
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEvent", &ProfilerInternal::BeginEvent);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEvent", &ProfilerInternal::EndEvent);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEventGPU", &ProfilerInternal::BeginEventGPU);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEventGPU", &ProfilerInternal::EndEventGPU);
#endif
    }
};

IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(Scripting, FlaxEngine, "FlaxEngine.Scripting", nullptr, nullptr);
