// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Engine/Platform/FileSystem.h"
#include "Engine/Animations/Graph/AnimGraph.h"
#include "Engine/Scripting/InternalCalls.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"

namespace UtilsInternal
{
    void MemoryCopy(void* source, void* destination, int32 length)
    {
        Platform::MemoryCopy(destination, source, length);
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

void registerFlaxEngineInternalCalls()
{
    AnimGraphExecutor::initRuntime();
    ADD_INTERNAL_CALL("FlaxEngine.Utils::MemoryCopy", &UtilsInternal::MemoryCopy);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogWrite", &DebugLogHandlerInternal::LogWrite);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_Log", &DebugLogHandlerInternal::Log);
    ADD_INTERNAL_CALL("FlaxEngine.DebugLogHandler::Internal_LogException", &DebugLogHandlerInternal::LogException);
    ADD_INTERNAL_CALL("FlaxEngine.FlaxLogWriter::Internal_WriteStringToLog", &FlaxLogWriterInternal::WriteStringToLog);
}
