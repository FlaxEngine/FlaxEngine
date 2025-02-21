// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GPUContext.h"
#include "GPUDevice.h"
#include "RenderTask.h"
#include "Textures/GPUTexture.h"

GPUContext::GPUContext(GPUDevice* device)
    : ScriptingObject(ScriptingObjectSpawnParams(Guid::New(), TypeInitializer))
    , _device(device)
{
}

#if !BUILD_RELEASE

#include "Engine/Core/Log.h"

void GPUContext::LogInvalidResourceUsage(int32 slot, const GPUResourceView* view, InvalidBindPoint bindPoint)
{
    GPUResource* resource = view ? view->GetParent() : nullptr;
    const Char* resourceType = TEXT("resource");
    const Char* flagType = TEXT("flags");
    if (resource)
    {
        switch (resource->GetResourceType())
        {
        case GPUResourceType::RenderTarget:
        case GPUResourceType::Texture:
        case GPUResourceType::CubeTexture:
        case GPUResourceType::VolumeTexture:
            resourceType = TEXT("texture");
            flagType = TEXT("GPUTextureFlags");
            break;
        case GPUResourceType::Buffer:
            resourceType = TEXT("buffer");
            flagType = TEXT("GPUBufferFlags");
            break;
        }
    }
    const Char* usage = TEXT("-");
    switch (bindPoint)
    {
    case InvalidBindPoint::SRV:
        usage = TEXT("shader resource");
        break;
    case InvalidBindPoint::UAV:
        usage = TEXT("unordered access");
        break;
    case InvalidBindPoint::DSV:
        usage = TEXT("depth stencil");
        break;
    case InvalidBindPoint::RTV:
        usage = TEXT("render target");
        break;
    }
    LOG(Error, "Incorrect {} bind at slot {} as {} (ensure to setup correct {} when creating that resource)", resourceType, slot, usage, flagType);
}

#endif

void GPUContext::FrameBegin()
{
    _lastRenderTime = Platform::GetTimeSeconds();
}

void GPUContext::FrameEnd()
{
    ClearState();
    FlushState();
}

void GPUContext::BindSR(int32 slot, GPUTexture* t)
{
    ASSERT_LOW_LAYER(t == nullptr || t->ResidentMipLevels() == 0 || t->IsShaderResource());
    BindSR(slot, GET_TEXTURE_VIEW_SAFE(t));
}

void GPUContext::DrawFullscreenTriangle(int32 instanceCount)
{
    auto vb = _device->GetFullscreenTriangleVB();
    BindVB(ToSpan(&vb, 1));
    DrawInstanced(3, instanceCount);
}

void GPUContext::Draw(GPUTexture* dst, GPUTexture* src)
{
    ASSERT_LOW_LAYER(dst && src);
    ResetRenderTarget();
    const float width = (float)dst->Width();
    const float height = (float)dst->Height();
    SetViewport(width, height);
    SetRenderTarget(dst->View());
    BindSR(0, src->View());
    SetState(_device->GetCopyLinearPS());
    DrawFullscreenTriangle();
}

void GPUContext::Draw(GPUTexture* rt)
{
    ASSERT_LOW_LAYER(rt);
    BindSR(0, rt);
    SetState(_device->GetCopyLinearPS());
    DrawFullscreenTriangle();
}

void GPUContext::Draw(GPUTextureView* rt)
{
    ASSERT_LOW_LAYER(rt);
    BindSR(0, rt);
    SetState(_device->GetCopyLinearPS());
    DrawFullscreenTriangle();
}

void GPUContext::SetResourceState(GPUResource* resource, uint64 state, int32 subresource)
{
}

void GPUContext::ForceRebindDescriptors()
{
}
