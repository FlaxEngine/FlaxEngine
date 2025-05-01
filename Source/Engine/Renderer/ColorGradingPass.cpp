// Copyright (c) Wojciech Figat. All rights reserved.

#include "ColorGradingPass.h"
#include "RenderList.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"
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

ColorGradingPass::ColorGradingPass()
    : _useVolumeTexture(false)
    , _lutFormat()
    , _shader(nullptr)
{
}

String ColorGradingPass::ToString() const
{
    return TEXT("ColorGradingPass");
}

bool ColorGradingPass::Init()
{
    // Detect if can use volume texture (3d) for a LUT (faster, requires geometry shader)
    const auto device = GPUDevice::Instance;
#if GPU_ALLOW_GEOMETRY_SHADERS
    _useVolumeTexture = device->Limits.HasGeometryShaders && device->Limits.HasVolumeTextureRendering;
#endif

    // Pick a proper LUT pixels format
    _lutFormat = PixelFormat::R10G10B10A2_UNorm;
    const auto formatSupport = device->GetFormatFeatures(_lutFormat).Support;
    FormatSupport formatSupportFlags = FormatSupport::ShaderSample | FormatSupport::RenderTarget;
    if (_useVolumeTexture)
        formatSupportFlags |= FormatSupport::Texture3D;
    else
        formatSupportFlags |= FormatSupport::Texture2D;
    if (EnumHasNoneFlags(formatSupport, formatSupportFlags))
    {
        // Fallback to format that is supported on every washing machine
        _lutFormat = PixelFormat::R8G8B8A8_UNorm;
    }

    // Create pipeline state
    _psLut.CreatePipelineStates();

    // Load shader
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
    // Wait for shader
    if (!_shader || !_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psLut.IsValid())
    {
        StringAnsiView psName;
#if GPU_ALLOW_GEOMETRY_SHADERS
        if (_useVolumeTexture)
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
    }

    return false;
}

void ColorGradingPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psLut.Delete();
    _shader = nullptr;
}

GPUTexture* ColorGradingPass::RenderLUT(RenderContext& renderContext)
{
    // Ensure to have valid data
    if (checkIfSkipPass())
        return nullptr;

    PROFILE_GPU_CPU("Color Grading LUT");

    // For a 3D texture, the viewport is 16x16 (per slice), for a 2D texture, it's unwrapped to 256x16
    const int32 LutSize = 32; // this must match value in shader (see ColorGrading.shader and PostProcessing.shader)
    GPUTextureDescription lutDesc;
#if GPU_ALLOW_GEOMETRY_SHADERS
    if (_useVolumeTexture)
    {
        lutDesc = GPUTextureDescription::New3D(LutSize, LutSize, LutSize, 1, _lutFormat);
    }
    else
#endif
    {
        lutDesc = GPUTextureDescription::New2D(LutSize * LutSize, LutSize, 1, _lutFormat);
    }
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
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->SetViewportAndScissors((float)lutDesc.Width, (float)lutDesc.Height);
    context->SetState(_psLut.Get((int32)toneMapping.Mode));
    context->BindSR(0, useLut ? colorGrading.LutTexture->GetTexture() : nullptr);

    // Draw
#if GPU_ALLOW_GEOMETRY_SHADERS
    if (_useVolumeTexture)
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
