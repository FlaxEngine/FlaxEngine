// Copyright (c) Wojciech Figat. All rights reserved.

#include "MainThreadManagedInvokeAction.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Scripting/ScriptingCalls.h"
#include "Engine/Scripting/ManagedCLR/MException.h"

MainThreadManagedInvokeAction* MainThreadManagedInvokeAction::Invoke(MMethod* method, MObject* instance, LogType exceptionLevel)
{
    ASSERT_LOW_LAYER(method != nullptr);

    if (IsInMainThread())
    {
        MainThreadManagedInvokeAction task(instance, method, exceptionLevel);
        task.Run();
        return nullptr;
    }
    else
    {
        auto task = New<MainThreadManagedInvokeAction>(instance, method, exceptionLevel);
        task->Start();
        return task;
    }
}

MainThreadManagedInvokeAction* MainThreadManagedInvokeAction::Invoke(MMethod* method, ParamsBuilder& params, MObject* instance, LogType exceptionLevel)
{
    ASSERT_LOW_LAYER(method != nullptr);

    if (IsInMainThread())
    {
        MainThreadManagedInvokeAction task(instance, method, exceptionLevel, params);
        task.Run();
        return nullptr;
    }
    else
    {
        auto task = New<MainThreadManagedInvokeAction>(instance, method, exceptionLevel, params);
        task->Start();
        return task;
    }
}

MainThreadManagedInvokeAction* MainThreadManagedInvokeAction::Invoke(void* methodThunk, ParamsBuilder& params, LogType exceptionLevel)
{
    ASSERT_LOW_LAYER(methodThunk != nullptr);

    if (IsInMainThread())
    {
        MainThreadManagedInvokeAction task(methodThunk, exceptionLevel, params);
        task.Run();
        return nullptr;
    }
    else
    {
        auto task = New<MainThreadManagedInvokeAction>(methodThunk, exceptionLevel, params);
        task->Start();
        return task;
    }
}

void MainThreadManagedInvokeAction::InvokeNow(MMethod* method, ParamsBuilder& params, MObject* instance, LogType exceptionLevel)
{
    ASSERT_LOW_LAYER(method != nullptr);

    void* paramsData[8];
    params.GetParams(paramsData);

    MObject* exception = nullptr;
    method->Invoke(instance, paramsData, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(exceptionLevel, TEXT("Main thread action"));
    }
}

bool MainThreadManagedInvokeAction::Run()
{
    bool failed = false;

    void* paramsData[8];
    _params.GetParams(paramsData);

    MObject* exception = nullptr;
    if (_method)
    {
        _method->Invoke(_instance, paramsData, &exception);
    }
    else if (_methodThunk)
    {
        switch (_params.Count)
        {
        case 0:
            ((Thunk_Void_0)_methodThunk)(&exception);
            break;
        case 1:
            ((Thunk_Void_1)_methodThunk)(paramsData[0], &exception);
            break;
        case 2:
            ((Thunk_Void_2)_methodThunk)(paramsData[0], paramsData[1], &exception);
            break;
        case 3:
            ((Thunk_Void_3)_methodThunk)(paramsData[0], paramsData[1], paramsData[2], &exception);
            break;
        case 4:
            ((Thunk_Void_4)_methodThunk)(paramsData[0], paramsData[1], paramsData[2], paramsData[3], &exception);
            break;
        default:
            exception = nullptr;
            CRASH;
        }
    }
    if (exception)
    {
        failed = true;
        MException ex(exception);
        ex.Log(_exceptionLevel, TEXT("Main thread action"));
    }

    return failed;
}
