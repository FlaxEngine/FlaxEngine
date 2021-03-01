// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_DEBUG_DRAW

#include "DebugDraw.h"
#include "Engine/Engine/Time.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Animations/AnimationUtils.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Render2D/FontAsset.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

// Debug draw service configuration
#define DEBUG_DRAW_INITIAL_VB_CAPACITY (4 * 1024)
//
#define DEBUG_DRAW_SPHERE_RESOLUTION 64
#define DEBUG_DRAW_SPHERE_LINES_COUNT (DEBUG_DRAW_SPHERE_RESOLUTION + 1) * 3
#define DEBUG_DRAW_SPHERE_VERTICES DEBUG_DRAW_SPHERE_LINES_COUNT * 2
//
#define DEBUG_DRAW_CIRCLE_RESOLUTION 32
#define DEBUG_DRAW_CIRCLE_LINES_COUNT DEBUG_DRAW_CIRCLE_RESOLUTION
#define DEBUG_DRAW_CIRCLE_VERTICES DEBUG_DRAW_CIRCLE_LINES_COUNT * 2
//
#define DEBUG_DRAW_CYLINDER_RESOLUTION 12
#define DEBUG_DRAW_CYLINDER_VERTICES (DEBUG_DRAW_CYLINDER_RESOLUTION * 4)
//
#define DEBUG_DRAW_TRIANGLE_SPHERE_RESOLUTION 12

struct DebugLine
{
    Vector3 Start;
    Vector3 End;
    Color32 Color;
    float TimeLeft;
};

struct DebugTriangle
{
    Vector3 V0;
    Vector3 V1;
    Vector3 V2;
    Color32 Color;
    float TimeLeft;
};

struct DebugText2D
{
    Array<Char, InlinedAllocation<64>> Text;
    Vector2 Position;
    int32 Size;
    Color Color;
    float TimeLeft;
};

struct DebugText3D
{
    Array<Char, InlinedAllocation<64>> Text;
    Transform Transform;
    bool FaceCamera;
    int32 Size;
    Color Color;
    float TimeLeft;
};

PACK_STRUCT(struct Vertex {
    Vector3 Position;
    Color32 Color;
    });

PACK_STRUCT(struct Data {
    Matrix ViewProjection;
    Vector3 Padding;
    bool EnableDepthTest;
    });

struct PsData
{
    GPUPipelineState* Depth = nullptr;
    GPUPipelineState* NoDepthTest = nullptr;
    GPUPipelineState* DepthWrite = nullptr;
    GPUPipelineState* NoDepthTestDepthWrite = nullptr;

    bool Create(GPUPipelineState::Description& desc)
    {
        desc.DepthTestEnable = true;
        desc.DepthWriteEnable = false;

        Depth = GPUDevice::Instance->CreatePipelineState();
        if (Depth->Init(desc))
            return true;
        desc.DepthTestEnable = false;
        NoDepthTest = GPUDevice::Instance->CreatePipelineState();
        if (NoDepthTest->Init(desc))
            return false;

        desc.DepthWriteEnable = true;

        NoDepthTestDepthWrite = GPUDevice::Instance->CreatePipelineState();
        if (NoDepthTestDepthWrite->Init(desc))
            return false;
        desc.DepthTestEnable = true;
        DepthWrite = GPUDevice::Instance->CreatePipelineState();
        return DepthWrite->Init(desc);
    }

    void Release()
    {
        SAFE_DELETE_GPU_RESOURCE(Depth);
        SAFE_DELETE_GPU_RESOURCE(NoDepthTest);
        SAFE_DELETE_GPU_RESOURCE(DepthWrite);
        SAFE_DELETE_GPU_RESOURCE(NoDepthTestDepthWrite);
    }

    FORCE_INLINE GPUPipelineState* Get(bool depthWrite, bool depthTest)
    {
        if (depthWrite)
            return depthTest ? DepthWrite : NoDepthTestDepthWrite;
        return depthTest ? Depth : NoDepthTest;
    }
};

template<typename T>
void UpdateList(float dt, Array<T>& list)
{
    for (int32 i = 0; i < list.Count() && list.HasItems(); i++)
    {
        list[i].TimeLeft -= dt;
        if (list[i].TimeLeft <= 0)
        {
            list.RemoveAt(i);
            i--;
        }
    }
}

struct DebugDrawData
{
    Array<DebugLine> DefaultLines;
    Array<DebugLine> OneFrameLines;
    Array<DebugTriangle> DefaultTriangles;
    Array<DebugTriangle> OneFrameTriangles;
    Array<DebugTriangle> DefaultWireTriangles;
    Array<DebugTriangle> OneFrameWireTriangles;
    Array<DebugText2D> DefaultText2D;
    Array<DebugText2D> OneFrameText2D;
    Array<DebugText3D> DefaultText3D;
    Array<DebugText3D> OneFrameText3D;

    inline int32 Count() const
    {
        return LinesCount() + TrianglesCount() + TextCount();
    }

    inline int32 LinesCount() const
    {
        return DefaultLines.Count() + OneFrameLines.Count();
    }

    inline int32 TrianglesCount() const
    {
        return DefaultTriangles.Count() + OneFrameTriangles.Count() + DefaultWireTriangles.Count() + OneFrameWireTriangles.Count();
    }

    inline int32 TextCount() const
    {
        return DefaultText2D.Count() + OneFrameText2D.Count() + DefaultText3D.Count() + OneFrameText3D.Count();
    }

    inline void Add(const DebugLine& l)
    {
        if (l.TimeLeft > 0)
            DefaultLines.Add(l);
        else
            OneFrameLines.Add(l);
    }

    inline void Add(const DebugTriangle& t)
    {
        if (t.TimeLeft > 0)
            DefaultTriangles.Add(t);
        else
            OneFrameTriangles.Add(t);
    }

    inline void AddWire(const DebugTriangle& t)
    {
        if (t.TimeLeft > 0)
            DefaultWireTriangles.Add(t);
        else
            OneFrameWireTriangles.Add(t);
    }

    inline void Update(float deltaTime)
    {
        UpdateList(deltaTime, DefaultLines);
        UpdateList(deltaTime, DefaultTriangles);
        UpdateList(deltaTime, DefaultWireTriangles);
        UpdateList(deltaTime, DefaultText2D);
        UpdateList(deltaTime, DefaultText3D);

        OneFrameLines.Clear();
        OneFrameTriangles.Clear();
        OneFrameWireTriangles.Clear();
        OneFrameText2D.Clear();
        OneFrameText3D.Clear();
    }

    inline void Clear()
    {
        DefaultLines.Clear();
        OneFrameLines.Clear();
        DefaultTriangles.Clear();
        OneFrameTriangles.Clear();
        DefaultWireTriangles.Clear();
        OneFrameWireTriangles.Clear();
        DefaultText2D.Clear();
        OneFrameText2D.Clear();
        DefaultText3D.Clear();
        OneFrameText3D.Clear();
    }

    inline void Release()
    {
        DefaultLines.Resize(0);
        OneFrameLines.Resize(0);
        DefaultTriangles.Resize(0);
        OneFrameTriangles.Resize(0);
        DefaultWireTriangles.Resize(0);
        OneFrameWireTriangles.Resize(0);
        DefaultText2D.Resize(0);
        OneFrameText2D.Resize(0);
        DefaultText3D.Resize(0);
        OneFrameText3D.Resize(0);
    }
};

struct DebugDrawContext
{
    DebugDrawData DebugDrawDefault;
    DebugDrawData DebugDrawDepthTest;
};

namespace
{
    DebugDrawContext GlobalContext;
    DebugDrawContext* Context;
    AssetReference<Shader> DebugDrawShader;
    AssetReference<FontAsset> DebugDrawFont;
    PsData DebugDrawPsLinesDefault;
    PsData DebugDrawPsLinesDepthTest;
    PsData DebugDrawPsWireTrianglesDefault;
    PsData DebugDrawPsWireTrianglesDepthTest;
    PsData DebugDrawPsTrianglesDefault;
    PsData DebugDrawPsTrianglesDepthTest;
    DynamicVertexBuffer* DebugDrawVB = nullptr;
    Vector3 SphereCache[DEBUG_DRAW_SPHERE_VERTICES];
    Vector3 CircleCache[DEBUG_DRAW_CIRCLE_VERTICES];
    Vector3 CylinderCache[DEBUG_DRAW_CYLINDER_VERTICES];
    Array<Vector3> SphereTriangleCache;
};

extern int32 BoxTrianglesIndicesCache[];

struct DebugDrawCall
{
    int32 StartVertex;
    int32 VertexCount;
};

DebugDrawCall WriteList(int32& vertexCounter, const Array<DebugLine>& list)
{
    DebugDrawCall drawCall;
    drawCall.StartVertex = vertexCounter;

    Vertex vv[2];
    for (int32 i = 0; i < list.Count(); i++)
    {
        const DebugLine& l = list[i];

        vv[0].Position = l.Start;
        vv[0].Color = l.Color;
        vv[1].Position = l.End;
        vv[1].Color = vv[0].Color;

        DebugDrawVB->Write(vv, sizeof(Vertex) * 2);
    }

    drawCall.VertexCount = list.Count() * 2;
    vertexCounter += drawCall.VertexCount;

    return drawCall;
}

DebugDrawCall WriteList(int32& vertexCounter, const Array<DebugTriangle>& list)
{
    DebugDrawCall drawCall;
    drawCall.StartVertex = vertexCounter;

    Vertex vv[3];
    for (int32 i = 0; i < list.Count(); i++)
    {
        const DebugTriangle& l = list[i];

        vv[0].Position = l.V0;
        vv[0].Color = l.Color;
        vv[1].Position = l.V1;
        vv[1].Color = vv[0].Color;
        vv[2].Position = l.V2;
        vv[2].Color = vv[0].Color;

        DebugDrawVB->Write(vv, sizeof(Vertex) * 3);
    }

    drawCall.VertexCount = list.Count() * 3;
    vertexCounter += drawCall.VertexCount;

    return drawCall;
}

template<typename T>
DebugDrawCall WriteLists(int32& vertexCounter, const Array<T>& listA, const Array<T>& listB)
{
    const DebugDrawCall drawCallA = WriteList(vertexCounter, listA);
    const DebugDrawCall drawCallB = WriteList(vertexCounter, listB);

    DebugDrawCall drawCall;
    drawCall.StartVertex = drawCallA.StartVertex;
    drawCall.VertexCount = drawCallA.VertexCount + drawCallB.VertexCount;
    return drawCall;
}

inline void DrawText3D(const DebugText3D& t, const RenderContext& renderContext, const Vector3& viewUp, const Matrix& f, const Matrix& vp, const Viewport& viewport, GPUContext* context, GPUTextureView* target, GPUTextureView* depthBuffer)
{
    Matrix w, fw, m;
    if (t.FaceCamera)
        Matrix::CreateWorld(t.Transform.Translation, renderContext.View.Direction, viewUp, w);
    else
        t.Transform.GetWorld(w);
    Matrix::Multiply(f, w, fw);
    Matrix::Multiply(fw, vp, m);
    Render2D::Begin(context, target, depthBuffer, viewport, m);
    const StringView text(t.Text.Get(), t.Text.Count() - 1);
    Render2D::DrawText(DebugDrawFont->CreateFont(t.Size), text, t.Color, Vector2::Zero);
    Render2D::End();
}

class DebugDrawService : public EngineService
{
public:

    DebugDrawService()
        : EngineService(TEXT("Debug Draw"), -80)
    {
    }

    bool Init() override;
    void Update() override;
    void Dispose() override;
};

DebugDrawService DebugDrawServiceInstance;

bool DebugDrawService::Init()
{
    Context = &GlobalContext;

    // Init wireframe sphere cache
    int32 index = 0;
    float step = TWO_PI / DEBUG_DRAW_SPHERE_RESOLUTION;
    for (float a = 0.0f; a < TWO_PI; a += step)
    {
        // Calculate sines and cosines
        float sinA = Math::Sin(a);
        float cosA = Math::Cos(a);
        float sinB = Math::Sin(a + step);
        float cosB = Math::Cos(a + step);

        // XY loop
        SphereCache[index++] = Vector3(cosA, sinA, 0.0f);
        SphereCache[index++] = Vector3(cosB, sinB, 0.0f);

        // XZ loop
        SphereCache[index++] = Vector3(cosA, 0.0f, sinA);
        SphereCache[index++] = Vector3(cosB, 0.0f, sinB);

        // YZ loop
        SphereCache[index++] = Vector3(0.0f, cosA, sinA);
        SphereCache[index++] = Vector3(0.0f, cosB, sinB);
    }

    // Init wireframe circle cache
    index = 0;
    step = TWO_PI / DEBUG_DRAW_CIRCLE_RESOLUTION;
    for (float a = 0.0f; a < TWO_PI; a += step)
    {
        // Calculate sines and cosines
        float sinA = Math::Sin(a);
        float cosA = Math::Cos(a);
        float sinB = Math::Sin(a + step);
        float cosB = Math::Cos(a + step);

        CircleCache[index++] = Vector3(cosA, sinA, 0.0f);
        CircleCache[index++] = Vector3(cosB, sinB, 0.0f);
    }

    // Init triangle sphere cache
    {
        const int32 verticalSegments = DEBUG_DRAW_TRIANGLE_SPHERE_RESOLUTION;
        const int32 horizontalSegments = DEBUG_DRAW_TRIANGLE_SPHERE_RESOLUTION * 2;

        Array<Vector3> vertices;
        vertices.Resize((verticalSegments + 1) * (horizontalSegments + 1));
        Array<int> indices;
        indices.Resize((verticalSegments) * (horizontalSegments + 1) * 6);

        int vertexCount = 0;

        // Generate the first extremity points
        for (int j = 0; j <= horizontalSegments; j++)
        {
            vertices[vertexCount++] = Vector3(0, -1, 0);
        }

        // Create rings of vertices at progressively higher latitudes
        for (int i = 1; i < verticalSegments; i++)
        {
            const float latitude = ((i * PI / verticalSegments) - PI / 2.0f);
            const float dy = Math::Sin(latitude);
            const float dxz = Math::Cos(latitude);

            // The first point
            auto& firstHorizontalVertex = vertices[vertexCount++];
            firstHorizontalVertex = Vector3(0, dy, dxz);

            // Create a single ring of vertices at this latitude
            for (int j = 1; j < horizontalSegments; j++)
            {
                const float longitude = (j * 2.0f * PI / horizontalSegments);
                const float dx = Math::Sin(longitude) * dxz;
                const float dz = Math::Cos(longitude) * dxz;

                vertices[vertexCount++] = Vector3(dx, dy, dz);
            }

            // The last point equal to the first point
            vertices[vertexCount++] = firstHorizontalVertex;
        }

        // Generate the end extremity points
        for (int j = 0; j <= horizontalSegments; j++)
        {
            vertices[vertexCount++] = Vector3(0, 1, 0);
        }

        // Fill the index buffer with triangles joining each pair of latitude rings
        const int stride = horizontalSegments + 1;
        int indexCount = 0;
        for (int i = 0; i < verticalSegments; i++)
        {
            for (int j = 0; j <= horizontalSegments; j++)
            {
                const int nextI = i + 1;
                const int nextJ = (j + 1) % stride;

                indices[indexCount++] = (i * stride + j);
                indices[indexCount++] = (nextI * stride + j);
                indices[indexCount++] = (i * stride + nextJ);

                indices[indexCount++] = (i * stride + nextJ);
                indices[indexCount++] = (nextI * stride + j);
                indices[indexCount++] = (nextI * stride + nextJ);
            }
        }

        // Create cached unit sphere triangles vertices locations
        SphereTriangleCache.Resize(indices.Count());
        for (int32 i = 0; i < indices.Count();)
        {
            SphereTriangleCache[i] = vertices[indices[i]];
            i++;
            SphereTriangleCache[i] = vertices[indices[i]];
            i++;
            SphereTriangleCache[i] = vertices[indices[i]];
            i++;
        }
    }

    return false;
}

void DebugDrawService::Update()
{
    // Special case for Null renderer
    if (GPUDevice::Instance->GetRendererType() == RendererType::Null)
    {
        GlobalContext.DebugDrawDefault.Clear();
        GlobalContext.DebugDrawDepthTest.Clear();
        return;
    }

    PROFILE_CPU();

    // Update lists
    float deltaTime = Time::Update.DeltaTime.GetTotalSeconds();
#if USE_EDITOR
    if (!Editor::IsPlayMode)
        deltaTime = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
#endif
    GlobalContext.DebugDrawDefault.Update(deltaTime);
    GlobalContext.DebugDrawDepthTest.Update(deltaTime);

    // Check if need to setup a resources
    if (DebugDrawShader == nullptr)
    {
        // Shader
        DebugDrawShader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/DebugDraw"));
        if (DebugDrawShader == nullptr)
        {
            LOG(Fatal, "Cannot load DebugDraw shader");
        }
    }
    if (DebugDrawVB == nullptr && DebugDrawShader && DebugDrawShader->IsLoaded())
    {
        bool failed = false;
        const auto shader = DebugDrawShader->GetShader();

        // Create pipeline states
        GPUPipelineState::Description desc = GPUPipelineState::Description::Default;
        desc.BlendMode = BlendingMode::AlphaBlend;
        desc.CullMode = CullMode::TwoSided;
        desc.VS = shader->GetVS("VS");

        // Default
        desc.PS = shader->GetPS("PS");
        desc.PrimitiveTopologyType = PrimitiveTopologyType::Line;
        failed |= DebugDrawPsLinesDefault.Create(desc);
        desc.PrimitiveTopologyType = PrimitiveTopologyType::Triangle;
        failed |= DebugDrawPsTrianglesDefault.Create(desc);
        desc.Wireframe = true;
        failed |= DebugDrawPsWireTrianglesDefault.Create(desc);

        // Depth Test
        desc.Wireframe = false;
        desc.PS = shader->GetPS("PS_DepthTest");
        desc.PrimitiveTopologyType = PrimitiveTopologyType::Line;
        failed |= DebugDrawPsLinesDepthTest.Create(desc);
        desc.PrimitiveTopologyType = PrimitiveTopologyType::Triangle;
        failed |= DebugDrawPsTrianglesDepthTest.Create(desc);
        desc.Wireframe = true;
        failed |= DebugDrawPsWireTrianglesDepthTest.Create(desc);

        if (failed)
        {
            LOG(Fatal, "Cannot setup DebugDraw service!");
        }

        // Vertex buffer
        DebugDrawVB = New<DynamicVertexBuffer>((uint32)(DEBUG_DRAW_INITIAL_VB_CAPACITY * sizeof(Vertex)), (uint32)sizeof(Vertex), TEXT("DebugDraw.VB"));
    }
}

void DebugDrawService::Dispose()
{
    // Clear lists
    GlobalContext.DebugDrawDefault.Release();
    GlobalContext.DebugDrawDepthTest.Release();

    // Release resources
    SphereTriangleCache.Resize(0);
    DebugDrawPsLinesDefault.Release();
    DebugDrawPsLinesDepthTest.Release();
    DebugDrawPsWireTrianglesDefault.Release();
    DebugDrawPsWireTrianglesDepthTest.Release();
    DebugDrawPsTrianglesDefault.Release();
    DebugDrawPsTrianglesDepthTest.Release();
    SAFE_DELETE(DebugDrawVB);
    DebugDrawShader = nullptr;
}

#if USE_EDITOR

void* DebugDraw::AllocateContext()
{
    auto context = (DebugDrawContext*)Allocator::Allocate(sizeof(DebugDrawContext));
    Memory::ConstructItem(context);
    return context;
}

void DebugDraw::FreeContext(void* context)
{
    Memory::DestructItem((DebugDrawContext*)context);
    Allocator::Free(context);
}

void DebugDraw::UpdateContext(void* context, float deltaTime)
{
    ((DebugDrawContext*)context)->DebugDrawDefault.Update(deltaTime);
    ((DebugDrawContext*)context)->DebugDrawDepthTest.Update(deltaTime);
}

void DebugDraw::SetContext(void* context)
{
    Context = context ? (DebugDrawContext*)context : &GlobalContext;
}

#endif

void DebugDraw::Draw(RenderContext& renderContext, GPUTextureView* target, GPUTextureView* depthBuffer, bool enableDepthTest)
{
    PROFILE_GPU_CPU("Debug Draw");

    // Ensure to have shader loaded and any lines to render
    const int32 debugDrawDepthTestCount = Context->DebugDrawDepthTest.Count();
    const int32 debugDrawDefaultCount = Context->DebugDrawDefault.Count();
    if (DebugDrawShader == nullptr || !DebugDrawShader->IsLoaded() || debugDrawDepthTestCount + debugDrawDefaultCount == 0)
        return;
    if (renderContext.Buffers == nullptr || !DebugDrawVB)
        return;
    auto context = GPUDevice::Instance->GetMainContext();

    // Fallback to task buffers
    if (target == nullptr && renderContext.Task)
        target = renderContext.Task->GetOutputView();

    // Fill vertex buffer and upload data
#if COMPILE_WITH_PROFILER
    const auto updateBufferProfileKey = ProfilerCPU::BeginEvent(TEXT("Update Buffer"));
#endif
    DebugDrawVB->Clear();
    int32 vertexCounter = 0;
    const DebugDrawCall depthTestLines = WriteLists(vertexCounter, Context->DebugDrawDepthTest.DefaultLines, Context->DebugDrawDepthTest.OneFrameLines);
    const DebugDrawCall defaultLines = WriteLists(vertexCounter, Context->DebugDrawDefault.DefaultLines, Context->DebugDrawDefault.OneFrameLines);
    const DebugDrawCall depthTestTriangles = WriteLists(vertexCounter, Context->DebugDrawDepthTest.DefaultTriangles, Context->DebugDrawDepthTest.OneFrameTriangles);
    const DebugDrawCall defaultTriangles = WriteLists(vertexCounter, Context->DebugDrawDefault.DefaultTriangles, Context->DebugDrawDefault.OneFrameTriangles);
    const DebugDrawCall depthTestWireTriangles = WriteLists(vertexCounter, Context->DebugDrawDepthTest.DefaultWireTriangles, Context->DebugDrawDepthTest.OneFrameWireTriangles);
    const DebugDrawCall defaultWireTriangles = WriteLists(vertexCounter, Context->DebugDrawDefault.DefaultWireTriangles, Context->DebugDrawDefault.OneFrameWireTriangles);
    DebugDrawVB->Flush(context);
#if COMPILE_WITH_PROFILER
    ProfilerCPU::EndEvent(updateBufferProfileKey);
#endif

    // Update constant buffer
    const auto cb = DebugDrawShader->GetShader()->GetCB(0);
    Data data;
    Matrix vp;
    Matrix::Multiply(renderContext.View.View, renderContext.View.NonJitteredProjection, vp);
    Matrix::Transpose(vp, data.ViewProjection);
    data.EnableDepthTest = enableDepthTest;
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    auto vb = DebugDrawVB->GetBuffer();

    // Draw with depth test
    if (depthTestLines.VertexCount + depthTestTriangles.VertexCount + depthTestWireTriangles.VertexCount > 0)
    {
        if (data.EnableDepthTest)
            context->BindSR(0, renderContext.Buffers->DepthBuffer);
        const bool enableDepthWrite = data.EnableDepthTest;

        context->SetRenderTarget(depthBuffer ? depthBuffer : *renderContext.Buffers->DepthBuffer, target);

        // Lines
        if (depthTestLines.VertexCount)
        {
            auto state = data.EnableDepthTest ? &DebugDrawPsLinesDepthTest : &DebugDrawPsLinesDefault;
            context->SetState(state->Get(enableDepthWrite, true));
            context->BindVB(ToSpan(&vb, 1));
            context->Draw(depthTestLines.StartVertex, depthTestLines.VertexCount);
        }

        // Wire Triangles
        if (depthTestWireTriangles.VertexCount)
        {
            auto state = data.EnableDepthTest ? &DebugDrawPsWireTrianglesDepthTest : &DebugDrawPsWireTrianglesDefault;
            context->SetState(state->Get(enableDepthWrite, true));
            context->BindVB(ToSpan(&vb, 1));
            context->Draw(depthTestWireTriangles.StartVertex, depthTestWireTriangles.VertexCount);
        }

        // Triangles
        if (depthTestTriangles.VertexCount)
        {
            auto state = data.EnableDepthTest ? &DebugDrawPsTrianglesDepthTest : &DebugDrawPsTrianglesDefault;
            context->SetState(state->Get(enableDepthWrite, true));
            context->BindVB(ToSpan(&vb, 1));
            context->Draw(depthTestTriangles.StartVertex, depthTestTriangles.VertexCount);
        }

        if (data.EnableDepthTest)
            context->UnBindSR(0);
    }

    // Draw without depth
    if (defaultLines.VertexCount + defaultTriangles.VertexCount + defaultWireTriangles.VertexCount > 0)
    {
        context->SetRenderTarget(target);

        // Lines
        if (defaultLines.VertexCount)
        {
            context->SetState(DebugDrawPsLinesDefault.Get(false, false));
            context->BindVB(ToSpan(&vb, 1));
            context->Draw(defaultLines.StartVertex, defaultLines.VertexCount);
        }

        // Wire Triangles
        if (defaultWireTriangles.VertexCount)
        {
            context->SetState(DebugDrawPsWireTrianglesDefault.Get(false, false));
            context->BindVB(ToSpan(&vb, 1));
            context->Draw(defaultWireTriangles.StartVertex, defaultWireTriangles.VertexCount);
        }

        // Triangles
        if (defaultTriangles.VertexCount)
        {
            context->SetState(DebugDrawPsTrianglesDefault.Get(false, false));
            context->BindVB(ToSpan(&vb, 1));
            context->Draw(defaultTriangles.StartVertex, defaultTriangles.VertexCount);
        }
    }

    // Text
    if (Context->DebugDrawDefault.TextCount() + Context->DebugDrawDepthTest.TextCount())
    {
        PROFILE_GPU_CPU_NAMED("Text");
        auto features = Render2D::Features;
        Render2D::Features = (Render2D::RenderingFeatures)((uint32)features & ~(uint32)Render2D::RenderingFeatures::VertexSnapping);

        if (!DebugDrawFont)
            DebugDrawFont = Content::LoadAsyncInternal<FontAsset>(TEXT("Editor/Fonts/Roboto-Regular"));
        if (DebugDrawFont && DebugDrawFont->IsLoaded())
        {
            Viewport viewport = renderContext.Task->GetViewport();

            if (Context->DebugDrawDefault.DefaultText2D.Count() + Context->DebugDrawDefault.OneFrameText2D.Count())
            {
                Render2D::Begin(context, target, nullptr, viewport);
                for (auto& t : Context->DebugDrawDefault.DefaultText2D)
                {
                    const StringView text(t.Text.Get(), t.Text.Count() - 1);
                    Render2D::DrawText(DebugDrawFont->CreateFont(t.Size), text, t.Color, t.Position);
                }
                for (auto& t : Context->DebugDrawDefault.OneFrameText2D)
                {
                    const StringView text(t.Text.Get(), t.Text.Count() - 1);
                    Render2D::DrawText(DebugDrawFont->CreateFont(t.Size), text, t.Color, t.Position);
                }
                Render2D::End();
            }

            if (Context->DebugDrawDefault.DefaultText3D.Count() + Context->DebugDrawDefault.OneFrameText3D.Count())
            {
                Matrix f;
                Matrix::RotationZ(PI, f);
                Vector3 viewUp;
                Vector3::Transform(Vector3::Up, Quaternion::LookRotation(renderContext.View.Direction, Vector3::Up), viewUp);
                for (auto& t : Context->DebugDrawDefault.DefaultText3D)
                    DrawText3D(t, renderContext, viewUp, f, vp, viewport, context, target, nullptr);
                for (auto& t : Context->DebugDrawDefault.OneFrameText3D)
                    DrawText3D(t, renderContext, viewUp, f, vp, viewport, context, target, nullptr);
            }
        }

        Render2D::Features = features;
    }
}

namespace
{
    bool DrawActorsTreeWalk(Actor* actor)
    {
        if (actor->IsActiveInHierarchy())
        {
            actor->OnDebugDraw();
            return true;
        }
        return false;
    }
}

void DebugDraw::DrawActors(Actor** selectedActors, int32 selectedActorsCount, bool drawScenes)
{
    PROFILE_CPU();

    if (selectedActors)
    {
        for (int32 i = 0; i < selectedActorsCount; i++)
        {
            auto a = selectedActors[i];
            if (a)
                a->OnDebugDrawSelected();
        }
    }

    if (drawScenes)
    {
        Function<bool(Actor*)> function = &DrawActorsTreeWalk;
        SceneQuery::TreeExecute(function);
    }
}

void DebugDraw::DrawLine(const Vector3& start, const Vector3& end, const Color& color, float duration, bool depthTest)
{
    // Create draw call entry
    DebugLine l = { start, end, Color32(color), duration };

    // Add line
    if (depthTest)
        Context->DebugDrawDepthTest.Add(l);
    else
        Context->DebugDrawDefault.Add(l);
}

void DebugDraw::DrawLines(const Span<Vector3>& lines, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    if (lines.Length() % 2 != 0)
    {
        DebugLog::ThrowException("Cannot draw debug lines with uneven amount of items in array");
        return;
    }

    // Create draw call entry
    DebugLine l = { Vector3::Zero, Vector3::Zero, Color32(color), duration };

    // Add lines
    const Vector3* p = lines.Get();
    Array<DebugLine>* list;

    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultLines : &Context->DebugDrawDepthTest.OneFrameLines;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultLines : &Context->DebugDrawDefault.OneFrameLines;

    list->EnsureCapacity(list->Count() + lines.Length());
    for (int32 i = 0; i < lines.Length(); i += 2)
    {
        Vector3::Transform(*p++, transform, l.Start);
        Vector3::Transform(*p++, transform, l.End);
        list->Add(l);
    }
}

void DebugDraw::DrawBezier(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, const Color& color, float duration, bool depthTest)
{
    // Create draw call entry
    Array<DebugLine>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultLines : &Context->DebugDrawDepthTest.OneFrameLines;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultLines : &Context->DebugDrawDefault.OneFrameLines;
    DebugLine l = { p1, Vector3::Zero, Color32(color), duration };

    // Find amount of segments to use
    const Vector3 d1 = p2 - p1;
    const Vector3 d2 = p3 - p2;
    const Vector3 d3 = p4 - p3;
    const float len = d1.Length() + d2.Length() + d3.Length();
    const int32 segmentCount = Math::Clamp(Math::CeilToInt(len * 0.05f), 1, 100);
    const float segmentCountInv = 1.0f / (float)segmentCount;
    list->EnsureCapacity(list->Count() + segmentCount + 2);

    // Draw segmented curve
    for (int32 i = 0; i <= segmentCount; i++)
    {
        const float t = (float)i * segmentCountInv;
        AnimationUtils::Bezier(p1, p2, p3, p4, t, l.End);
        list->Add(l);
        l.Start = l.End;
    }
}

#define DRAW_WIRE_BOX_LINE(i0, i1, list) l.Start = corners[i0]; l.End = corners[i1]; Context->list.Add(l)
#define DRAW_WIRE_BOX(list) \
	DRAW_WIRE_BOX_LINE(0, 1, list); \
	DRAW_WIRE_BOX_LINE(0, 3, list); \
	DRAW_WIRE_BOX_LINE(0, 4, list); \
	DRAW_WIRE_BOX_LINE(1, 2, list); \
	DRAW_WIRE_BOX_LINE(1, 5, list); \
	DRAW_WIRE_BOX_LINE(2, 3, list); \
	DRAW_WIRE_BOX_LINE(2, 6, list); \
	DRAW_WIRE_BOX_LINE(3, 7, list); \
	DRAW_WIRE_BOX_LINE(4, 5, list); \
	DRAW_WIRE_BOX_LINE(4, 7, list); \
	DRAW_WIRE_BOX_LINE(5, 6, list); \
	DRAW_WIRE_BOX_LINE(6, 7, list)

void DebugDraw::DrawWireBox(const BoundingBox& box, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    box.GetCorners(corners);

    // Draw lines
    DebugLine l;
    l.Color = Color32(color);
    l.TimeLeft = duration;
    if (depthTest)
    {
        DRAW_WIRE_BOX(DebugDrawDepthTest);
    }
    else
    {
        DRAW_WIRE_BOX(DebugDrawDefault);
    }
}

void DebugDraw::DrawWireFrustum(const BoundingFrustum& frustum, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    frustum.GetCorners(corners);

    // Draw lines
    DebugLine l;
    l.Color = Color32(color);
    l.TimeLeft = duration;
    if (depthTest)
    {
        DRAW_WIRE_BOX(DebugDrawDepthTest);
    }
    else
    {
        DRAW_WIRE_BOX(DebugDrawDefault);
    }
}

void DebugDraw::DrawWireBox(const OrientedBoundingBox& box, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    box.GetCorners(corners);

    // Draw lines
    DebugLine l;
    l.Color = Color32(color);
    l.TimeLeft = duration;
    if (depthTest)
    {
        DRAW_WIRE_BOX(DebugDrawDepthTest);
    }
    else
    {
        DRAW_WIRE_BOX(DebugDrawDefault);
    }
}

void DebugDraw::DrawWireSphere(const BoundingSphere& sphere, const Color& color, float duration, bool depthTest)
{
    // Draw lines of the unit sphere after linear transform
    DebugLine l;
    l.Color = Color32(color);
    l.TimeLeft = duration;
    if (depthTest)
    {
        for (int32 i = 0; i < DEBUG_DRAW_SPHERE_VERTICES;)
        {
            l.Start = sphere.Center + SphereCache[i++] * sphere.Radius;
            l.End = sphere.Center + SphereCache[i++] * sphere.Radius;
            Context->DebugDrawDepthTest.Add(l);
        }
    }
    else
    {
        for (int32 i = 0; i < DEBUG_DRAW_SPHERE_VERTICES;)
        {
            l.Start = sphere.Center + SphereCache[i++] * sphere.Radius;
            l.End = sphere.Center + SphereCache[i++] * sphere.Radius;
            Context->DebugDrawDefault.Add(l);
        }
    }
}

void DebugDraw::DrawSphere(const BoundingSphere& sphere, const Color& color, float duration, bool depthTest)
{
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;

    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    list->EnsureCapacity(list->Count() + SphereTriangleCache.Count());

    for (int32 i = 0; i < SphereTriangleCache.Count();)
    {
        t.V0 = sphere.Center + SphereTriangleCache[i++] * sphere.Radius;
        t.V1 = sphere.Center + SphereTriangleCache[i++] * sphere.Radius;
        t.V2 = sphere.Center + SphereTriangleCache[i++] * sphere.Radius;

        list->Add(t);
    }
}

void DebugDraw::DrawCircle(const Vector3& position, const Vector3& normal, float radius, const Color& color, float duration, bool depthTest)
{
    // Create matrix transform for unit circle points
    Matrix world, scale, matrix;
    Vector3 right, up;
    if (Vector3::Dot(normal, Vector3::Up) > 0.99f)
        right = Vector3::Right;
    else if (Vector3::Dot(normal, Vector3::Down) > 0.99f)
        right = Vector3::Left;
    else
        Vector3::Cross(normal, Vector3::Up, right);
    Vector3::Cross(right, normal, up);
    Matrix::Scaling(radius, scale);
    Matrix::CreateWorld(position, normal, up, world);
    Matrix::Multiply(scale, world, matrix);

    // Draw lines of the unit circle after linear transform
    Vector3 prev = Vector3::Transform(CircleCache[0], matrix);
    for (int32 i = 1; i < DEBUG_DRAW_CIRCLE_VERTICES;)
    {
        Vector3 cur = Vector3::Transform(CircleCache[i++], matrix);
        DrawLine(prev, cur, color, duration, depthTest);
        prev = cur;
    }
}

void DebugDraw::DrawWireTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Color& color, float duration, bool depthTest)
{
    DrawLine(v0, v1, color, duration, depthTest);
    DrawLine(v1, v2, color, duration, depthTest);
    DrawLine(v2, v0, color, duration, depthTest);
}

void DebugDraw::DrawTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Color& color, float duration, bool depthTest)
{
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    t.V0 = v0;
    t.V1 = v1;
    t.V2 = v2;

    if (depthTest)
    {
        Context->DebugDrawDepthTest.Add(t);
    }
    else
    {
        Context->DebugDrawDefault.Add(t);
    }
}

void DebugDraw::DrawTriangles(const Span<Vector3>& vertices, const Color& color, float duration, bool depthTest)
{
    ASSERT(vertices.Length() % 3 == 0);

    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;

    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    list->EnsureCapacity(list->Count() + vertices.Length() / 3);

    for (int32 i = 0; i < vertices.Length();)
    {
        t.V0 = vertices[i++];
        t.V1 = vertices[i++];
        t.V2 = vertices[i++];
        list->Add(t);
    }
}

void DebugDraw::DrawTriangles(const Array<Vector3>& vertices, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Vector3>(vertices.Get(), vertices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Span<Vector3>& vertices, const Span<int32>& indices, const Color& color, float duration, bool depthTest)
{
    ASSERT(indices.Length() % 3 == 0);

    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;

    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    list->EnsureCapacity(list->Count() + indices.Length() / 3);

    for (int32 i = 0; i < indices.Length();)
    {
        t.V0 = vertices[indices[i++]];
        t.V1 = vertices[indices[i++]];
        t.V2 = vertices[indices[i++]];
        list->Add(t);
    }
}

void DebugDraw::DrawTriangles(const Array<Vector3>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Vector3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawWireTriangles(const Span<Vector3>& vertices, const Color& color, float duration, bool depthTest)
{
    ASSERT(vertices.Length() % 3 == 0);

    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;

    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultWireTriangles : &Context->DebugDrawDepthTest.OneFrameWireTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultWireTriangles : &Context->DebugDrawDefault.OneFrameWireTriangles;
    list->EnsureCapacity(list->Count() + vertices.Length() / 3);

    for (int32 i = 0; i < vertices.Length();)
    {
        t.V0 = vertices[i++];
        t.V1 = vertices[i++];
        t.V2 = vertices[i++];
        list->Add(t);
    }
}

void DebugDraw::DrawWireTriangles(const Array<Vector3>& vertices, const Color& color, float duration, bool depthTest)
{
    DrawWireTriangles(Span<Vector3>(vertices.Get(), vertices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawWireTriangles(const Span<Vector3>& vertices, const Span<int32>& indices, const Color& color, float duration, bool depthTest)
{
    ASSERT(indices.Length() % 3 == 0);

    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;

    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultWireTriangles : &Context->DebugDrawDepthTest.OneFrameWireTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultWireTriangles : &Context->DebugDrawDefault.OneFrameWireTriangles;
    list->EnsureCapacity(list->Count() + indices.Length() / 3);

    for (int32 i = 0; i < indices.Length();)
    {
        t.V0 = vertices[indices[i++]];
        t.V1 = vertices[indices[i++]];
        t.V2 = vertices[indices[i++]];
        list->Add(t);
    }
}

void DebugDraw::DrawWireTriangles(const Array<Vector3>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color, float duration, bool depthTest)
{
    DrawWireTriangles(Span<Vector3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawWireTube(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color, float duration, bool depthTest)
{
    // Check if has no length (just sphere)
    if (length < ZeroTolerance)
    {
        // Draw sphere
        DrawWireSphere(BoundingSphere(position, radius), color, duration, depthTest);
    }
    else
    {
        // Set up
        const float step = TWO_PI / DEBUG_DRAW_SPHERE_RESOLUTION;
        radius = Math::Max(radius, 0.05f);
        length = Math::Max(length, 0.05f);
        const float halfLength = length / 2.0f;
        Matrix rotation, translation, world;
        Matrix::RotationQuaternion(orientation, rotation);
        Matrix::Translation(position, translation);
        Matrix::Multiply(rotation, translation, world);

        // Write vertices
#define DRAW_WIRE_BOX_LINE(x1, y1, z1, x2, y2, z2) DrawLine(Vector3::Transform(Vector3(x1, y1, z1), world), Vector3::Transform(Vector3(x2, y2, z2), world), color, duration, depthTest)
        for (float a = 0.0f; a < TWO_PI; a += step)
        {
            // Calculate sines and cosines
            // TODO: optimize this stuff
            float sinA = Math::Sin(a) * radius;
            float cosA = Math::Cos(a) * radius;
            float sinB = Math::Sin(a + step) * radius;
            float cosB = Math::Cos(a + step) * radius;

            // First XY loop
            DRAW_WIRE_BOX_LINE(cosA, sinA, -halfLength, cosB, sinB, -halfLength);

            // Second loop
            DRAW_WIRE_BOX_LINE(cosA, sinA, halfLength, cosB, sinB, halfLength);

            if (a >= PI)
            {
                // First XZ loop
                DRAW_WIRE_BOX_LINE(cosA, 0, sinA - halfLength, cosB, 0, sinB - halfLength);

                // First YZ loop
                DRAW_WIRE_BOX_LINE(0, cosA, sinA - halfLength, 0, cosB, sinB - halfLength);
            }
            else
            {
                // Second XZ loop
                DRAW_WIRE_BOX_LINE(cosA, 0, sinA + halfLength, cosB, 0, sinB + halfLength);

                // Second YZ loop
                DRAW_WIRE_BOX_LINE(0, cosA, sinA + halfLength, 0, cosB, sinB + halfLength);
            }

            // Connection
            if (Math::NearEqual(sinA, radius) || Math::NearEqual(cosA, radius) || Math::NearEqual(sinA, -radius) || Math::NearEqual(cosA, -radius))
            {
                DRAW_WIRE_BOX_LINE(cosA, sinA, -halfLength, cosA, sinA, halfLength);
            }
        }
#undef DRAW_WIRE_BOX_LINE
    }
}

void DebugDraw::DrawWireCylinder(const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color, float duration, bool depthTest)
{
    // Setup
    // TODO: move this to init!!
    const float angleBetweenFacets = TWO_PI / DEBUG_DRAW_CYLINDER_RESOLUTION;
    float verticalOffset = height * 0.5f;
    int32 index = 0;
    for (int32 i = 0; i < DEBUG_DRAW_CYLINDER_RESOLUTION; i++)
    {
        // Cache data
        float theta = i * angleBetweenFacets;
        float x = Math::Cos(theta) * radius;
        float z = Math::Sin(theta) * radius;

        // Top cap
        CylinderCache[index++] = Vector3(x, verticalOffset, z);

        // Top part of body
        CylinderCache[index++] = Vector3(x, verticalOffset, z);

        // Bottom part of body
        CylinderCache[index++] = Vector3(x, -verticalOffset, z);

        // Bottom cap
        CylinderCache[index++] = Vector3(x, -verticalOffset, z);
    }

    MISSING_CODE("missing rendering cylinder");

    // Write lines
    // TODO: optimize this to draw less lines
    /*for (uint32 i = 0; i < DEBUG_DRAW_CYLINDER_VERTICES; i += 4)
    {
        // Each iteration, the loop advances to the next vertex column
        // Four triangles per column (except for the four degenerate cap triangles)

        // Top cap triangles
        auto nextIndex = (ushort)((i + 4) % DEBUG_DRAW_CYLINDER_VERTICES);
        if (nextIndex != 0) //Don't add cap indices if it's going to be a degenerate triangle.
        {
            _vertexList.Items.Add(new VertexMeshInput(CylinderCache[i]));
            _vertexList.Items.Add(new VertexMeshInput(CylinderCache[nextIndex]));
            _vertexList.Items.Add(new VertexMeshInput(CylinderCache[0]));
        }

        // Body triangles
        nextIndex = (ushort)((i + 5) % DEBUG_DRAW_CYLINDER_VERTICES);
        _vertexList.Items.Add(new VertexMeshInput(CylinderCache[(i + 1)]));
        _vertexList.Items.Add(new VertexMeshInput(CylinderCache[(i + 2)]));
        _vertexList.Items.Add(new VertexMeshInput(CylinderCache[nextIndex]));

        _vertexList.Items.Add(new VertexMeshInput(CylinderCache[nextIndex]));
        _vertexList.Items.Add(new VertexMeshInput(CylinderCache[(i + 2)]));
        _vertexList.Items.Add(new VertexMeshInput(CylinderCache[((i + 6) % DEBUG_DRAW_CYLINDER_VERTICES)]));

        // Bottom cap triangles
        nextIndex = (ushort)((i + 7) % DEBUG_DRAW_CYLINDER_VERTICES);
        if (nextIndex != 3) //Don't add cap indices if it's going to be a degenerate triangle.
        {
            _vertexList.Items.Add(new VertexMeshInput(CylinderCache[(i + 3)]));
            _vertexList.Items.Add(new VertexMeshInput(CylinderCache[3]));
            _vertexList.Items.Add(new VertexMeshInput(CylinderCache[nextIndex]));
        }
    }
    _vertexList.UpdateResource();

    // Draw
    Matrix world = Matrix.RotationQuaternion(orientation) * Matrix.Translation(position);
    bool posOnly;
    auto material = _boxMaterial.Material;
    material.Params[0].Value = color;
    material.Params[1].Value = brightness;
    material.Apply(view, ref world, out posOnly);
    _vertexList.Draw(PrimitiveType.TriangleList, vertices);*/
}

void DebugDraw::DrawWireArrow(const Vector3& position, const Quaternion& orientation, float scale, const Color& color, float duration, bool depthTest)
{
    Vector3 direction, up, right;
    Vector3::Transform(Vector3::Forward, orientation, direction);
    Vector3::Transform(Vector3::Up, orientation, up);
    Vector3::Transform(Vector3::Right, orientation, right);
    const auto end = position + direction * (100.0f * scale);
    const auto capEnd = position + direction * (60.0f * scale);
    const float arrowSidesRatio = scale * 40.0f;

    DrawLine(position, end, color, duration, depthTest);
    DrawLine(end, capEnd + up * arrowSidesRatio, color, duration, depthTest);
    DrawLine(end, capEnd - up * arrowSidesRatio, color, duration, depthTest);
    DrawLine(end, capEnd + right * arrowSidesRatio, color, duration, depthTest);
    DrawLine(end, capEnd - right * arrowSidesRatio, color, duration, depthTest);
}

void DebugDraw::DrawBox(const BoundingBox& box, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    box.GetCorners(corners);

    // Draw triangles
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    list->EnsureCapacity(list->Count() + 36);
    for (int i0 = 0; i0 < 36;)
    {
        t.V0 = corners[BoxTrianglesIndicesCache[i0++]];
        t.V1 = corners[BoxTrianglesIndicesCache[i0++]];
        t.V2 = corners[BoxTrianglesIndicesCache[i0++]];

        list->Add(t);
    }
}

void DebugDraw::DrawBox(const OrientedBoundingBox& box, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    box.GetCorners(corners);

    // Draw triangles
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    list->EnsureCapacity(list->Count() + 36);
    for (int i0 = 0; i0 < 36;)
    {
        t.V0 = corners[BoxTrianglesIndicesCache[i0++]];
        t.V1 = corners[BoxTrianglesIndicesCache[i0++]];
        t.V2 = corners[BoxTrianglesIndicesCache[i0++]];

        list->Add(t);
    }
}

void DebugDraw::DrawText(const StringView& text, const Vector2& position, const Color& color, int32 size, float duration)
{
    if (text.Length() == 0 || size < 4)
        return;
    Array<DebugText2D>* list = duration > 0 ? &Context->DebugDrawDefault.DefaultText2D : &Context->DebugDrawDefault.OneFrameText2D;
    auto& t = list->AddOne();
    t.Text.Resize(text.Length() + 1);
    Platform::MemoryCopy(t.Text.Get(), text.Get(), text.Length() * sizeof(Char));
    t.Text[text.Length()] = 0;
    t.Position = position;
    t.Size = size;
    t.Color = color;
    t.TimeLeft = duration;
}

void DebugDraw::DrawText(const StringView& text, const Vector3& position, const Color& color, int32 size, float duration)
{
    if (text.Length() == 0 || size < 4)
        return;
    Array<DebugText3D>* list = duration > 0 ? &Context->DebugDrawDefault.DefaultText3D : &Context->DebugDrawDefault.OneFrameText3D;
    auto& t = list->AddOne();
    t.Text.Resize(text.Length() + 1);
    Platform::MemoryCopy(t.Text.Get(), text.Get(), text.Length() * sizeof(Char));
    t.Text[text.Length()] = 0;
    t.Transform = position;
    t.FaceCamera = true;
    t.Size = size;
    t.Color = color;
    t.TimeLeft = duration;
}

void DebugDraw::DrawText(const StringView& text, const Transform& transform, const Color& color, int32 size, float duration)
{
    if (text.Length() == 0 || size < 4)
        return;
    Array<DebugText3D>* list = duration > 0 ? &Context->DebugDrawDefault.DefaultText3D : &Context->DebugDrawDefault.OneFrameText3D;
    auto& t = list->AddOne();
    t.Text.Resize(text.Length() + 1);
    Platform::MemoryCopy(t.Text.Get(), text.Get(), text.Length() * sizeof(Char));
    t.Text[text.Length()] = 0;
    t.Transform = transform;
    t.FaceCamera = false;
    t.Size = size;
    t.Color = color;
    t.TimeLeft = duration;
}

#endif
