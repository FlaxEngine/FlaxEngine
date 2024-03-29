#include "CustomDepthPass.h"
#include "RenderList.h"

#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"

String CustomDepthPass::ToString() const
{
    return TEXT("CustomDepthPass");
}

void CustomDepthPass::Render(RenderContext& renderContext)
{
    PROFILE_GPU_CPU_NAMED("CustomDepth");

    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();

    renderContext.View.Pass = DrawPass::CustomDepth;

    context->ClearDepth(*renderContext.Buffers->CustomDepthBuffer);

    context->SetRenderTarget(nullptr, *renderContext.Buffers->CustomDepthBuffer);
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::CustomDepth);

    context->ResetRenderTarget();
}
