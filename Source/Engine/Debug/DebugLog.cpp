// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "DebugLog.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/MainThreadManagedInvokeAction.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "FlaxEngine.Gen.h"
#include <ThirdParty/mono-2.0/mono/metadata/exception.h>
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

namespace Impl
{
    MMethod* Internal_SendLog = nullptr;
    MMethod* Internal_SendLogException = nullptr;
    MMethod* Internal_GetStackTrace = nullptr;
    CriticalSection Locker;
}

using namespace Impl;

void ClearMethods(MAssembly*)
{
    Internal_SendLog = nullptr;
    Internal_SendLogException = nullptr;
    Internal_GetStackTrace = nullptr;
}

bool CacheMethods()
{
    if (Internal_SendLog && Internal_SendLogException && Internal_GetStackTrace)
        return false;
    ScopeLock lock(Locker);
    if (Internal_SendLog && Internal_SendLogException && Internal_GetStackTrace)
        return false;

    auto engine = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    if (engine == nullptr || !engine->IsLoaded())
        return true;
    auto debugLogHandlerClass = engine->GetClass("FlaxEngine.DebugLogHandler");
    if (!debugLogHandlerClass)
        return false;

    Internal_SendLog = debugLogHandlerClass->GetMethod("Internal_SendLog", 3);
    if (!Internal_SendLog)
        return false;

    Internal_SendLogException = debugLogHandlerClass->GetMethod("Internal_SendLogException", 1);
    if (!Internal_SendLogException)
        return false;

    Internal_GetStackTrace = debugLogHandlerClass->GetMethod("Internal_GetStackTrace");
    if (!Internal_GetStackTrace)
        return false;

    engine->Unloading.Bind<ClearMethods>();

    return false;
}

void DebugLog::Log(LogType type, const StringView& message)
{
    if (CacheMethods())
        return;

    auto scriptsDomain = Scripting::GetScriptsDomain();
    MainThreadManagedInvokeAction::ParamsBuilder params;
    params.AddParam(type);
    params.AddParam(message, scriptsDomain->GetNative());
#if BUILD_RELEASE
    params.AddParam(StringView::Empty, scriptsDomain->GetNative());
#else
    const String stackTrace = Platform::GetStackTrace(1);
    params.AddParam(stackTrace, scriptsDomain->GetNative());
#endif
    MainThreadManagedInvokeAction::Invoke(Internal_SendLog, params);
}

void DebugLog::LogException(MonoObject* exceptionObject)
{
    if (exceptionObject == nullptr || CacheMethods())
        return;

    MainThreadManagedInvokeAction::ParamsBuilder params;
    params.AddParam(exceptionObject);
    MainThreadManagedInvokeAction::Invoke(Internal_SendLogException, params);
}

String DebugLog::GetStackTrace()
{
    String result;
    if (!CacheMethods())
    {
        auto stackTraceObj = Internal_GetStackTrace->Invoke(nullptr, nullptr, nullptr);
        MUtils::ToString((MonoString*)stackTraceObj, result);
    }
    return result;
}

void DebugLog::ThrowException(const char* msg)
{
    // Throw exception to the C# world
    auto ex = mono_exception_from_name_msg(mono_get_corlib(), "System", "Exception", msg);
    mono_raise_exception(ex);
}

void DebugLog::ThrowNullReference()
{
    //LOG(Warning, "Invalid null reference.");
    //LOG_STR(Warning, DebugLog::GetStackTrace());

    // Throw exception to the C# world
    auto ex = mono_get_exception_null_reference();
    mono_raise_exception(ex);
}

void DebugLog::ThrowArgument(const char* arg, const char* msg)
{
    // Throw exception to the C# world
    auto ex = mono_get_exception_argument(arg, msg);
    mono_raise_exception(ex);
}

void DebugLog::ThrowArgumentNull(const char* arg)
{
    // Throw exception to the C# world
    auto ex = mono_get_exception_argument_null(arg);
    mono_raise_exception(ex);
}

void DebugLog::ThrowArgumentOutOfRange(const char* arg)
{
    // Throw exception to the C# world
    auto ex = mono_get_exception_argument_out_of_range(arg);
    mono_raise_exception(ex);
}

void DebugLog::ThrowNotSupported(const char* msg)
{
    // Throw exception to the C# world
    auto ex = mono_get_exception_not_supported(msg);
    mono_raise_exception(ex);
}
