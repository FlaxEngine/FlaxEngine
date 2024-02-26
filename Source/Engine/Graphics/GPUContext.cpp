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
