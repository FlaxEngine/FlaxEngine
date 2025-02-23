// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Animations/AnimationUtils.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Render2D/FontAsset.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Engine/Level/Actors/Light.h"
#include "Engine/Physics/Colliders/Collider.h"
#endif

// Debug draw service configuration
#define DEBUG_DRAW_INITIAL_VB_CAPACITY (4 * 1024)
//
#define DEBUG_DRAW_SPHERE_LOD0_RESOLUTION 64
#define DEBUG_DRAW_SPHERE_LOD0_SCREEN_SIZE 0.2f
#define DEBUG_DRAW_SPHERE_LOD1_RESOLUTION 16
#define DEBUG_DRAW_SPHERE_LOD1_SCREEN_SIZE 0.08f
#define DEBUG_DRAW_SPHERE_LOD2_RESOLUTION 8
//
#define DEBUG_DRAW_CIRCLE_RESOLUTION 32
#define DEBUG_DRAW_CIRCLE_LINES_COUNT DEBUG_DRAW_CIRCLE_RESOLUTION
#define DEBUG_DRAW_CIRCLE_VERTICES DEBUG_DRAW_CIRCLE_LINES_COUNT * 2
//
#define DEBUG_DRAW_CYLINDER_RESOLUTION 12
#define DEBUG_DRAW_CYLINDER_VERTICES (DEBUG_DRAW_CYLINDER_RESOLUTION * 4)
//
#define DEBUG_DRAW_CONE_RESOLUTION 32
//
#define DEBUG_DRAW_ARC_RESOLUTION 32
//
#define DEBUG_DRAW_TRIANGLE_SPHERE_RESOLUTION 12

struct DebugSphereCache
{
    Array<Float3, FixedAllocation<(DEBUG_DRAW_SPHERE_LOD0_RESOLUTION + 1) * 3 * 2>> Vertices;

    void Init(int32 resolution)
    {
        const int32 verticesCount = (resolution + 1) * 3 * 2;
        Vertices.Resize(verticesCount);
        int32 index = 0;
        const float step = TWO_PI / (float)resolution;
        for (float a = 0.0f; a < TWO_PI; a += step)
        {
            // Calculate sines and cosines
            const float sinA = Math::Sin(a);
            const float cosA = Math::Cos(a);
            const float sinB = Math::Sin(a + step);
            const float cosB = Math::Cos(a + step);

            // XY loop
            Vertices[index++] = Float3(cosA, sinA, 0.0f);
            Vertices[index++] = Float3(cosB, sinB, 0.0f);

            // XZ loop
            Vertices[index++] = Float3(cosA, 0.0f, sinA);
            Vertices[index++] = Float3(cosB, 0.0f, sinB);

            // YZ loop
            Vertices[index++] = Float3(0.0f, cosA, sinA);
            Vertices[index++] = Float3(0.0f, cosB, sinB);
        }
    }
};

struct DebugLine
{
    Float3 Start;
    Float3 End;
    Color32 Color;
    float TimeLeft;
};

struct DebugGeometryBuffer
{
    GPUBuffer* Buffer;
    float TimeLeft;
    Matrix Transform;
};

struct DebugTriangle
{
    Float3 V0;
    Float3 V1;
    Float3 V2;
    Color32 Color;
    float TimeLeft;
};

struct DebugText2D
{
    Array<Char, InlinedAllocation<64>> Text;
    Float2 Position;
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

typedef DebugDraw::Vertex Vertex;

GPU_CB_STRUCT(ShaderData {
    Matrix ViewProjection;
    Float2 Padding;
    float ClipPosZBias;
    uint32 EnableDepthTest;
    });

struct PsData
{
    GPUPipelineState* Depth = nullptr;
    GPUPipelineState* NoDepthTest = nullptr;
    GPUPipelineState* DepthWrite = nullptr;
    GPUPipelineState* NoDepthTestDepthWrite = nullptr;

    bool Create(GPUPipelineState::Description& desc)
    {
        desc.DepthEnable = true;
        desc.DepthWriteEnable = false;

        Depth = GPUDevice::Instance->CreatePipelineState();
        if (Depth->Init(desc))
            return true;
        desc.DepthEnable = false;
        NoDepthTest = GPUDevice::Instance->CreatePipelineState();
        if (NoDepthTest->Init(desc))
            return false;

        desc.DepthWriteEnable = true;

        NoDepthTestDepthWrite = GPUDevice::Instance->CreatePipelineState();
        if (NoDepthTestDepthWrite->Init(desc))
            return false;
        desc.DepthEnable = true;
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

void TeleportList(const Float3& delta, Array<DebugLine>& list)
{
    for (auto& l : list)
    {
        l.Start += delta;
        l.End += delta;
    }
}

void TeleportList(const Float3& delta, Array<Vertex>& list)
{
    for (auto& v : list)
    {
        v.Position += delta;
    }
}

void TeleportList(const Float3& delta, Array<DebugTriangle>& list)
{
    for (auto& v : list)
    {
        v.V0 += delta;
        v.V1 += delta;
        v.V2 += delta;
    }
}

void TeleportList(const Float3& delta, Array<DebugText3D>& list)
{
    for (auto& v : list)
    {
        v.Transform.Translation += delta;
    }
}

struct DebugDrawData
{
    Array<DebugGeometryBuffer> GeometryBuffers;
    Array<DebugLine> DefaultLines;
    Array<Vertex> OneFrameLines;
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
        return LinesCount() + TrianglesCount() + TextCount() + GeometryBuffers.Count();
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
        UpdateList(deltaTime, GeometryBuffers);
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

    void Teleport(const Float3& delta)
    {
        TeleportList(delta, DefaultLines);
        TeleportList(delta, OneFrameLines);
        TeleportList(delta, DefaultTriangles);
        TeleportList(delta, OneFrameTriangles);
        TeleportList(delta, DefaultWireTriangles);
        TeleportList(delta, OneFrameWireTriangles);
        TeleportList(delta, DefaultText3D);
        TeleportList(delta, OneFrameText3D);
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
    Vector3 Origin = Vector3::Zero;
    DebugDrawData DebugDrawDefault;
    DebugDrawData DebugDrawDepthTest;
    Float3 LastViewPos = Float3::Zero;
    Matrix LastViewProj = Matrix::Identity;
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
    Float3 CircleCache[DEBUG_DRAW_CIRCLE_VERTICES];
    Array<Float3> SphereTriangleCache;
    DebugSphereCache SphereCache[3];

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        DebugDrawPsLinesDefault.Release();
        DebugDrawPsLinesDepthTest.Release();
        DebugDrawPsWireTrianglesDefault.Release();
        DebugDrawPsWireTrianglesDepthTest.Release();
        DebugDrawPsTrianglesDefault.Release();
        DebugDrawPsTrianglesDepthTest.Release();
    }

#endif
};

extern int32 BoxTrianglesIndicesCache[];

int32 BoxLineIndicesCache[] =
{
	// @formatter:off
    0, 1,
	0, 3,
	0, 4,
	1, 2,
	1, 5,
	2, 3,
	2, 6,
	3, 7,
	4, 5,
	4, 7,
	5, 6,
	6, 7,
    // @formatter:on
};

struct DebugDrawCall
{
    int32 StartVertex;
    int32 VertexCount;
};

DebugDrawCall WriteList(int32& vertexCounter, const Array<Vertex>& list)
{
    DebugDrawCall drawCall;
    drawCall.StartVertex = vertexCounter;
    drawCall.VertexCount = list.Count();
    DebugDrawVB->Write(list.Get(), sizeof(Vertex) * drawCall.VertexCount);
    vertexCounter += drawCall.VertexCount;
    return drawCall;
}

DebugDrawCall WriteList(int32& vertexCounter, const Array<DebugLine>& list)
{
    DebugDrawCall drawCall;
    drawCall.StartVertex = vertexCounter;
    drawCall.VertexCount = list.Count() * 2;
    vertexCounter += drawCall.VertexCount;
    Vertex* dst = DebugDrawVB->WriteReserve<Vertex>(list.Count() * 2);
    for (int32 i = 0, j = 0; i < list.Count(); i++)
    {
        const DebugLine& l = list.Get()[i];
        dst[j++] = { l.Start, l.Color };
        dst[j++] = { l.End, l.Color };
    }
    return drawCall;
}

DebugDrawCall WriteList(int32& vertexCounter, const Array<DebugTriangle>& list)
{
    DebugDrawCall drawCall;
    drawCall.StartVertex = vertexCounter;
    drawCall.VertexCount = list.Count() * 3;
    vertexCounter += drawCall.VertexCount;
    Vertex* dst = DebugDrawVB->WriteReserve<Vertex>(list.Count() * 3);
    for (int32 i = 0, j = 0; i < list.Count(); i++)
    {
        const DebugTriangle& l = list.Get()[i];
        dst[j++] = { l.V0, l.Color };
        dst[j++] = { l.V1, l.Color };
        dst[j++] = { l.V2, l.Color };
    }
    return drawCall;
}

template<typename T, typename U>
DebugDrawCall WriteLists(int32& vertexCounter, const Array<T>& listA, const Array<U>& listB)
{
    const DebugDrawCall drawCallA = WriteList(vertexCounter, listA);
    const DebugDrawCall drawCallB = WriteList(vertexCounter, listB);
    DebugDrawCall drawCall;
    drawCall.StartVertex = drawCallA.StartVertex;
    drawCall.VertexCount = drawCallA.VertexCount + drawCallB.VertexCount;
    return drawCall;
}

FORCE_INLINE DebugTriangle* AppendTriangles(int32 count, float duration, bool depthTest)
{
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    const int32 startIndex = list->Count();
    list->AddUninitialized(count);
    return list->Get() + startIndex;
}

inline void DrawText3D(const DebugText3D& t, const RenderContext& renderContext, const Float3& viewUp, const Matrix& f, const Matrix& vp, const Viewport& viewport, GPUContext* context, GPUTextureView* target, GPUTextureView* depthBuffer)
{
    Matrix w, fw, m;
    if (t.FaceCamera)
    {
        Matrix s, ss;
        Matrix::Scaling(t.Transform.Scale.X, s);
        Matrix::CreateWorld(t.Transform.Translation, renderContext.View.Direction, viewUp, ss);
        Matrix::Multiply(s, ss, w);
    }
    else
        t.Transform.GetWorld(w);
    Matrix::Multiply(f, w, fw);
    Matrix::Multiply(fw, vp, m);
    Render2D::Begin(context, target, depthBuffer, viewport, m);
    const StringView text(t.Text.Get(), t.Text.Count() - 1);
    Render2D::DrawText(DebugDrawFont->CreateFont((float)t.Size), text, t.Color, Vector2::Zero);
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
    SphereCache[0].Init(DEBUG_DRAW_SPHERE_LOD0_RESOLUTION);
    SphereCache[1].Init(DEBUG_DRAW_SPHERE_LOD1_RESOLUTION);
    SphereCache[2].Init(DEBUG_DRAW_SPHERE_LOD2_RESOLUTION);

    // Init wireframe circle cache
    int32 index = 0;
    float step = TWO_PI / (float)DEBUG_DRAW_CIRCLE_RESOLUTION;
    for (float a = 0.0f; a < TWO_PI; a += step)
    {
        // Calculate sines and cosines
        float sinA = Math::Sin(a);
        float cosA = Math::Cos(a);
        float sinB = Math::Sin(a + step);
        float cosB = Math::Cos(a + step);

        CircleCache[index++] = Float3(cosA, sinA, 0.0f);
        CircleCache[index++] = Float3(cosB, sinB, 0.0f);
    }

    // Init triangle sphere cache
    {
        const int32 verticalSegments = DEBUG_DRAW_TRIANGLE_SPHERE_RESOLUTION;
        const int32 horizontalSegments = DEBUG_DRAW_TRIANGLE_SPHERE_RESOLUTION * 2;

        Array<Float3> vertices;
        vertices.Resize((verticalSegments + 1) * (horizontalSegments + 1));
        Array<int> indices;
        indices.Resize((verticalSegments) * (horizontalSegments + 1) * 6);

        int vertexCount = 0;

        // Generate the first extremity points
        for (int j = 0; j <= horizontalSegments; j++)
        {
            vertices[vertexCount++] = Float3(0, -1, 0);
        }

        // Create rings of vertices at progressively higher latitudes
        for (int i = 1; i < verticalSegments; i++)
        {
            const float latitude = ((i * PI / verticalSegments) - PI / 2.0f);
            const float dy = Math::Sin(latitude);
            const float dxz = Math::Cos(latitude);

            // The first point
            auto& firstHorizontalVertex = vertices[vertexCount++];
            firstHorizontalVertex = Float3(0, dy, dxz);

            // Create a single ring of vertices at this latitude
            for (int j = 1; j < horizontalSegments; j++)
            {
                const float longitude = (j * 2.0f * PI / horizontalSegments);
                const float dx = Math::Sin(longitude) * dxz;
                const float dz = Math::Cos(longitude) * dxz;

                vertices[vertexCount++] = Float3(dx, dy, dz);
            }

            // The last point equal to the first point
            vertices[vertexCount++] = firstHorizontalVertex;
        }

        // Generate the end extremity points
        for (int j = 0; j <= horizontalSegments; j++)
        {
            vertices[vertexCount++] = Float3(0, 1, 0);
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

    // Lazy-init resources
    if (DebugDrawShader == nullptr)
    {
        DebugDrawShader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/DebugDraw"));
        if (DebugDrawShader == nullptr)
        {
            LOG(Fatal, "Cannot load DebugDraw shader");
        }
#if COMPILE_WITH_DEV_ENV
        DebugDrawShader->OnReloading.Bind(&OnShaderReloading);
#endif
    }
    if (DebugDrawPsWireTrianglesDepthTest.Depth == nullptr && DebugDrawShader && DebugDrawShader->IsLoaded())
    {
        bool failed = false;
        const auto shader = DebugDrawShader->GetShader();

        // Create pipeline states
        GPUPipelineState::Description desc = GPUPipelineState::Description::Default;
        desc.BlendMode = BlendingMode::AlphaBlend;
        desc.CullMode = CullMode::TwoSided;
        desc.VS = shader->GetVS("VS");

        // Default
        desc.PS = shader->GetPS("PS", 0);
        desc.PrimitiveTopology = PrimitiveTopologyType::Line;
        failed |= DebugDrawPsLinesDefault.Create(desc);
        desc.PS = shader->GetPS("PS", 1);
        desc.PrimitiveTopology = PrimitiveTopologyType::Triangle;
        failed |= DebugDrawPsTrianglesDefault.Create(desc);
        desc.Wireframe = true;
        failed |= DebugDrawPsWireTrianglesDefault.Create(desc);

        // Depth Test
        desc.Wireframe = false;
        desc.PS = shader->GetPS("PS", 2);
        desc.PrimitiveTopology = PrimitiveTopologyType::Line;
        failed |= DebugDrawPsLinesDepthTest.Create(desc);
        desc.PS = shader->GetPS("PS", 3);
        desc.PrimitiveTopology = PrimitiveTopologyType::Triangle;
        failed |= DebugDrawPsTrianglesDepthTest.Create(desc);
        desc.Wireframe = true;
        failed |= DebugDrawPsWireTrianglesDepthTest.Create(desc);

        if (failed)
        {
            LOG(Fatal, "Cannot setup DebugDraw service!");
        }
    }

    // Vertex buffer
    if (DebugDrawVB == nullptr)
        DebugDrawVB = New<DynamicVertexBuffer>((uint32)(DEBUG_DRAW_INITIAL_VB_CAPACITY * sizeof(Vertex)), (uint32)sizeof(Vertex), TEXT("DebugDraw.VB"));
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
    ASSERT(context);
    Memory::DestructItem((DebugDrawContext*)context);
    Allocator::Free(context);
}

void DebugDraw::UpdateContext(void* context, float deltaTime)
{
    if (!context)
        context = &GlobalContext;
    ((DebugDrawContext*)context)->DebugDrawDefault.Update(deltaTime);
    ((DebugDrawContext*)context)->DebugDrawDepthTest.Update(deltaTime);
}

void DebugDraw::SetContext(void* context)
{
    Context = context ? (DebugDrawContext*)context : &GlobalContext;
}

#endif

Vector3 DebugDraw::GetViewPos()
{
    return Context->LastViewPos;
}

void DebugDraw::Draw(RenderContext& renderContext, GPUTextureView* target, GPUTextureView* depthBuffer, bool enableDepthTest)
{
    PROFILE_GPU_CPU("Debug Draw");

    // Ensure to have shader loaded and any lines to render
    const int32 debugDrawDepthTestCount = Context->DebugDrawDepthTest.Count();
    const int32 debugDrawDefaultCount = Context->DebugDrawDefault.Count();
    if (DebugDrawShader == nullptr || !DebugDrawShader->IsLoaded() || debugDrawDepthTestCount + debugDrawDefaultCount == 0 || DebugDrawPsWireTrianglesDepthTest.Depth == nullptr)
        return;
    if (renderContext.Buffers == nullptr || !DebugDrawVB)
        return;
    auto context = GPUDevice::Instance->GetMainContext();
    const RenderView& view = renderContext.View;
    if (Context->Origin != view.Origin)
    {
        // Teleport existing debug shapes to maintain their location
        Float3 delta = Context->Origin - view.Origin;
        Context->DebugDrawDefault.Teleport(delta);
        Context->DebugDrawDepthTest.Teleport(delta);
        Context->Origin = view.Origin;
    }
    Context->LastViewPos = view.Position;
    Context->LastViewProj = view.Projection;
    TaaJitterRemoveContext taaJitterRemove(view);

    // Fallback to task buffers
    if (target == nullptr && renderContext.Task)
        target = renderContext.Task->GetOutputView();

    // Fill vertex buffer and upload data
    DebugDrawCall depthTestLines, defaultLines, depthTestTriangles, defaultTriangles, depthTestWireTriangles, defaultWireTriangles;
    {
        PROFILE_CPU_NAMED("Update Buffer");
        DebugDrawVB->Clear();
        int32 vertexCounter = 0;
        depthTestLines = WriteLists(vertexCounter, Context->DebugDrawDepthTest.DefaultLines, Context->DebugDrawDepthTest.OneFrameLines);
        defaultLines = WriteLists(vertexCounter, Context->DebugDrawDefault.DefaultLines, Context->DebugDrawDefault.OneFrameLines);
        depthTestTriangles = WriteLists(vertexCounter, Context->DebugDrawDepthTest.DefaultTriangles, Context->DebugDrawDepthTest.OneFrameTriangles);
        defaultTriangles = WriteLists(vertexCounter, Context->DebugDrawDefault.DefaultTriangles, Context->DebugDrawDefault.OneFrameTriangles);
        depthTestWireTriangles = WriteLists(vertexCounter, Context->DebugDrawDepthTest.DefaultWireTriangles, Context->DebugDrawDepthTest.OneFrameWireTriangles);
        defaultWireTriangles = WriteLists(vertexCounter, Context->DebugDrawDefault.DefaultWireTriangles, Context->DebugDrawDefault.OneFrameWireTriangles);
        {
            PROFILE_CPU_NAMED("Flush");
            DebugDrawVB->Flush(context);
        }
    }

    // Update constant buffer
    const auto cb = DebugDrawShader->GetShader()->GetCB(0);
    ShaderData data;
    Matrix vp;
    Matrix::Multiply(view.View, view.Projection, vp);
    Matrix::Transpose(vp, data.ViewProjection);
    data.ClipPosZBias = view.IsPerspectiveProjection() ? -0.2f : 0.0f; // Reduce Z-fighting artifacts (eg. editor grid)
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

        context->SetRenderTarget(depthBuffer ? depthBuffer : (data.EnableDepthTest ? nullptr : renderContext.Buffers->DepthBuffer->View()), target);

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

        // Geometries
        for (auto& geometry : Context->DebugDrawDepthTest.GeometryBuffers)
        {
            auto tmp = data;
            Matrix mvp;
            Matrix::Multiply(geometry.Transform, vp, mvp);
            Matrix::Transpose(mvp, tmp.ViewProjection);
            context->UpdateCB(cb, &tmp);
            auto state = data.EnableDepthTest ? &DebugDrawPsLinesDepthTest : &DebugDrawPsLinesDefault;
            context->SetState(state->Get(enableDepthWrite, true));
            context->BindVB(ToSpan(&geometry.Buffer, 1));
            context->Draw(0, geometry.Buffer->GetElementsCount());
        }
        if (Context->DebugDrawDepthTest.GeometryBuffers.HasItems())
            context->UpdateCB(cb, &data);

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

        // Geometries
        for (auto& geometry : Context->DebugDrawDefault.GeometryBuffers)
        {
            auto tmp = data;
            Matrix mvp;
            Matrix::Multiply(geometry.Transform, vp, mvp);
            Matrix::Transpose(mvp, tmp.ViewProjection);
            context->UpdateCB(cb, &tmp);
            context->SetState(DebugDrawPsLinesDefault.Get(false, false));
            context->BindVB(ToSpan(&geometry.Buffer, 1));
            context->Draw(0, geometry.Buffer->GetElementsCount());
        }
    }

    // Text
    if (Context->DebugDrawDefault.TextCount() + Context->DebugDrawDepthTest.TextCount())
    {
        PROFILE_GPU_CPU_NAMED("Text");
        auto features = Render2D::Features;

        // Disable vertex snapping when rendering 3D text
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
                    Render2D::DrawText(DebugDrawFont->CreateFont((float)t.Size), text, t.Color, t.Position);
                }
                for (auto& t : Context->DebugDrawDefault.OneFrameText2D)
                {
                    const StringView text(t.Text.Get(), t.Text.Count() - 1);
                    Render2D::DrawText(DebugDrawFont->CreateFont((float)t.Size), text, t.Color, t.Position);
                }
                Render2D::End();
            }

            if (Context->DebugDrawDefault.DefaultText3D.Count() + Context->DebugDrawDefault.OneFrameText3D.Count())
            {
                Matrix f;
                Matrix::RotationZ(PI, f);
                Float3 viewUp;
                Float3::Transform(Float3::Up, Quaternion::LookRotation(view.Direction, Float3::Up), viewUp);
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
    bool DrawActorsTreeWalk(Actor* a)
    {
        if (a->IsActiveInHierarchy())
        {
            a->OnDebugDraw();
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

void DebugDraw::DrawActorsTree(Actor* actor)
{
    Function<bool(Actor*)> function = &DrawActorsTreeWalk;
    actor->TreeExecute(function);
}

#if USE_EDITOR

void DebugDraw::DrawColliderDebugPhysics(Collider* collider, RenderView& view)
{
    if (!collider)
        return;
    collider->DrawPhysicsDebug(view);
}

void DebugDraw::DrawLightDebug(Light* light, RenderView& view)
{
    if (!light)
        return;
    light->DrawLightsDebug(view);
}

#endif

void DebugDraw::DrawAxisFromDirection(const Vector3& origin, const Vector3& direction, float size, float duration, bool depthTest)
{
    CHECK_DEBUG(direction.IsNormalized());
    const auto rot = Quaternion::FromDirection(direction);
    const Vector3 up = (rot * Vector3::Up);
    const Vector3 forward = (rot * Vector3::Forward);
    const Vector3 right = (rot * Vector3::Right);
    const float sizeHalf = size * 0.5f;
    DrawLine(origin, origin + up * sizeHalf + up, Color::Green, duration, depthTest);
    DrawLine(origin, origin + forward * sizeHalf + forward, Color::Blue, duration, depthTest);
    DrawLine(origin, origin + right * sizeHalf + right, Color::Red, duration, depthTest);
}

void DebugDraw::DrawDirection(const Vector3& origin, const Vector3& direction, const Color& color, float duration, bool depthTest)
{
    auto dir = origin + direction;
    if (dir.IsNanOrInfinity())
        return;
    DrawLine(origin, origin + direction, color, duration, depthTest);
}

void DebugDraw::DrawRay(const Vector3& origin, const Vector3& direction, const Color& color, float duration, bool depthTest)
{
    DrawLine(origin, origin + direction, color, duration, depthTest);
}

void DebugDraw::DrawRay(const Vector3& origin, const Vector3& direction, const Color& color, float length, float duration, bool depthTest)
{
    CHECK_DEBUG(direction.IsNormalized());
    if (isnan(length) || isinf(length))
        return;
    DrawLine(origin, origin + (direction * length), color, duration, depthTest);
}

void DebugDraw::DrawRay(const Ray& ray, const Color& color, float length, float duration, bool depthTest)
{
    if (isnan(length) || isinf(length))
        return;
    DrawLine(ray.Position, ray.Position + (ray.Direction * length), color, duration, depthTest);
}

void DebugDraw::DrawLine(const Vector3& start, const Vector3& end, const Color& color, float duration, bool depthTest)
{
    const Float3 startF = start - Context->Origin, endF = end - Context->Origin;
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        DebugLine l = { startF, endF, Color32(color), duration };
        debugDrawData.DefaultLines.Add(l);
    }
    else
    {
        Vertex l = { startF, Color32(color) };
        debugDrawData.OneFrameLines.Add(l);
        l.Position = endF;
        debugDrawData.OneFrameLines.Add(l);
    }
}

void DebugDraw::DrawLine(const Vector3& start, const Vector3& end, const Color& startColor, const Color& endColor, float duration, bool depthTest)
{
    const Float3 startF = start - Context->Origin, endF = end - Context->Origin;
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        // TODO: separate start/end colors for persistent lines
        DebugLine l = { startF, endF, Color32(startColor), duration };
        debugDrawData.DefaultLines.Add(l);
    }
    else
    {
        Vertex l = { startF, Color32(startColor) };
        debugDrawData.OneFrameLines.Add(l);
        l.Position = endF;
        l.Color = Color32(endColor);
        debugDrawData.OneFrameLines.Add(l);
    }
}

void DebugDraw::DrawLines(const Span<Float3>& lines, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    if (lines.Length() == 0)
        return;
    if (lines.Length() % 2 != 0)
    {
        DebugLog::ThrowException("Cannot draw debug lines with uneven amount of items in array");
        return;
    }

    // Draw lines
    const Float3* p = lines.Get();
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    const Matrix transformF = transform * Matrix::Translation(-Context->Origin);
    if (duration > 0)
    {
        DebugLine l = { Float3::Zero, Float3::Zero, Color32(color), duration };
        debugDrawData.DefaultLines.EnsureCapacity(debugDrawData.DefaultLines.Count() + lines.Length());
        for (int32 i = 0; i < lines.Length(); i += 2)
        {
            Float3::Transform(*p++, transformF, l.Start);
            Float3::Transform(*p++, transformF, l.End);
            debugDrawData.DefaultLines.Add(l);
        }
    }
    else
    {
        Vertex l = { Float3::Zero, Color32(color) };
        debugDrawData.OneFrameLines.EnsureCapacity(debugDrawData.OneFrameLines.Count() + lines.Length() * 2);
        for (int32 i = 0; i < lines.Length(); i += 2)
        {
            Float3::Transform(*p++, transformF, l.Position);
            debugDrawData.OneFrameLines.Add(l);
            Float3::Transform(*p++, transformF, l.Position);
            debugDrawData.OneFrameLines.Add(l);
        }
    }
}

void DebugDraw::DrawLines(GPUBuffer* lines, const Matrix& transform, float duration, bool depthTest)
{
    if (lines == nullptr || lines->GetSize() == 0)
        return;
    if (lines->GetSize() % (sizeof(Vertex) * 2) != 0)
    {
        DebugLog::ThrowException("Cannot draw debug lines with uneven amount of items in array");
        return;
    }

    // Draw lines
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    auto& geometry = debugDrawData.GeometryBuffers.AddOne();
    geometry.Buffer = lines;
    geometry.TimeLeft = duration;
    geometry.Transform = transform * Matrix::Translation(-Context->Origin);
}

void DebugDraw::DrawLines(const Array<Float3>& lines, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    DrawLines(Span<Float3>(lines.Get(), lines.Count()), transform, color, duration, depthTest);
}

void DebugDraw::DrawLines(const Span<Double3>& lines, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    if (lines.Length() == 0)
        return;
    if (lines.Length() % 2 != 0)
    {
        DebugLog::ThrowException("Cannot draw debug lines with uneven amount of items in array");
        return;
    }

    // Draw lines
    const Double3* p = lines.Get();
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    const Matrix transformF = transform * Matrix::Translation(-Context->Origin);
    if (duration > 0)
    {
        DebugLine l = { Float3::Zero, Float3::Zero, Color32(color), duration };
        debugDrawData.DefaultLines.EnsureCapacity(debugDrawData.DefaultLines.Count() + lines.Length());
        for (int32 i = 0; i < lines.Length(); i += 2)
        {
            Float3::Transform(*p++, transformF, l.Start);
            Float3::Transform(*p++, transformF, l.End);
            debugDrawData.DefaultLines.Add(l);
        }
    }
    else
    {
        Vertex l = { Float3::Zero, Color32(color) };
        debugDrawData.OneFrameLines.EnsureCapacity(debugDrawData.OneFrameLines.Count() + lines.Length() * 2);
        for (int32 i = 0; i < lines.Length(); i += 2)
        {
            Float3::Transform(*p++, transformF, l.Position);
            debugDrawData.OneFrameLines.Add(l);
            Float3::Transform(*p++, transformF, l.Position);
            debugDrawData.OneFrameLines.Add(l);
        }
    }
}

void DebugDraw::DrawLines(const Array<Double3, HeapAllocation>& lines, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    DrawLines(Span<Double3>(lines.Get(), lines.Count()), transform, color, duration, depthTest);
}

void DebugDraw::DrawBezier(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, const Color& color, float duration, bool depthTest)
{
    const Float3 p1F = p1 - Context->Origin, p2F = p2 - Context->Origin, p3F = p3 - Context->Origin, p4F = p4 - Context->Origin;

    // Find amount of segments to use
    const Float3 d1 = p2F - p1F;
    const Float3 d2 = p3F - p2F;
    const Float3 d3 = p4F - p3F;
    const float len = d1.Length() + d2.Length() + d3.Length();
    const int32 segmentCount = Math::Clamp(Math::CeilToInt(len * 0.05f), 1, 100);
    const float segmentCountInv = 1.0f / (float)segmentCount;

    // Draw segmented curve from lines
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        DebugLine l = { p1F, Float3::Zero, Color32(color), duration };
        debugDrawData.DefaultLines.EnsureCapacity(debugDrawData.DefaultLines.Count() + segmentCount + 2);
        for (int32 i = 0; i <= segmentCount; i++)
        {
            const float t = (float)i * segmentCountInv;
            Float3 end;
            AnimationUtils::Bezier(p1F, p2F, p3F, p4F, t, end);
            l.End = end;
            debugDrawData.DefaultLines.Add(l);
            l.Start = l.End;
        }
    }
    else
    {
        Vertex l = { p1F, Color32(color) };
        debugDrawData.OneFrameLines.EnsureCapacity(debugDrawData.OneFrameLines.Count() + segmentCount * 2 + 4);
        for (int32 i = 0; i <= segmentCount; i++)
        {
            const float t = (float)i * segmentCountInv;
            debugDrawData.OneFrameLines.Add(l);
            Float3 position;
            AnimationUtils::Bezier(p1F, p2F, p3F, p4F, t, position);
            l.Position = position;
            debugDrawData.OneFrameLines.Add(l);
        }
    }
}

void DebugDraw::DrawWireBox(const BoundingBox& box, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    box.GetCorners(corners);
    for (Vector3& c : corners)
        c -= Context->Origin;

    // Draw lines
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        DebugLine l = { Float3::Zero, Float3::Zero, Color32(color), duration };
        for (uint32 i = 0; i < ARRAY_COUNT(BoxLineIndicesCache);)
        {
            l.Start = corners[BoxLineIndicesCache[i++]];
            l.End = corners[BoxLineIndicesCache[i++]];
            debugDrawData.DefaultLines.Add(l);
        }
    }
    else
    {
        // TODO: draw lines as strips to reuse vertices with a single draw call
        Vertex l = { Float3::Zero, Color32(color) };
        for (uint32 i = 0; i < ARRAY_COUNT(BoxLineIndicesCache);)
        {
            l.Position = corners[BoxLineIndicesCache[i++]];
            debugDrawData.OneFrameLines.Add(l);
            l.Position = corners[BoxLineIndicesCache[i++]];
            debugDrawData.OneFrameLines.Add(l);
        }
    }
}

void DebugDraw::DrawWireFrustum(const BoundingFrustum& frustum, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    frustum.GetCorners(corners);
    for (Vector3& c : corners)
        c -= Context->Origin;

    // Draw lines
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        DebugLine l = { Float3::Zero, Float3::Zero, Color32(color), duration };
        for (uint32 i = 0; i < ARRAY_COUNT(BoxLineIndicesCache);)
        {
            l.Start = corners[BoxLineIndicesCache[i++]];
            l.End = corners[BoxLineIndicesCache[i++]];
            debugDrawData.DefaultLines.Add(l);
        }
    }
    else
    {
        // TODO: draw lines as strips to reuse vertices with a single draw call
        Vertex l = { Float3::Zero, Color32(color) };
        for (uint32 i = 0; i < ARRAY_COUNT(BoxLineIndicesCache);)
        {
            l.Position = corners[BoxLineIndicesCache[i++]];
            debugDrawData.OneFrameLines.Add(l);
            l.Position = corners[BoxLineIndicesCache[i++]];
            debugDrawData.OneFrameLines.Add(l);
        }
    }
}

void DebugDraw::DrawWireBox(const OrientedBoundingBox& box, const Color& color, float duration, bool depthTest)
{
    // Get corners
    Vector3 corners[8];
    box.GetCorners(corners);
    for (Vector3& c : corners)
        c -= Context->Origin;

    // Draw lines
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        DebugLine l = { Float3::Zero, Float3::Zero, Color32(color), duration };
        for (uint32 i = 0; i < ARRAY_COUNT(BoxLineIndicesCache);)
        {
            l.Start = corners[BoxLineIndicesCache[i++]];
            l.End = corners[BoxLineIndicesCache[i++]];
            debugDrawData.DefaultLines.Add(l);
        }
    }
    else
    {
        // TODO: draw lines as strips to reuse vertices with a single draw call
        Vertex l = { Float3::Zero, Color32(color) };
        for (uint32 i = 0; i < ARRAY_COUNT(BoxLineIndicesCache);)
        {
            l.Position = corners[BoxLineIndicesCache[i++]];
            debugDrawData.OneFrameLines.Add(l);
            l.Position = corners[BoxLineIndicesCache[i++]];
            debugDrawData.OneFrameLines.Add(l);
        }
    }
}

void DebugDraw::DrawWireSphere(const BoundingSphere& sphere, const Color& color, float duration, bool depthTest)
{
    // Select LOD
    int32 index;
    const Float3 centerF = sphere.Center - Context->Origin;
    const float radiusF = (float)sphere.Radius;
    const float screenRadiusSquared = RenderTools::ComputeBoundsScreenRadiusSquared(centerF, radiusF, Context->LastViewPos, Context->LastViewProj);
    if (screenRadiusSquared > DEBUG_DRAW_SPHERE_LOD0_SCREEN_SIZE * DEBUG_DRAW_SPHERE_LOD0_SCREEN_SIZE * 0.25f)
        index = 0;
    else if (screenRadiusSquared > DEBUG_DRAW_SPHERE_LOD1_SCREEN_SIZE * DEBUG_DRAW_SPHERE_LOD1_SCREEN_SIZE * 0.25f)
        index = 1;
    else
        index = 2;
    auto& cache = SphereCache[index];

    // Draw lines of the unit sphere after linear transform
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    if (duration > 0)
    {
        DebugLine l = { Float3::Zero, Float3::Zero, Color32(color), duration };
        for (int32 i = 0; i < cache.Vertices.Count();)
        {
            l.Start = centerF + cache.Vertices.Get()[i++] * radiusF;
            l.End = centerF + cache.Vertices.Get()[i++] * radiusF;
            debugDrawData.DefaultLines.Add(l);
        }
    }
    else
    {
        Vertex l = { Float3::Zero, Color32(color) };
        for (int32 i = 0; i < cache.Vertices.Count();)
        {
            l.Position = centerF + cache.Vertices.Get()[i++] * radiusF;
            debugDrawData.OneFrameLines.Add(l);
            l.Position = centerF + cache.Vertices.Get()[i++] * radiusF;
            debugDrawData.OneFrameLines.Add(l);
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

    const Float3 centerF = sphere.Center - Context->Origin;
    const float radiusF = (float)sphere.Radius;
    for (int32 i = 0; i < SphereTriangleCache.Count();)
    {
        t.V0 = centerF + SphereTriangleCache[i++] * radiusF;
        t.V1 = centerF + SphereTriangleCache[i++] * radiusF;
        t.V2 = centerF + SphereTriangleCache[i++] * radiusF;
        list->Add(t);
    }
}

void DebugDraw::DrawCircle(const Vector3& position, const Float3& normal, float radius, const Color& color, float duration, bool depthTest)
{
    // Create matrix transform for unit circle points
    Matrix world, scale, matrix;
    Float3 right, up;
    if (Float3::Dot(normal, Float3::Up) > 0.99f)
        right = Float3::Right;
    else if (Float3::Dot(normal, Float3::Down) > 0.99f)
        right = Float3::Left;
    else
        Float3::Cross(normal, Float3::Up, right);
    Float3::Cross(right, normal, up);
    Matrix::Scaling(radius, scale);
    const Float3 positionF = position - Context->Origin;
    Matrix::CreateWorld(positionF, normal, up, world);
    Matrix::Multiply(scale, world, matrix);

    // Draw lines of the unit circle after linear transform
    Float3 prev = Float3::Transform(CircleCache[0], matrix);
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
    for (int32 i = 1; i < DEBUG_DRAW_CIRCLE_VERTICES;)
    {
        Float3 cur = Float3::Transform(CircleCache[i++], matrix);
        if (duration > 0)
        {
            DebugLine l = { prev, cur, Color32(color), duration };
            debugDrawData.DefaultLines.Add(l);
        }
        else
        {
            Vertex l = { prev, Color32(color) };
            debugDrawData.OneFrameLines.Add(l);
            l.Position = cur;
            debugDrawData.OneFrameLines.Add(l);
        }
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
    t.V0 = v0 - Context->Origin;
    t.V1 = v1 - Context->Origin;
    t.V2 = v2 - Context->Origin;
    if (depthTest)
        Context->DebugDrawDepthTest.Add(t);
    else
        Context->DebugDrawDefault.Add(t);
}

void DebugDraw::DrawTriangles(const Span<Float3>& vertices, const Color& color, float duration, bool depthTest)
{
    CHECK(vertices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(vertices.Length() / 3, duration, depthTest);
    const Float3 origin = Context->Origin;
    for (int32 i = 0; i < vertices.Length();)
    {
        t.V0 = vertices.Get()[i++] - origin;
        t.V1 = vertices.Get()[i++] - origin;
        t.V2 = vertices.Get()[i++] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Span<Float3>& vertices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    CHECK(vertices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(vertices.Length() / 3, duration, depthTest);
    const Matrix transformF = transform * Matrix::Translation(-Context->Origin);
    for (int32 i = 0; i < vertices.Length();)
    {
        Float3::Transform(vertices.Get()[i++], transformF, t.V0);
        Float3::Transform(vertices.Get()[i++], transformF, t.V1);
        Float3::Transform(vertices.Get()[i++], transformF, t.V2);
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Array<Float3>& vertices, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Float3>(vertices.Get(), vertices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Array<Float3>& vertices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Float3>(vertices.Get(), vertices.Count()), transform, color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Span<Float3>& vertices, const Span<int32>& indices, const Color& color, float duration, bool depthTest)
{
    CHECK(indices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(indices.Length() / 3, duration, depthTest);
    const Float3 origin = Context->Origin;
    for (int32 i = 0; i < indices.Length();)
    {
        t.V0 = vertices[indices.Get()[i++]] - origin;
        t.V1 = vertices[indices.Get()[i++]] - origin;
        t.V2 = vertices[indices.Get()[i++]] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Span<Float3>& vertices, const Span<int32>& indices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    CHECK(indices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(indices.Length() / 3, duration, depthTest);
    const Matrix transformF = transform * Matrix::Translation(-Context->Origin);
    for (int32 i = 0; i < indices.Length();)
    {
        Float3::Transform(vertices[indices.Get()[i++]], transformF, t.V0);
        Float3::Transform(vertices[indices.Get()[i++]], transformF, t.V1);
        Float3::Transform(vertices[indices.Get()[i++]], transformF, t.V2);
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Array<Float3>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Float3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Array<Float3>& vertices, const Array<int32, HeapAllocation>& indices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Float3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), transform, color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Span<Double3>& vertices, const Color& color, float duration, bool depthTest)
{
    CHECK(vertices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(vertices.Length() / 3, duration, depthTest);
    const Double3 origin = Context->Origin;
    for (int32 i = 0; i < vertices.Length();)
    {
        t.V0 = vertices.Get()[i++] - origin;
        t.V1 = vertices.Get()[i++] - origin;
        t.V2 = vertices.Get()[i++] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Span<Double3>& vertices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    CHECK(vertices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(vertices.Length() / 3, duration, depthTest);
    const Matrix transformF = transform * Matrix::Translation(-Context->Origin);
    for (int32 i = 0; i < vertices.Length();)
    {
        Float3::Transform(vertices.Get()[i++], transformF, t.V0);
        Float3::Transform(vertices.Get()[i++], transformF, t.V1);
        Float3::Transform(vertices.Get()[i++], transformF, t.V2);
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Array<Double3>& vertices, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Double3>(vertices.Get(), vertices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Array<Double3>& vertices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Double3>(vertices.Get(), vertices.Count()), transform, color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Span<Double3>& vertices, const Span<int32>& indices, const Color& color, float duration, bool depthTest)
{
    CHECK(indices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(indices.Length() / 3, duration, depthTest);
    const Double3 origin = Context->Origin;
    for (int32 i = 0; i < indices.Length();)
    {
        t.V0 = vertices[indices.Get()[i++]] - origin;
        t.V1 = vertices[indices.Get()[i++]] - origin;
        t.V2 = vertices[indices.Get()[i++]] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Span<Double3>& vertices, const Span<int32>& indices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    CHECK(indices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(indices.Length() / 3, duration, depthTest);
    const Matrix transformF = transform * Matrix::Translation(-Context->Origin);
    for (int32 i = 0; i < indices.Length();)
    {
        Float3::Transform(vertices[indices.Get()[i++]], transformF, t.V0);
        Float3::Transform(vertices[indices.Get()[i++]], transformF, t.V1);
        Float3::Transform(vertices[indices.Get()[i++]], transformF, t.V2);
        *dst++ = t;
    }
}

void DebugDraw::DrawTriangles(const Array<Double3>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Double3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawTriangles(const Array<Double3>& vertices, const Array<int32, HeapAllocation>& indices, const Matrix& transform, const Color& color, float duration, bool depthTest)
{
    DrawTriangles(Span<Double3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), transform, color, duration, depthTest);
}

void DebugDraw::DrawWireTriangles(const Span<Float3>& vertices, const Color& color, float duration, bool depthTest)
{
    CHECK(vertices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(vertices.Length() / 3, duration, depthTest);
    const Float3 origin = Context->Origin;
    for (int32 i = 0; i < vertices.Length();)
    {
        t.V0 = vertices.Get()[i++] - origin;
        t.V1 = vertices.Get()[i++] - origin;
        t.V2 = vertices.Get()[i++] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawWireTriangles(const Array<Float3>& vertices, const Color& color, float duration, bool depthTest)
{
    DrawWireTriangles(Span<Float3>(vertices.Get(), vertices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawWireTriangles(const Span<Float3>& vertices, const Span<int32>& indices, const Color& color, float duration, bool depthTest)
{
    CHECK(indices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(indices.Length() / 3, duration, depthTest);
    const Float3 origin = Context->Origin;
    for (int32 i = 0; i < indices.Length();)
    {
        t.V0 = vertices[indices.Get()[i++]] - origin;
        t.V1 = vertices[indices.Get()[i++]] - origin;
        t.V2 = vertices[indices.Get()[i++]] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawWireTriangles(const Array<Float3>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color, float duration, bool depthTest)
{
    DrawWireTriangles(Span<Float3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawWireTriangles(const Span<Double3>& vertices, const Color& color, float duration, bool depthTest)
{
    CHECK(vertices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(vertices.Length() / 3, duration, depthTest);
    const Double3 origin = Context->Origin;
    for (int32 i = 0; i < vertices.Length();)
    {
        t.V0 = vertices.Get()[i++] - origin;
        t.V1 = vertices.Get()[i++] - origin;
        t.V2 = vertices.Get()[i++] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawWireTriangles(const Array<Double3>& vertices, const Color& color, float duration, bool depthTest)
{
    DrawWireTriangles(Span<Double3>(vertices.Get(), vertices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawWireTriangles(const Span<Double3>& vertices, const Span<int32>& indices, const Color& color, float duration, bool depthTest)
{
    CHECK(indices.Length() % 3 == 0);
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    auto dst = AppendTriangles(indices.Length() / 3, duration, depthTest);
    const Double3 origin = Context->Origin;
    for (int32 i = 0; i < indices.Length();)
    {
        t.V0 = vertices[indices.Get()[i++]] - origin;
        t.V1 = vertices[indices.Get()[i++]] - origin;
        t.V2 = vertices[indices.Get()[i++]] - origin;
        *dst++ = t;
    }
}

void DebugDraw::DrawWireTriangles(const Array<Double3>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color, float duration, bool depthTest)
{
    DrawWireTriangles(Span<Double3>(vertices.Get(), vertices.Count()), Span<int32>(indices.Get(), indices.Count()), color, duration, depthTest);
}

void DebugDraw::DrawTube(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color, float duration, bool depthTest)
{
    DrawCapsule(position, orientation, radius, length, color, duration, depthTest);
}

void DebugDraw::DrawCapsule(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color, float duration, bool depthTest)
{
    // Check if has no length (just sphere)
    if (length < ZeroTolerance)
    {
        DrawSphere(BoundingSphere(position, radius), color, duration, depthTest);
    }
    else
    {
        const Float3 dir = orientation * Float3::Forward;
        radius = Math::Max(radius, 0.05f);
        length = Math::Max(length, 0.05f);
        const float halfLength = length / 2.0f;
        DrawSphere(BoundingSphere(position + dir * halfLength, radius), color, duration, depthTest);
        DrawSphere(BoundingSphere(position - dir * halfLength, radius), color, duration, depthTest);
        DrawCylinder(position, orientation * Quaternion::Euler(90.0f, 0, 0), radius, length, color, duration, depthTest);
    }
}

void DebugDraw::DrawWireTube(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color, float duration, bool depthTest)
{
    DrawWireCapsule(position, orientation, radius, length, color, duration, depthTest);
}

void DebugDraw::DrawWireCapsule(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color, float duration, bool depthTest)
{
    // Check if has no length (just sphere)
    if (length < ZeroTolerance)
    {
        DrawWireSphere(BoundingSphere(position, radius), color, duration, depthTest);
    }
    else
    {
        // Set up
        const int32 resolution = 64;
        const float step = TWO_PI / (float)resolution;
        radius = Math::Max(radius, 0.05f);
        length = Math::Max(length, 0.05f);
        const float halfLength = length / 2.0f;
        Matrix rotation, translation, world;
        Matrix::RotationQuaternion(orientation, rotation);
        const Float3 positionF = position - Context->Origin;
        Matrix::Translation(positionF, translation);
        Matrix::Multiply(rotation, translation, world);

        // Write vertices
        auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
        Color32 color32(color);
        if (duration > 0)
        {
#define DRAW_WIRE_BOX_LINE(x1, y1, z1, x2, y2, z2) debugDrawData.DefaultLines.Add({ Float3::Transform(Float3(x1, y1, z1), world), Float3::Transform(Float3(x2, y2, z2), world), color32, duration });
            for (float a = 0.0f; a < TWO_PI; a += step)
            {
                // Calculate sines and cosines
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
        else
        {
#define DRAW_WIRE_BOX_LINE(x1, y1, z1, x2, y2, z2) debugDrawData.OneFrameLines.Add({ Float3::Transform(Float3(x1, y1, z1), world), color32 }); debugDrawData.OneFrameLines.Add({ Float3::Transform(Float3(x2, y2, z2), world), color32 });
            for (float a = 0.0f; a < TWO_PI; a += step)
            {
                // Calculate sines and cosines
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
#undef DRAW_WIRE_BOX_LINE
            }
        }
    }
}

namespace
{
    void DrawCylinder(Array<DebugTriangle>* list, const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color, float duration)
    {
        // Setup cache
        Float3 CylinderCache[DEBUG_DRAW_CYLINDER_VERTICES];
        const float angleBetweenFacets = TWO_PI / DEBUG_DRAW_CYLINDER_RESOLUTION;
        const float verticalOffset = height * 0.5f;
        int32 index = 0;
        for (int32 i = 0; i < DEBUG_DRAW_CYLINDER_RESOLUTION; i++)
        {
            const float theta = i * angleBetweenFacets;
            const float x = Math::Cos(theta) * radius;
            const float z = Math::Sin(theta) * radius;

            // Top cap
            CylinderCache[index++] = Float3(x, verticalOffset, z);

            // Top part of body
            CylinderCache[index++] = Float3(x, verticalOffset, z);

            // Bottom part of body
            CylinderCache[index++] = Float3(x, -verticalOffset, z);

            // Bottom cap
            CylinderCache[index++] = Float3(x, -verticalOffset, z);
        }

        DebugTriangle t;
        t.Color = Color32(color);
        t.TimeLeft = duration;
        const Float3 positionF = position - Context->Origin;
        const Matrix world = Matrix::RotationQuaternion(orientation) * Matrix::Translation(positionF);

        // Write triangles
        for (uint32 i = 0; i < DEBUG_DRAW_CYLINDER_VERTICES; i += 4)
        {
            // Each iteration, the loop advances to the next vertex column
            // Four triangles per column (except for the four degenerate cap triangles)

            // Top cap triangles
            auto nextIndex = (uint16)((i + 4) % DEBUG_DRAW_CYLINDER_VERTICES);
            if (nextIndex != 0)
            {
                Float3::Transform(CylinderCache[i], world, t.V0);
                Float3::Transform(CylinderCache[nextIndex], world, t.V1);
                Float3::Transform(CylinderCache[0], world, t.V2);
                list->Add(t);
            }

            // Body triangles
            nextIndex = (uint16)((i + 5) % DEBUG_DRAW_CYLINDER_VERTICES);
            Float3::Transform(CylinderCache[(i + 1)], world, t.V0);
            Float3::Transform(CylinderCache[(i + 2)], world, t.V1);
            Float3::Transform(CylinderCache[nextIndex], world, t.V2);
            list->Add(t);

            Float3::Transform(CylinderCache[nextIndex], world, t.V0);
            Float3::Transform(CylinderCache[(i + 2)], world, t.V1);
            Float3::Transform(CylinderCache[((i + 6) % DEBUG_DRAW_CYLINDER_VERTICES)], world, t.V2);
            list->Add(t);

            // Bottom cap triangles
            nextIndex = (uint16)((i + 7) % DEBUG_DRAW_CYLINDER_VERTICES);
            if (nextIndex != 3)
            {
                Float3::Transform(CylinderCache[(i + 3)], world, t.V0);
                Float3::Transform(CylinderCache[3], world, t.V1);
                Float3::Transform(CylinderCache[nextIndex], world, t.V2);
                list->Add(t);
            }
        }
    }

    void DrawCone(Array<DebugTriangle>* list, const Vector3& position, const Quaternion& orientation, float radius, float angleXY, float angleXZ, const Color& color, float duration)
    {
        const float tolerance = 0.001f;
        const float angle1 = Math::Clamp(angleXY, tolerance, PI - tolerance);
        const float angle2 = Math::Clamp(angleXZ, tolerance, PI - tolerance);

        const float sinX1 = Math::Sin(0.5f * angle1);
        const float sinY2 = Math::Sin(0.5f * angle2);
        const float sinSqX1 = sinX1 * sinX1;
        const float sinSqY2 = sinY2 * sinY2;

        DebugTriangle t;
        t.Color = Color32(color);
        t.TimeLeft = duration;
        const Float3 positionF = position - Context->Origin;
        const Matrix world = Matrix::RotationQuaternion(orientation) * Matrix::Translation(positionF);
        t.V0 = world.GetTranslation();

        Float3 vertices[DEBUG_DRAW_CONE_RESOLUTION];
        for (uint32 i = 0; i < DEBUG_DRAW_CONE_RESOLUTION; i++)
        {
            const float alpha = (float)i / (float)DEBUG_DRAW_CONE_RESOLUTION;
            const float azimuth = alpha * TWO_PI;

            const float phi = Math::Atan2(Math::Sin(azimuth) * sinY2, Math::Cos(azimuth) * sinX1);
            const float sinPhi = Math::Sin(phi);
            const float cosPhi = Math::Cos(phi);
            const float sinSqPhi = sinPhi * sinPhi;
            const float cosSqPhi = cosPhi * cosPhi;

            const float rSq = sinSqX1 * sinSqY2 / (sinSqX1 * sinSqPhi + sinSqY2 * cosSqPhi);
            const float r = Math::Sqrt(rSq);
            const float s = Math::Sqrt(1 - rSq);

            Float3 vertex = Float3(2 * s * (r * cosPhi), 2 * s * (r * sinPhi), 1 - 2 * rSq) * radius;
            Float3::Transform(vertex, world, vertices[i]);
        }

        for (uint32 i = 0; i < DEBUG_DRAW_CONE_RESOLUTION; i++)
        {
            t.V1 = vertices[i];
            t.V2 = vertices[(i + 1) % DEBUG_DRAW_CONE_RESOLUTION];
            list->Add(t);
        }
    }
}

void DebugDraw::DrawCylinder(const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color, float duration, bool depthTest)
{
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    ::DrawCylinder(list, position, orientation, radius, height, color, duration);
}

void DebugDraw::DrawWireCylinder(const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color, float duration, bool depthTest)
{
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultWireTriangles : &Context->DebugDrawDepthTest.OneFrameWireTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultWireTriangles : &Context->DebugDrawDefault.OneFrameWireTriangles;
    ::DrawCylinder(list, position, orientation, radius, height, color, duration);
}

void DebugDraw::DrawCone(const Vector3& position, const Quaternion& orientation, float radius, float angleXY, float angleXZ, const Color& color, float duration, bool depthTest)
{
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    ::DrawCone(list, position, orientation, radius, angleXY, angleXZ, color, duration);
}

void DebugDraw::DrawWireCone(const Vector3& position, const Quaternion& orientation, float radius, float angleXY, float angleXZ, const Color& color, float duration, bool depthTest)
{
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultWireTriangles : &Context->DebugDrawDepthTest.OneFrameWireTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultWireTriangles : &Context->DebugDrawDefault.OneFrameWireTriangles;
    ::DrawCone(list, position, orientation, radius, angleXY, angleXZ, color, duration);
}

void DebugDraw::DrawArc(const Vector3& position, const Quaternion& orientation, float radius, float angle, const Color& color, float duration, bool depthTest)
{
    if (angle <= 0)
        return;
    if (angle > TWO_PI)
        angle = TWO_PI;
    Array<DebugTriangle>* list;
    if (depthTest)
        list = duration > 0 ? &Context->DebugDrawDepthTest.DefaultTriangles : &Context->DebugDrawDepthTest.OneFrameTriangles;
    else
        list = duration > 0 ? &Context->DebugDrawDefault.DefaultTriangles : &Context->DebugDrawDefault.OneFrameTriangles;
    const int32 resolution = Math::CeilToInt((float)DEBUG_DRAW_CONE_RESOLUTION / TWO_PI * angle);
    const float angleStep = angle / (float)resolution;
    const Float3 positionF = position - Context->Origin;
    const Matrix world = Matrix::RotationQuaternion(orientation) * Matrix::Translation(positionF);
    float currentAngle = 0.0f;
    DebugTriangle t;
    t.Color = Color32(color);
    t.TimeLeft = duration;
    t.V0 = world.GetTranslation();
    Float3 pos(Math::Cos(0.0f) * radius, Math::Sin(0.0f) * radius, 0);
    Float3::Transform(pos, world, t.V2);
    for (int32 i = 0; i < resolution; i++)
    {
        t.V1 = t.V2;
        currentAngle += angleStep;
        pos = Float3(Math::Cos(currentAngle) * radius, Math::Sin(currentAngle) * radius, 0);
        Float3::Transform(pos, world, t.V2);
        list->Add(t);
    }
}

void DebugDraw::DrawWireArc(const Vector3& position, const Quaternion& orientation, float radius, float angle, const Color& color, float duration, bool depthTest)
{
    if (angle <= 0)
        return;
    if (angle > TWO_PI)
        angle = TWO_PI;
    const int32 resolution = Math::CeilToInt((float)DEBUG_DRAW_CONE_RESOLUTION / TWO_PI * angle);
    const float angleStep = angle / (float)resolution;
    const Float3 positionF = position - Context->Origin;
    const Matrix world = Matrix::RotationQuaternion(orientation) * Matrix::Translation(positionF);
    float currentAngle = 0.0f;
    Float3 prevPos(world.GetTranslation());
    if (angle >= TWO_PI)
    {
        prevPos = Float3(Math::Cos(TWO_PI - angleStep) * radius, Math::Sin(TWO_PI - angleStep) * radius, 0);
        Float3::Transform(prevPos, world, prevPos);
    }
    const Color32 color32(color);
    auto& debugDrawData = depthTest ? Context->DebugDrawDepthTest : Context->DebugDrawDefault;
#define ADD_LINE(a, b) if (duration > 0) debugDrawData.DefaultLines.Add({ a, b, color32, duration }); else { debugDrawData.OneFrameLines.Add({ a, color32 }); debugDrawData.OneFrameLines.Add({ b, color32 }); }
    for (int32 i = 0; i <= resolution; i++)
    {
        Float3 pos(Math::Cos(currentAngle) * radius, Math::Sin(currentAngle) * radius, 0);
        Float3::Transform(pos, world, pos);
        ADD_LINE(prevPos, pos);
        currentAngle += angleStep;
        prevPos = pos;
    }
    if (angle < TWO_PI)
    {
        Float3 pos(world.GetTranslation());
        ADD_LINE(prevPos, pos);
    }
#undef ADD_LINE
}

void DebugDraw::DrawWireArrow(const Vector3& position, const Quaternion& orientation, float scale, float capScale, const Color& color, float duration, bool depthTest)
{
    Float3 direction, up, right;
    Float3::Transform(Float3::Forward, orientation, direction);
    Float3::Transform(Float3::Up, orientation, up);
    Float3::Transform(Float3::Right, orientation, right);
    const Vector3 end = position + direction * (100.0f * scale);
    const Vector3 capEnd = end - (direction * (100 * Math::Min(capScale, scale * 0.5f)));
    const float arrowSidesRatio = Math::Min(capScale, scale * 0.5f) * 30.0f;

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
    for (Vector3& c : corners)
        c -= Context->Origin;

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
    for (Vector3& c : corners)
        c -= Context->Origin;

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

void DebugDraw::DrawText(const StringView& text, const Float2& position, const Color& color, int32 size, float duration)
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

void DebugDraw::DrawText(const StringView& text, const Vector3& position, const Color& color, int32 size, float duration, float scale)
{
    if (text.Length() == 0 || size < 4)
        return;
    Array<DebugText3D>* list = duration > 0 ? &Context->DebugDrawDefault.DefaultText3D : &Context->DebugDrawDefault.OneFrameText3D;
    auto& t = list->AddOne();
    t.Text.Resize(text.Length() + 1);
    Platform::MemoryCopy(t.Text.Get(), text.Get(), text.Length() * sizeof(Char));
    t.Text[text.Length()] = 0;
    t.Transform = position - Context->Origin;
    t.Transform.Scale.X = scale;
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
    t.Transform.Translation -= Context->Origin;
    t.FaceCamera = false;
    t.Size = size;
    t.Color = color;
    t.TimeLeft = duration;
}

void DebugDraw::Clear(void* context)
{
    UpdateContext(context, MAX_float);
}

#endif
