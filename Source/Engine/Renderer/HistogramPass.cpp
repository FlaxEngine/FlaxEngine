// Copyright (c) Wojciech Figat. All rights reserved.

#include "HistogramPass.h"
#include "RenderList.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"

// Those defines must match the HLSL
#define THREADGROUP_SIZE_X 16
#define THREADGROUP_SIZE_Y 16
#define HISTOGRAM_SIZE 64

GPU_CB_STRUCT(HistogramData {
    uint32 InputSizeX;
    uint32 InputSizeY;
    float HistogramMul;
    float HistogramAdd;
    });

GPUBuffer* HistogramPass::Render(RenderContext& renderContext, GPUTexture* colorBuffer)
{
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    if (checkIfSkipPass() || !_isSupported)
        return nullptr;
    PROFILE_GPU_CPU("Histogram");

    // Setup constants
    HistogramData data;
    const uint32 colorBufferX = colorBuffer->Width();
    const uint32 colorBufferY = colorBuffer->Height();
    data.InputSizeX = colorBufferX;
    data.InputSizeY = colorBufferY;
    GetHistogramMad(data.HistogramMul, data.HistogramAdd);

    // Update constants
    const auto shader = _shader->GetShader();
    const auto cb0 = shader->GetCB(0);
    context->UpdateCB(cb0, &data);
    context->BindCB(0, cb0);

    // Clear the histogram buffer
    context->BindUA(0, _histogramBuffer->View());
    context->Dispatch(_csClearHistogram, (HISTOGRAM_SIZE + THREADGROUP_SIZE_X - 1) / THREADGROUP_SIZE_X, 1, 1);

    // Generate the histogram
    context->BindSR(0, colorBuffer);
    context->BindUA(0, _histogramBuffer->View());
    context->Dispatch(_csGenerateHistogram, (colorBufferX + THREADGROUP_SIZE_X - 1) / THREADGROUP_SIZE_X, (colorBufferY + THREADGROUP_SIZE_Y - 1) / THREADGROUP_SIZE_Y, 1);

    // Cleanup
    context->ResetUA();
    context->ResetSR();

    return _histogramBuffer;
}

void HistogramPass::GetHistogramMad(float& multiply, float& add)
{
    const float histogramLogMin = -8.0f;
    const float histogramLogMax = 6.0f;
    const float histogramLogRange = histogramLogMax - histogramLogMin;
    multiply = 1.0f / histogramLogRange;
    add = -histogramLogMin * multiply;
}

String HistogramPass::ToString() const
{
    return TEXT("HistogramPass");
}

bool HistogramPass::Init()
{
    _isSupported = GPUDevice::Instance->Limits.HasCompute;
    if (!_isSupported)
        return false;

    // Create buffer for histogram
    _histogramBuffer = GPUDevice::Instance->CreateBuffer(TEXT("Histogram"));
    if (_histogramBuffer->Init(GPUBufferDescription::Buffer(HISTOGRAM_SIZE * sizeof(uint32), GPUBufferFlags::Structured | GPUBufferFlags::ShaderResource | GPUBufferFlags::UnorderedAccess, PixelFormat::R32_UInt, nullptr, sizeof(uint32))))
        return true;

    // Load shaders
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Histogram"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<HistogramPass, &HistogramPass::OnShaderReloading>(this);
#endif

    return false;
}

void HistogramPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_histogramBuffer);
    _shader = nullptr;
}

bool HistogramPass::setupResources()
{
    // Wait for shader
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(HistogramData))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, HistogramData);
        return true;
    }

    _csClearHistogram = shader->GetCS("CS_ClearHistogram");
    _csGenerateHistogram = shader->GetCS("CS_GenerateHistogram");

    return false;
}
