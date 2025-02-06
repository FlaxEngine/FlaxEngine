// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUFence.h"
#include "GPUDevice.h"
#include "PixelFormatExtensions.h"
#include "RenderTask.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Threading/Threading.h"

GPUFence* GPUFence::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreateFence();
}

GPUFence* GPUFence::New()
{
    return GPUDevice::Instance->CreateFence();
}

GPUFence::GPUFence() : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
{

}
