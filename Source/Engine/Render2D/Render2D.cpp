// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Render2D.h"
#include "Font.h"
#include "FontManager.h"
#include "FontTextureAtlas.h"
#include "RotatedRectangle.h"
#include "SpriteAtlas.h"
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/Content.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Animations/AnimationUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Half.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Engine/EngineService.h"

#if USE_EDITOR
#define RENDER2D_CHECK_RENDERING_STATE \
    if (!Render2D::IsRendering()) \
    { \
        LOG(Error, "Calling Render2D is only valid during rendering."); \
        return; \
    }
#else
#define RENDER2D_CHECK_RENDERING_STATE
#endif

#if USE_EDITOR
#define RENDER2D_INITIAL_VB_CAPACITY (16 * 1024)
#else
#define RENDER2D_INITIAL_VB_CAPACITY (4 * 1024)
#endif
#define RENDER2D_INITIAL_IB_CAPACITY (1024)
#define RENDER2D_INITIAL_DRAW_CALL_CAPACITY (512)

#define RENDER2D_BLUR_MAX_SAMPLES 64

// The format for the blur effect temporary buffer
#define PS_Blur_Format PixelFormat::R8G8B8A8_UNorm

// True if enable downscaling when rendering blur
const bool DownsampleForBlur = false;

GPU_CB_STRUCT(Data {
    Matrix ViewProjection;
    });

GPU_CB_STRUCT(BlurData {
    Float2 InvBufferSize;
    uint32 SampleCount;
    float Dummy0;
    Float4 Bounds;
    Float4 WeightAndOffsets[RENDER2D_BLUR_MAX_SAMPLES / 2];
    });

enum class DrawCallType : byte
{
    FillRect,
    FillRectNoAlpha,
    FillRT,
    FillTexture,
    FillTexturePoint,
    DrawChar,
    DrawCharMaterial,
    Custom,
    Material,
    Blur,
    ClipScissors,
    LineAA,

    MAX
};

struct Render2DDrawCall
{
    DrawCallType Type;
    uint32 StartIB;
    uint32 CountIB;

    union
    {
        struct
        {
            GPUTextureView* Ptr;
        } AsRT;

        struct
        {
            GPUTexture* Ptr;
        } AsTexture;

        struct
        {
            GPUTexture* Tex;
            MaterialBase* Mat;
        } AsChar;

        struct
        {
            GPUTexture* Tex;
            GPUPipelineState* Pso;
        } AsCustom;

        struct
        {
            MaterialBase* Mat;
            float Width;
            float Height;
        } AsMaterial;

        struct
        {
            float Strength;
            float Width;
            float Height;
            float UpperLeftX;
            float UpperLeftY;
            float BottomRightX;
            float BottomRightY;
        } AsBlur;

        struct
        {
            float X;
            float Y;
            float Width;
            float Height;
        } AsClipScissors;
    };
};

struct Render2DVertex
{
    Float2 Position;
    Half2 TexCoord;
    Color Color;
    Float2 CustomData;
    RotatedRectangle ClipMask;
};

struct CachedPSO
{
    bool Inited = false;
    bool UseDepth;

    GPUPipelineState* PS_Image;

    GPUPipelineState* PS_ImagePoint;

    GPUPipelineState* PS_Color;
    GPUPipelineState* PS_Color_NoAlpha;

    GPUPipelineState* PS_Font;

    GPUPipelineState* PS_BlurH;
    GPUPipelineState* PS_BlurV;
    GPUPipelineState* PS_Downscale;

    GPUPipelineState* PS_LineAA;

    bool Init(GPUShader* shader, bool useDepth);
    void Dispose();
};

// Clip
struct ClipMask
{
    RotatedRectangle Mask;
    Rectangle Bounds;
};

Render2D::RenderingFeatures Render2D::Features = RenderingFeatures::VertexSnapping | RenderingFeatures::FallbackFonts;

namespace
{
    // Private Stuff
    GPUContext* Context = nullptr;
    GPUTextureView* Output = nullptr;
    GPUTextureView* DepthBuffer = nullptr;
    Viewport View;
    Matrix ViewProjection;

    // Drawing
    Array<Render2DDrawCall> DrawCalls;
    Array<FontLineCache> Lines;
    Array<Float2> Lines2;
    bool IsScissorsRectEmpty;
    bool IsScissorsRectEnabled;

    // Transform
    // Note: we use Matrix3x3 instead of Matrix because we use only 2D transformations on CPU side
    // Matrix layout:
    // [ m1, m2, 0 ]
    // [ m3, m4, 0 ]
    // [ t1, t2, 1 ]
    // where 'm' is 2D transformation (scale, shear and rotate), 't' is translation
    Array<Matrix3x3, InlinedAllocation<64>> TransformLayersStack;
    Matrix3x3 TransformCached;

    Array<ClipMask, InlinedAllocation<64>> ClipLayersStack;
    Array<Color, InlinedAllocation<64>> TintLayersStack;

    // Shader
    AssetReference<Shader> GUIShader;
    CachedPSO PsoDepth;
    CachedPSO PsoNoDepth;
    CachedPSO* CurrentPso = nullptr;
    DynamicVertexBuffer VB(RENDER2D_INITIAL_VB_CAPACITY, (uint32)sizeof(Render2DVertex), TEXT("Render2D.VB"));
    DynamicIndexBuffer IB(RENDER2D_INITIAL_IB_CAPACITY, sizeof(uint32), TEXT("Render2D.IB"));
    uint32 VBIndex = 0;
    uint32 IBIndex = 0;
}

#define RENDER2D_WRITE_IB_QUAD(indices) \
    indices[0] = VBIndex + 0; \
    indices[1] = VBIndex + 1; \
    indices[2] = VBIndex + 2; \
    indices[3] = VBIndex + 2; \
    indices[4] = VBIndex + 3; \
    indices[5] = VBIndex + 0; \
    IB.Write(indices, sizeof(indices))

FORCE_INLINE void ApplyTransform(const Float2& value, Float2& result)
{
    Matrix3x3::Transform2DPoint(value, TransformCached, result);
}

void ApplyTransform(const Rectangle& value, RotatedRectangle& result)
{
    const RotatedRectangle rotated(value);
    Matrix3x3::Transform2DPoint(rotated.TopLeft, TransformCached, result.TopLeft);
    Matrix3x3::Transform2DVector(rotated.ExtentX, TransformCached, result.ExtentX);
    Matrix3x3::Transform2DVector(rotated.ExtentY, TransformCached, result.ExtentY);
}

FORCE_INLINE Render2DVertex MakeVertex(const Float2& pos, const Float2& uv, const Color& color)
{
    Float2 point;
    ApplyTransform(pos, point);

    return
    {
        point,
        Half2(uv),
        color * TintLayersStack.Peek(),
        { 0.0f, (float)Render2D::Features },
        ClipLayersStack.Peek().Mask
    };
}

FORCE_INLINE Render2DVertex MakeVertex(const Float2& point, const Float2& uv, const Color& color, const RotatedRectangle& mask, const Float2& customData)
{
    return
    {
        point,
        Half2(uv),
        color * TintLayersStack.Peek(),
        customData,
        mask,
    };
}

FORCE_INLINE Render2DVertex MakeVertex(const Float2& point, const Float2& uv, const Color& color, const RotatedRectangle& mask, const Float2& customData, const Color& tint)
{
    return
    {
        point,
        Half2(uv),
        color * tint,
        customData,
        mask
    };
}

void WriteTri(const Float2& p0, const Float2& p1, const Float2& p2, const Float2& uv0, const Float2& uv1, const Float2& uv2, const Color& color0, const Color& color1, const Color& color2)
{
    Render2DVertex tris[3];
    tris[0] = MakeVertex(p0, uv0, color0);
    tris[1] = MakeVertex(p1, uv1, color1);
    tris[2] = MakeVertex(p2, uv2, color2);
    VB.Write(tris, sizeof(tris));

    uint32 indices[3];
    indices[0] = VBIndex + 0;
    indices[1] = VBIndex + 1;
    indices[2] = VBIndex + 2;
    IB.Write(indices, sizeof(indices));

    VBIndex += 3;
    IBIndex += 3;
}

FORCE_INLINE void WriteTri(const Float2& p0, const Float2& p1, const Float2& p2, const Color& color0, const Color& color1, const Color& color2)
{
    WriteTri(p0, p1, p2, Float2::Zero, Float2::Zero, Float2::Zero, color0, color1, color2);
}

FORCE_INLINE void WriteTri(const Float2& p0, const Float2& p1, const Float2& p2, const Float2& uv0, const Float2& uv1, const Float2& uv2)
{
    WriteTri(p0, p1, p2, uv0, uv1, uv2, Color::Black, Color::Black, Color::Black);
}

void WriteRect(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4)
{
    const Float2 uvUpperLeft = Float2::Zero;
    const Float2 uvBottomRight = Float2::One;

    Render2DVertex quad[4];
    quad[0] = MakeVertex(rect.GetBottomRight(), uvBottomRight, color3);
    quad[1] = MakeVertex(rect.GetBottomLeft(), Float2(uvUpperLeft.X, uvBottomRight.Y), color4);
    quad[2] = MakeVertex(rect.GetUpperLeft(), uvUpperLeft, color1);
    quad[3] = MakeVertex(rect.GetUpperRight(), Float2(uvBottomRight.X, uvUpperLeft.Y), color2);
    VB.Write(quad, sizeof(quad));

    uint32 indices[6];
    RENDER2D_WRITE_IB_QUAD(indices);

    VBIndex += 4;
    IBIndex += 6;
}

void WriteRect(const Rectangle& rect, const Color& color, const Float2& uvUpperLeft, const Float2& uvBottomRight)
{
    Render2DVertex quad[4];
    quad[0] = MakeVertex(rect.GetBottomRight(), uvBottomRight, color);
    quad[1] = MakeVertex(rect.GetBottomLeft(), Float2(uvUpperLeft.X, uvBottomRight.Y), color);
    quad[2] = MakeVertex(rect.GetUpperLeft(), uvUpperLeft, color);
    quad[3] = MakeVertex(rect.GetUpperRight(), Float2(uvBottomRight.X, uvUpperLeft.Y), color);
    VB.Write(quad, sizeof(quad));

    uint32 indices[6];
    RENDER2D_WRITE_IB_QUAD(indices);

    VBIndex += 4;
    IBIndex += 6;
}

FORCE_INLINE void WriteRect(const Rectangle& rect, const Color& color)
{
    WriteRect(rect, color, Float2::Zero, Float2::One);
}

void Write9SlicingRect(const Rectangle& rect, const Color& color, const Float4& border, const Float4& borderUVs)
{
    const Rectangle upperLeft(rect.Location.X, rect.Location.Y, border.X, border.Z);
    const Rectangle upperRight(rect.Location.X + rect.Size.X - border.Y, rect.Location.Y, border.Y, border.Z);
    const Rectangle bottomLeft(rect.Location.X, rect.Location.Y + rect.Size.Y - border.W, border.X, border.W);
    const Rectangle bottomRight(rect.Location.X + rect.Size.X - border.Y, rect.Location.Y + rect.Size.Y - border.W, border.Y, border.W);

    const Float2 upperLeftUV(borderUVs.X, borderUVs.Z);
    const Float2 upperRightUV(1.0f - borderUVs.Y, borderUVs.Z);
    const Float2 bottomLeftUV(borderUVs.X, 1.0f - borderUVs.W);
    const Float2 bottomRightUV(1.0f - borderUVs.Y, 1.0f - borderUVs.W);

    WriteRect(upperLeft, color, Float2::Zero, upperLeftUV); // Upper left corner
    WriteRect(upperRight, color, Float2(upperRightUV.X, 0), Float2(1, upperLeftUV.Y)); // Upper right corner
    WriteRect(bottomLeft, color, Float2(0, bottomLeftUV.Y), Float2(bottomLeftUV.X, 1)); // Bottom left corner
    WriteRect(bottomRight, color, bottomRightUV, Float2::One); // Bottom right corner

    WriteRect(Rectangle(upperLeft.GetUpperRight(), upperRight.GetBottomLeft() - upperLeft.GetUpperRight()), color, Float2(upperLeftUV.X, 0), upperRightUV); // Top side
    WriteRect(Rectangle(upperLeft.GetBottomLeft(), bottomLeft.GetUpperRight() - upperLeft.GetBottomLeft()), color, Float2(0, upperLeftUV.Y), bottomLeftUV); // Left side
    WriteRect(Rectangle(bottomLeft.GetUpperRight(), bottomRight.GetBottomLeft() - bottomLeft.GetUpperRight()), color, bottomLeftUV, Float2(bottomRightUV.X, 1)); // Bottom side
    WriteRect(Rectangle(upperRight.GetBottomLeft(), bottomRight.GetUpperRight() - upperRight.GetBottomLeft()), color, upperRightUV, Float2(1, bottomRightUV.Y)); // Right Side

    WriteRect(Rectangle(upperLeft.GetBottomRight(), bottomRight.GetUpperLeft() - upperLeft.GetBottomRight()), color, upperLeftUV, bottomRightUV); // Center
}

void Write9SlicingRect(const Rectangle& rect, const Color& color, const Float4& border, const Float4& borderUVs, const Float2& uvLocation, const Float2& uvSize)
{
    const Rectangle upperLeft(rect.Location.X, rect.Location.Y, border.X, border.Z);
    const Rectangle upperRight(rect.Location.X + rect.Size.X - border.Y, rect.Location.Y, border.Y, border.Z);
    const Rectangle bottomLeft(rect.Location.X, rect.Location.Y + rect.Size.Y - border.W, border.X, border.W);
    const Rectangle bottomRight(rect.Location.X + rect.Size.X - border.Y, rect.Location.Y + rect.Size.Y - border.W, border.Y, border.W);

    const Float2 upperLeftUV = Float2(borderUVs.X, borderUVs.Z) * uvSize + uvLocation;
    const Float2 upperRightUV = Float2(1.0f - borderUVs.Y, borderUVs.Z) * uvSize + uvLocation;
    const Float2 bottomLeftUV = Float2(borderUVs.X, 1.0f - borderUVs.W) * uvSize + uvLocation;
    const Float2 bottomRightUV = Float2(1.0f - borderUVs.Y, 1.0f - borderUVs.W) * uvSize + uvLocation;
    const Float2 uvEnd = uvLocation + uvSize;

    WriteRect(upperLeft, color, uvLocation, upperLeftUV);
    WriteRect(upperRight, color, Float2(upperRightUV.X, uvLocation.Y), Float2(uvEnd.X, upperLeftUV.Y));
    WriteRect(bottomLeft, color, Float2(uvLocation.X, bottomLeftUV.Y), Float2(bottomLeftUV.X, uvEnd.Y));
    WriteRect(bottomRight, color, bottomRightUV, uvEnd);

    WriteRect(Rectangle(upperLeft.GetUpperRight(), upperRight.GetBottomLeft() - upperLeft.GetUpperRight()), color, Float2(upperLeftUV.X, uvLocation.Y), upperRightUV);
    WriteRect(Rectangle(upperLeft.GetBottomLeft(), bottomLeft.GetUpperRight() - upperLeft.GetBottomLeft()), color, Float2(uvLocation.X, upperLeftUV.Y), bottomLeftUV);
    WriteRect(Rectangle(bottomLeft.GetUpperRight(), bottomRight.GetBottomLeft() - bottomLeft.GetUpperRight()), color, bottomLeftUV, Float2(bottomRightUV.X, uvEnd.Y));
    WriteRect(Rectangle(upperRight.GetBottomLeft(), bottomRight.GetUpperRight() - upperRight.GetBottomLeft()), color, upperRightUV, Float2(uvEnd.X, bottomRightUV.Y));

    WriteRect(Rectangle(upperLeft.GetBottomRight(), bottomRight.GetUpperLeft() - upperLeft.GetBottomRight()), color, upperRightUV, bottomRightUV);
}

typedef bool (*CanDrawCallCallback)(const Render2DDrawCall&, const Render2DDrawCall&);

bool CanDrawCallCallbackTrue(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return true;
}

bool CanDrawCallCallbackFalse(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return false;
}

bool CanDrawCallCallbackRT(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.AsRT.Ptr == d2.AsRT.Ptr;
}

bool CanDrawCallCallbackTexture(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.AsTexture.Ptr == d2.AsTexture.Ptr;
}

bool CanDrawCallCallbackChar(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.AsChar.Tex == d2.AsChar.Tex;
}

bool CanDrawCallCallbackCharMaterial(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.AsChar.Tex == d2.AsChar.Tex && d1.AsChar.Mat == d2.AsChar.Mat;
}

bool CanDrawCallCallbackCustom(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.AsCustom.Tex == d2.AsCustom.Tex && d1.AsCustom.Pso == d2.AsCustom.Pso;
}

bool CanDrawCallCallbackMaterial(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.AsMaterial.Mat == d2.AsMaterial.Mat;
}

// @formatter:off
CanDrawCallCallback CanDrawCallBatch[] =
{
    CanDrawCallCallbackTrue, // FillRect,
    CanDrawCallCallbackTrue,  // FillRectNoAlpha,
    CanDrawCallCallbackRT, // FillRT,
    CanDrawCallCallbackTexture, // FillTexture,
    CanDrawCallCallbackTexture, // FillTexturePoint,
    CanDrawCallCallbackChar, // DrawChar,
    CanDrawCallCallbackCharMaterial, // DrawCharMaterial,
    CanDrawCallCallbackFalse, // Custom,
    CanDrawCallCallbackMaterial, // Material,
    CanDrawCallCallbackFalse, // Blur,
    CanDrawCallCallbackFalse, // ClipScissors,
    CanDrawCallCallbackTrue, // LineAA,
};
static_assert(ARRAY_COUNT(CanDrawCallBatch) == (int32)DrawCallType::MAX, "Invalid draw calls batching descriptor.");
// @formatter:on

bool CanBatchDrawCalls(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
{
    return d1.Type == d2.Type && CanDrawCallBatch[(int32)d1.Type](d1, d2);
}

void DrawBatch(int32 startIndex, int32 count);

bool CachedPSO::Init(GPUShader* shader, bool useDepth)
{
    if (Inited)
    {
        Dispose();
    }

    UseDepth = useDepth;

    // Create pipeline states
    GPUPipelineState::Description desc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    desc.DepthEnable = useDepth;
    desc.DepthWriteEnable = false;
    desc.DepthClipEnable = false;
    desc.VS = shader->GetVS("VS");
    desc.PS = shader->GetPS("PS_Image");
    desc.CullMode = CullMode::TwoSided;
    desc.BlendMode = BlendingMode::AlphaBlend;
    PS_Image = GPUDevice::Instance->CreatePipelineState();
    if (PS_Image->Init(desc))
        return true;
    //
    desc.BlendMode = BlendingMode::AlphaBlend;
    desc.PS = shader->GetPS("PS_ImagePoint");
    PS_ImagePoint = GPUDevice::Instance->CreatePipelineState();
    if (PS_ImagePoint->Init(desc))
        return true;
    //
    desc.BlendMode = BlendingMode::AlphaBlend;
    desc.PS = shader->GetPS("PS_Color");
    PS_Color = GPUDevice::Instance->CreatePipelineState();
    if (PS_Color->Init(desc))
        return true;
    //
    desc.BlendMode = BlendingMode::Opaque;
    PS_Color_NoAlpha = GPUDevice::Instance->CreatePipelineState();
    if (PS_Color_NoAlpha->Init(desc))
        return true;
    //
    desc.BlendMode = BlendingMode::AlphaBlend;
    desc.PS = shader->GetPS("PS_Font");
    PS_Font = GPUDevice::Instance->CreatePipelineState();
    if (PS_Font->Init(desc))
        return true;
    //
    desc.PS = shader->GetPS("PS_LineAA");
    PS_LineAA = GPUDevice::Instance->CreatePipelineState();
    if (PS_LineAA->Init(desc))
        return true;
    //
    desc.VS = GPUPipelineState::Description::DefaultFullscreenTriangle.VS;
    desc.PS = shader->GetPS("PS_Blur");
    desc.BlendMode = BlendingMode::Opaque;
    PS_BlurH = GPUDevice::Instance->CreatePipelineState();
    if (PS_BlurH->Init(desc))
        return true;
    //
    desc.PS = shader->GetPS("PS_Blur", 1);
    PS_BlurV = GPUDevice::Instance->CreatePipelineState();
    if (PS_BlurV->Init(desc))
        return true;
    //
    desc.PS = shader->GetPS("PS_Downscale");
    PS_Downscale = GPUDevice::Instance->CreatePipelineState();
    if (PS_Downscale->Init(desc))
        return true;

    Inited = true;

    return false;
}

void CachedPSO::Dispose()
{
    if (!Inited)
        return;

    SAFE_DELETE_GPU_RESOURCE(PS_Image);
    SAFE_DELETE_GPU_RESOURCE(PS_ImagePoint);
    SAFE_DELETE_GPU_RESOURCE(PS_Color);
    SAFE_DELETE_GPU_RESOURCE(PS_Color_NoAlpha);
    SAFE_DELETE_GPU_RESOURCE(PS_Font);
    SAFE_DELETE_GPU_RESOURCE(PS_BlurH);
    SAFE_DELETE_GPU_RESOURCE(PS_BlurV);
    SAFE_DELETE_GPU_RESOURCE(PS_Downscale);
    SAFE_DELETE_GPU_RESOURCE(PS_LineAA);

    Inited = false;
}

class Render2DService : public EngineService
{
public:
    Render2DService()
        : EngineService(TEXT("Render2D"), 10)
    {
    }

    bool Init() override;
    void Dispose() override;
};

Render2DService Render2DServiceInstance;

bool Render2D::IsRendering()
{
    return Context != nullptr;
}

const Viewport& Render2D::GetViewport()
{
    return View;
}

#if COMPILE_WITH_DEV_ENV

void OnGUIShaderReloading(Asset* obj)
{
    PsoDepth.Dispose();
    PsoNoDepth.Dispose();
}

#endif

bool Render2DService::Init()
{
    // GUI Shader
    GUIShader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GUI"));
    if (GUIShader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    GUIShader.Get()->OnReloading.Bind<OnGUIShaderReloading>();
#endif

    DrawCalls.EnsureCapacity(RENDER2D_INITIAL_DRAW_CALL_CAPACITY);

    return false;
}

void Render2DService::Dispose()
{
    TintLayersStack.Resize(0);
    ClipLayersStack.Resize(0);
    DrawCalls.Resize(0);
    Lines.Resize(0);
    Lines2.Resize(0);

    GUIShader = nullptr;

    PsoDepth.Dispose();
    PsoNoDepth.Dispose();

    VB.Dispose();
    IB.Dispose();
}

void Render2D::BeginFrame()
{
    ASSERT(!IsRendering());
}

void Render2D::Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer)
{
    ASSERT(output != nullptr);
    Begin(context, output->View(), depthBuffer ? depthBuffer->View() : nullptr, Viewport(output->Size()));
}

void Render2D::Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer, const Matrix& viewProjection)
{
    ASSERT(output != nullptr);
    Begin(context, output->View(), depthBuffer ? depthBuffer->View() : nullptr, Viewport(output->Size()), viewProjection);
}

void Render2D::Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, const Viewport& viewport)
{
    Matrix view, projection, viewProjection;
    const float halfWidth = viewport.Width * 0.5f;
    const float halfHeight = viewport.Height * 0.5f;
    const float zNear = 0.0f;
    const float zFar = 1.0f;
    Matrix::OrthoOffCenter(-halfWidth, halfWidth, halfHeight, -halfHeight, zNear, zFar, projection);
    Matrix::Translation(-halfWidth, -halfHeight, 0, view);
    Matrix::Multiply(view, projection, viewProjection);

    Begin(context, output, depthBuffer, viewport, viewProjection);

    IsScissorsRectEnabled = true;
}

void Render2D::Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, const Viewport& viewport, const Matrix& viewProjection)
{
    ASSERT(Context == nullptr && Output == nullptr);
    ASSERT(context != nullptr && output != nullptr);

    // Setup
    Context = context;
    Output = output;
    DepthBuffer = depthBuffer;
    View = viewport;
    ViewProjection = viewProjection;
    DrawCalls.Clear();

    // Initialize default transform
    const Matrix3x3 defaultTransform = Matrix3x3::Identity;
    TransformLayersStack.Clear();
    TransformLayersStack.Push(defaultTransform);
    TransformCached = defaultTransform;

    // Initialize default clip mask
    const Rectangle defaultBounds(viewport.Location, viewport.Size);
    const RotatedRectangle defaultMask(defaultBounds);
    ClipLayersStack.Clear();
    ClipLayersStack.Add({ defaultMask, defaultBounds });

    // Initialize default tint stack
    TintLayersStack.Clear();
    TintLayersStack.Add({ 1, 1, 1, 1 });

    // Scissors can be enabled only for 2D orthographic projections
    IsScissorsRectEnabled = false;

    // Reset geometry buffer
    VB.Clear();
    IB.Clear();
    VBIndex = 0;
    IBIndex = 0;
}

void Render2D::End()
{
    RENDER2D_CHECK_RENDERING_STATE;
    ASSERT(Context != nullptr && Output != nullptr);
    ASSERT(GUIShader != nullptr);

    // Skip if has nothing to draw
    if (DrawCalls.IsEmpty())
    {
        // End
        Context = nullptr;
        Output = nullptr;
        return;
    }

    PROFILE_GPU_CPU_NAMED("Render2D");

    // Prepare shader
    GPUShader* shader;
    {
        if (!GUIShader->IsLoaded() && GUIShader->WaitForLoaded())
        {
            // End
            DrawCalls.Clear();
            Context = nullptr;
            Output = nullptr;
            return;
        }
        shader = GUIShader->GetShader();
    }

    // Flush geometry buffers
    VB.Flush(Context);
    IB.Flush(Context);

    // Set output
    Context->ResetSR();
    Context->SetRenderTarget(DepthBuffer, Output);
    Context->SetViewportAndScissors(View);
    Context->FlushState();

    // Prepare constant buffer
    GPUConstantBuffer* constantBuffer = shader->GetCB(0);
    Data data;
    Matrix::Transpose(ViewProjection, data.ViewProjection);
    Context->UpdateCB(constantBuffer, &data);
    Context->BindCB(0, constantBuffer);

    // Prepare PSO
    if (!PsoDepth.Inited)
    {
        PsoDepth.Init(GUIShader.Get()->GetShader(), true);
        PsoNoDepth.Init(GUIShader.Get()->GetShader(), false);
    }
    CurrentPso = DepthBuffer ? &PsoDepth : &PsoNoDepth;

    // Flush draw calls
    int32 batchStart = 0, batchSize = 0;
    IsScissorsRectEmpty = false;
    for (int32 i = 0; i < DrawCalls.Count(); i++)
    {
        // Peek draw call
        const auto& drawCall = DrawCalls[i];

        // Check if cannot add element to the batching
        if (batchSize != 0 && !CanBatchDrawCalls(DrawCalls[batchStart], drawCall))
        {
            // Flush batched elements
            DrawBatch(batchStart, batchSize);
            batchStart += batchSize;
            batchSize = 0;
        }

        // Add element to batching
        batchSize++;
    }

    // Flush end of batched elements
    if (batchSize != 0)
    {
        DrawBatch(batchStart, batchSize);
    }

    // End
    DrawCalls.Clear();
    Context = nullptr;
    Output = nullptr;
}

void Render2D::EndFrame()
{
    ASSERT(!IsRendering());

    // Synchronize the texture atlases data
    FontManager::Flush();
}

void Render2D::PushTransform(const Matrix3x3& transform)
{
    RENDER2D_CHECK_RENDERING_STATE;

    // Combine transformation
    Matrix3x3 finalTransform;
    Matrix3x3::Multiply(transform, TransformCached, finalTransform);

    // Push it
    TransformLayersStack.Push(finalTransform);
    TransformCached = TransformLayersStack.Peek();
}

void Render2D::PeekTransform(Matrix3x3& transform)
{
    transform = TransformCached;
}

void Render2D::PopTransform()
{
    RENDER2D_CHECK_RENDERING_STATE;

    ASSERT(TransformLayersStack.HasItems());
    TransformLayersStack.Pop();
    TransformCached = TransformLayersStack.Peek();
}

void OnClipScissors()
{
    if (!IsScissorsRectEnabled)
        return;

    const auto& mask = ClipLayersStack.Peek();

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::ClipScissors;
    drawCall.AsClipScissors.X = mask.Bounds.GetX();
    drawCall.AsClipScissors.Y = mask.Bounds.GetY();
    drawCall.AsClipScissors.Width = mask.Bounds.GetWidth();
    drawCall.AsClipScissors.Height = mask.Bounds.GetHeight();
}

void Render2D::PushClip(const Rectangle& clipRect)
{
    RENDER2D_CHECK_RENDERING_STATE;

    const auto& mask = ClipLayersStack.Peek();
    RotatedRectangle clipRectTransformed;
    ApplyTransform(clipRect, clipRectTransformed);
    const Rectangle bounds = Rectangle::Shared(clipRectTransformed.ToBoundingRect(), mask.Bounds);
    ClipLayersStack.Push({ RotatedRectangle::Shared(clipRectTransformed, mask.Bounds), bounds });

    OnClipScissors();
}

void Render2D::PeekClip(Rectangle& clipRect)
{
    clipRect = ClipLayersStack.Peek().Bounds;
}

void Render2D::PopClip()
{
    RENDER2D_CHECK_RENDERING_STATE;

    ClipLayersStack.Pop();

    OnClipScissors();
}

void Render2D::PushTint(const Color& tint, bool inherit)
{
    RENDER2D_CHECK_RENDERING_STATE;

    TintLayersStack.Push(inherit ? tint * TintLayersStack.Peek() : tint);
}

void Render2D::PeekTint(Color& tint)
{
    tint = TintLayersStack.Peek();
}

void Render2D::PopTint()
{
    RENDER2D_CHECK_RENDERING_STATE;

    TintLayersStack.Pop();
}

void CalculateKernelSize(float strength, int32& kernelSize, int32& downSample)
{
    kernelSize = Math::RoundToInt(strength * 3.0f);

    if (DownsampleForBlur && kernelSize > 9)
    {
        downSample = kernelSize >= 64 ? 4 : 2;
        kernelSize /= downSample;
    }

    if (kernelSize % 2 == 0)
    {
        kernelSize++;
    }

    kernelSize = Math::Clamp(kernelSize, 3, RENDER2D_BLUR_MAX_SAMPLES / 2);
}

static float GetWeight(float dist, float strength)
{
    float strength2 = strength * strength;
    return (1.0f / Math::Sqrt(2 * PI * strength2)) * Math::Exp(-(dist * dist) / (2 * strength2));
}

static Float2 GetWeightAndOffset(float dist, float sigma)
{
    float offset1 = dist;
    float weight1 = GetWeight(offset1, sigma);

    float offset2 = dist + 1;
    float weight2 = GetWeight(offset2, sigma);

    float totalWeight = weight1 + weight2;

    float offset = 0;
    if (totalWeight > 0)
    {
        offset = (weight1 * offset1 + weight2 * offset2) / totalWeight;
    }

    return Float2(totalWeight, offset);
}

static uint32 ComputeBlurWeights(int32 kernelSize, float sigma, Float4* outWeightsAndOffsets)
{
    const uint32 numSamples = Math::DivideAndRoundUp((uint32)kernelSize, 2u);
    outWeightsAndOffsets[0] = Float4(Float2(GetWeight(0, sigma), 0), GetWeightAndOffset(1, sigma));
    uint32 sampleIndex = 1;
    for (int32 x = 3; x < kernelSize; x += 4)
    {
        outWeightsAndOffsets[sampleIndex] = Float4(GetWeightAndOffset((float)x, sigma), GetWeightAndOffset((float)(x + 2), sigma));
        sampleIndex++;
    }
    return numSamples;
}

void DrawBatch(int32 startIndex, int32 count)
{
    const Render2DDrawCall& d = DrawCalls[startIndex];
    GPUBuffer* vb = VB.GetBuffer();
    GPUBuffer* ib = IB.GetBuffer();
    uint32 countIb = 0;
    for (int32 i = 0; i < count; i++)
        countIb += DrawCalls[startIndex + i].CountIB;

    if (d.Type == DrawCallType::ClipScissors)
    {
        Rectangle* scissorsRect = (Rectangle*)&d.AsClipScissors.X;
        Context->SetScissor(*scissorsRect);
        IsScissorsRectEmpty = scissorsRect->Size.IsAnyZero();
        return;
    }
    if (IsScissorsRectEmpty)
        return;

    switch (d.Type)
    {
    case DrawCallType::FillRect:
        Context->SetState(CurrentPso->PS_Color);
        break;
    case DrawCallType::FillRectNoAlpha:
        Context->SetState(CurrentPso->PS_Color_NoAlpha);
        break;
    case DrawCallType::FillRT:
        Context->BindSR(0, d.AsRT.Ptr);
        Context->SetState(CurrentPso->PS_Image);
        break;
    case DrawCallType::FillTexture:
        Context->BindSR(0, d.AsTexture.Ptr);
        Context->SetState(CurrentPso->PS_Image);
        break;
    case DrawCallType::FillTexturePoint:
        Context->BindSR(0, d.AsTexture.Ptr);
        Context->SetState(CurrentPso->PS_ImagePoint);
        break;
    case DrawCallType::DrawChar:
        Context->BindSR(0, d.AsChar.Tex);
        Context->SetState(CurrentPso->PS_Font);
        break;
    case DrawCallType::DrawCharMaterial:
    {
        // Apply and bind material
        auto material = d.AsChar.Mat;
        MaterialBase::BindParameters bindParams(Context, *(RenderContext*)nullptr);
        Render2D::CustomData customData;
        customData.ViewProjection = ViewProjection;
        customData.ViewSize = Float2::One;
        bindParams.CustomData = &customData;
        material->Bind(bindParams);

        // Bind font atlas as a material parameter
        static StringView FontParamName = TEXT("Font");
        auto param = material->Params.Get(FontParamName);
        if (param && param->GetParameterType() == MaterialParameterType::Texture)
        {
            Context->BindSR(param->GetRegister(), d.AsChar.Tex);
        }

        // Bind index and vertex buffers
        Context->BindIB(ib);
        Context->BindVB(ToSpan(&vb, 1));

        // Draw
        Context->DrawIndexed(countIb, 0, d.StartIB);

        // Restore pipeline (material apply overrides it)
        const auto cb = GUIShader->GetShader()->GetCB(0);
        Context->BindCB(0, cb);

        return;
    }
    case DrawCallType::Custom:
        Context->BindSR(0, d.AsCustom.Tex);
        Context->SetState(d.AsCustom.Pso);
        break;
    case DrawCallType::Material:
    {
        // Bind material
        auto material = (MaterialBase*)d.AsMaterial.Mat;
        MaterialBase::BindParameters bindParams(Context, *(RenderContext*)nullptr);
        Render2D::CustomData customData;
        customData.ViewProjection = ViewProjection;
        customData.ViewSize = Float2(d.AsMaterial.Width, d.AsMaterial.Height);
        bindParams.CustomData = &customData;
        material->Bind(bindParams);

        // Bind index and vertex buffers
        Context->BindIB(ib);
        Context->BindVB(ToSpan(&vb, 1));

        // Draw
        Context->DrawIndexed(countIb, 0, d.StartIB);

        // Restore pipeline (material apply overrides it)
        const auto cb = GUIShader->GetShader()->GetCB(0);
        Context->BindCB(0, cb);

        return;
    }
    case DrawCallType::Blur:
    {
        PROFILE_GPU("Blur");

        const Float4 bounds(d.AsBlur.UpperLeftX, d.AsBlur.UpperLeftY, d.AsBlur.BottomRightX, d.AsBlur.BottomRightY);
        float blurStrength = Math::Max(d.AsBlur.Strength, 1.0f);
        const auto& limits = GPUDevice::Instance->Limits;
        int32 renderTargetWidth = Math::Min(Math::RoundToInt(d.AsBlur.Width), limits.MaximumTexture2DSize);
        int32 renderTargetHeight = Math::Min(Math::RoundToInt(d.AsBlur.Height), limits.MaximumTexture2DSize);

        int32 kernelSize = 0, downSample = 0;
        CalculateKernelSize(blurStrength, kernelSize, downSample);
        if (downSample > 0)
        {
            renderTargetWidth = Math::DivideAndRoundUp(renderTargetWidth, downSample);
            renderTargetHeight = Math::DivideAndRoundUp(renderTargetHeight, downSample);
            blurStrength /= downSample;
        }

        // Skip if no chance to render anything
        renderTargetWidth = Math::AlignDown(renderTargetWidth, 4);
        renderTargetHeight = Math::AlignDown(renderTargetHeight, 4);
        if (renderTargetWidth <= 0 || renderTargetHeight <= 0)
            return;

        // Get temporary textures
        auto desc = GPUTextureDescription::New2D(renderTargetWidth, renderTargetHeight, PS_Blur_Format);
        auto blurA = RenderTargetPool::Get(desc);
        auto blurB = RenderTargetPool::Get(desc);
        RENDER_TARGET_POOL_SET_NAME(blurA, "Render2D.BlurA");
        RENDER_TARGET_POOL_SET_NAME(blurB, "Render2D.BlurB");

        // Prepare blur data
        BlurData data;
        data.Bounds.X = bounds.X;
        data.Bounds.Y = bounds.Y;
        data.Bounds.Z = bounds.Z - bounds.X;
        data.Bounds.W = bounds.W - bounds.Y;
        data.InvBufferSize.X = 1.0f / (float)renderTargetWidth;
        data.InvBufferSize.Y = 1.0f / (float)renderTargetHeight;
        data.SampleCount = ComputeBlurWeights(kernelSize, blurStrength, data.WeightAndOffsets);
        const auto cb = GUIShader->GetShader()->GetCB(1);
        Context->UpdateCB(cb, &data);
        Context->BindCB(1, cb);

        // Downscale (or not) and extract the background image for the blurring
        Context->ResetRenderTarget();
        Context->SetRenderTarget(blurA->View());
        Context->SetViewportAndScissors((float)renderTargetWidth, (float)renderTargetHeight);
        Context->BindSR(0, Output);
        Context->SetState(CurrentPso->PS_Downscale);
        Context->DrawFullscreenTriangle();

        // Render the blur (1st pass)
        Context->ResetRenderTarget();
        Context->SetRenderTarget(blurB->View());
        Context->BindSR(0, blurA->View());
        Context->SetState(CurrentPso->PS_BlurH);
        Context->DrawFullscreenTriangle();

        // Render the blur (2nd pass)
        Context->ResetRenderTarget();
        Context->SetRenderTarget(blurA->View());
        Context->BindSR(0, blurB->View());
        Context->SetState(CurrentPso->PS_BlurV);
        Context->DrawFullscreenTriangle();

        // Restore output
        Context->ResetRenderTarget();
        Context->SetRenderTarget(DepthBuffer, Output);
        Context->SetViewportAndScissors(View);
        Context->UnBindCB(1);

        // Link for drawing final blur as a texture
        Context->BindSR(0, blurA->View());
        Context->SetState(CurrentPso->PS_Image);

        // Cleanup
        RenderTargetPool::Release(blurA);
        RenderTargetPool::Release(blurB);

        break;
    }
    case DrawCallType::ClipScissors:
        Context->SetScissor(*(Rectangle*)&d.AsClipScissors.X);
        return;
    case DrawCallType::LineAA:
        Context->SetState(CurrentPso->PS_LineAA);
        break;
#if !BUILD_RELEASE
    default:
        CRASH;
#endif
    }

    // Draw
    Context->BindVB(ToSpan(&vb, 1));
    Context->BindIB(ib);
    Context->DrawIndexed(countIb, 0, d.StartIB);
}

void Render2D::DrawText(Font* font, const StringView& text, const Color& color, const Float2& location, MaterialBase* customMaterial)
{
    RENDER2D_CHECK_RENDERING_STATE;

    // Check if there is no need to do anything
    if (font == nullptr ||
        text.Length() < 0 ||
        (customMaterial && (!customMaterial->IsReady() || !customMaterial->IsGUI())))
        return;

    // Temporary data
    uint32 fontAtlasIndex = 0;
    FontTextureAtlas* fontAtlas = nullptr;
    Float2 invAtlasSize = Float2::One;
    FontCharacterEntry previous;
    int32 kerning;
    float scale = 1.0f / FontManager::FontScale;
    const bool enableFallbackFonts = EnumHasAllFlags(Features, RenderingFeatures::FallbackFonts);

    // Render all characters
    FontCharacterEntry entry;
    Render2DDrawCall drawCall;
    if (customMaterial)
    {
        drawCall.Type = DrawCallType::DrawCharMaterial;
        drawCall.AsChar.Mat = customMaterial;
    }
    else
    {
        drawCall.Type = DrawCallType::DrawChar;
        drawCall.AsChar.Mat = nullptr;
    }
    Float2 pointer = location;
    for (int32 currentIndex = 0; currentIndex < text.Length(); currentIndex++)
    {
        // Cache current character
        const Char currentChar = text[currentIndex];

        // Check if it isn't a newline character
        if (currentChar != '\n')
        {
            // Get character entry
            font->GetCharacter(currentChar, entry, enableFallbackFonts);

            // Check if need to select/change font atlas (since characters even in the same font may be located in different atlases)
            if (fontAtlas == nullptr || entry.TextureIndex != fontAtlasIndex)
            {
                // Get texture atlas that contains current character
                fontAtlasIndex = entry.TextureIndex;
                fontAtlas = FontManager::GetAtlas(fontAtlasIndex);
                if (fontAtlas)
                {
                    fontAtlas->EnsureTextureCreated();
                    drawCall.AsChar.Tex = fontAtlas->GetTexture();
                    invAtlasSize = 1.0f / fontAtlas->GetSize();
                }
                else
                {
                    drawCall.AsChar.Tex = nullptr;
                    invAtlasSize = 1.0f;
                }
            }

            // Check if character is a whitespace
            const bool isWhitespace = StringUtils::IsWhitespace(currentChar);

            // Get kerning
            if (!isWhitespace && previous.IsValid)
            {
                kerning = entry.Font->GetKerning(previous.Character, entry.Character);
            }
            else
            {
                kerning = 0;
            }
            pointer.X += kerning * scale;
            previous = entry;

            // Omit whitespace characters
            if (!isWhitespace)
            {
                // Calculate character size and atlas coordinates
                const float x = pointer.X + entry.OffsetX * scale;
                const float y = pointer.Y + (font->GetHeight() + font->GetDescender() - entry.OffsetY) * scale;

                Rectangle charRect(x, y, entry.UVSize.X * scale, entry.UVSize.Y * scale);

                Float2 upperLeftUV = entry.UV * invAtlasSize;
                Float2 rightBottomUV = (entry.UV + entry.UVSize) * invAtlasSize;

                // Add draw call
                drawCall.StartIB = IBIndex;
                drawCall.CountIB = 6;
                DrawCalls.Add(drawCall);
                WriteRect(charRect, color, upperLeftUV, rightBottomUV);
            }

            // Move
            pointer.X += entry.AdvanceX * scale;
        }
        else
        {
            // Move
            pointer.X = location.X;
            pointer.Y += font->GetHeight() * scale;
        }
    }
}

void Render2D::DrawText(Font* font, const StringView& text, const TextRange& textRange, const Color& color, const Float2& location, MaterialBase* customMaterial)
{
    DrawText(font, textRange.Substring(text), color, location, customMaterial);
}

void Render2D::DrawText(Font* font, const StringView& text, const Color& color, const TextLayoutOptions& layout, MaterialBase* customMaterial)
{
    RENDER2D_CHECK_RENDERING_STATE;

    // Check if there is no need to do anything
    if (font == nullptr ||
        text.IsEmpty() ||
        layout.Scale <= ZeroTolerance ||
        (customMaterial && (!customMaterial->IsReady() || !customMaterial->IsGUI())))
        return;

    // Temporary data
    uint32 fontAtlasIndex = 0;
    FontTextureAtlas* fontAtlas = nullptr;
    Float2 invAtlasSize = Float2::One;
    FontCharacterEntry previous;
    int32 kerning;
    float scale = layout.Scale / FontManager::FontScale;
    const bool enableFallbackFonts = EnumHasAllFlags(Features, RenderingFeatures::FallbackFonts);

    // Process text to get lines
    Lines.Clear();
    font->ProcessText(text, Lines, layout);

    // Render all lines
    FontCharacterEntry entry;
    Render2DDrawCall drawCall;
    if (customMaterial)
    {
        drawCall.Type = DrawCallType::DrawCharMaterial;
        drawCall.AsChar.Mat = customMaterial;
    }
    else
    {
        drawCall.Type = DrawCallType::DrawChar;
        drawCall.AsChar.Mat = nullptr;
    }
    for (int32 lineIndex = 0; lineIndex < Lines.Count(); lineIndex++)
    {
        const FontLineCache& line = Lines[lineIndex];
        Float2 pointer = line.Location;

        // Render all characters from the line
        for (int32 charIndex = line.FirstCharIndex; charIndex <= line.LastCharIndex; charIndex++)
        {
            // Cache current character
            const Char currentChar = text[charIndex];

            // Check if it isn't a newline character
            if (currentChar != '\n')
            {
                // Get character entry
                font->GetCharacter(currentChar, entry, enableFallbackFonts);

                // Check if need to select/change font atlas (since characters even in the same font may be located in different atlases)
                if (fontAtlas == nullptr || entry.TextureIndex != fontAtlasIndex)
                {
                    // Get texture atlas that contains current character
                    fontAtlasIndex = entry.TextureIndex;
                    fontAtlas = FontManager::GetAtlas(fontAtlasIndex);
                    if (fontAtlas)
                    {
                        fontAtlas->EnsureTextureCreated();
                        invAtlasSize = 1.0f / fontAtlas->GetSize();
                        drawCall.AsChar.Tex = fontAtlas->GetTexture();
                    }
                    else
                    {
                        invAtlasSize = 1.0f;
                        drawCall.AsChar.Tex = nullptr;
                    }
                }

                // Get kerning
                const bool isWhitespace = StringUtils::IsWhitespace(currentChar);
                if (!isWhitespace && previous.IsValid)
                {
                    kerning = entry.Font->GetKerning(previous.Character, entry.Character);
                }
                else
                {
                    kerning = 0;
                }
                pointer.X += (float)kerning * scale;
                previous = entry;

                // Omit whitespace characters
                if (!isWhitespace)
                {
                    // Calculate character size and atlas coordinates
                    const float x = pointer.X + entry.OffsetX * scale;
                    const float y = pointer.Y - entry.OffsetY * scale + Math::Ceil((font->GetHeight() + font->GetDescender()) * scale);

                    Rectangle charRect(x, y, entry.UVSize.X * scale, entry.UVSize.Y * scale);
                    charRect.Offset(layout.Bounds.Location);

                    Float2 upperLeftUV = entry.UV * invAtlasSize;
                    Float2 rightBottomUV = (entry.UV + entry.UVSize) * invAtlasSize;

                    // Add draw call
                    drawCall.StartIB = IBIndex;
                    drawCall.CountIB = 6;
                    DrawCalls.Add(drawCall);
                    WriteRect(charRect, color, upperLeftUV, rightBottomUV);
                }

                // Move
                pointer.X += entry.AdvanceX * scale;
            }
        }
    }
}

void Render2D::DrawText(Font* font, const StringView& text, const TextRange& textRange, const Color& color, const TextLayoutOptions& layout, MaterialBase* customMaterial)
{
    DrawText(font, textRange.Substring(text), color, layout, customMaterial);
}

FORCE_INLINE bool NeedAlphaWithTint(const Color& color)
{
    const float tint = TintLayersStack.Peek().A;
    return color.A * tint < 1.0f;
}

FORCE_INLINE bool NeedAlphaWithTint(const Color& color1, const Color& color2)
{
    const float tint = TintLayersStack.Peek().A;
    return color1.A * tint < 1.0f || color2.A * tint < 1.0f;
}

FORCE_INLINE bool NeedAlphaWithTint(const Color& color1, const Color& color2, const Color& color3)
{
    const float tint = TintLayersStack.Peek().A;
    return color1.A * tint < 1.0f || color2.A * tint < 1.0f || color3.A * tint < 1.0f;
}

FORCE_INLINE bool NeedAlphaWithTint(const Color& color1, const Color& color2, const Color& color3, const Color& color4)
{
    const float tint = TintLayersStack.Peek().A;
    return color1.A * tint < 1.0f || color2.A * tint < 1.0f || color3.A * tint < 1.0f || color4.A * tint < 1.0f;
}

void Render2D::FillRectangle(const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = NeedAlphaWithTint(color) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    WriteRect(rect, color);
}

void Render2D::FillRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = NeedAlphaWithTint(color1, color2, color3, color4) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    WriteRect(rect, color1, color2, color3, color4);
}

void Render2D::DrawRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4, float thickness)
{
    RENDER2D_CHECK_RENDERING_STATE;

    const auto& mask = ClipLayersStack.Peek().Mask;
    thickness *= (TransformCached.M11 + TransformCached.M22 + TransformCached.M33) * 0.3333333f;

    Float2 points[5];
    ApplyTransform(rect.GetUpperLeft(), points[0]);
    ApplyTransform(rect.GetUpperRight(), points[1]);
    ApplyTransform(rect.GetBottomRight(), points[2]);
    ApplyTransform(rect.GetBottomLeft(), points[3]);
    points[4] = points[0];

    Color colors[5];
    colors[0] = color1;
    colors[1] = color2;
    colors[2] = color3;
    colors[3] = color4;
    colors[4] = colors[0];

    Render2DVertex v[4];
    uint32 indices[6];
    Float2 p1t, p2t;
    Color c1t, c2t;

    p1t = points[0];
    c1t = colors[0];

#if RENDER2D_USE_LINE_AA
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::LineAA;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 4 * (6 + 3);

    // This must be the same as in HLSL code
    const float filterScale = 1.0f;
    const float thicknessHalf = (2.82842712f + thickness) * 0.5f + filterScale;

    for (int32 i = 1; i < 5; i++)
    {
        p2t = points[i];
        c2t = colors[i];

        Float2 line = p2t - p1t;
        Float2 up = thicknessHalf * Float2::Normalize(Float2(-line.Y, line.X));
        Float2 right = thicknessHalf * Float2::Normalize(line);

        // Line

        v[0] = MakeVertex(p2t + up, Float2::UnitX, c2t, mask, { thickness, (float)Features });
        v[1] = MakeVertex(p1t + up, Float2::UnitX, c1t, mask, { thickness, (float)Features });
        v[2] = MakeVertex(p1t - up, Float2::Zero, c1t, mask, { thickness, (float)Features });
        v[3] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { thickness, (float)Features });
        VB.Write(v, sizeof(Render2DVertex) * 4);

        indices[0] = VBIndex + 0;
        indices[1] = VBIndex + 1;
        indices[2] = VBIndex + 2;
        indices[3] = VBIndex + 2;
        indices[4] = VBIndex + 3;
        indices[5] = VBIndex + 0;
        IB.Write(indices, sizeof(uint32) * 6);

        VBIndex += 4;
        IBIndex += 6;

        // Corner cap

        const float tmp = thickness * 0.69f;
        v[0] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { tmp, (float)Features });
        v[1] = MakeVertex(p2t + right, Float2::Zero, c2t, mask, { tmp, (float)Features });
        v[2] = MakeVertex(p2t, Float2(0.5f, 0.0f), c2t, mask, { tmp, (float)Features });
        VB.Write(v, sizeof(Render2DVertex) * 4);

        indices[0] = VBIndex + 1;
        indices[1] = VBIndex + 2;
        indices[2] = VBIndex + 0;

        IB.Write(indices, sizeof(uint32) * 3);

        VBIndex += 4;
        IBIndex += 3;

        p1t = p2t;
        c1t = c2t;
    }
#else
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = NeedAlphaWithTint(color1, color2) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 4 * (6 + 3);

    const float thicknessHalf = thickness * 0.5f;

    for (int32 i = 1; i < 5; i++)
    {
        p2t = points[i];
        c2t = colors[i];

        Float2 line = p2t - p1t;
        Float2 up = thicknessHalf * Float2::Normalize(Float2(-line.Y, line.X));
        Float2 right = thicknessHalf * Float2::Normalize(line);

        // Line

        v[0] = MakeVertex(p2t + up, Float2::UnitX, c2t, mask, { 0.0f, (float)Features });
        v[1] = MakeVertex(p1t + up, Float2::UnitX, c1t, mask, { 0.0f, (float)Features });
        v[2] = MakeVertex(p1t - up, Float2::Zero, c1t, mask, { 0.0f, (float)Features });
        v[3] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { 0.0f, (float)Features });
        VB.Write(v, sizeof(Render2DVertex) * 4);

        indices[0] = VBIndex + 0;
        indices[1] = VBIndex + 1;
        indices[2] = VBIndex + 2;
        indices[3] = VBIndex + 2;
        indices[4] = VBIndex + 3;
        indices[5] = VBIndex + 0;
        IB.Write(indices, sizeof(uint32) * 6);

        VBIndex += 4;
        IBIndex += 6;

        // Corner cap

        v[0] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { 0.0f, (float)Features });
        v[1] = MakeVertex(p2t + right, Float2::Zero, c2t, mask, { 0.0f, (float)Features });
        v[2] = MakeVertex(p2t, Float2(0.5f, 0.0f), c2t, mask, { 0.0f, (float)Features });
        VB.Write(v, sizeof(Render2DVertex) * 4);

        indices[0] = VBIndex + 1;
        indices[1] = VBIndex + 2;
        indices[2] = VBIndex + 0;

        IB.Write(indices, sizeof(uint32) * 3);

        VBIndex += 4;
        IBIndex += 3;

        p1t = p2t;
        c1t = c2t;
    }
#endif
}

void Render2D::DrawTexture(GPUTextureView* rt, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillRT;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsRT.Ptr = rt;
    WriteRect(rect, color);
}

void Render2D::DrawTexture(GPUTexture* t, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall drawCall;
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsTexture.Ptr = t;
    DrawCalls.Add(drawCall);
    WriteRect(rect, color);
}

void Render2D::DrawTexture(TextureBase* t, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall drawCall;
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsTexture.Ptr = t ? t->GetTexture() : nullptr;
    DrawCalls.Add(drawCall);
    WriteRect(rect, color);
}

void Render2D::DrawSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
        return;

    Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
    WriteRect(rect, color, sprite->Area.GetUpperLeft(), sprite->Area.GetBottomRight());
}

void Render2D::DrawTexturePoint(GPUTexture* t, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexturePoint;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsTexture.Ptr = t;
    WriteRect(rect, color);
}

void Render2D::DrawSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
        return;

    Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexturePoint;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
    WriteRect(rect, color, sprite->Area.GetUpperLeft(), sprite->Area.GetBottomRight());
}

void Render2D::Draw9SlicingTexture(TextureBase* t, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall drawCall;
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6 * 9;
    drawCall.AsTexture.Ptr = t ? t->GetTexture() : nullptr;
    DrawCalls.Add(drawCall);
    Write9SlicingRect(rect, color, border, borderUVs);
}

void Render2D::Draw9SlicingTexturePoint(TextureBase* t, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall drawCall;
    drawCall.Type = DrawCallType::FillTexturePoint;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6 * 9;
    drawCall.AsTexture.Ptr = t ? t->GetTexture() : nullptr;
    DrawCalls.Add(drawCall);
    Write9SlicingRect(rect, color, border, borderUVs);
}

void Render2D::Draw9SlicingSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
        return;

    Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6 * 9;
    drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
    Write9SlicingRect(rect, color, border, borderUVs, sprite->Area.Location, sprite->Area.Size);
}

void Render2D::Draw9SlicingSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
        return;

    Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexturePoint;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6 * 9;
    drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
    Write9SlicingRect(rect, color, border, borderUVs, sprite->Area.Location, sprite->Area.Size);
}

void Render2D::DrawCustom(GPUTexture* t, const Rectangle& rect, GPUPipelineState* ps, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    if (ps == nullptr || !ps->IsValid())
        return;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::Custom;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsCustom.Tex = t;
    drawCall.AsCustom.Pso = ps;
    WriteRect(rect, color);
}

#if RENDER2D_USE_LINE_AA

void DrawLineCap(const Float2& capOrigin, const Float2& capDirection, const Float2& up, const Color& color, float thickness)
{
    const auto& mask = ClipLayersStack.Peek().Mask;

    Render2DVertex v[5];
    v[0] = MakeVertex(capOrigin, Float2(0.5f, 0.0f), color, mask, { thickness, (float)Render2D::Features });
    v[1] = MakeVertex(capOrigin + capDirection + up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
    v[2] = MakeVertex(capOrigin + capDirection - up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
    v[3] = MakeVertex(capOrigin + up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
    v[4] = MakeVertex(capOrigin - up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
    VB.Write(v, sizeof(v));

    uint32 indices[9];
    indices[0] = VBIndex + 0;
    indices[1] = VBIndex + 3;
    indices[2] = VBIndex + 1;
    indices[3] = VBIndex + 0;
    indices[4] = VBIndex + 1;
    indices[5] = VBIndex + 2;
    indices[6] = VBIndex + 0;
    indices[7] = VBIndex + 2;
    indices[8] = VBIndex + 4;
    IB.Write(indices, sizeof(indices));

    VBIndex += 5;
    IBIndex += 9;
}

#endif

void DrawLines(const Float2* points, int32 pointsCount, const Color& color1, const Color& color2, float thickness)
{
    ASSERT(points && pointsCount >= 2);
    const auto& mask = ClipLayersStack.Peek().Mask;

    thickness *= (TransformCached.M11 + TransformCached.M22 + TransformCached.M33) * 0.3333333f;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.StartIB = IBIndex;

    Render2DVertex v[4];
    uint32 indices[6];
    Float2 p1t, p2t;

#if RENDER2D_USE_LINE_AA
    // This must be the same as in HLSL code
    const float filterScale = 1.0f;
    const float thicknessHalf = (2.82842712f + thickness) * 0.5f + filterScale;

    drawCall.Type = DrawCallType::LineAA;
    drawCall.CountIB = 9 + 9;

    Float2 line;
    Float2 normal;
    Float2 up;
    ApplyTransform(points[0], p1t);

    // Starting cap
    {
        ApplyTransform(points[1], p2t);

        line = p2t - p1t;
        normal = Float2::Normalize(Float2(-line.Y, line.X));
        up = normal * thicknessHalf;
        const Float2 capDirection = thicknessHalf * Float2::Normalize(p1t - p2t);

        DrawLineCap(p1t, capDirection, up, color1, thickness);
    }

    // Lines
    for (int32 i = 1; i < pointsCount; i++)
    {
        ApplyTransform(points[i], p2t);

        line = p2t - p1t;
        normal = Float2::Normalize(Float2(-line.Y, line.X));
        up = normal * thicknessHalf;

        v[0] = MakeVertex(p2t + up, Float2::UnitX, color2, mask, { thickness, (float)Render2D::Features });
        v[1] = MakeVertex(p1t + up, Float2::UnitX, color1, mask, { thickness, (float)Render2D::Features });
        v[2] = MakeVertex(p1t - up, Float2::Zero, color1, mask, { thickness, (float)Render2D::Features });
        v[3] = MakeVertex(p2t - up, Float2::Zero, color2, mask, { thickness, (float)Render2D::Features });
        VB.Write(v, sizeof(Render2DVertex) * 4);

        indices[0] = VBIndex + 0;
        indices[1] = VBIndex + 1;
        indices[2] = VBIndex + 2;
        indices[3] = VBIndex + 2;
        indices[4] = VBIndex + 3;
        indices[5] = VBIndex + 0;
        IB.Write(indices, sizeof(uint32) * 6);

        VBIndex += 4;
        IBIndex += 6;
        drawCall.CountIB += 6;

        p1t = p2t;
    }

    // Ending cap
    {
        ApplyTransform(points[pointsCount - 2], p1t);
        //ApplyTransform(points[pointsCount - 1], p2t);

        const Float2 capDirection = thicknessHalf * Float2::Normalize(p2t - p1t);

        DrawLineCap(p2t, capDirection, up, color2, thickness);
    }
#else
    const float thicknessHalf = thickness * 0.5f;

    drawCall.Type = NeedAlphaWithTint(color1, color2) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.CountIB = 0;

    ApplyTransform(points[0], p1t);
    for (int32 i = 1; i < pointsCount; i++)
    {
        ApplyTransform(points[i], p2t);

        const Float2 line = p2t - p1t;
        const Float2 direction = thicknessHalf * Float2::Normalize(p2t - p1t);
        const Float2 normal = Float2::Normalize(Float2(-line.Y, line.X));

        v[0] = MakeVertex(p2t + thicknessHalf * normal + direction, Float2::Zero, color2, mask, { 0.0f, (float)Render2D::Features });
        v[1] = MakeVertex(p1t + thicknessHalf * normal - direction, Float2::Zero, color1, mask, { 0.0f, (float)Render2D::Features });
        v[2] = MakeVertex(p1t - thicknessHalf * normal - direction, Float2::Zero, color1, mask, { 0.0f, (float)Render2D::Features });
        v[3] = MakeVertex(p2t - thicknessHalf * normal + direction, Float2::Zero, color2, mask, { 0.0f, (float)Render2D::Features });
        VB.Write(v, sizeof(Render2DVertex) * 4);

        indices[0] = VBIndex + 0;
        indices[1] = VBIndex + 1;
        indices[2] = VBIndex + 2;
        indices[3] = VBIndex + 2;
        indices[4] = VBIndex + 3;
        indices[5] = VBIndex + 0;
        IB.Write(indices, sizeof(uint32) * 6);

        VBIndex += 4;
        IBIndex += 6;
        drawCall.CountIB += 6;

        p1t = p2t;
    }
#endif
}

void Render2D::DrawLine(const Float2& p1, const Float2& p2, const Color& color1, const Color& color2, float thickness)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Float2 points[2];
    points[0] = p1;
    points[1] = p2;

    DrawLines(points, 2, color1, color2, thickness);
}

void Render2D::DrawBezier(const Float2& p1, const Float2& p2, const Float2& p3, const Float2& p4, const Color& color, float thickness)
{
    RENDER2D_CHECK_RENDERING_STATE;

    // Find amount of segments to use
    const Float2 d1 = p2 - p1;
    const Float2 d2 = p3 - p2;
    const Float2 d3 = p4 - p3;
    const float len = d1.Length() + d2.Length() + d3.Length();
    const int32 segmentCount = Math::Clamp(Math::CeilToInt(len * 0.05f), 1, 100);
    const float segmentCountInv = 1.0f / (float)segmentCount;

    // Draw segmented curve
    Lines2.Clear();
    Lines2.Add(p1);
    for (int32 i = 1; i < segmentCount; i++)
    {
        const float t = (float)i * segmentCountInv;
        Float2 p;
        AnimationUtils::Bezier(p1, p2, p3, p4, t, p);
        Lines2.Add(p);
    }
    Lines2.Add(p4);
    DrawLines(Lines2.Get(), Lines2.Count(), color, color, thickness);
}

void Render2D::DrawSpline(const Float2& p1, const Float2& p2, const Float2& p3, const Float2& p4, const Color& color, float thickness)
{
    RENDER2D_CHECK_RENDERING_STATE;

    // Find amount of segments to use
    const Float2 d1 = p2 - p1;
    const Float2 d2 = p3 - p2;
    const Float2 d3 = p4 - p3;
    const float len = d1.Length() + d2.Length() + d3.Length();
    const int32 segmentCount = Math::Clamp(Math::CeilToInt(len * 0.05f), 1, 100);
    const float segmentCountInv = 1.0f / (float)segmentCount;

    // Draw segmented curve
    Lines2.Clear();
    Lines2.Add(p1);
    for (int32 i = 1; i < segmentCount; i++)
    {
        const float t = (float)i * segmentCountInv;
        Float2 p;
        p.X = Math::Lerp(p1.X, p4.X, t);
        AnimationUtils::Bezier(p1.Y, p2.Y, p3.Y, p4.Y, t, p.Y);
        Lines2.Add(p);
    }
    Lines2.Add(p4);
    DrawLines(Lines2.Get(), Lines2.Count(), color, color, thickness);
}

void Render2D::DrawMaterial(MaterialBase* material, const Rectangle& rect, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    if (material == nullptr || !material->IsReady() || !material->IsGUI())
        return;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::Material;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsMaterial.Mat = material;
    drawCall.AsMaterial.Width = rect.GetWidth();
    drawCall.AsMaterial.Height = rect.GetHeight();
    WriteRect(rect, color);
}

void Render2D::DrawBlur(const Rectangle& rect, float blurStrength)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Float2 p;
    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::Blur;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 6;
    drawCall.AsBlur.Strength = blurStrength;
    drawCall.AsBlur.Width = rect.GetWidth();
    drawCall.AsBlur.Height = rect.GetHeight();
    ApplyTransform(rect.GetUpperLeft(), p);
    drawCall.AsBlur.UpperLeftX = p.X;
    drawCall.AsBlur.UpperLeftY = p.Y;
    ApplyTransform(rect.GetBottomRight(), p);
    drawCall.AsBlur.BottomRightX = p.X;
    drawCall.AsBlur.BottomRightY = p.Y;
    WriteRect(rect, Color::White);
}

void Render2D::DrawTriangles(const Span<Float2>& vertices, const Color& color, float thickness)
{
    RENDER2D_CHECK_RENDERING_STATE;
    CHECK(vertices.Length() % 3 == 0);

    Float2 points[2];
    for (int32 i = 0; i < vertices.Length(); i += 3)
    {
#if 0
        // TODO: fix this
        DrawLines(&vertices.Get()[i], 3, color, color, thickness);
#else
        points[0] = vertices.Get()[i + 0];
        points[1] = vertices.Get()[i + 1];
        DrawLines(points, 2, color, color, thickness);
        points[0] = vertices.Get()[i + 2];
        DrawLines(points, 2, color, color, thickness);
        points[1] = vertices.Get()[i + 0];
        DrawLines(points, 2, color, color, thickness);
#endif
    }
}

void Render2D::DrawTriangles(const Span<Float2>& vertices, const Span<Color>& colors, float thickness)
{
    RENDER2D_CHECK_RENDERING_STATE;
    CHECK(vertices.Length() % 3 == 0);

    Float2 points[2];
    Color cols[2];
    for (int32 i = 0; i < vertices.Length(); i += 3)
    {
        points[0] = vertices.Get()[i + 0];
        points[1] = vertices.Get()[i + 1];
        cols[0] = colors.Get()[i + 0];
        cols[1] = colors.Get()[i + 1];
        DrawLines(points, 2, cols[0], cols[1], thickness);
        points[0] = vertices.Get()[i + 2];
        cols[0] = colors.Get()[i + 2];
        DrawLines(points, 2, cols[0], cols[1], thickness);
        points[1] = vertices.Get()[i + 0];
        cols[1] = colors.Get()[i + 0];
        DrawLines(points, 2, cols[0], cols[1], thickness);
    }
}

void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs)
{
    RENDER2D_CHECK_RENDERING_STATE;
    CHECK(vertices.Length() % 3 == 0);
    CHECK(vertices.Length() == uvs.Length());

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = vertices.Length();
    drawCall.AsTexture.Ptr = t;

    for (int32 i = 0; i < vertices.Length(); i += 3)
        WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], uvs[i], uvs[i + 1], uvs[i + 2]);
}

void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;
    CHECK(vertices.Length() % 3 == 0);
    CHECK(vertices.Length() == uvs.Length());

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = vertices.Length();
    drawCall.AsTexture.Ptr = t;

    for (int32 i = 0; i < vertices.Length(); i += 3)
        WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], uvs[i], uvs[i + 1], uvs[i + 2], color, color, color);
}

void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs, const Span<Color>& colors)
{
    RENDER2D_CHECK_RENDERING_STATE;
    CHECK(vertices.Length() % 3 == 0);
    CHECK(vertices.Length() == uvs.Length());
    CHECK(vertices.Length() == colors.Length());

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = vertices.Length();
    drawCall.AsTexture.Ptr = t;

    for (int32 i = 0; i < vertices.Length(); i += 3)
        WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], uvs[i], uvs[i + 1], uvs[i + 2], colors[i], colors[i + 1], colors[i + 2]);
}

void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<uint16>& indices, const Span<Float2>& vertices, const Span<Float2>& uvs, const Span<Color>& colors)
{
    RENDER2D_CHECK_RENDERING_STATE;
    CHECK(vertices.Length() == uvs.Length());
    CHECK(vertices.Length() == colors.Length());

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = DrawCallType::FillTexture;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = indices.Length();
    drawCall.AsTexture.Ptr = t;

    for (int32 i = 0; i < indices.Length();)
    {
        const uint16 i0 = indices.Get()[i++];
        const uint16 i1 = indices.Get()[i++];
        const uint16 i2 = indices.Get()[i++];
        WriteTri(vertices[i0], vertices[i1], vertices[i2], uvs[i0], uvs[i1], uvs[i2], colors[i0], colors[i1], colors[i2]);
    }
}

void Render2D::FillTriangles(const Span<Float2>& vertices, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = NeedAlphaWithTint(color) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = vertices.Length();

    for (int32 i = 0; i < vertices.Length(); i += 3)
        WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], color, color, color);
}

void Render2D::FillTriangles(const Span<Float2>& vertices, const Span<Color>& colors, bool useAlpha)
{
    CHECK(vertices.Length() == colors.Length());
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = useAlpha ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = vertices.Length();

    for (int32 i = 0; i < vertices.Length(); i += 3)
        WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], colors[i], colors[i + 1], colors[i + 2]);
}

void Render2D::FillTriangle(const Float2& p0, const Float2& p1, const Float2& p2, const Color& color)
{
    RENDER2D_CHECK_RENDERING_STATE;

    Render2DDrawCall& drawCall = DrawCalls.AddOne();
    drawCall.Type = NeedAlphaWithTint(color) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
    drawCall.StartIB = IBIndex;
    drawCall.CountIB = 3;
    WriteTri(p0, p1, p2, color, color, color);
}
