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

GPUBuffer* GPUFence::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreateBuffer(String::Empty);
}

GPUBuffer* GPUFence::New()
{
    return GPUDevice::Instance->CreateBuffer(String::Empty);
}

GPUFence::GPUFence() : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
{

}
