#include "CustomDepthPass.h"
#include "RenderList.h"

#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Engine/Engine.h"

String CustomDepthPass::ToString() const
{
    return TEXT("CustomDepthPass");
}

void CustomDepthPass::Render(RenderContext& renderContext)
{
    PROFILE_GPU_CPU_NAMED("CustomDepth");

    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();

    if (!renderContext.Buffers->CustomDepthBuffer->IsAllocated())
    {
        int width = renderContext.Buffers->GetWidth();
        int height = renderContext.Buffers->GetHeight();
        GPUTextureDescription desc = GPUTextureDescription::New2D(width, height, GPU_DEPTH_BUFFER_PIXEL_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil);
        if (GPUDevice::Instance->Limits.HasReadOnlyDepth)
            desc.Flags |= GPUTextureFlags::ReadOnlyDepthView;

        renderContext.Buffers->CustomDepthBuffer->Init(desc);
    }

    renderContext.View.Pass = DrawPass::CustomDepth;

    context->ClearDepth(*renderContext.Buffers->CustomDepthBuffer);

    context->SetRenderTarget(*renderContext.Buffers->CustomDepthBuffer, static_cast<GPUTextureView*>(nullptr));
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::CustomDepth);

    context->ResetRenderTarget();
    renderContext.Buffers->CustomDepthClear = false;
    renderContext.Buffers->LastFrameCustomDepth = Engine::FrameCount;
}

void CustomDepthPass::Clear(RenderContext& renderContext)
{
    // Release the buffer after a while
    if (Engine::FrameCount - renderContext.Buffers->LastFrameCustomDepth > 240 && renderContext.Buffers->CustomDepthBuffer->IsAllocated())
        renderContext.Buffers->CustomDepthBuffer->ReleaseGPU();

    // Only clear depth once and only if it's allocated
    if (renderContext.Buffers->CustomDepthClear || !renderContext.Buffers->CustomDepthBuffer->IsAllocated())
        return;

    PROFILE_GPU_CPU_NAMED("CustomDepthClear");

    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    context->ClearDepth(*renderContext.Buffers->CustomDepthBuffer);
    renderContext.Buffers->CustomDepthClear = true;
}
