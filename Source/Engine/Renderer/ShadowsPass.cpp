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
#include "Engine/Engine/Units.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Utilities/RectPack.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif

#define SHADOWS_POSITION_ERROR METERS_TO_UNITS(0.1f)
#define SHADOWS_ROTATION_ERROR 0.9999f
#define SHADOWS_MAX_TILES 6
#define SHADOWS_MIN_RESOLUTION 32
#define SHADOWS_MAX_STATIC_ATLAS_CAPACITY_TO_DEFRAG 0.7f
#define SHADOWS_BASE_LIGHT_RESOLUTION(atlasResolution) atlasResolution / MAX_CSM_CASCADES // Allow to store 4 CSM cascades in a single row in all cases
#define NormalOffsetScaleTweak METERS_TO_UNITS(1)
#define LocalLightNearPlane METERS_TO_UNITS(0.1f)

GPU_CB_STRUCT(Data {
    ShaderGBufferData GBuffer;
    ShaderLightData Light;
    Matrix WVP;
    Matrix ViewProjectionMatrix;
    float Dummy0;
    float TemporalTime;
    float ContactShadowsDistance;
    float ContactShadowsLength;
    });

struct ShadowsAtlasRectTile : RectPackNode<uint16>
{
    bool IsStatic;

    ShadowsAtlasRectTile(Size x, Size y, Size width, Size height)
        : RectPackNode(x, y, width, height)
    {
    }

    void OnInsert(class ShadowsCustomBuffer* buffer, bool isStatic);
    void OnFree(ShadowsCustomBuffer* buffer);
};

uint16 QuantizeResolution(float input)
{
    uint16 output = Math::FloorToInt(input);
    uint16 alignment = 32;
    if (output >= 512)
        alignment = 128;
    else if (output >= 256)
        alignment = 64;
    output = Math::AlignDown<uint16>(output, alignment);
    return output;
}

// State for shadow projection
struct ShadowAtlasLightTile
{
    ShadowsAtlasRectTile* RectTile;
    ShadowsAtlasRectTile* StaticRectTile;
    Matrix WorldToShadow;
    float FramesToUpdate; // Amount of frames (with fraction) until the next shadow update can happen
    bool SkipUpdate;
    bool HasStaticGeometry;
    Viewport CachedViewport; // The viewport used the last time to render shadow to the atlas

    void FreeDynamic(ShadowsCustomBuffer* buffer);
    void FreeStatic(ShadowsCustomBuffer* buffer);

    void Free(ShadowsCustomBuffer* buffer)
    {
        FreeDynamic(buffer);
        FreeStatic(buffer);
    }

    void ClearDynamic()
    {
        RectTile = nullptr;
        FramesToUpdate = 0;
        SkipUpdate = false;
    }

    void ClearStatic()
    {
        StaticRectTile = nullptr;
        FramesToUpdate = 0;
        SkipUpdate = false;
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
    bool StaticValid;
    bool DynamicValid;
    float ShadowsUpdateRate;
    float ShadowsUpdateRateAtDistance;
    uint32 ShadowFrame;
    float OuterConeAngle;
    Float3 Position;
    float Radius;
    Float3 Direction;
    float Distance;
    Float4 CascadeSplits;
    Float3 ViewDirection;
    int32 ShadowsResolution;

    void Set(const RenderView& view, const RenderLightData& light, const Float4& cascadeSplits = Float4::Zero)
    {
        StaticValid = true;
        DynamicValid = true;
        Distance = light.ShadowsDistance;
        ShadowsUpdateRate = light.ShadowsUpdateRate;
        ShadowsUpdateRateAtDistance = light.ShadowsUpdateRateAtDistance;
        Direction = light.Direction;
        ShadowFrame = light.ShadowFrame;
        ShadowsResolution = light.ShadowsResolution;
        if (light.IsDirectionalLight)
        {
            // Sun
            Position = view.Position;
            ViewDirection = view.Direction;
            CascadeSplits = cascadeSplits;
        }
        else
        {
            // Local light
            const auto& localLight = (const RenderLocalLightData&)light;
            Position = light.Position;
            Radius = localLight.Radius;
            if (light.IsSpotLight)
                OuterConeAngle = ((const RenderSpotLightData&)light).OuterConeAngle;
        }
    }
};

// State for light's shadows rendering
struct ShadowAtlasLight
{
    // Static shadow map is created in 2 passes:
    // - once to check if any static objects are in-use per tile (ShadowAtlasLightTile::HasStaticGeometry)
    // - then to render those objects into the shadow map.
    // When any static objects gets modified in the light range the second step is repeated.
    // When light is changed then both steps are repeated.
    enum StaticStates
    {
        // Not using static shadow map at all.
        Unused,
        // Static objects are rendered separately to dynamic objects to check if light projections need to allocate static shadow map.
        WaitForGeometryCheck,
        // Static objects will be rendered into static shadow map.
        UpdateStaticShadow,
        // Static objects are up-to-date and can be copied from static shadow map.
        CopyStaticShadow,
        // None of the tiles has static geometry nearby.
        NoStaticGeometry,
        // One of the tiles failed to insert into static atlas so fallback to default dynamic logic.
        FailedToInsertTiles,
    };

    uint64 LastFrameUsed;
    int32 ContextIndex;
    int32 ContextCount;
    uint16 Resolution;
    uint16 StaticResolution;
    uint8 TilesNeeded;
    uint8 TilesCount;
    bool HasStaticShadowContext;
    StaticStates StaticState;
    BoundingSphere Bounds;
    float Sharpness, Fade, NormalOffsetScale, Bias, FadeDistance, Distance, TileBorder;
    Float4 CascadeSplits;
    ShadowAtlasLightTile Tiles[SHADOWS_MAX_TILES];
    ShadowAtlasLightCache Cache;

    ShadowAtlasLight()
    {
        Platform::MemoryClear(this, sizeof(ShadowAtlasLight));
    }

    POD_COPYABLE(ShadowAtlasLight);

    bool HasStaticGeometry() const
    {
        for (auto& tile : Tiles)
        {
            if (tile.HasStaticGeometry)
                return true;
        }
        return false;
    }

    float CalculateUpdateRateInv(const RenderLightData& light, float distanceFromView, bool& freezeUpdate) const
    {
        if (!GPU_SPREAD_WORKLOAD)
        {
            freezeUpdate = false;
            return 1.0f;
        }
        const float shadowsUpdateRate = light.ShadowsUpdateRate;
        const float shadowsUpdateRateAtDistance = shadowsUpdateRate * light.ShadowsUpdateRateAtDistance;
        float updateRate = Math::Lerp(shadowsUpdateRate, shadowsUpdateRateAtDistance, Math::Saturate(distanceFromView / Distance));
        updateRate *= Graphics::ShadowUpdateRate;
        freezeUpdate = updateRate <= ZeroTolerance;
        if (freezeUpdate)
            return 0.0f;
        return 1.0f / updateRate;
    }

    void ValidateCache(const RenderView& view, const RenderLightData& light)
    {
        if (!Cache.StaticValid || !Cache.DynamicValid)
            return;
        if (!Math::NearEqual(Cache.Distance, light.ShadowsDistance) ||
            !Math::NearEqual(Cache.ShadowsUpdateRate, light.ShadowsUpdateRate) ||
            !Math::NearEqual(Cache.ShadowsUpdateRateAtDistance, light.ShadowsUpdateRateAtDistance) ||
            Cache.ShadowFrame != light.ShadowFrame ||
            Cache.ShadowsResolution != light.ShadowsResolution ||
            Float3::Dot(Cache.Direction, light.Direction) < SHADOWS_ROTATION_ERROR)
        {
            // Invalidate
            Cache.StaticValid = false;
        }
        if (light.IsDirectionalLight)
        {
            // Sun
            if (!Float3::NearEqual(Cache.Position, view.Position, SHADOWS_POSITION_ERROR) ||
                !Float4::NearEqual(Cache.CascadeSplits, CascadeSplits) ||
                Float3::Dot(Cache.ViewDirection, view.Direction) < SHADOWS_ROTATION_ERROR)
            {
                // Invalidate
                Cache.StaticValid = false;
            }
        }
        else
        {
            // Local light
            const auto& localLight = (const RenderLocalLightData&)light;
            if (!Float3::NearEqual(Cache.Position, light.Position, SHADOWS_POSITION_ERROR) ||
                !Math::NearEqual(Cache.Radius, localLight.Radius))
            {
                // Invalidate
                Cache.StaticValid = false;
            }
            if (light.IsSpotLight && !Math::NearEqual(Cache.OuterConeAngle, ((const RenderSpotLightData&)light).OuterConeAngle))
            {
                // Invalidate
                Cache.StaticValid = false;
            }
        }
        Cache.DynamicValid &= Cache.StaticValid;
        for (int32 i = 0; i < TilesCount && !Cache.DynamicValid; i++)
        {
            auto& tile = Tiles[i];
            if (tile.CachedViewport != Viewport(tile.RectTile->X, tile.RectTile->Y, tile.RectTile->Width, tile.RectTile->Height))
            {
                // Invalidate
                Cache.DynamicValid = false;
            }
        }
    }
};

class ShadowsCustomBuffer : public RenderBuffers::CustomBuffer, public ISceneRenderingListener
{
public:
    int32 MaxShadowsQuality = 0;
    int32 Resolution = 0;
    int32 AtlasPixelsUsed = 0;
    int32 StaticAtlasPixelsUsed = 0;
    bool EnableStaticShadows = true;
    mutable bool ClearShadowMapAtlas = true;
    mutable bool ClearStaticShadowMapAtlas = false;
    Vector3 ViewOrigin;
    GPUTexture* ShadowMapAtlas = nullptr;
    GPUTexture* StaticShadowMapAtlas = nullptr;
    DynamicTypedBuffer ShadowsBuffer;
    GPUBufferView* ShadowsBufferView = nullptr;
    RectPackAtlas<ShadowsAtlasRectTile> Atlas;
    RectPackAtlas<ShadowsAtlasRectTile> StaticAtlas;
    Dictionary<Guid, ShadowAtlasLight> Lights;

    ShadowsCustomBuffer()
        : ShadowsBuffer(1024, PixelFormat::R32G32B32A32_Float, false, TEXT("ShadowsBuffer"))
    {
        ShadowMapAtlas = GPUDevice::Instance->CreateTexture(TEXT("Shadow Map Atlas"));
    }

    void ClearDynamic()
    {
        ClearShadowMapAtlas = true;
        for (auto it = Lights.Begin(); it.IsNotEnd(); ++it)
        {
            auto& atlasLight = it->Value;
            atlasLight.Cache.DynamicValid = false;
            for (int32 i = 0; i < atlasLight.TilesCount; i++)
                atlasLight.Tiles[i].ClearDynamic();
        }
        Atlas.Clear();
        AtlasPixelsUsed = 0;
    }

    void ClearStatic()
    {
        ClearStaticShadowMapAtlas = true;
        for (auto it = Lights.Begin(); it.IsNotEnd(); ++it)
        {
            auto& atlasLight = it->Value;
            atlasLight.StaticState = ShadowAtlasLight::Unused;
            atlasLight.Cache.StaticValid = false;
            for (int32 i = 0; i < atlasLight.TilesCount; i++)
                atlasLight.Tiles[i].ClearDynamic();
        }
        StaticAtlas.Clear();
        StaticAtlasPixelsUsed = 0;
    }

    void Reset()
    {
        Lights.Clear();
        ClearDynamic();
        ClearStatic();
    }

    void InitStaticAtlas()
    {
        const int32 atlasResolution = Resolution * 2;
        if (StaticAtlas.Width == atlasResolution)
            return;
        StaticAtlas.Init(atlasResolution, atlasResolution);
        if (!StaticShadowMapAtlas)
            StaticShadowMapAtlas = GPUDevice::Instance->CreateTexture(TEXT("Static Shadow Map Atlas"));
        auto desc = ShadowMapAtlas->GetDescription();
        desc.Width = desc.Height = atlasResolution;
        if (StaticShadowMapAtlas->Init(desc))
        {
            LOG(Fatal, "Failed to setup shadow map of size {0}x{1} and format {2}", desc.Width, desc.Height, ScriptingEnum::ToString(desc.Format));
            return;
        }
        ClearStaticShadowMapAtlas = true;
    }

    void DirtyStaticBounds(const BoundingSphere& bounds)
    {
        // TODO: use octree to improve bounds-testing
        // TODO: build list of modified bounds and dirty them in batch on next frame start (ideally in async within shadows setup job)
        for (auto& e : Lights)
        {
            auto& atlasLight = e.Value;
            if ((atlasLight.StaticState == ShadowAtlasLight::CopyStaticShadow || atlasLight.StaticState == ShadowAtlasLight::NoStaticGeometry) 
                && atlasLight.Bounds.Intersects(bounds))
            {
                // Invalidate static shadow
                atlasLight.Cache.StaticValid = false;
            }
        }
    }

    ~ShadowsCustomBuffer()
    {
        Reset();
        SAFE_DELETE_GPU_RESOURCE(ShadowMapAtlas);
        SAFE_DELETE_GPU_RESOURCE(StaticShadowMapAtlas);
    }

    // [ISceneRenderingListener]
    void OnSceneRenderingAddActor(Actor* a) override
    {
        if (a->HasStaticFlag(StaticFlags::Shadow))
            DirtyStaticBounds(a->GetSphere());
    }

    void OnSceneRenderingUpdateActor(Actor* a, const BoundingSphere& prevBounds, UpdateFlags flags) override
    {
        // Dirty static objects to redraw when changed (eg. material modification)
        if (a->HasStaticFlag(StaticFlags::Shadow))
        {
            DirtyStaticBounds(prevBounds);
            DirtyStaticBounds(a->GetSphere());
        }
        else if (flags & StaticFlags)
        {
            DirtyStaticBounds(a->GetSphere());
        }
    }

    void OnSceneRenderingRemoveActor(Actor* a) override
    {
        if (a->HasStaticFlag(StaticFlags::Shadow))
            DirtyStaticBounds(a->GetSphere());
    }

    void OnSceneRenderingClear(SceneRendering* scene) override
    {
    }
};

void ShadowsAtlasRectTile::OnInsert(ShadowsCustomBuffer* buffer, bool isStatic)
{
    IsStatic = isStatic;
    const int32 pixels = (int32)Width * (int32)Height;
    if (isStatic)
        buffer->StaticAtlasPixelsUsed += pixels;
    else
        buffer->AtlasPixelsUsed += pixels;
}

void ShadowsAtlasRectTile::OnFree(ShadowsCustomBuffer* buffer)
{
    const int32 pixels = (int32)Width * (int32)Height;
    if (IsStatic)
        buffer->StaticAtlasPixelsUsed -= pixels;
    else
        buffer->AtlasPixelsUsed -= pixels;
}

void ShadowAtlasLightTile::FreeDynamic(ShadowsCustomBuffer* buffer)
{
    if (RectTile)
    {
        buffer->Atlas.Free(RectTile, buffer);
        RectTile = nullptr;
    }
}

void ShadowAtlasLightTile::FreeStatic(ShadowsCustomBuffer* buffer)
{
    if (StaticRectTile)
    {
        buffer->StaticAtlas.Free(StaticRectTile, buffer);
        StaticRectTile = nullptr;
    }
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
    _psShadowPointInside.CreatePipelineStates();
    _psShadowSpot.CreatePipelineStates();
    _psShadowSpotInside.CreatePipelineStates();

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
    if (!_psShadowDir.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RG;
        if (_psShadowDir.Create(psDesc, shader, "PS_DirLight"))
            return true;
    }
    if (!_psShadowPoint.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RG;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.DepthEnable = true;
        psDesc.CullMode = CullMode::Normal;
        if (_psShadowPoint.Create(psDesc, shader, "PS_PointLight"))
            return true;
        psDesc.DepthFunc = ComparisonFunc::Greater;
        psDesc.CullMode = CullMode::Inverted;
        if (_psShadowPointInside.Create(psDesc, shader, "PS_PointLight"))
            return true;
    }
    if (!_psShadowSpot.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RG;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.DepthEnable = true;
        psDesc.CullMode = CullMode::Normal;
        if (_psShadowSpot.Create(psDesc, shader, "PS_SpotLight"))
            return true;
        psDesc.DepthFunc = ComparisonFunc::Greater;
        psDesc.CullMode = CullMode::Inverted;
        if (_psShadowSpotInside.Create(psDesc, shader, "PS_SpotLight"))
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
    if (_psDepthCopy == nullptr)
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = GPUDevice::Instance->QuadShader->GetPS("PS_DepthCopy");
        psDesc.DepthEnable = true;
        psDesc.DepthWriteEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::None;
        _psDepthCopy = GPUDevice::Instance->CreatePipelineState();
        if (_psDepthCopy->Init(psDesc))
            return true;
    }

    return false;
}

void ShadowsPass::SetupRenderContext(RenderContext& renderContext, RenderContext& shadowContext, ShadowAtlasLight* atlasLight, RenderContext* dynamicContext)
{
    const auto& view = renderContext.View;

    // Use the current render view to sync model LODs with the shadow maps rendering stage
    shadowContext.LodProxyView = &renderContext.View;

    // Prepare properties
    auto& shadowView = shadowContext.View;
    if (dynamicContext)
    {
        // Duplicate dynamic view but with static only geometry
        shadowView = dynamicContext->View;
        shadowView.StaticFlagsMask = StaticFlags::Shadow;
        shadowView.StaticFlagsCompare = StaticFlags::Shadow;
    }
    else
    {
        shadowView.Flags = view.Flags;
        shadowView.StaticFlagsMask = view.StaticFlagsMask;
        shadowView.StaticFlagsCompare = view.StaticFlagsCompare;
        shadowView.RenderLayersMask = view.RenderLayersMask;
        shadowView.IsOfflinePass = view.IsOfflinePass;
        shadowView.ModelLODBias = view.ModelLODBias;
        shadowView.ModelLODDistanceFactor = view.ModelLODDistanceFactor;
        shadowView.Pass = DrawPass::Depth;
        shadowView.Origin = view.Origin;
        if (atlasLight && atlasLight->StaticState != ShadowAtlasLight::Unused && atlasLight->StaticState != ShadowAtlasLight::FailedToInsertTiles)
        {
            // Draw only dynamic geometry
            shadowView.StaticFlagsMask = StaticFlags::Shadow;
            shadowView.StaticFlagsCompare = StaticFlags::None;
        }
    }
    shadowContext.List = RenderList::GetFromPool();
    shadowContext.Buffers = renderContext.Buffers;
    shadowContext.Task = renderContext.Task;
    shadowContext.List->Clear();
}

void ShadowsPass::SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderLightData& light, ShadowAtlasLight& atlasLight)
{
    // Copy light properties
    atlasLight.Sharpness = light.ShadowsSharpness;
    atlasLight.Fade = light.ShadowsStrength;
    atlasLight.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / (float)atlasLight.Resolution);
    atlasLight.Bias = light.ShadowsDepthBias;
    atlasLight.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    atlasLight.Distance = Math::Min(renderContext.View.Far, light.ShadowsDistance);
    atlasLight.Bounds.Center = light.Position + renderContext.View.Origin; // Keep bounds in world-space to properly handle DirtyStaticBounds
    atlasLight.Bounds.Radius = 0.0f;
}

bool ShadowsPass::SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderLocalLightData& light, ShadowAtlasLight& atlasLight)
{
    SetupLight(shadows, renderContext, renderContextBatch, (RenderLightData&)light, atlasLight);
    atlasLight.Bounds.Radius = light.Radius;

    // Fade shadow on distance
    const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
    const float dstLightToView = Float3::Distance(light.Position, renderContext.View.Position) - light.Radius;
    const float fade = 1 - Math::Saturate((dstLightToView - atlasLight.Distance + fadeDistance) / fadeDistance);
    atlasLight.Fade *= fade;

    // Update cached state (invalidate it if the light changed)
    atlasLight.ValidateCache(renderContext.View, light);

    // Update static shadow logic
    atlasLight.HasStaticShadowContext = shadows.EnableStaticShadows && EnumHasAllFlags(light.StaticFlags, StaticFlags::Shadow);
    if (atlasLight.HasStaticShadowContext)
    {
        // Calculate static resolution for the light based on the world-bounds, not view-dependant
        shadows.InitStaticAtlas();
        const int32 baseLightResolution = SHADOWS_BASE_LIGHT_RESOLUTION(shadows.Resolution) / 2;
        int32 staticResolution = Math::RoundToInt(Math::Saturate(light.Radius / METERS_TO_UNITS(10)) * baseLightResolution);
        staticResolution = Math::Clamp<int32>(staticResolution, atlasLight.Resolution, atlasLight.Resolution * 2); // Limit static shadow to be max x2 the current dynamic shadow res
        if (!Math::IsPowerOfTwo(staticResolution))
            staticResolution = Math::RoundUpToPowerOf2(staticResolution); // Round up to power of two to reduce fragmentation of the static atlas and redraws
        if (staticResolution != atlasLight.StaticResolution)
        {
            atlasLight.StaticResolution = staticResolution;
            atlasLight.StaticState = ShadowAtlasLight::Unused;
            for (auto& tile : atlasLight.Tiles)
                tile.FreeStatic(&shadows);
        }
    }
    else
        atlasLight.StaticState = ShadowAtlasLight::Unused;
    switch (atlasLight.StaticState)
    {
    case ShadowAtlasLight::Unused:
        if (atlasLight.HasStaticShadowContext)
            atlasLight.StaticState = ShadowAtlasLight::WaitForGeometryCheck;
        break;
    case ShadowAtlasLight::WaitForGeometryCheck:
        if (atlasLight.HasStaticGeometry())
        {
            shadows.InitStaticAtlas();

            // Allocate static shadow map slot for all used tiles
            for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
            {
                auto& tile = atlasLight.Tiles[tileIndex];
                if (tile.StaticRectTile == nullptr)
                {
                    tile.StaticRectTile = shadows.StaticAtlas.Insert(atlasLight.StaticResolution, atlasLight.StaticResolution, &shadows, true);
                    if (!tile.StaticRectTile)
                    {
                        // Failed to insert tile to switch back to the default rendering
                        atlasLight.StaticState = ShadowAtlasLight::FailedToInsertTiles;
                        for (int32 i = 0; i < tileIndex; i++)
                            atlasLight.Tiles[i].FreeStatic(&shadows);
                        break;
                    }
                }
            }
            if (atlasLight.StaticState == ShadowAtlasLight::WaitForGeometryCheck)
            {
                // Now we know the tiles with static geometry and we can render those
                atlasLight.StaticState = ShadowAtlasLight::UpdateStaticShadow;
            }
        }
        else
        {
            // Not using static geometry for this light shadows
            atlasLight.StaticState = ShadowAtlasLight::NoStaticGeometry;
        }
        break;
    case ShadowAtlasLight::CopyStaticShadow:
        // Light was modified so update the static shadows
        if (!atlasLight.Cache.StaticValid && atlasLight.HasStaticShadowContext)
            atlasLight.StaticState = ShadowAtlasLight::UpdateStaticShadow;
        break;
    }
    switch (atlasLight.StaticState)
    {
    case ShadowAtlasLight::NoStaticGeometry:
        // Light was modified so attempt to find the static shadow again
        if (!atlasLight.Cache.StaticValid && atlasLight.HasStaticShadowContext)
        {
            atlasLight.StaticState = ShadowAtlasLight::WaitForGeometryCheck;
            break;
        }
    case ShadowAtlasLight::CopyStaticShadow:
    case ShadowAtlasLight::FailedToInsertTiles:
        // Skip collecting static draws
        atlasLight.HasStaticShadowContext = false;
        break;
    }
    if (atlasLight.HasStaticShadowContext)
    {
        // If rendering finds any static draws then it will be set to true
        for (auto& tile : atlasLight.Tiles)
            tile.HasStaticGeometry = false;
    }

    // Calculate update rate based on the distance to the view
    bool freezeUpdate;
    const float updateRateInv = atlasLight.CalculateUpdateRateInv(light, dstLightToView, freezeUpdate);
    float& framesToUpdate = atlasLight.Tiles[0].FramesToUpdate; // Use the first tile for all local light projections to be in sync
    if ((framesToUpdate > 0.0f || freezeUpdate) && atlasLight.Cache.DynamicValid && !atlasLight.HasStaticShadowContext)
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

void ShadowsPass::SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderDirectionalLightData& light, ShadowAtlasLight& atlasLight)
{
    SetupLight(shadows, renderContext, renderContextBatch, (RenderLightData&)light, atlasLight);

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
        if ((tile.FramesToUpdate > 0.0f || freezeUpdate) && atlasLight.Cache.DynamicValid)
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
    if (atlasLight.ContextCount == 0)
        return;
    renderContextBatch.Contexts.AddDefault(atlasLight.ContextCount);
    atlasLight.Cache.Set(renderContext.View, light, atlasLight.CascadeSplits);

    // Calculate view frustum corners (un-jittered) in view-space
    Float3 frustumCorners[8];
    {
        BoundingFrustum stableViewFrustum;
        Matrix m;
        Matrix::Multiply(renderContext.View.View, renderContext.View.NonJitteredProjection, m);
        stableViewFrustum.SetMatrix(m);
        stableViewFrustum.GetCorners(frustumCorners);
    }
    for (int32 i = 0; i < 8; i++)
        Float3::Transform(frustumCorners[i], renderContext.View.View, frustumCorners[i]);

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
        Float3 frustumCornersVs[8];
        for (int32 j = 0; j < 4; j++)
        {
            float overlapWithPrevSplit = 0.1f * (splitMinRatio - oldSplitMinRatio); // CSM blending overlap
            const auto frustumRangeVS = frustumCorners[j + 4] - frustumCorners[j];
            frustumCornersVs[j] = frustumCorners[j] + frustumRangeVS * (splitMinRatio - overlapWithPrevSplit);
            frustumCornersVs[j + 4] = frustumCorners[j] + frustumRangeVS * splitMaxRatio;
        }

        // Transform the frustum from camera view space to world-space
        Float3 frustumCornersWs[8];
        for (int32 i = 0; i < 8; i++)
            Float3::Transform(frustumCornersVs[i], renderContext.View.IV, frustumCornersWs[i]);

        // Calculate the centroid of the view frustum slice
        Float3 frustumCenter = Float3::Zero;
        for (int32 i = 0; i < 8; i++)
            frustumCenter += frustumCornersWs[i];
        frustumCenter *= 1.0f / 8.0f;

        // Calculate the radius of a bounding sphere surrounding the frustum corners
        float frustumRadius = 0.0f;
        for (int32 i = 0; i < 8; i++)
            frustumRadius = Math::Max(frustumRadius, (frustumCornersWs[i] - frustumCenter).LengthSquared());
        frustumRadius = Math::Ceil(Math::Sqrt(frustumRadius) * 16.0f) / 16.0f;

        // Snap cascade center to the texel size
        float texelsPerUnit = (float)atlasLight.Resolution / (frustumRadius * 2.0f);
        frustumCenter *= texelsPerUnit;
        frustumCenter = Float3::Floor(frustumCenter);
        frustumCenter /= texelsPerUnit;

        // Cascade bounds are built around the sphere at the frustum center to reduce shadow shimmering
        Float3 maxExtents = Float3(frustumRadius);
        Float3 minExtents = -maxExtents;
        Float3 cascadeExtents = maxExtents - minExtents;

        Matrix shadowView, shadowProjection, shadowVP, cullingVP;

        // Create view matrix
        Matrix::LookAt(frustumCenter + light.Direction * minExtents.Z, frustumCenter, Float3::Up, shadowView);

        // Create viewport for culling with extended near/far planes due to culling issues (aka pancaking)
        const float cullRangeExtent = 100000.0f;
        Matrix::OrthoOffCenter(minExtents.X, maxExtents.X, minExtents.Y, maxExtents.Y, -cullRangeExtent, cascadeExtents.Z + cullRangeExtent, shadowProjection);
        Matrix::Multiply(shadowView, shadowProjection, cullingVP);

        // Create projection matrix
        Matrix::OrthoOffCenter(minExtents.X, maxExtents.X, minExtents.Y, maxExtents.Y, 0.0f, cascadeExtents.Z, shadowProjection);
        Matrix::Multiply(shadowView, shadowProjection, shadowVP);

        // Round the projection matrix by projecting the world-space origin and calculating the fractional offset in texel space of the shadow map
        Float4 shadowOrigin = Float4(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin = Float4::Transform(shadowOrigin, shadowVP);
        shadowOrigin = shadowOrigin * (shadowMapsSize / 2.0f);
        Float4 roundedOrigin = Float4::Round(shadowOrigin);
        Float4 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset = roundOffset * (2.0f / shadowMapsSize);
        roundOffset.Z = 0.0f;
        roundOffset.W = 0.0f;
        shadowProjection.SetRow4(shadowProjection.GetRow4() + roundOffset);

        // Calculate view*projection matrix
        Matrix::Multiply(shadowView, shadowProjection, shadowVP);
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

void ShadowsPass::SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderPointLightData& light, ShadowAtlasLight& atlasLight)
{
    if (SetupLight(shadows, renderContext, renderContextBatch, (RenderLocalLightData&)light, atlasLight))
        return;

    // Prevent sampling shadow map at borders that includes nearby data due to filtering of virtual cubemap sides
    atlasLight.TileBorder = 1.0f * (shadows.MaxShadowsQuality + 1);
    const float borderScale = (float)atlasLight.Resolution / (atlasLight.Resolution + 2 * atlasLight.TileBorder);
    Matrix borderScaleMatrix;
    Matrix::Scaling(borderScale, borderScale, 1.0f, borderScaleMatrix);

    atlasLight.ContextIndex = renderContextBatch.Contexts.Count();
    atlasLight.ContextCount = atlasLight.HasStaticShadowContext ? 12 : 6;
    renderContextBatch.Contexts.AddDefault(atlasLight.ContextCount);

    // Render depth to all 6 faces of the cube map
    int32 contextIndex = 0;
    for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
        SetupRenderContext(renderContext, shadowContext, &atlasLight);
        shadowContext.View.SetUpCube(LocalLightNearPlane, light.Radius, light.Position);

        // Apply border to the projection matrix
        shadowContext.View.Projection = shadowContext.View.Projection * borderScaleMatrix;
        shadowContext.View.NonJitteredProjection = shadowContext.View.Projection;
        Matrix::Invert(shadowContext.View.Projection, shadowContext.View.IP);

        shadowContext.View.SetFace(faceIndex);
        const auto shadowMapsSize = (float)atlasLight.Resolution;
        shadowContext.View.PrepareCache(shadowContext, shadowMapsSize, shadowMapsSize, Float2::Zero, &renderContext.View);
        atlasLight.Tiles[faceIndex].SetWorldToShadow(shadowContext.View.ViewProjection());

        // Draw static geometry separately to be cached
        if (atlasLight.HasStaticShadowContext)
        {
            auto& shadowContextStatic = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
            SetupRenderContext(renderContext, shadowContextStatic, &atlasLight, &shadowContext);
        }
    }
}

void ShadowsPass::SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderSpotLightData& light, ShadowAtlasLight& atlasLight)
{
    if (SetupLight(shadows, renderContext, renderContextBatch, (RenderLocalLightData&)light, atlasLight))
        return;

    atlasLight.ContextIndex = renderContextBatch.Contexts.Count();
    atlasLight.ContextCount = atlasLight.HasStaticShadowContext ? 2 : 1;
    renderContextBatch.Contexts.AddDefault(atlasLight.ContextCount);

    // Render depth to a single projection
    auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex];
    SetupRenderContext(renderContext, shadowContext, &atlasLight);
    shadowContext.View.SetProjector(LocalLightNearPlane, light.Radius, light.Position, light.Direction, light.UpVector, light.OuterConeAngle * 2.0f);
    shadowContext.View.PrepareCache(shadowContext, atlasLight.Resolution, atlasLight.Resolution, Float2::Zero, &renderContext.View);
    atlasLight.Tiles[0].SetWorldToShadow(shadowContext.View.ViewProjection());

    // Draw static geometry separately to be cached
    if (atlasLight.HasStaticShadowContext)
    {
        auto& shadowContextStatic = renderContextBatch.Contexts[atlasLight.ContextIndex + 1];
        SetupRenderContext(renderContext, shadowContextStatic, &atlasLight, &shadowContext);
    }
}

void ShadowsPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psShadowDir.Delete();
    _psShadowPoint.Delete();
    _psShadowPointInside.Delete();
    _psShadowSpot.Delete();
    _psShadowSpotInside.Delete();
    _shader = nullptr;
    _sphereModel = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_psDepthClear);
    SAFE_DELETE_GPU_RESOURCE(_psDepthCopy);
}

void ShadowsPass::SetupShadows(RenderContext& renderContext, RenderContextBatch& renderContextBatch)
{
    PROFILE_CPU();

    // Early out and skip shadows setup if no lights is actively casting shadows
    // RenderBuffers will automatically free any old ShadowsCustomBuffer after a few frames if we don't update LastFrameUsed
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
    const auto currentFrame = Engine::FrameCount;
    if (_shadowMapFormat == PixelFormat::Unknown ||
        EnumHasNoneFlags(renderContext.View.Flags, ViewFlags::Shadows) || 
        checkIfSkipPass() || 
        shadowedLights.IsEmpty())
    {
        // Invalidate any existing custom buffer that could have been used by the same task (eg. when rendering 6 sides of env probe)
        if (auto* old = (ShadowsCustomBuffer*)renderContext.Buffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"), false))
        {
            if (old->LastFrameUsed == currentFrame)
                old->LastFrameUsed = 0;
        }
        return;
    }

    // Initialize shadow atlas
    auto& shadows = *renderContext.Buffers->GetCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"), false);
    if (shadows.LastFrameUsed == currentFrame)
        shadows.Reset();
    shadows.LastFrameUsed = currentFrame;
    shadows.MaxShadowsQuality = Math::Clamp(Math::Min<int32>((int32)Graphics::ShadowsQuality, (int32)renderContext.View.MaxShadowsQuality), 0, (int32)Quality::MAX - 1);
    shadows.EnableStaticShadows = !renderContext.View.IsOfflinePass && !renderContext.View.IsSingleFrame;
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
    if (shadows.Resolution != atlasResolution)
    {
        shadows.Reset();
        shadows.Atlas.Reset();
        shadows.StaticAtlas.Reset();
        auto desc = GPUTextureDescription::New2D(atlasResolution, atlasResolution, _shadowMapFormat, GPUTextureFlags::ShaderResource | GPUTextureFlags::DepthStencil);
        if (shadows.ShadowMapAtlas->Init(desc))
        {
            LOG(Fatal, "Failed to setup shadow map of size {0}x{1} and format {2}", desc.Width, desc.Height, ScriptingEnum::ToString(desc.Format));
            return;
        }
        shadows.ClearShadowMapAtlas = true;
        shadows.Resolution = atlasResolution;
        shadows.ViewOrigin = renderContext.View.Origin;
    }
    if (renderContext.View.Origin != shadows.ViewOrigin)
    {
        // Large Worlds chunk movement so invalidate cached shadows
        shadows.Reset();
        shadows.ViewOrigin = renderContext.View.Origin;
    }
    if (shadows.StaticAtlas.Width != 0 && (float)shadows.StaticAtlasPixelsUsed / (shadows.StaticAtlas.Width * shadows.StaticAtlas.Height) < SHADOWS_MAX_STATIC_ATLAS_CAPACITY_TO_DEFRAG)
    {
        // Defragment static shadow atlas if it failed to insert any light but it's still should have space
        bool anyStaticFailed = false;
        for (auto& e : shadows.Lights)
        {
            if (e.Value.StaticState == ShadowAtlasLight::FailedToInsertTiles)
            {
                anyStaticFailed = true;
                break;
            }
        }
        if (anyStaticFailed)
        {
            shadows.ClearStatic();
        }
    }
    if (!shadows.Atlas.IsInitialized())
        shadows.Atlas.Init(atlasResolution, atlasResolution);

    // Update/add lights
    const int32 baseLightResolution = SHADOWS_BASE_LIGHT_RESOLUTION(atlasResolution);
    for (const RenderLightData* light : shadowedLights)
    {
        auto& atlasLight = shadows.Lights[light->ID];

        // Calculate resolution for this light
        atlasLight.Resolution = light->ShadowsResolution;
        if (atlasLight.Resolution == 0)
        {
            // ScreenSize-based automatic shadowmap resolution
            atlasLight.Resolution = QuantizeResolution(baseLightResolution * light->ScreenSize);
        }

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
        atlasLight.Cache.DynamicValid = false;
        for (ShadowAtlasLightTile& tile : atlasLight.Tiles)
            tile.FreeDynamic(&shadows);
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
            auto rectTile = shadows.Atlas.Insert(atlasLight.Resolution, atlasLight.Resolution, &shadows, false);
            if (!rectTile)
            {
                // Free any previous tiles that were added
                for (int32 i = 0; i < tileIndex; i++)
                    atlasLight.Tiles[i].FreeDynamic(&shadows);
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
            shadows.ClearDynamic();
            goto RETRY_ATLAS_SETUP;
        }
    }

    // Setup shadows for all lights
    for (RenderLightData* light : shadowedLights)
    {
        auto& atlasLight = shadows.Lights[light->ID];

        // Reset frame-data
        atlasLight.ContextIndex = 0;
        atlasLight.ContextCount = 0;

        if (atlasLight.Tiles[0].RectTile && atlasLight.Tiles[0].RectTile->Width == atlasLight.Resolution)
        {
            // Invalidate cache when whole atlas will be cleared
            if (shadows.ClearShadowMapAtlas)
                atlasLight.Cache.DynamicValid = false;
            if (shadows.ClearStaticShadowMapAtlas)
                atlasLight.Cache.StaticValid = false;

            light->HasShadow = true;
            atlasLight.TilesCount = atlasLight.TilesNeeded;
            if (light->IsPointLight)
                SetupLight(shadows, renderContext, renderContextBatch, *(RenderPointLightData*)light, atlasLight);
            else if (light->IsSpotLight)
                SetupLight(shadows, renderContext, renderContextBatch, *(RenderSpotLightData*)light, atlasLight);
            else //if (light->IsDirectionalLight)
                SetupLight(shadows, renderContext, renderContextBatch, *(RenderDirectionalLightData*)light, atlasLight);
        }
    }
    if (shadows.StaticAtlas.IsInitialized())
    {
        // Register for active scenes changes to invalidate static shadows
        for (SceneRendering* scene : renderContext.List->Scenes)
            shadows.ListenSceneRendering(scene);
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
        const float tileBorder = atlasLight.TileBorder;
        for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
        {
            // Shadow projection info
            const ShadowAtlasLightTile& tile = atlasLight.Tiles[tileIndex];
            ASSERT(tile.RectTile);
            auto* packed = shadows.ShadowsBuffer.WriteReserve<Float4>(5);
            // UV to AtlasUV via a single MAD instruction
            packed[0] = Float4(tile.RectTile->Width - tileBorder * 2, tile.RectTile->Height - tileBorder * 2, tile.RectTile->X + tileBorder, tile.RectTile->Y + tileBorder) * atlasResolutionInv;
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
    const ShadowsCustomBuffer* shadowsPtr = renderContext.Buffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"), false);
    if (shadowsPtr == nullptr || shadowsPtr->Lights.IsEmpty() || shadowsPtr->LastFrameUsed != Engine::FrameCount)
        return;
    PROFILE_GPU_CPU("Shadow Maps");
    const ShadowsCustomBuffer& shadows = *shadowsPtr;
    GPUContext* context = GPUDevice::Instance->GetMainContext();
    context->ResetSR();
    GPUConstantBuffer* quadShaderCB = GPUDevice::Instance->QuadShader->GetCB(0);
    QuadShaderData quadShaderData;

    // Update static shadows
    if (shadows.StaticShadowMapAtlas)
    {
        PROFILE_GPU_CPU("Static");
        if (shadows.ClearStaticShadowMapAtlas)
            context->ClearDepth(shadows.StaticShadowMapAtlas->View());
        bool renderedAny = false;
        for (auto& e : shadows.Lights)
        {
            ShadowAtlasLight& atlasLight = e.Value;
            if (!atlasLight.HasStaticShadowContext || atlasLight.ContextCount == 0)
                continue;
            int32 contextIndex = 0;

            if (atlasLight.StaticState == ShadowAtlasLight::WaitForGeometryCheck)
            {
                // Check for any static geometry to use in static shadow map
                for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
                {
                    ShadowAtlasLightTile& tile = atlasLight.Tiles[tileIndex];
                    contextIndex++; // Skip dynamic context
                    auto& shadowContextStatic = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
                    if (!shadowContextStatic.List->DrawCallsLists[(int32)DrawCallsListType::Depth].IsEmpty() || !shadowContextStatic.List->ShadowDepthDrawCallsList.IsEmpty())
                    {
                        tile.HasStaticGeometry = true;
                    }
                }
            }

            if (atlasLight.StaticState != ShadowAtlasLight::UpdateStaticShadow)
                continue;

            contextIndex = 0;
            for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
            {
                ShadowAtlasLightTile& tile = atlasLight.Tiles[tileIndex];
                if (!tile.RectTile)
                    break;
                if (!tile.StaticRectTile)
                    continue;
                if (!renderedAny)
                {
                    renderedAny = true;
                    context->SetRenderTarget(shadows.StaticShadowMapAtlas->View(), (GPUTextureView*)nullptr);
                }

                // Set viewport for tile
                context->SetViewportAndScissors(Viewport(tile.StaticRectTile->X, tile.StaticRectTile->Y, tile.StaticRectTile->Width, tile.StaticRectTile->Height));
                if (!shadows.ClearStaticShadowMapAtlas)
                {
                    // Color.r is used by PS_DepthClear in Quad shader to clear depth
                    quadShaderData.Color = Float4::One;
                    context->UpdateCB(quadShaderCB, &quadShaderData);
                    context->BindCB(0, quadShaderCB);

                    // Clear tile depth
                    context->SetState(_psDepthClear);
                    context->DrawFullscreenTriangle();
                }

                // Draw objects depth
                contextIndex++; // Skip dynamic context
                auto& shadowContextStatic = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
                if (!shadowContextStatic.List->DrawCallsLists[(int32)DrawCallsListType::Depth].IsEmpty() || !shadowContextStatic.List->ShadowDepthDrawCallsList.IsEmpty())
                {
                    shadowContextStatic.List->ExecuteDrawCalls(shadowContextStatic, DrawCallsListType::Depth);
                    shadowContextStatic.List->ExecuteDrawCalls(shadowContextStatic, shadowContextStatic.List->ShadowDepthDrawCallsList, renderContext.List, nullptr);
                    tile.HasStaticGeometry = true;
                }
            }

            // Go into copying shadow for the next draw
            atlasLight.StaticState = ShadowAtlasLight::CopyStaticShadow;
        }
        shadows.ClearStaticShadowMapAtlas = false;
        if (renderedAny)
        {
            context->ResetSR();
            context->ResetRenderTarget();
        }
    }

    // Render depth to all shadow map tiles
    if (shadows.ClearShadowMapAtlas)
        context->ClearDepth(shadows.ShadowMapAtlas->View());
    context->SetRenderTarget(shadows.ShadowMapAtlas->View(), (GPUTextureView*)nullptr);
    for (auto& e : shadows.Lights)
    {
        ShadowAtlasLight& atlasLight = e.Value;
        if (atlasLight.ContextCount == 0)
            continue;
        int32 contextIndex = 0;
        for (int32 tileIndex = 0; tileIndex < atlasLight.TilesCount; tileIndex++)
        {
            ShadowAtlasLightTile& tile = atlasLight.Tiles[tileIndex];
            if (!tile.RectTile)
                break;
            if (tile.SkipUpdate)
                continue;

            // Set viewport for tile
            context->SetViewportAndScissors(tile.CachedViewport);
            if (tile.StaticRectTile && atlasLight.StaticState == ShadowAtlasLight::CopyStaticShadow)
            {
                // Color.xyzw is used by PS_DepthCopy in Quad shader to scale input texture UVs
                const float staticAtlasResolutionInv = 1.0f / shadows.StaticShadowMapAtlas->Width();
                quadShaderData.Color = Float4(tile.StaticRectTile->Width, tile.StaticRectTile->Height, tile.StaticRectTile->X, tile.StaticRectTile->Y) * staticAtlasResolutionInv;
                context->UpdateCB(quadShaderCB, &quadShaderData);
                context->BindCB(0, quadShaderCB);

                // Copy tile depth
                context->BindSR(0, shadows.StaticShadowMapAtlas->View());
                context->SetState(_psDepthCopy);
                context->DrawFullscreenTriangle();
            }
            else if (!shadows.ClearShadowMapAtlas)
            {
                // Color.r is used by PS_DepthClear in Quad shader to clear depth
                quadShaderData.Color = Float4::One;
                context->UpdateCB(quadShaderCB, &quadShaderData);
                context->BindCB(0, quadShaderCB);

                // Clear tile depth
                context->SetState(_psDepthClear);
                context->DrawFullscreenTriangle();
            }

            // Draw objects depth
            auto& shadowContext = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
            shadowContext.List->ExecuteDrawCalls(shadowContext, DrawCallsListType::Depth);
            shadowContext.List->ExecuteDrawCalls(shadowContext, shadowContext.List->ShadowDepthDrawCallsList, renderContext.List, nullptr);
            if (atlasLight.HasStaticShadowContext)
            {
                auto& shadowContextStatic = renderContextBatch.Contexts[atlasLight.ContextIndex + contextIndex++];
                if (!shadowContextStatic.List->DrawCallsLists[(int32)DrawCallsListType::Depth].IsEmpty() || !shadowContextStatic.List->ShadowDepthDrawCallsList.IsEmpty())
                {
                    if (atlasLight.StaticState != ShadowAtlasLight::CopyStaticShadow)
                    {
                        // Draw static objects directly to the shadow map
                        shadowContextStatic.List->ExecuteDrawCalls(shadowContextStatic, DrawCallsListType::Depth);
                        shadowContextStatic.List->ExecuteDrawCalls(shadowContextStatic, shadowContextStatic.List->ShadowDepthDrawCallsList, renderContext.List, nullptr);
                    }
                    tile.HasStaticGeometry = true;
                }
            }
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
    const ShadowsCustomBuffer& shadows = *renderContext.Buffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"), false);
    ASSERT(shadows.LastFrameUsed == Engine::FrameCount);
    auto& view = renderContext.View;
    auto shader = _shader->GetShader();
    const bool isLocalLight = light.IsPointLight || light.IsSpotLight;
    int32 shadowQuality = shadows.MaxShadowsQuality;
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
    bool isViewInside;
    if (isLocalLight)
    {
        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp;
        RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, light.Position, ((RenderLocalLightData&)light).Radius, world, isViewInside);
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
        context->SetState((isViewInside ? _psShadowPointInside : _psShadowPoint).Get(permutationIndex));
        _sphereModel->LODs.Get()[0].Meshes.Get()[0].Render(context);
    }
    else if (light.IsSpotLight)
    {
        context->SetState((isViewInside ? _psShadowSpotInside : _psShadowSpot).Get(permutationIndex));
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
    const ShadowsCustomBuffer* shadowsPtr = renderBuffers->FindCustomBuffer<ShadowsCustomBuffer>(TEXT("Shadows"), false);
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
