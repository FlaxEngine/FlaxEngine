// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "RenderTargetPool.h"
#include "GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerCPU.h"

struct Entry
{
    GPUTexture* RT;
    uint64 LastFrameReleased;
    uint32 DescriptionHash;
    bool IsOccupied;
};

namespace
{
    Array<Entry> TemporaryRTs;
}

void RenderTargetPool::Flush(bool force, int32 framesOffset)
{
    PROFILE_CPU();
    if (framesOffset < 0)
        framesOffset = 3 * 60; // For how many frames RTs should be cached (by default)
    const uint64 frameCount = Engine::FrameCount;
    const uint64 maxReleaseFrame = frameCount - Math::Min<uint64>(frameCount, framesOffset);
    force |= Engine::ShouldExit();

    for (int32 i = 0; i < TemporaryRTs.Count(); i++)
    {
        const auto& e = TemporaryRTs[i];
        if (!e.IsOccupied && (force || e.LastFrameReleased < maxReleaseFrame))
        {
            e.RT->DeleteObjectNow();
            TemporaryRTs.RemoveAt(i--);
            if (TemporaryRTs.IsEmpty())
                break;
        }
    }
}

GPUTexture* RenderTargetPool::Get(const GPUTextureDescription& desc)
{
    PROFILE_CPU();

    // Find free render target with the same properties
    const uint32 descHash = GetHash(desc);
    for (int32 i = 0; i < TemporaryRTs.Count(); i++)
    {
        auto& e = TemporaryRTs[i];
        if (!e.IsOccupied && e.DescriptionHash == descHash)
        {
            // Mark as used
            e.IsOccupied = true;
            return e.RT;
        }
    }
#if !BUILD_RELEASE
    if (TemporaryRTs.Count() > 2000)
    {
        LOG(Fatal, "Too many textures allocated in RenderTargetPool. Know your limits, sir!");
        return nullptr;
    }
#endif

    // Create new rt
    const String name = TEXT("TemporaryRT_") + StringUtils::ToString(TemporaryRTs.Count());
    GPUTexture* rt = GPUDevice::Instance->CreateTexture(name);
    if (rt->Init(desc))
    {
        Delete(rt);
        LOG(Error, "Cannot create temporary render target. Description: {0}", desc.ToString());
        return nullptr;
    }

    // Create temporary rt entry
    Entry e;
    e.IsOccupied = true;
    e.LastFrameReleased = 0;
    e.RT = rt;
    e.DescriptionHash = descHash;
    TemporaryRTs.Add(e);

    return rt;
}

void RenderTargetPool::Release(GPUTexture* rt)
{
    if (!rt)
        return;
    for (int32 i = 0; i < TemporaryRTs.Count(); i++)
    {
        auto& e = TemporaryRTs[i];
        if (e.RT == rt)
        {
            // Mark as free
            ASSERT(e.IsOccupied);
            e.IsOccupied = false;
            e.LastFrameReleased = Engine::FrameCount;
            return;
        }
    }
    LOG(Error, "Trying to release temporary render target which has not been registered in service!");
}
