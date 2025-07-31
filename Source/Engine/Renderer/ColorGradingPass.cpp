// Copyright (c) Wojciech Figat. All rights reserved.

#include "ColorGradingPass.h"
#include "RenderList.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTask.h"

GPU_CB_STRUCT(Data {
    Float4 ColorSaturationShadows;
    Float4 ColorContrastShadows;
    Float4 ColorGammaShadows;
    Float4 ColorGainShadows;
    Float4 ColorOffsetShadows;

    Float4 ColorSaturationMidtones;
    Float4 ColorContrastMidtones;
    Float4 ColorGammaMidtones;
    Float4 ColorGainMidtones;
    Float4 ColorOffsetMidtones;

    Float4 ColorSaturationHighlights;
    Float4 ColorContrastHighlights;
    Float4 ColorGammaHighlights;
    Float4 ColorGainHighlights;
    Float4 ColorOffsetHighlights;

    float ColorCorrectionShadowsMax;
    float ColorCorrectionHighlightsMin;
    float WhiteTemp;
    float WhiteTint;

    Float3 Dummy;
    float LutWeight;
    });

String ColorGradingPass::ToString() const
{
    return TEXT("ColorGradingPass");
}

bool ColorGradingPass::Init()
{
    _psLut.CreatePipelineStates();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/ColorGrading"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ColorGradingPass, &ColorGradingPass::OnShaderReloading>(this);
#endif
    return false;
}

bool ColorGradingPass::setupResources()
{
    if (!_shader || !_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);

    // Create pipeline stage
    auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    StringAnsiView psName;
#if GPU_ALLOW_GEOMETRY_SHADERS
    if (_use3D == 1)
    {
        psDesc.VS = shader->GetVS("VS_WriteToSlice");
        psDesc.GS = shader->GetGS("GS_WriteToSlice");
        psName = "PS_Lut3D";
    }
    else
#endif
    {
        psName = "PS_Lut2D";
    }
    if (_psLut.Create(psDesc, shader, psName))
        return true;

    return false;
}

void ColorGradingPass::Dispose()
{
    RendererPass::Dispose();

    _psLut.Delete();
    _shader = nullptr;
}

GPUTexture* ColorGradingPass::RenderLUT(RenderContext& renderContext)
{
    // Check if can use volume texture (3D) for a LUT (faster on modern platforms, requires geometry shader)
    const auto device = GPUDevice::Instance;
    bool use3D = GPU_ALLOW_GEOMETRY_SHADERS && Graphics::PostProcessing::ColorGradingVolumeLUT;
    use3D &= device->Limits.HasGeometryShaders && device->Limits.HasVolumeTextureRendering;
    use3D &= !PLATFORM_SWITCH; // TODO: move this in future to platform-specific configs

    // Rebuild PSO on change
    if (_use3D != (int32)use3D)
    {
        invalidateResources();
        _use3D = use3D;
    }

    // Ensure to have valid data
    if (checkIfSkipPass())
        return nullptr;
    PROFILE_GPU_CPU("Color Grading LUT");

    // Pick a proper LUT pixels format
    auto lutFormat = PixelFormat::R10G10B10A2_UNorm;
    const auto formatSupport = device->GetFormatFeatures(lutFormat).Support;
    FormatSupport formatSupportFlags = FormatSupport::ShaderSample | FormatSupport::RenderTarget;
    formatSupportFlags |= use3D ? FormatSupport::Texture3D : FormatSupport::Texture2D;
    if (EnumHasNoneFlags(formatSupport, formatSupportFlags))
        lutFormat = PixelFormat::R8G8B8A8_UNorm;

    // For a 3D texture, the viewport is 32x32 (per slice), for a 2D texture, it's unwrapped to 1024x32
    constexpr int32 lutSize = 32; // this must match value in shader (see ColorGrading.shader and PostProcessing.shader)
    GPUTextureDescription lutDesc;
    if (use3D)
        lutDesc = GPUTextureDescription::New3D(lutSize, lutSize, lutSize, 1, lutFormat);
    else
        lutDesc = GPUTextureDescription::New2D(lutSize * lutSize, lutSize, 1, lutFormat);
    const auto lut = RenderTargetPool::Get(lutDesc);
    RENDER_TARGET_POOL_SET_NAME(lut, "ColorGrading.LUT");

    // Prepare the parameters
    Data data;
    auto& toneMapping = renderContext.List->Settings.ToneMapping;
    auto& colorGrading = renderContext.List->Settings.ColorGrading;
    // White Balance
    data.WhiteTemp = toneMapping.WhiteTemperature;
    data.WhiteTint = toneMapping.WhiteTint;
    // Shadows
    data.ColorSaturationShadows = colorGrading.ColorSaturationShadows * colorGrading.ColorSaturation;
    data.ColorContrastShadows = colorGrading.ColorContrastShadows * colorGrading.ColorContrast;
    data.ColorGammaShadows = colorGrading.ColorGammaShadows * colorGrading.ColorGamma;
    data.ColorGainShadows = colorGrading.ColorGainShadows * colorGrading.ColorGain;
    data.ColorOffsetShadows = colorGrading.ColorOffsetShadows + colorGrading.ColorOffset;
    data.ColorCorrectionShadowsMax = colorGrading.ShadowsMax;
    // Midtones
    data.ColorSaturationMidtones = colorGrading.ColorSaturationMidtones * colorGrading.ColorSaturation;
    data.ColorContrastMidtones = colorGrading.ColorContrastMidtones * colorGrading.ColorContrast;
    data.ColorGammaMidtones = colorGrading.ColorGammaMidtones * colorGrading.ColorGamma;
    data.ColorGainMidtones = colorGrading.ColorGainMidtones * colorGrading.ColorGain;
    data.ColorOffsetMidtones = colorGrading.ColorOffsetMidtones + colorGrading.ColorOffset;
    // Highlights
    data.ColorSaturationHighlights = colorGrading.ColorSaturationHighlights * colorGrading.ColorSaturation;
    data.ColorContrastHighlights = colorGrading.ColorContrastHighlights * colorGrading.ColorContrast;
    data.ColorGammaHighlights = colorGrading.ColorGammaHighlights * colorGrading.ColorGamma;
    data.ColorGainHighlights = colorGrading.ColorGainHighlights * colorGrading.ColorGain;
    data.ColorOffsetHighlights = colorGrading.ColorOffsetHighlights + colorGrading.ColorOffset;
    data.ColorCorrectionHighlightsMin = colorGrading.HighlightsMin;
    //
    const bool useLut = colorGrading.LutTexture && colorGrading.LutTexture->IsLoaded() && colorGrading.LutTexture->GetResidentMipLevels() > 0 && colorGrading.LutWeight > ZeroTolerance;
    data.LutWeight = useLut ? colorGrading.LutWeight : 0.0f;

    // Prepare
    auto context = device->GetMainContext();
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->SetViewportAndScissors((float)lutDesc.Width, (float)lutDesc.Height);
    context->SetState(_psLut.Get((int32)toneMapping.Mode));
    context->BindSR(0, useLut ? colorGrading.LutTexture->GetTexture() : nullptr);

    // Draw
#if GPU_ALLOW_GEOMETRY_SHADERS
    if (use3D)
    {
        context->SetRenderTarget(lut->ViewVolume());

        // Render one fullscreen-triangle per slice intersecting the bounds
        const int32 numInstances = lutDesc.Depth;
        context->DrawFullscreenTriangle(numInstances);
    }
    else
#endif
    {
        context->SetRenderTarget(lut->View());
        context->DrawFullscreenTriangle();
    }
    context->UnBindSR(0);

    return lut;
}
