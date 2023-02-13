// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "RenderTargetPool.h"
#include "GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"

struct Entry
{
    bool IsOccupied;
    GPUTexture* RT;
    uint64 LastFrameTaken;
    uint64 LastFrameReleased;
    uint32 DescriptionHash;
};

namespace
{
    Array<Entry> TemporaryRTs(64);
}

void RenderTargetPool::Flush(bool force)
{
    const uint64 framesOffset = 3 * 60;
    const uint64 maxReleaseFrame = Engine::FrameCount - framesOffset;
    force |= Engine::ShouldExit();

    for (int32 i = 0; i < TemporaryRTs.Count(); i++)
    {
        auto& tmp = TemporaryRTs[i];

        if (!tmp.IsOccupied && (force || (tmp.LastFrameReleased < maxReleaseFrame)))
        {
            // Release
            tmp.RT->DeleteObjectNow();
            TemporaryRTs.RemoveAt(i);
            i--;

            if (TemporaryRTs.IsEmpty())
                break;
        }
    }
}

GPUTexture* RenderTargetPool::Get(const GPUTextureDescription& desc)
{
    // Find free render target with the same properties
    const uint32 descHash = GetHash(desc);
    for (int32 i = 0; i < TemporaryRTs.Count(); i++)
    {
        auto& tmp = TemporaryRTs[i];

        if (!tmp.IsOccupied && tmp.DescriptionHash == descHash)
        {
            ASSERT(tmp.RT);

            // Mark as used
            tmp.IsOccupied = true;
            tmp.LastFrameTaken = Engine::FrameCount;
            return tmp.RT;
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
    auto newRenderTarget = GPUDevice::Instance->CreateTexture(name);
    if (newRenderTarget->Init(desc))
    {
        Delete(newRenderTarget);
        LOG(Error, "Cannot create temporary render target. Description: {0}", desc.ToString());
        return nullptr;
    }

    // Create temporary rt entry
    Entry entry;
    entry.IsOccupied = true;
    entry.LastFrameReleased = 0;
    entry.LastFrameTaken = Engine::FrameCount;
    entry.RT = newRenderTarget;
    entry.DescriptionHash = descHash;
    TemporaryRTs.Add(entry);

    return newRenderTarget;
}

void RenderTargetPool::Release(GPUTexture* rt)
{
    if (!rt)
        return;

    for (int32 i = 0; i < TemporaryRTs.Count(); i++)
    {
        auto& tmp = TemporaryRTs[i];

        if (tmp.RT == rt)
        {
            // Mark as free
            ASSERT(tmp.IsOccupied);
            tmp.IsOccupied = false;
            tmp.LastFrameReleased = Engine::FrameCount;
            return;
        }
    }

    LOG(Error, "Trying to release temporary render target which has not been registered in service!");
}
