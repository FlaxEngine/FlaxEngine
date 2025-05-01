// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/Platform.h"
#include "Engine/Animations/Graph/AnimGraph.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#if USE_MONO
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#endif
#include "Engine/Scripting/Internal/InternalCalls.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Profiler/Profiler.h"
#include "FlaxEngine.Gen.h"
#if TRACY_ENABLE && !PROFILE_CPU_USE_TRANSIENT_DATA
#include "Engine/Core/Collections/ChunkedArray.h"
#endif
#include "Engine/Threading/Threading.h"

#if !COMPILE_WITHOUT_CSHARP

#if USE_MONO
DEFINE_INTERNAL_CALL(MObject*) UtilsInternal_ExtractArrayFromList(MObject* obj)
{
    MClass* klass = MCore::Object::GetClass(obj);
    MField* field = klass->GetField("_items");
    MObject* o;
    field->GetValue(obj, &o);
    return o;
}
#endif

DEFINE_INTERNAL_CALL(void) PlatformInternal_MemoryCopy(void* dst, const void* src, uint64 size)
{
    Platform::MemoryCopy(dst, src, size);
}

DEFINE_INTERNAL_CALL(void) PlatformInternal_MemoryClear(void* dst, uint64 size)
{
    Platform::MemoryClear(dst, size);
}

DEFINE_INTERNAL_CALL(int32) PlatformInternal_MemoryCompare(const void* buf1, const void* buf2, uint64 size)
{
    return Platform::MemoryCompare(buf1, buf2, size);
}

DEFINE_INTERNAL_CALL(void) DebugLogHandlerInternal_LogWrite(LogType level, MString* msgObj)
{
    StringView msg;
    MUtils::ToString(msgObj, msg);
    Log::Logger::Write(level, msg);
}

DEFINE_INTERNAL_CALL(void) DebugLogHandlerInternal_Log(LogType level, MString* msgObj, ScriptingObject* obj, MString* stackTrace)
{
    if (msgObj == nullptr)
        return;

    // Get info
    StringView msg;
    MUtils::ToString(msgObj, msg);
    //const String objName = obj ? obj->ToString() : String::Empty;

    // Send event
    // TODO: maybe option for build to threat warnings and errors as fatal errors?
    //const String logMessage = String::Format(TEXT("Debug:{1} {2}"), objName, *msg);
    Log::Logger::Write(level, msg);
}

DEFINE_INTERNAL_CALL(void) DebugLogHandlerInternal_LogException(MObject* exception, ScriptingObject* obj)
{
#if USE_CSHARP
    if (exception == nullptr)
        return;

    // Get info
    MException ex(exception);
    const String objName = obj ? obj->ToString() : String::Empty;

    // Print exception including inner exceptions
    // TODO: maybe option for build to threat warnings and errors as fatal errors?
    ex.Log(LogType::Warning, objName.GetText());
#endif
}

#endif

#if USE_CSHARP

namespace
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
    ThreadLocal<uint32> ManagedEventsCount;
#endif
#endif
}

DEFINE_INTERNAL_CALL(void) ProfilerInternal_BeginEvent(MString* nameObj)
{
#if COMPILE_WITH_PROFILER
    StringView name;
    MUtils::ToString(nameObj, name);
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
    const bool tracyActive = tracy::ScopedZone::Begin(srcLoc);
    if (tracyActive)
        ManagedEventsCount.Get()++;
#endif
#endif
#endif
}

DEFINE_INTERNAL_CALL(void) ProfilerInternal_EndEvent()
{
#if COMPILE_WITH_PROFILER
#if TRACY_ENABLE
    uint32& tracyActive = ManagedEventsCount.Get();
    if (tracyActive > 0)
    {
        tracyActive--;
        tracy::ScopedZone::End();
    }
#endif
    ProfilerCPU::EndEvent();
#endif
}

DEFINE_INTERNAL_CALL(void) ProfilerInternal_BeginEventGPU(MString* nameObj)
{
#if COMPILE_WITH_PROFILER
    const StringView nameChars = MCore::String::GetChars(nameObj);
    const auto index = ProfilerGPU::BeginEvent(nameChars.Get());
    ManagedEventsGPU.Push(index);
#endif
}

DEFINE_INTERNAL_CALL(void) ProfilerInternal_EndEventGPU()
{
#if COMPILE_WITH_PROFILER
    const auto index = ManagedEventsGPU.Pop();
    ProfilerGPU::EndEvent(index);
#endif
}

DEFINE_INTERNAL_CALL(bool) ScriptingInternal_HasGameModulesLoaded()
{
    return Scripting::HasGameModulesLoaded();
}

DEFINE_INTERNAL_CALL(bool) ScriptingInternal_IsTypeFromGameScripts(MTypeObject* type)
{
    return Scripting::IsTypeFromGameScripts(MUtils::GetClass(INTERNAL_TYPE_OBJECT_GET(type)));
}

DEFINE_INTERNAL_CALL(void) ScriptingInternal_FlushRemovedObjects()
{
    ObjectsRemovalService::Flush();
}

#endif

void registerFlaxEngineInternalCalls()
{
    AnimGraphExecutor::initRuntime();
#if USE_CSHARP
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCopy", &PlatformInternal_MemoryCopy);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryClear", &PlatformInternal_MemoryClear);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCompare", &PlatformInternal_MemoryCompare);
#if USE_MONO
    ADD_INTERNAL_CALL("FlaxEngine.Utils::Internal_ExtractArrayFromList", &UtilsInternal_ExtractArrayFromList);
#endif
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogWrite", &DebugLogHandlerInternal_LogWrite);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_Log", &DebugLogHandlerInternal_Log);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogException", &DebugLogHandlerInternal_LogException);
#endif
}

class ScriptingInternal
{
public:
    static void InitRuntime()
    {
        // Scripting API
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::HasGameModulesLoaded", &ScriptingInternal_HasGameModulesLoaded);
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::IsTypeFromGameScripts", &ScriptingInternal_IsTypeFromGameScripts);
        ADD_INTERNAL_CALL("FlaxEngine.Scripting::FlushRemovedObjects", &ScriptingInternal_FlushRemovedObjects);

        // Profiler API
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEvent", &ProfilerInternal_BeginEvent);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEvent", &ProfilerInternal_EndEvent);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEventGPU", &ProfilerInternal_BeginEventGPU);
        ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEventGPU", &ProfilerInternal_EndEventGPU);
    }
};

IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(Scripting, FlaxEngine, "FlaxEngine.Scripting", nullptr, nullptr);
