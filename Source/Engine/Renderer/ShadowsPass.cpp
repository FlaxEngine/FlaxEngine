// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ShadowsPass.h"
#include "GBufferPass.h"
#include "VolumetricFogPass.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Utilities/RectPack.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif

#define SHADOWS_MAX_TILES 6
#define SHADOWS_MIN_RESOLUTION 16
#define NormalOffsetScaleTweak 100.0f
#define LocalLightNearPlane 10.0f

PACK_STRUCT(struct Data{
    ShaderGBufferData GBuffer;
    ShaderLightData Light;
    Matrix WVP;
    Matrix ViewProjectionMatrix;
    float Dummy0;
    float TemporalTime;
    float ContactShadowsDistance;
    float ContactShadowsLength;
    });

struct ShadowsAtlasRectTile : RectPack<ShadowsAtlasRectTile, uint16>
{
    ShadowsAtlasRectTile(uint16 x, uint16 y, uint16 width, uint16 height)
        : RectPack<ShadowsAtlasRectTile, uint16>(x, y, width, height)
    {
    }

    void OnInsert(class ShadowsCustomBuffer* buffer);
    void OnFree(ShadowsCustomBuffer* buffer);
};

uint16 QuantizeResolution(float input)
{
    uint16 output = Math::FloorToInt(input);
    uint16 alignment = 16;
    if (output >= 512)
        alignment = 64;
    else if (output >= 256)
        alignment = 32;
    output = Math::AlignDown<uint16>(output, alignment);
    return output;
}

// State for shadow projection
struct ShadowAtlasLightTile
{
    ShadowsAtlasRectTile* RectTile;
    Matrix WorldToShadow;
    float FramesToUpdate; // Amount of frames (with fraction) until the next shadow update can happen
    bool SkipUpdate;
    Viewport CachedViewport; // The viewport used the last time to render shadow to the atlas

    void Free(ShadowsCustomBuffer* buffer)
    {
        if (RectTile)
        {
            RectTile->Free(buffer);
            RectTile = nullptr;
        }
    }

    void SetWorldToShadow(const Matrix& shadowViewProjection)
    {
        // Transform Clip Space [-1,+1]^2 to UV Space [0,1]^2 (saves MAD instruction in shader)
        const Matrix ClipToUV(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f);
        Matrix m;
        Matrix::Multiply(shadowViewProjection, ClipToUV, m);
        Matrix::Transpose(m, WorldToShadow);
    }
};

// State for shadow cache sed to invalidate any prerendered shadow depths
struct ShadowAtlasLightCache
{
    bool Valid;
    float ShadowsUpdateRate;
    float ShadowsUpdateRateAtDistance;
    Float3 Position;
    float Radius;
    Float3 Direction;
    float Distance;
    Float4 CascadeSplits;

    void Set(const RenderView& view, const RenderLightData& light, const Float4& cascadeSplits = Float4::Zero)
    {
        Valid = true;
        Distance = light.ShadowsDistance;
        ShadowsUpdateRate = light.ShadowsUpdateRate;
        ShadowsUpdateRateAtDistance = light.ShadowsUpdateRateAtDistance;
        if (light.IsDirectionalLight)
        {
            // Sun
            Position = view.Position;
            Direction = light.Direction;
            CascadeSplits = cascadeSplits;
        }
        else
        {
            // Local light
            const auto& localLight = (const RenderLocalLightData&)light;
            Position = light.Position;
            Radius = localLight.Radius;
        }
    }
};

// State for light's shadows rendering
struct ShadowAtlasLight
{
    uint64 LastFrameUsed;
    int32 ContextIndex;
    int32 ContextCount;
    uint16 Resolution;
    uint8 TilesNeeded;
    uint8 TilesCount;
    float Sharpness, Fade, NormalOffsetScale, Bias, FadeDistance, Distance;
    Float4 CascadeSplits;
    ShadowAtlasLightTile Tiles[SHADOWS_MAX_TILES];
    ShadowAtlasLightCache Cache;

    ShadowAtlasLight()
    {
        Platform::MemoryClear(this, sizeof(ShadowAtlasLight));
    }

    POD_COPYABLE(ShadowAtlasLight);

    float CalculateUpdateRateInv(const RenderLightData& light, float distanceFromView, bool& freezeUpdate) const
    {
        const float shadowsUpdateRate = light.ShadowsUpdateRate;
        const float shadowsUpdateRateAtDistance = shadowsUpdateRate * light.ShadowsUpdateRateAtDistance;
        float updateRate = Math::Lerp(shadowsUpdateRate, shadowsUpdateRateAtDistance, Math::Saturate(distanceFromView / Distance));
        // TODO: add global shadows update rate scale to be adjusted per-platform
        freezeUpdate = updateRate <= ZeroTolerance;
        if (freezeUpdate)
            return 0.0f;
        return 1.0f / updateRate;
    }

    void ValidateCache(const RenderView& view, const RenderLightData& light)
    {
        if (!Cache.Valid)
            return;
        if (!Math::NearEqual(Cache.Distance, light.ShadowsDistance) ||
            !Math::NearEqual(Cache.ShadowsUpdateRate, light.ShadowsUpdateRate) ||
            !Math::NearEqual(Cache.ShadowsUpdateRateAtDistance, light.ShadowsUpdateRateAtDistance))
        {
            // Invalidate
            Cache.Valid = false;
        }
        if (light.IsDirectionalLight)
        {
            // Sun
            if (Float3::Dot(Cache.Direction, light.Direction) < 0.999999f ||
                !Float3::NearEqual(Cache.Position, view.Position, 1.0f) ||
                !Float4::NearEqual(Cache.CascadeSplits, CascadeSplits))
            {
                // Invalidate
                Cache.Valid = false;
            }
        }
        else
        {
            // Local light
            const auto& localLight = (const RenderLocalLightData&)light;
            if (!Float3::NearEqual(Cache.Position, light.Position, 1.0f) ||
                !Math::NearEqual(Cache.Radius, localLight.Radius))
            {
                // Invalidate
                Cache.Valid = false;
            }
        }
        for (int32 i = 0; i < TilesCount && Cache.Valid; i++)
        {
            auto& tile = Tiles[i];
            if (tile.CachedViewport != Viewport(tile.RectTile->X, tile.RectTile->Y, tile.RectTile->Width, tile.RectTile->Height))
            {
                // Invalidate
                Cache.Valid = false;
            }
        }
    }
};

class ShadowsCustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    int32 Resolution = 0;
    int32 AtlasPixelsUsed = 0;
    mutable bool ClearShadowMapAtlas = true;
    Vector3 ViewOrigin;
    GPUTexture* ShadowMapAtlas = nullptr;
    DynamicTypedBuffer ShadowsBuffer;
    GPUBufferView* ShadowsBufferView = nullptr;
    ShadowsAtlasRectTile* AtlasTiles = nullptr; // TODO: optimize with a single allocation for atlas tiles
    Dictionary<Guid, ShadowAtlasLight> Lights;

    ShadowsCustomBuffer()
        : ShadowsBuffer(1024, PixelFormat::R32G32B32A32_Float, false, TEXT("ShadowsBuffer"))
    {
        ShadowMapAtlas = GPUDevice::Instance->CreateTexture(TEXT("Shadow Map Atlas"));
    }

    void ClearTiles()
    {
        ClearShadowMapAtlas = true;
        AtlasPixelsUsed = 0;
        SAFE_DELETE(AtlasTiles);
        for (auto it = Lights.Begin(); it.IsNotEnd(); ++it)
        {
            auto& atlasLight = it->Value;
            Platform::MemoryClear(atlasLight.Tiles, sizeof(atlasLight.Tiles));
            Platform::MemoryClear(&atlasLight.Cache, sizeof(atlasLight.Cache));
        }
    }

    void Reset()
    {
        Lights.Clear();
        ClearTiles();
        ViewOrigin = Vector3::Zero;
    }

    ~ShadowsCustomBuffer()
    {
        Reset();
        SAFE_DELETE_GPU_RESOURCE(ShadowMapAtlas);
    }
};

void ShadowsAtlasRectTile::OnInsert(ShadowsCustomBuffer* buffer)
{
    buffer->AtlasPixelsUsed += (int32)Width * (int32)Height;
}

void ShadowsAtlasRectTile::OnFree(ShadowsCustomBuffer* buffer)
{
    buffer->AtlasPixelsUsed -= (int32)Width * (int32)Height;
}

String ShadowsPass::ToString() const
{
    return TEXT("ShadowsPass");
}

bool ShadowsPass::Init()
{
    // Create pipeline states
    _psShadowDir.CreatePipelineStates();
    _psShadowPoint.CreatePipelineStates();
    _psShadowSpot.CreatePipelineStates();

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Shadows"));
    _sphereModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Sphere"));
    if (_shader == nullptr || _sphereModel == nullptr)
        return true;

#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ShadowsPass, &ShadowsPass::OnShaderReloading>(this);
#endif

    // Select format for shadow maps
    _shadowMapFormat = PixelFormat::Unknown;
#if !PLATFORM_SWITCH // TODO: fix shadows performance issue on Switch
    for (const PixelFormat format : { PixelFormat::D16_UNorm, PixelFormat::D24_UNorm_S8_UInt, PixelFormat::D32_Float })
    {
        const auto formatTexture = PixelFormatExtensions::FindShaderResourceFormat(format, false);
        const auto formatFeaturesDepth = GPUDevice::Instance->GetFormatFeatures(format);
        const auto formatFeaturesTexture = GPUDevice::Instance->GetFormatFeatures(formatTexture);
        if (EnumHasAllFlags(formatFeaturesDepth.Support, FormatSupport::DepthStencil | FormatSupport::Texture2D | FormatSupport::TextureCube) &&
            EnumHasAllFlags(formatFeaturesTexture.Support, FormatSupport::ShaderSample | FormatSupport::ShaderSampleComparison))
        {
            _shadowMapFormat = format;
            break;
        }
    }
#endif
    if (_shadowMapFormat == PixelFormat::Unknown)
        LOG(Warning, "GPU doesn't support shadows rendering");

    return false;
}

bool ShadowsPass::setupResources()
{
    // Wait for the assets
    if (!_sphereModel->CanBeRendered() || !_shader->IsLoaded())
        return true;
    auto shader = _shader->GetShader();

    // Validate shader constant buffers sizes
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psShadowPoint.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.CullMode = CullMode::TwoSided;
        psDesc.VS = shader->GetVS("VS_Model");
        if (_psShadowPoint.Create(psDesc, shader, "PS_PointLight"))
            return true;
    }
    if (!_psShadowDir.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        if (_psShadowDir.Create(psDesc, shader, "PS_DirLight"))
            return true;
    }
    if (!_psShadowSpot.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.CullMode = CullMode::TwoSided;
        psDesc.VS = shader->GetVS("VS_Model");
        if (_psShadowSpot.Create(psDesc, shader, "PS_SpotLight"))
            return true;
    }
    if (_psDepthClear == nullptr)
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = GPUDevice::Instance->QuadShader->GetPS("PS_DepthClear");
        psDesc.DepthEnable = true;
        psDesc.DepthWriteEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::None;
        _psDepthClear = GPUDevice::Instance->CreatePipelineState();
        if (_psDepthClear->Init(psDesc))
            return true;
    }

    return false;
}

void ShadowsPass::SetupRenderContext(RenderContext& renderContext, RenderContext& shadowContext)
{
    const auto& view = renderContext.View;

    // Use the current render view to sync model LODs with the shadow maps rendering stage
    shadowContext.LodProxyView = &renderContext.View;

    // Prepare properties
    auto& shadowView = shadowContext.View;
    shadowView.Flags = view.Flags;
    shadowView.StaticFlagsMask = view.StaticFlagsMask;
    shadowView.RenderLayersMask = view.RenderLayersMask;
    shadowView.IsOfflinePass = view.IsOfflinePass;
    shadowView.ModelLODBias = view.ModelLODBias;
    shadowView.ModelLODDistanceFactor = view.ModelLODDistanceFactor;
    shadowView.Pass = DrawPass::Depth;
    shadowView.Origin = view.Origin;
    shadowContext.List = RenderList::GetFromPool();
    shadowContext.Buffers = renderContext.Buffers;
    shadowContext.Task = renderContext.Task;
    shadowContext.List->Clear();
}

void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderLightData& light, ShadowAtlasLight& atlasLight)
{
    // Copy light properties
    atlasLight.Sharpness = light.ShadowsSharpness;
    atlasLight.Fade = light.ShadowsStrength;
    atlasLight.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / (float)atlasLight.Resolution);
    atlasLight.Bias = light.ShadowsDepthBias;
    atlasLight.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    atlasLight.Distance = Math::Min(renderContext.View.Far, light.ShadowsDistance);
}

bool ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderLocalLightData& light, ShadowAtlasLight& atlasLight)
{
    SetupLight(renderContext, renderContextBatch, (RenderLightData&)light, atlasLight);

    // Fade shadow on distance
    const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    const float dstLightToView = Float3::Distance(light.Position, renderContext.View.Position) - light.Radius;
    const float fade = 1 - Math::Saturate((dstLightToView - atlasLight.Distance + fadeDistance) / fadeDistance);
    atlasLight.Fade *= fade;

    // Update cached state (invalidate it if the light changed)
    atlasLight.ValidateCache(renderContext.View, light);

    // Calculate update rate based on the distance to the view
    bool freezeUpdate;
    const float updateRateInv = atlasLight.CalculateUpdateRateInv(light, dstLightToView, freezeUpdate);
    float& framesToUpdate = atlasLight.Tiles[0].FramesToUpdate; // Use the first tile for all local light projections to be in sync
    if ((framesToUpdate > 0.0f || freezeUpdate) && atlasLight.Cache.Valid)
    {
        // Light state matches the cached state and the update rate allows us to reuse the cached shadow map so skip update
        if (!freezeUpdate)
            framesToUpdate -= 1.0f;
        for (auto& tile : atlasLight.Tiles)
            tile.SkipUpdate = true;
        return true;
    }
    framesToUpdate += updateRateInv - 1.0f;

    // Cache the current state
    atlasLight.Cache.Set(renderContext.View, light);
    for (int32 i = 0; i < atlasLight.TilesCount; i++)
    {
        auto& tile = atlasLight.Tiles[i];
        tile.SkipUpdate = false;
        tile.CachedViewport = Viewport(tile.RectTile->X, tile.RectTile->Y, tile.RectTile->Width, tile.RectTile->Height);
    }

    return false;
}

void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderDirectionalLightData& light, ShadowAtlasLight& atlasLight)
{
    SetupLight(renderContext, renderContextBatch, (RenderLightData&)light, atlasLight);

    const RenderView& view = renderContext.View;
    const int32 csmCount = atlasLight.TilesCount;
    const auto shadowMapsSize = (float)atlasLight.Resolution;

    // Calculate cascade splits
    const float minDistance = view.Near;
    const float maxDistance = view.Near + atlasLight.Distance;
    const float viewRange = view.Far - view.Near;
    float cascadeSplits[MAX_CSM_CASCADES];
    {
        PartitionMode partitionMode = light.PartitionMode;
        float splitDistance0 = light.Cascade1Spacing;
        float splitDistance1 = Math::Max(splitDistance0, light.Cascade2Spacing);
        float splitDistance2 = Math::Max(splitDistance1, light.Cascade3Spacing);
        float splitDistance3 = Math::Max(splitDistance2, light.Cascade4Spacing);

        // Compute the split distances based on the partitioning mode
        if (partitionMode == PartitionMode::Manual)
        {
            if (csmCount == 1)
            {
                cascadeSplits[0] = minDistance + splitDistance3 * maxDistance;
            }
            else if (csmCount == 2)
            {
                cascadeSplits[0] = minDistance + splitDistance1 * maxDistance;
                cascadeSplits[1] = minDistance + splitDistance3 * maxDistance;
            }
            else if (csmCount == 3)
            {
                cascadeSplits[0] = minDistance + splitDistance1 * maxDistance;
                cascadeSplits[1] = minDistance + splitDistance2 * maxDistance;
                cascadeSplits[2] = minDistance + splitDistance3 * maxDistance;
            }
            else if (csmCount == 4)
            {
                cascadeSplits[0] = minDistance + splitDistance0 * maxDistance;
                cascadeSplits[1] = minDistance + splitDistance1 * maxDistance;
                cascadeSplits[2] = minDistance + splitDistance2 * maxDistance;
                cascadeSplits[3] = minDistance + splitDistance3 * maxDistance;
            }
        }
        else if (partitionMode == PartitionMode::Logarithmic || partitionMode == PartitionMode::PSSM)
        {
            const float pssmFactor = 0.5f;
            const float lambda = partitionMode == PartitionMode::PSSM ? pssmFactor : 1.0f;
            const auto range = maxDistance - minDistance;
            const auto ratio = maxDistance / minDistance;
            const auto logRatio = Math::Clamp(1.0f - lambda, 0.0f, 1.0f);
            for (int32 cascadeLevel = 0; cascadeLevel < csmCount; cascadeLevel++)
            {
                // Compute cascade split (between znear and zfar)
                const float distribute = static_cast<float>(cascadeLevel + 1) / csmCount;
                float logZ = minDistance * Math::Pow(ratio, distribute);
                float uniformZ = minDistance + range * distribute;
                cascadeSplits[cascadeLevel] = Math::Lerp(uniformZ, logZ, logRatio);
            }
        }

        // Convert distance splits to ratios cascade in the range [0, 1]
        for (int32 i = 0; i < MAX_CSM_CASCADES; i++)
            cascadeSplits[i] = (cascadeSplits[i] - view.Near) / viewRange;
    }
    atlasLight.CascadeSplits = view.Near + Float4(cascadeSplits) * viewRange;

    // Update cached state (invalidate it if the light changed)
    atlasLight.ValidateCache(renderContext.View, light);

    // Update cascades to check which should be updated this frame
    atlasLight.ContextIndex = renderContextBatch.Contexts.Count();
    atlasLight.ContextCount = 0;
    for (int32 cascadeIndex = 0; cascadeIndex < csmCount; cascadeIndex++)
    {
        const float dstToCascade = atlasLight.CascadeSplits.Raw[cascadeIndex];
        bool freezeUpdate;
        const float updateRateInv = atlasLight.CalculateUpdateRateInv(light, dstToCascade, freezeUpdate);
        auto& tile = atlasLight.Tiles[cascadeIndex];
        if ((tile.FramesToUpdate > 0.0f || freezeUpdate) && atlasLight.Cache.Valid)
        {
            // Light state matches the cached state and the update rate allows us to reuse the cached shadow map so skip update
            if (!freezeUpdate)
                tile.FramesToUpdate -= 1.0f;
            tile.SkipUpdate = true;
            continue;
        }
        tile.FramesToUpdate += updateRateInv - 1.0f;

        // Cache the current state
        tile.SkipUpdate = false;
        tile.CachedViewport = Viewport(tile.RectTile->X, tile.RectTile->Y, tile.RectTile->Width, tile.RectTile->Height);
        atlasLight.ContextCount++;
    }

    // Init shadow data
    atlasLight.ContextIndex = renderContextBatch.Contexts.Count();
    if (atlasLight.ContextCount == 0)
        return;
    renderContextBatch.Contexts.AddDefault(atlasLight.ContextCount);
    atlasLight.Cache.Set(renderContext.View, light, atlasLight.CascadeSplits);

    // Select best Up vector
    Float3 side = Float3::UnitX;
    Float3 upDirection = Float3::UnitX;
    Float3 vectorUps[] = { Float3::UnitY, Float3::UnitX, Float3::UnitZ };
    for (int32 i = 0; i < ARRAY_COUNT(vectorUps); i++)
    {
        const Float3 vectorUp = vectorUps[i];
        if (Math::Abs(Float3::Dot(light.Direction, vectorUp)) < (1.0f - 0.0001f))
        {
            side = Float3::Normalize(Float3::Cross(vectorUp, light.Direction));
            upDirection = Float3::Normalize(Float3::Cross(light.Direction, side));
            break;
        }
    }

    // Create the different view and projection matrices for each split
    float splitMinRatio = 0;
    float splitMaxRatio = (minDistance - view.Near) / viewRange;
    int32 contextIndex = 0;
    for (int32 cascadeIndex = 0; cascadeIndex < csmCount; cascadeIndex++)
    {
        const auto oldSplitMinRatio = splitMinRatio;
        splitMinRatio = splitMaxRatio;
        splitMaxRatio = cascadeSplits[cascadeIndex];

        auto& tile = atlasLight.Tiles[cascadeIndex];
        if (tile.SkipUpdate)
            continue;

        // Calculate cascade split frustum corners in view space
        Float3 frustumCorners[8];
        for (int32 j = 0; j < 4; j++)
        {
            float overlap = 0.1f * (splitMinRatio - oldSplitMinRatio); // CSM blending overlap
            const RenderList* mainCache = renderContext.List;
            const auto frustumRangeVS = mainCache->FrustumCornersVs[j + 4] - mainCache->FrustumCornersVs[j];
            frustumCorners[j] = mainCache->FrustumCornersVs[j] + frustumRangeVS * (splitMinRatio - overlap);
            frustumCorners[j + 4] = mainCache->FrustumCornersVs[j] + frustumRangeVS * splitMaxRatio;
        }

        // Perform stabilization
        enum StabilizationMode
        {
            None,
            ProjectionSnapping,
            ViewSnapping,
        };
        const StabilizationMode stabilization = ViewSnapping; // TODO: expose to graphics settings maybe
        Float3 cascadeMinBoundLS;
        Float3 cascadeMaxBoundLS;
        Float3 target;
        {
            // Make sure we are using the same direction when stabilizing
            BoundingSphere boundingVS;
            BoundingSphere::FromPoints(frustumCorners, ARRAY_COUNT(frustumCorners), boundingVS);

            // Compute bounding box center
            Float3::TransformCoordinate(boundingVS.Center, view.IV, target);
            float boundingVSRadius = (float)boundingVS.Radius;
            cascadeMaxBoundLS = Float3(boundingVSRadius);
            cascadeMinBoundLS = -cascadeMaxBoundLS;

            if (stabilization == ViewSnapping)
            {
                // Snap the target to the texel units (reference: ShaderX7 - Practical Cascaded Shadows Maps)
                float shadowMapHalfSize = shadowMapsSize * 0.5f;
                float x = Math::Ceil(Float3::Dot(target, upDirection) * shadowMapHalfSize / boundingVSRadius) * boundingVSRadius / shadowMapHalfSize;
                float y = Math::Ceil(Float3::Dot(target, side) * shadowMapHalfSize / boundingVSRadius) * boundingVSRadius / shadowMapHalfSize;
                float z = Float3::Dot(target, light.Direction);
                target = upDirection * x + side * y + light.Direction * z;
            }
        }

        const auto nearClip = 0.0f;
        const auto farClip = cascadeMaxBoundLS.Z - cascadeMinBoundLS.Z;

        // Create shadow view matrix
        Matrix shadowView, shadowProjection, shadowVP;
        Matrix::LookAt(target - light.Direction * cascadeMaxBoundLS.Z, target, upDirection, shadowView);

        // Create viewport for culling with extended near/far planes due to culling issues
        Matrix cullingVP;
        {
            const float cullRangeExtent = 100000.0f;
            Matrix::OrthoOffCenter(cascadeMinBoundLS.X, cascadeMaxBoundLS.X, cascadeMinBoundLS.Y, cascadeMaxBoundLS.Y, -cullRangeExtent, farClip + cullRangeExtent, shadowProjection);
            Matrix::Multiply(shadowView, shadowProjection, cullingVP);
        }

        // Create shadow projection matrix
        Matrix::OrthoOffCenter(cascadeMinBoundLS.X, cascadeMaxBoundLS.X, cascadeMinBoundLS.Y, cascadeMaxBoundLS.Y, nearClip, farClip, shadowProjection);

        // Construct shadow matrix (View * Projection)
        Matrix::Multiply(shadowView, shadowProjection, shadowVP);

        // Stabilize the shadow matrix on the projection
        if (stabilization == ProjectionSnapping)
        {
            Float3 shadowPixelPosition = shadowVP.GetTranslation() * (shadowMapsSize * 0.5f);
            shadowPixelPosition.Z = 0;
            const Float3 shadowPixelPositionRounded(Math::Round(shadowPixelPosition.X), Math::Round(shadowPixelPosition.Y), 0.0f);
            const Float4 shadowPixelOffset((shadowPixelPositionRounded - shadowPixelPosition) * (2.0f / shadowMapsSize), 0.0f);
            shadowProjection.SetRow4(shadowProjection.GetRow4() + shadowPixelOffset);
            Matrix::Multiply(shadowView, shadowProjection, shadowVP);
        }

        tile.SetWorldToShadow(shadowVP);

        // Setup context for cascade
        auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
        SetupRenderContext(renderContext, shadowContext);
        shadowContext.View.Position = light.Direction * -atlasLight.Distance + view.Position;
        shadowContext.View.Direction = light.Direction;
        shadowContext.View.SetUp(shadowView, shadowProjection);
        shadowContext.View.CullingFrustum.SetMatrix(cullingVP);
        shadowContext.View.PrepareCache(shadowContext, shadowMapsSize, shadowMapsSize, Float2::Zero, &view);
    }
}

void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderPointLightData& light, ShadowAtlasLight& atlasLight)
{
    if (SetupLight(renderContext, renderContextBatch, (RenderLocalLightData&)light, atlasLight))
        return;

    // Render depth to all 6 faces of the cube map
    atlasLight.ContextIndex = renderContextBatch.Contexts.Count();
    atlasLight.ContextCount = 6;
    renderContextBatch.Contexts.AddDefault(atlasLight.ContextCount);
    for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex + faceIndex];
        SetupRenderContext(renderContext, shadowContext);
        shadowContext.View.SetUpCube(LocalLightNearPlane, light.Radius, light.Position);
        shadowContext.View.SetFace(faceIndex);
        const auto shadowMapsSize = (float)atlasLight.Resolution;
        shadowContext.View.PrepareCache(shadowContext, shadowMapsSize, shadowMapsSize, Float2::Zero, &renderContext.View);
        atlasLight.Tiles[faceIndex].SetWorldToShadow(shadowContext.View.ViewProjection());
    }
}

void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderSpotLightData& light, ShadowAtlasLight& atlasLight)
{
    if (SetupLight(renderContext, renderContextBatch, (RenderLocalLightData&)light, atlasLight))
        return;

    // Render depth to a single projection
    atlasLight.ContextIndex = renderContextBatch.Contexts.Count();
    atlasLight.ContextCount = 1;
    renderContextBatch.Contexts.AddDefault(atlasLight.ContextCount);
    auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex];
    SetupRenderContext(renderContext, shadowContext);
    shadowContext.View.SetProjector(LocalLightNearPlane, light.Radius, light.Position, light.Direction, light.UpVector, light.OuterConeAngle * 2.0f);
    const auto shadowMapsSize = (float)atlasLight.Resolution;
    shadowContext.View.PrepareCache(shadowContext, shadowMapsSize, shadowMapsSize, Float2::Zero, &renderContext.View);
    atlasLight.Tiles[0].SetWorldToShadow(shadowContext.View.ViewProjection());
}

void ShadowsPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psShadowDir.Delete();
    _psShadowPoint.Delete();
    _psShadowSpot.Delete();
    _shader = nullptr;
    _sphereModel = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_psDepthClear);
}

void ShadowsPass::SetupShadows(RenderContext& renderContext, RenderContextBatch& renderContextBatch)
{
    PROFILE_CPU();
    _maxShadowsQuality = Math::Clamp(Math::Min<int32>((int32)Graphics::ShadowsQuality, (int32)renderContext.View.MaxShadowsQuality), 0, (int32)Quality::MAX - 1);

    // Early out and skip shadows setup if no lights is actively casting shadows
    // RenderBuffers will automatically free any old ShadowsCustomBuffer after a few frames if we don't update LastFrameUsed
    if (_shadowMapFormat == PixelFormat::Unknown || checkIfSkipPass() || EnumHasNoneFlags(renderContext.View.Flags, ViewFlags::Shadows))
        return;
    Array<RenderLightData*, RendererAllocation> shadowedLights;
    for (auto& light : renderContext.List->DirectionalLights)
    {
        if (light.CanRenderShadow(renderContext.View))
            shadowedLights.Add(&light);
    }
    for (auto& light : renderContext.List->SpotLights)
    {
        if (light.CanRenderShadow(renderContext.View))
            shadowedLights.Add(&light);
    }
    for (auto& light : renderContext.List->PointLights)
    {
        if (light.CanRenderShadow(renderContext.View))
            shadowedLights.Add(&light);
    }
    if (shadowedLights.IsEmpty())
        return;

    // Initialize shadow atlas
    auto& shadows = *renderContext.Buffers->GetCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"));
    const auto currentFrame = Engine::FrameCount;
    shadows.LastFrameUsed = currentFrame;
    int32 atlasResolution;
    switch (Graphics::ShadowMapsQuality)
    {
    case Quality::Low:
        atlasResolution = 1024;
        break;
    case Quality::Medium:
        atlasResolution = 2048;
        break;
    case Quality::High:
        atlasResolution = 4096;
        break;
    case Quality::Ultra:
        atlasResolution = 8192;
        break;
    default:
        return;
    }
    const int32 baseLightResolution = atlasResolution / MAX_CSM_CASCADES; // Allow to store 4 CSM cascades in a single row in all cases
    if (shadows.Resolution != atlasResolution)
    {
        shadows.Reset();
        auto desc = GPUTextureDescription::New2D(atlasResolution, atlasResolution, _shadowMapFormat, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil);
        if (shadows.ShadowMapAtlas->Init(desc))
        {
            LOG(Fatal, "Failed to setup shadow map of size {0}x{1} and format {2}", desc.Width, desc.Height, ScriptingEnum::ToString(desc.Format));
            return;
        }
        shadows.ClearShadowMapAtlas = true;
        shadows.Resolution = atlasResolution;
    }
    if (renderContext.View.Origin != shadows.ViewOrigin)
    {
        // Large Worlds chunk movement so invalidate cached shadows
        shadows.Reset();
        shadows.ViewOrigin = renderContext.View.Origin;
    }
    if (!shadows.AtlasTiles)
        shadows.AtlasTiles = New<ShadowsAtlasRectTile>(0, 0, atlasResolution, atlasResolution);

    // Update/add lights
    for (const RenderLightData* light : shadowedLights)
    {
        auto& atlasLight = shadows.Lights[light->ID];

        // Calculate resolution for this light
        // TODO: add support for fixed shadow map resolution assigned per-light
        float lightResolutionFloat = baseLightResolution * light->ScreenSize;
        atlasLight.Resolution = QuantizeResolution(lightResolutionFloat);

        // Cull too small lights
        if (atlasLight.Resolution < SHADOWS_MIN_RESOLUTION)
            continue;

        if (light->IsDirectionalLight)
        {
            atlasLight.TilesNeeded = Math::Clamp(((const RenderDirectionalLightData*)light)->CascadeCount, 1, MAX_CSM_CASCADES);

            // Views with orthographic cameras cannot use cascades, we force it to 1 shadow map here
            if (renderContext.View.IsOrthographicProjection())
                atlasLight.TilesNeeded = 1;
        }
        else if (light->IsPointLight)
            atlasLight.TilesNeeded = 6;
        else
            atlasLight.TilesNeeded = 1;
        atlasLight.LastFrameUsed = currentFrame;
    }

    // Remove unused lights (before inserting any new ones to make space in the atlas)
    for (auto it = shadows.Lights.Begin(); it.IsNotEnd(); ++it)
    {
        if (it->Value.LastFrameUsed != currentFrame)
        {
            for (ShadowAtlasLightTile& tile : it->Value.Tiles)
                tile.Free(&shadows);
            shadows.Lights.Remove(it);
        }
    }

    // Calculate size requirements for atlas
    int32 atlasPixelsNeeded = 0;
    for (auto it = shadows.Lights.Begin(); it.IsNotEnd(); ++it)
    {
        const auto& atlasLight = it->Value;
        atlasPixelsNeeded += atlasLight.Resolution * atlasLight.Resolution * atlasLight.TilesNeeded;
    }
    const int32 atlasPixelsAllowed = atlasResolution * atlasResolution;
    const float atlasPixelsCoverage = (float)atlasPixelsNeeded / atlasPixelsAllowed;

    // If atlas is overflown then scale down the shadows resolution
    float resolutionScale = 1.0f;
    if (atlasPixelsCoverage > 1.0f)
        resolutionScale /= atlasPixelsCoverage;
    float finalScale = 1.0f;
    bool defragDone = false;
RETRY_ATLAS_SETUP:

    // Apply additional scale to the shadows resolution
    if (!Math::IsOne(resolutionScale))
    {
        finalScale *= resolutionScale;
        for (const RenderLightData* light : shadowedLights)
        {
            auto& atlasLight = shadows.Lights[light->ID];
            if (light->IsDirectionalLight && !defragDone)
                continue; // Reduce scaling on directional light shadows (before defrag)
            atlasLight.Resolution = QuantizeResolution(atlasLight.Resolution * resolutionScale);
        }
    }

    // Macro checks if light has proper amount of tiles already assigned and the resolution is matching
#define IS_LIGHT_TILE_REUSABLE (atlasLight.TilesCount == atlasLight.TilesNeeded && atlasLight.Tiles[0].RectTile && atlasLight.Tiles[0].RectTile->Width == atlasLight.Resolution)

    // Remove incorrect tiles before allocating new ones
    for (RenderLightData* light : shadowedLights)
    {
        ShadowAtlasLight& atlasLight = shadows.Lights[light->ID];
        if (IS_LIGHT_TILE_REUSABLE)
            continue;

        // Remove existing tiles
        for (ShadowAtlasLightTile& tile : atlasLight.Tiles)
            tile.Free(&shadows);
    }

    // Insert tiles into the atlas (already sorted to favor the first ones)
    for (RenderLightData* light : shadowedLights)
    {
        auto& atlasLight = shadows.Lights[light->ID];
        if (IS_LIGHT_TILE_REUSABLE || atlasLight.Resolution < SHADOWS_MIN_RESOLUTION)
            continue;

        // Try to insert tiles
        bool failedToInsert = false;
        for (int32 tileIndex = 0; tileIndex < atlasLight.TilesNeeded; tileIndex++)
        {
            auto rectTile = shadows.AtlasTiles->Insert(atlasLight.Resolution, atlasLight.Resolution, 0, &shadows);
            if (!rectTile)
            {
                // Free any previous tiles that were added
                for (int32 i = 0; i < tileIndex; i++)
                    atlasLight.Tiles[i].Free(&shadows);
                failedToInsert = true;
                break;
            }
            atlasLight.Tiles[tileIndex].RectTile = rectTile;
        }
        if (failedToInsert)
        {
            if (defragDone)
            {
                // Already defragmented atlas so scale it down
                resolutionScale = 0.8f;
            }
            else
            {
                // Defragment atlas without changing scale
                defragDone = true;
                resolutionScale = 1.0f;
            }

            // Rebuild atlas
            shadows.ClearTiles();
            shadows.AtlasTiles = New<ShadowsAtlasRectTile>(0, 0, atlasResolution, atlasResolution);
            goto RETRY_ATLAS_SETUP;
        }
    }

    // Setup shadows for all lights
    for (RenderLightData* light : shadowedLights)
    {
        auto& atlasLight = shadows.Lights[light->ID];
        if (atlasLight.Tiles[0].RectTile && atlasLight.Tiles[0].RectTile->Width == atlasLight.Resolution)
        {
            // Invalidate cache when whole atlas will be cleared
            if (shadows.ClearShadowMapAtlas)
                atlasLight.Cache.Valid = false;

            light->HasShadow = true;
            atlasLight.TilesCount = atlasLight.TilesNeeded;
            if (light->IsPointLight)
                SetupLight(renderContext, renderContextBatch, *(RenderPointLightData*)light, atlasLight);
            else if (light->IsSpotLight)
                SetupLight(renderContext, renderContextBatch, *(RenderSpotLightData*)light, atlasLight);
            else //if (light->IsDirectionalLight)
                SetupLight(renderContext, renderContextBatch, *(RenderDirectionalLightData*)light, atlasLight);
        }
    }

#undef IS_LIGHT_TILE_REUSABLE

    // Update shadows buffer (contains packed data with all shadow projections in the atlas)
    const float atlasResolutionInv = 1.0f / (float)atlasResolution;
    shadows.ShadowsBuffer.Clear();
    shadows.ShadowsBuffer.Write(Float4::Zero); // Insert dummy prefix so ShadowsBufferAddress=0 indicates no shadow
    for (RenderLightData* light : shadowedLights)
    {
        auto& atlasLight = shadows.Lights[light->ID];
        if (atlasLight.Tiles[0].RectTile == nullptr)
        {
            light->ShadowsBufferAddress = 0; // Clear to indicate no shadow
            continue;
        }

        // Cache start of the shadow data for this light
        light->ShadowsBufferAddress = shadows.ShadowsBuffer.Data.Count() / sizeof(Float4);

        // Write shadow data (this must match HLSL)
        {
            // Shadow info
            auto* packed = shadows.ShadowsBuffer.WriteReserve<Float4>(2);
            Color32 packed0x((byte)(atlasLight.Sharpness * (255.0f / 10.0f)), (byte)(atlasLight.Fade * 255.0f), (byte)atlasLight.TilesCount, 0);
            packed[0] = Float4(*(const float*)&packed0x, atlasLight.FadeDistance, atlasLight.NormalOffsetScale, atlasLight.Bias);
            packed[1] = atlasLight.CascadeSplits;
        }
        for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
        {
            // Shadow projection info
            const ShadowAtlasLightTile& tile = atlasLight.Tiles[tileIndex];
            ASSERT(tile.RectTile);
            auto* packed = shadows.ShadowsBuffer.WriteReserve<Float4>(5);
            packed[0] = Float4(tile.RectTile->Width, tile.RectTile->Height, tile.RectTile->X, tile.RectTile->Y) * atlasResolutionInv; // UV to AtlasUV via a single MAD instruction
            packed[1] = tile.WorldToShadow.GetColumn1();
            packed[2] = tile.WorldToShadow.GetColumn2();
            packed[3] = tile.WorldToShadow.GetColumn3();
            packed[4] = tile.WorldToShadow.GetColumn4();
        }
    }
    GPUContext* context = GPUDevice::Instance->GetMainContext();
    shadows.ShadowsBuffer.Flush(context);
    shadows.ShadowsBufferView = shadows.ShadowsBuffer.GetBuffer()->View();
}

void ShadowsPass::RenderShadowMaps(RenderContextBatch& renderContextBatch)
{
    const RenderContext& renderContext = renderContextBatch.GetMainContext();
    const ShadowsCustomBuffer* shadowsPtr = renderContext.Buffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"));
    if (shadowsPtr == nullptr || shadowsPtr->Lights.IsEmpty() || shadowsPtr->LastFrameUsed != Engine::FrameCount)
        return;
    PROFILE_GPU_CPU("ShadowMaps");
    const ShadowsCustomBuffer& shadows = *shadowsPtr;
    GPUContext* context = GPUDevice::Instance->GetMainContext();
    context->ResetSR();
    context->SetRenderTarget(shadows.ShadowMapAtlas->View(), (GPUTextureView*)nullptr);
    GPUConstantBuffer* quadShaderCB;
    if (shadows.ClearShadowMapAtlas)
    {
        context->ClearDepth(shadows.ShadowMapAtlas->View());
    }
    else
    {
        QuadShaderData quadShaderData;
        quadShaderData.Color = Float4::One; // Color.r is used by PS_DepthClear in Quad shader to clear depth
        quadShaderCB = GPUDevice::Instance->QuadShader->GetCB(0);
        context->UpdateCB(quadShaderCB, &quadShaderData);
    }

    // Render depth to all shadow map tiles
    for (auto& e : shadows.Lights)
    {
        const ShadowAtlasLight& atlasLight = e.Value;
        int32 contextIndex = 0;
        for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
        {
            const ShadowAtlasLightTile& tile = atlasLight.Tiles[tileIndex];
            if (!tile.RectTile)
                break;
            if (tile.SkipUpdate)
                continue;

            // Set viewport for tile
            ASSERT_LOW_LAYER(tile.CachedViewport == Viewport(tile.RectTile->X, tile.RectTile->Y, tile.RectTile->Width, tile.RectTile->Height));
            context->SetViewportAndScissors(tile.CachedViewport);

            if (!shadows.ClearShadowMapAtlas)
            {
                // Clear tile depth
                context->BindCB(0, quadShaderCB);
                context->SetState(_psDepthClear);
                context->DrawFullscreenTriangle();
            }

            // Draw objects depth
            auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
            shadowContext.List->ExecuteDrawCalls(shadowContext, DrawCallsListType::Depth);
            shadowContext.List->ExecuteDrawCalls(shadowContext, shadowContext.List->ShadowDepthDrawCallsList, renderContext.List->DrawCalls, nullptr);
        }
    }

    // Restore GPU context
    context->ResetSR();
    context->ResetRenderTarget();
    context->SetViewportAndScissors(renderContext.Task->GetViewport());
    shadows.ClearShadowMapAtlas = false;
}

void ShadowsPass::RenderShadowMask(RenderContextBatch& renderContextBatch, RenderLightData& light, GPUTextureView* shadowMask)
{
    ASSERT(light.HasShadow);
    PROFILE_GPU_CPU("Shadow");
    GPUContext* context = GPUDevice::Instance->GetMainContext();
    RenderContext& renderContext = renderContextBatch.GetMainContext();
    const ShadowsCustomBuffer& shadows = *renderContext.Buffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"));
    ASSERT(shadows.LastFrameUsed == Engine::FrameCount);
    auto& view = renderContext.View;
    auto shader = _shader->GetShader();
    const bool isLocalLight = light.IsPointLight || light.IsSpotLight;
    int32 shadowQuality = _maxShadowsQuality;
    if (isLocalLight)
    {
        // Reduce shadows quality for smaller lights
        if (light.ScreenSize < 0.25f)
            shadowQuality--;
        if (light.ScreenSize < 0.1f)
            shadowQuality--;
        shadowQuality = Math::Max(shadowQuality, 0);
    }

    // Setup shader data
    Data sperLight;
    GBufferPass::SetInputs(view, sperLight.GBuffer);
    if (light.IsDirectionalLight)
        ((RenderDirectionalLightData&)light).SetShaderData(sperLight.Light, true);
    else if (light.IsPointLight)
        ((RenderPointLightData&)light).SetShaderData(sperLight.Light, true);
    else if (light.IsSpotLight)
        ((RenderSpotLightData&)light).SetShaderData(sperLight.Light, true);
    Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
    sperLight.TemporalTime = renderContext.List->Setup.UseTemporalAAJitter ? RenderTools::ComputeTemporalTime() : 0.0f;
    sperLight.ContactShadowsDistance = light.ShadowsDistance;
    sperLight.ContactShadowsLength = EnumHasAnyFlags(view.Flags, ViewFlags::ContactShadows) ? light.ContactShadowsLength : 0.0f;
    if (isLocalLight)
    {
        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp;
        bool isInside;
        RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, light.Position, ((RenderLocalLightData&)light).Radius, world, isInside);
        Matrix::Multiply(world, view.ViewProjection(), wvp);
        Matrix::Transpose(wvp, sperLight.WVP);
    }

    // Render shadow in screen space
    GPUConstantBuffer* cb0 = shader->GetCB(0);
    context->UpdateCB(cb0, &sperLight);
    context->BindCB(0, cb0);
    context->BindSR(5, shadows.ShadowsBufferView);
    context->BindSR(6, shadows.ShadowMapAtlas);
    const int32 permutationIndex = shadowQuality + (sperLight.ContactShadowsLength > ZeroTolerance ? 4 : 0);
    context->SetRenderTarget(shadowMask);
    if (light.IsPointLight)
    {
        context->SetState(_psShadowPoint.Get(permutationIndex));
        _sphereModel->LODs.Get()[0].Meshes.Get()[0].Render(context);
    }
    else if (light.IsSpotLight)
    {
        context->SetState(_psShadowSpot.Get(permutationIndex));
        _sphereModel->LODs.Get()[0].Meshes.Get()[0].Render(context);
    }
    else //if (light.IsDirectionalLight)
    {
        context->SetState(_psShadowDir.Get(permutationIndex));
        context->DrawFullscreenTriangle();
    }

    // Cleanup
    context->ResetRenderTarget();
    context->UnBindSR(5);
    context->UnBindSR(6);
}

void ShadowsPass::GetShadowAtlas(const RenderBuffers* renderBuffers, GPUTexture*& shadowMapAtlas, GPUBufferView*& shadowsBuffer)
{
    const ShadowsCustomBuffer* shadowsPtr = renderBuffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"));
    if (shadowsPtr && shadowsPtr->ShadowMapAtlas && shadowsPtr->LastFrameUsed == Engine::FrameCount)
    {
        shadowMapAtlas = shadowsPtr->ShadowMapAtlas;
        shadowsBuffer = shadowsPtr->ShadowsBufferView;
    }
    else
    {
        shadowMapAtlas = nullptr;
        shadowsBuffer = nullptr;
    }
}
