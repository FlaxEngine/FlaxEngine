// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/Animations/Graph/AnimGraph.h"
#include "Engine/Scripting/InternalCalls.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"

#if !COMPILE_WITHOUT_CSHARP

#if USE_MONO && !USE_NETCORE
DEFINE_INTERNAL_CALL(MonoObject*) UtilsInternal_ExtractArrayFromList(MonoObject* obj)
{
    auto klass = mono_object_get_class(obj);
    auto field = mono_class_get_field_from_name(klass, "_items");
    MonoObject* o;
    mono_field_get_value(obj, field, &o);
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

DEFINE_INTERNAL_CALL(void) DebugLogHandlerInternal_LogWrite(LogType level, MonoString* msgObj)
{
    StringView msg;
    MUtils::ToString(msgObj, msg);
    Log::Logger::Write(level, msg);
}

DEFINE_INTERNAL_CALL(void) DebugLogHandlerInternal_Log(LogType level, MonoString* msgObj, ScriptingObject* obj, MonoString* stackTrace)
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

DEFINE_INTERNAL_CALL(void) DebugLogHandlerInternal_LogException(MonoException* exception, ScriptingObject* obj)
{
#if USE_MONO
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

void registerFlaxEngineInternalCalls()
{
    AnimGraphExecutor::initRuntime();
#if USE_MONO
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCopy", &PlatformInternal_MemoryCopy);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryClear", &PlatformInternal_MemoryClear);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCompare", &PlatformInternal_MemoryCompare);
#if USE_MONO && !USE_NETCORE
    ADD_INTERNAL_CALL("FlaxEngine.Utils::Internal_ExtractArrayFromList", &UtilsInternal::ExtractArrayFromList);
#endif
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogWrite", &DebugLogHandlerInternal_LogWrite);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_Log", &DebugLogHandlerInternal_Log);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogException", &DebugLogHandlerInternal_LogException);
#endif
}
