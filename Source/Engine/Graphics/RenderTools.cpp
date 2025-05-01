// Copyright (c) Wojciech Figat. All rights reserved.

#include "RenderTools.h"
#include "PixelFormatExtensions.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "PixelFormat.h"
#include "RenderView.h"
#include "GPUDevice.h"
#include "RenderTask.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Packed.h"
#include "Engine/Engine/Time.h"

const Char* ToString(RendererType value)
{
    const Char* result;
    switch (value)
    {
    case RendererType::Unknown:
        result = TEXT("Unknown");
        break;
    case RendererType::DirectX10:
        result = TEXT("DirectX 10");
        break;
    case RendererType::DirectX10_1:
        result = TEXT("DirectX 10.1");
        break;
    case RendererType::DirectX11:
        result = TEXT("DirectX 11");
        break;
    case RendererType::DirectX12:
        result = TEXT("DirectX 12");
        break;
    case RendererType::OpenGL4_1:
        result = TEXT("OpenGL 4.1");
        break;
    case RendererType::OpenGL4_4:
        result = TEXT("OpenGL 4.4");
        break;
    case RendererType::OpenGLES3:
        result = TEXT("OpenGL ES 3");
        break;
    case RendererType::OpenGLES3_1:
        result = TEXT("OpenGL ES 3.1");
        break;
    case RendererType::Null:
        result = TEXT("Null");
        break;
    case RendererType::Vulkan:
        result = TEXT("Vulkan");
        break;
    case RendererType::PS4:
        result = TEXT("PS4");
        break;
    case RendererType::PS5:
        result = TEXT("PS5");
        break;
    default:
        result = TEXT("?");
    }
    return result;
}

const Char* ToString(ShaderProfile value)
{
    const Char* result;
    switch (value)
    {
    case ShaderProfile::Unknown:
        result = TEXT("Unknown");
        break;
    case ShaderProfile::DirectX_SM4:
        result = TEXT("DirectX SM4");
        break;
    case ShaderProfile::DirectX_SM5:
        result = TEXT("DirectX SM5");
        break;
    case ShaderProfile::DirectX_SM6:
        result = TEXT("DirectX SM6");
        break;
    case ShaderProfile::GLSL_410:
        result = TEXT("GLSL 410");
        break;
    case ShaderProfile::GLSL_440:
        result = TEXT("GLSL 440");
        break;
    case ShaderProfile::Vulkan_SM5:
        result = TEXT("Vulkan SM5");
        break;
    case ShaderProfile::PS4:
        result = TEXT("PS4");
        break;
    case ShaderProfile::PS5:
        result = TEXT("PS5");
        break;
    default:
        result = TEXT("?");
    }
    return result;
}

const Char* ToString(FeatureLevel value)
{
    const Char* result;
    switch (value)
    {
    case FeatureLevel::ES2:
        result = TEXT("ES2");
        break;
    case FeatureLevel::ES3:
        result = TEXT("ES3");
        break;
    case FeatureLevel::ES3_1:
        result = TEXT("ES3_1");
        break;
    case FeatureLevel::SM4:
        result = TEXT("SM4");
        break;
    case FeatureLevel::SM5:
        result = TEXT("SM5");
        break;
    case FeatureLevel::SM6:
        result = TEXT("SM6");
        break;
    default:
        result = TEXT("?");
    }
    return result;
}

const Char* ToString(MSAALevel value)
{
    const Char* result;
    switch (value)
    {
    case MSAALevel::None:
        result = TEXT("None");
        break;
    case MSAALevel::X2:
        result = TEXT("X2");
        break;
    case MSAALevel::X4:
        result = TEXT("X4");
        break;
    case MSAALevel::X8:
        result = TEXT("X8");
        break;
    default:
        result = TEXT("?");
    }
    return result;
}

bool BlendingMode::operator==(const BlendingMode& other) const
{
#define EQUAL(x) x == other.x &&
    return EQUAL(BlendEnable)
            EQUAL(SrcBlend)
            EQUAL(DestBlend)
            EQUAL(BlendOp)
            EQUAL(SrcBlendAlpha)
            EQUAL(DestBlendAlpha)
            EQUAL(BlendOpAlpha)
            EQUAL(RenderTargetWriteMask)
            AlphaToCoverageEnable == other.AlphaToCoverageEnable;
#undef EQUAL
}

uint32 GetHash(const BlendingMode& key)
{
    uint32 hash = key.AlphaToCoverageEnable ? 1 : 0;
    CombineHash(hash, key.BlendEnable ? 1 : 0);
    CombineHash(hash, (uint32)key.SrcBlend);
    CombineHash(hash, (uint32)key.DestBlend);
    CombineHash(hash, (uint32)key.BlendOp);
    CombineHash(hash, (uint32)key.SrcBlendAlpha);
    CombineHash(hash, (uint32)key.DestBlendAlpha);
    CombineHash(hash, (uint32)key.BlendOpAlpha);
    CombineHash(hash, (uint32)key.RenderTargetWriteMask);
    return hash;
}

// @formatter:off

BlendingMode BlendingMode::Opaque =
{
    false, // AlphaToCoverageEnable
    false, // BlendEnable
    Blend::One, // SrcBlend
    Blend::Zero, // DestBlend
    Operation::Add, // BlendOp
    Blend::One, // SrcBlendAlpha
    Blend::Zero, // DestBlendAlpha
    Operation::Add, // BlendOpAlpha
    ColorWrite::All, // RenderTargetWriteMask
};

BlendingMode BlendingMode::Additive =
{
    false, // AlphaToCoverageEnable
    true, // BlendEnable
    Blend::SrcAlpha, // SrcBlend
    Blend::One, // DestBlend
    Operation::Add, // BlendOp
    Blend::SrcAlpha, // SrcBlendAlpha
    Blend::One, // DestBlendAlpha
    Operation::Add, // BlendOpAlpha
    ColorWrite::All, // RenderTargetWriteMask
};

BlendingMode BlendingMode::AlphaBlend =
{
    false, // AlphaToCoverageEnable
    true, // BlendEnable
    Blend::SrcAlpha, // SrcBlend
    Blend::InvSrcAlpha, // DestBlend
    Operation::Add, // BlendOp
    Blend::One, // SrcBlendAlpha
    Blend::InvSrcAlpha, // DestBlendAlpha
    Operation::Add, // BlendOpAlpha
    ColorWrite::All, // RenderTargetWriteMask
};

BlendingMode BlendingMode::Add =
{
    false, // AlphaToCoverageEnable
    true, // BlendEnable
    Blend::One, // SrcBlend
    Blend::One, // DestBlend
    Operation::Add, // BlendOp
    Blend::One, // SrcBlendAlpha
    Blend::One, // DestBlendAlpha
    Operation::Add, // BlendOpAlpha
    ColorWrite::All, // RenderTargetWriteMask
};

BlendingMode BlendingMode::Multiply =
{
    false, // AlphaToCoverageEnable
    true, // BlendEnable
    Blend::Zero, // SrcBlend
    Blend::SrcColor, // DestBlend
    Operation::Add, // BlendOp
    Blend::Zero, // SrcBlendAlpha
    Blend::SrcAlpha, // DestBlendAlpha
    Operation::Add, // BlendOpAlpha
    ColorWrite::All, // RenderTargetWriteMask
};

// @formatter:on

FeatureLevel RenderTools::GetFeatureLevel(ShaderProfile profile)
{
    switch (profile)
    {
    case ShaderProfile::DirectX_SM6:
    case ShaderProfile::PS5:
        return FeatureLevel::SM6;
    case ShaderProfile::DirectX_SM5:
    case ShaderProfile::Vulkan_SM5:
    case ShaderProfile::PS4:
        return FeatureLevel::SM5;
    case ShaderProfile::DirectX_SM4:
        return FeatureLevel::SM4;
    case ShaderProfile::GLSL_440:
    case ShaderProfile::GLSL_410:
    case ShaderProfile::Unknown:
        return FeatureLevel::ES2;
    default:
        return FeatureLevel::ES2;
    }
}

bool RenderTools::CanSupportTessellation(ShaderProfile profile)
{
    switch (profile)
    {
    case ShaderProfile::Vulkan_SM5:
    case ShaderProfile::DirectX_SM6:
    case ShaderProfile::DirectX_SM5:
    case ShaderProfile::PS4:
    case ShaderProfile::PS5:
        return true;
    default:
        return false;
    }
}

void RenderTools::ComputePitch(PixelFormat format, int32 width, int32 height, uint32& rowPitch, uint32& slicePitch)
{
    switch (format)
    {
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC4_SNorm:
    {
        const uint32 nbw = Math::Max<uint32>(1, (width + 3) / 4);
        const uint32 nbh = Math::Max<uint32>(1, (height + 3) / 4);
        rowPitch = nbw * 8;
        slicePitch = rowPitch * nbh;
    }
    break;
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC5_UNorm:
    case PixelFormat::BC5_SNorm:
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
    case PixelFormat::BC7_UNorm_sRGB:
    {
        const uint32 nbw = Math::Max<uint32>(1, (width + 3) / 4);
        const uint32 nbh = Math::Max<uint32>(1, (height + 3) / 4);
        rowPitch = nbw * 16;
        slicePitch = rowPitch * nbh;
    }
    break;
    case PixelFormat::ASTC_4x4_UNorm:
    case PixelFormat::ASTC_4x4_UNorm_sRGB:
    case PixelFormat::ASTC_6x6_UNorm:
    case PixelFormat::ASTC_6x6_UNorm_sRGB:
    case PixelFormat::ASTC_8x8_UNorm:
    case PixelFormat::ASTC_8x8_UNorm_sRGB:
    case PixelFormat::ASTC_10x10_UNorm:
    case PixelFormat::ASTC_10x10_UNorm_sRGB:
    {
        const int32 blockSize = PixelFormatExtensions::ComputeBlockSize(format);
        const uint32 nbw = Math::Max<uint32>(1, Math::DivideAndRoundUp(width, blockSize));
        const uint32 nbh = Math::Max<uint32>(1, Math::DivideAndRoundUp(height, blockSize));
        rowPitch = nbw * 16; // All ASTC blocks use 128 bits
        slicePitch = rowPitch * nbh;
    }
    break;
    case PixelFormat::R8G8_B8G8_UNorm:
    case PixelFormat::G8R8_G8B8_UNorm:
    case PixelFormat::YUY2:
        rowPitch = ((width + 1) >> 1) * 4;
        slicePitch = rowPitch * height;
        break;
    case PixelFormat::NV12:
        rowPitch = width;
        slicePitch = width * height * 3 / 2;
        break;
    default:
        // Default byte alignment
        rowPitch = (width * PixelFormatExtensions::SizeInBits(format) + 7) / 8;
        slicePitch = rowPitch * height;
        break;
    }
}

void RenderTools::UpdateModelLODTransition(byte& lodTransition)
{
    // TODO: expose this parameter to be configured per game (global config)
    const float ModelLODTransitionTime = 0.3f;

    // Update LOD transition (note: LODTransition is mapped from [0-255] to [0-ModelLODTransitionTime] using fixed point value to use 8bit only)
    const float normalizedProgress = static_cast<float>(lodTransition) * (1.0f / 255.0f);
    const float deltaProgress = Time::Draw.UnscaledDeltaTime.GetTotalSeconds() / ModelLODTransitionTime;
    const auto newProgress = static_cast<int32>((normalizedProgress + deltaProgress) * 255.0f);
    lodTransition = static_cast<byte>(Math::Min<int32>(newProgress, 255));
}

uint64 RenderTools::CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 mipLevels)
{
    uint64 result = 0;

    if (mipLevels == 0)
        mipLevels = 69;

    uint32 rowPitch, slicePitch;
    while (mipLevels > 0 && (width >= 1 || height >= 1))
    {
        ComputePitch(format, width, height, rowPitch, slicePitch);
        result += slicePitch;

        if (width > 1)
            width >>= 1;
        if (height > 1)
            height >>= 1;

        mipLevels--;
    }

    return result;
}

uint64 RenderTools::CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 depth, int32 mipLevels)
{
    return CalculateTextureMemoryUsage(format, width, height, mipLevels) * depth;
}

float RenderTools::ComputeBoundsScreenRadiusSquared(const Float3& origin, float radius, const Float3& viewOrigin, const Matrix& projectionMatrix)
{
    const float screenMultiple = 0.5f * Math::Max(projectionMatrix.Values[0][0], projectionMatrix.Values[1][1]);
    const float distSqr = Float3::DistanceSquared(origin, viewOrigin) * projectionMatrix.Values[2][3];
    return Math::Square(screenMultiple * radius) / Math::Max(1.0f, distSqr);
}

int32 RenderTools::ComputeModelLOD(const Model* model, const Float3& origin, float radius, const RenderContext& renderContext)
{
    const auto lodView = (renderContext.LodProxyView ? renderContext.LodProxyView : &renderContext.View);
    const float screenRadiusSquared = ComputeBoundsScreenRadiusSquared(origin, radius, *lodView) * renderContext.View.ModelLODDistanceFactorSqrt;

    // Check if model is being culled
    if (Math::Square(model->MinScreenSize * 0.5f) > screenRadiusSquared)
        return -1;

    // Skip if no need to calculate LOD
    if (model->LODs.Count() <= 1)
        return 0;

    // Iterate backwards and return the first matching LOD
    for (int32 lodIndex = model->LODs.Count() - 1; lodIndex >= 0; lodIndex--)
    {
        if (Math::Square(model->LODs[lodIndex].ScreenSize * 0.5f) >= screenRadiusSquared)
        {
            return lodIndex;
        }
    }

    return 0;
}

int32 RenderTools::ComputeSkinnedModelLOD(const SkinnedModel* model, const Float3& origin, float radius, const RenderContext& renderContext)
{
    const auto lodView = (renderContext.LodProxyView ? renderContext.LodProxyView : &renderContext.View);
    const float screenRadiusSquared = ComputeBoundsScreenRadiusSquared(origin, radius, *lodView) * renderContext.View.ModelLODDistanceFactorSqrt;

    // Check if model is being culled
    if (Math::Square(model->MinScreenSize * 0.5f) > screenRadiusSquared)
        return -1;

    // Skip if no need to calculate LOD
    if (model->LODs.Count() <= 1)
        return 0;

    // Iterate backwards and return the first matching LOD
    for (int32 lodIndex = model->LODs.Count() - 1; lodIndex >= 0; lodIndex--)
    {
        if (Math::Square(model->LODs[lodIndex].ScreenSize * 0.5f) >= screenRadiusSquared)
        {
            return lodIndex;
        }
    }

    return 0;
}

void RenderTools::ComputeCascadeUpdateFrequency(int32 cascadeIndex, int32 cascadeCount, int32& updateFrequency, int32& updatePhrase, int32 updateMaxCountPerFrame)
{
    switch (updateMaxCountPerFrame)
    {
    case 1: // 1 cascade update per frame
        switch (cascadeIndex)
        {
        case 0: // Cascade 0
            updateFrequency = 2;
            updatePhrase = 0;
            break;
        case 1: // Cascade 1
            updateFrequency = 4;
            updatePhrase = 1;
            break;
        case 2: // Cascade 2
            updateFrequency = 8;
            updatePhrase = 3;
            break;
        default:
            if (cascadeCount > 4)
            {
                if (cascadeIndex == 3)
                {
                    // Cascade 3
                    updateFrequency = 16;
                    updatePhrase = 7;
                }
                else
                {
                    // Other
                    updateFrequency = 16;
                    updatePhrase = 15;
                }
            }
            else
            {
                // Cascade 3
                updateFrequency = 8;
                updatePhrase = 7;
            }
        }
        break;
    case 2: // 2 cascade2 update per frame
        switch (cascadeIndex)
        {
        case 0: // Cascade 0
            updateFrequency = 1;
            updatePhrase = 0;
            break;
        case 1: // Cascade 1
            updateFrequency = 2;
            updatePhrase = 0;
            break;
        case 2: // Cascade 2
            updateFrequency = 4;
            updatePhrase = 1;
            break;
        default:
            if (cascadeCount > 4)
            {
                if (cascadeIndex == 3)
                {
                    // Cascade 3
                    updateFrequency = 8;
                    updatePhrase = 3;
                }
                else
                {
                    // Other
                    updateFrequency = 8;
                    updatePhrase = 7;
                }
            }
            else
            {
                // Cascade 3
                updateFrequency = 4;
                updatePhrase = 3;
            }
        }
        break;
    default: // Every frame
        updateFrequency = 1;
        updatePhrase = 1;
        break;
    }
}

float RenderTools::ComputeTemporalTime()
{
    const float time = Time::Draw.UnscaledTime.GetTotalSeconds();
    const float scale = 10;
    const float integral = roundf(time / scale) * scale;
    return time - integral;
}

void RenderTools::CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal)
{
    // [Deprecated in v1.10]
    Float3 n;
    Float4 t;
    CalculateTangentFrame(n, t, normal);
    resultNormal = FloatR10G10B10A2(n, 0);
    resultTangent = FloatR10G10B10A2(t);
}

void RenderTools::CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal, const Float3& tangent)
{
    // [Deprecated in v1.10]
    Float3 n;
    Float4 t;
    CalculateTangentFrame(n, t, normal, tangent);
    resultNormal = FloatR10G10B10A2(n, 0);
    resultTangent = FloatR10G10B10A2(t);
}

void RenderTools::CalculateTangentFrame(Float3& resultNormal, Float4& resultTangent, const Float3& normal)
{
    // Calculate tangent
    const Float3 c1 = Float3::Cross(normal, Float3::UnitZ);
    const Float3 c2 = Float3::Cross(normal, Float3::UnitY);
    const Float3 tangent = c1.LengthSquared() > c2.LengthSquared() ? c1 : c2;

    // Calculate bitangent sign
    const Float3 bitangent = Float3::Normalize(Float3::Cross(normal, tangent));
    const byte sign = static_cast<byte>(Float3::Dot(Float3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

    // Set tangent frame
    resultNormal = normal * 0.5f + 0.5f;
    resultTangent = Float4(tangent * 0.5f + 0.5f, sign);
}

void RenderTools::CalculateTangentFrame(Float3& resultNormal, Float4& resultTangent, const Float3& normal, const Float3& tangent)
{
    // Calculate bitangent sign
    const Float3 bitangent = Float3::Normalize(Float3::Cross(normal, tangent));
    const byte sign = static_cast<byte>(Float3::Dot(Float3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

    // Set tangent frame
    resultNormal = normal * 0.5f + 0.5f;
    resultTangent = Float4(tangent * 0.5f + 0.5f, sign);
}

void RenderTools::ComputeSphereModelDrawMatrix(const RenderView& view, const Float3& position, float radius, Matrix& resultWorld, bool& resultIsViewInside)
{
    // Construct world matrix
    constexpr float sphereModelScale = 0.0205f; // Manually tweaked for 'Engine/Models/Sphere' with some slack
    const float scaling = radius * sphereModelScale;
    resultWorld = Matrix::Identity;
    resultWorld.M11 = scaling;
    resultWorld.M22 = scaling;
    resultWorld.M33 = scaling;
    resultWorld.M41 = position.X;
    resultWorld.M42 = position.Y;
    resultWorld.M43 = position.Z;

    // Check if view is inside the sphere
    resultIsViewInside = Float3::DistanceSquared(view.Position, position) < Math::Square(radius * 1.1f); // Manually tweaked bias
}

int32 MipLevelsCount(int32 width)
{
    int32 result = 1;
    while (width > 1)
    {
        width >>= 1;
        result++;
    }
    return result;
}

int32 MipLevelsCount(int32 width, int32 height)
{
    int32 result = 1;
    while (width > 1 || height > 1)
    {
        if (width > 1)
            width >>= 1;
        if (height > 1)
            height >>= 1;
        result++;
    }
    return result;
}

int32 MipLevelsCount(int32 width, int32 height, int32 depth)
{
    int32 result = 1;
    while (width > 1 || height > 1 || depth > 1)
    {
        if (width > 1)
            width >>= 1;
        if (height > 1)
            height >>= 1;
        if (depth > 1)
            depth >>= 1;
        result++;
    }
    return result;
}
