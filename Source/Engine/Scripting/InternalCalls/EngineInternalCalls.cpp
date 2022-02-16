// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Engine/Platform/FileSystem.h"
#include "Engine/Animations/Graph/AnimGraph.h"
#include "Engine/Scripting/InternalCalls.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"

#if USE_MONO

namespace UtilsInternal
{
    MonoObject* ExtractArrayFromList(MonoObject* obj)
    {
        auto klass = mono_object_get_class(obj);
        auto field = mono_class_get_field_from_name(klass, "_items");
        MonoObject* o;
        mono_field_get_value(obj, field, &o);
        return o;
    }
}

namespace DebugLogHandlerInternal
{
    void LogWrite(LogType level, MonoString* msgObj)
    {
        StringView msg;
        MUtils::ToString(msgObj, msg);
        Log::Logger::Write(level, msg);
    }

    void Log(LogType level, MonoString* msgObj, ScriptingObject* obj, MonoString* stackTrace)
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

    void LogException(MonoException* exception, ScriptingObject* obj)
    {
        if (exception == nullptr)
            return;

        // Get info
        MException ex(exception);
        const String objName = obj ? obj->ToString() : String::Empty;

        // Print exception including inner exceptions
        // TODO: maybe option for build to threat warnings and errors as fatal errors?
        ex.Log(LogType::Warning, objName.GetText());
    }
}

namespace FlaxLogWriterInternal
{
    void WriteStringToLog(MonoString* msgObj)
    {
        if (msgObj == nullptr)
            return;
        StringView msg;
        MUtils::ToString(msgObj, msg);
        LOG_STR(Info, msg);
    }
}

#endif

void registerFlaxEngineInternalCalls()
{
    AnimGraphExecutor::initRuntime();
#if USE_MONO
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCopy", &Platform::MemoryCopy);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryClear", &Platform::MemoryClear);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCompare", &Platform::MemoryCompare);
    ADD_INTERNAL_CALL("FlaxEngine.Utils::Internal_ExtractArrayFromList", &UtilsInternal::ExtractArrayFromList);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogWrite", &DebugLogHandlerInternal::LogWrite);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_Log", &DebugLogHandlerInternal::Log);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogException", &DebugLogHandlerInternal::LogException);
    ADD_INTERNAL_CALL("FlaxEngine.FlaxLogWriter::Internal_WriteStringToLog", &FlaxLogWriterInternal::WriteStringToLog);
#endif
}
