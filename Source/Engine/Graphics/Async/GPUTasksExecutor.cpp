// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUTasksExecutor.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"

GPUTasksExecutor::~GPUTasksExecutor()
{
    // Stats
    int32 totalJobsDone = 0;
    for (int32 i = 0; i < _contextList.Count(); i++)
        totalJobsDone += _contextList[i]->GetTotalTasksDoneCount();
    LOG(Info, "Total GPU tasks done: {0}", totalJobsDone);

    _contextList.ClearDelete();
}

GPUTasksContext* GPUTasksExecutor::createContext()
{
    auto context = GPUDevice::Instance->CreateTasksContext();
    if (context == nullptr)
    {
        LOG(Error, "Cannot create new GPU Tasks Context");
    }
    _contextList.Add(context);
    return context;
}
