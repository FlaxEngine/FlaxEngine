// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GPUDevice.h"
#include "RenderTargetPool.h"
#include "GPUPipelineState.h"
#include "RenderTask.h"
#include "RenderTools.h"
#include "Graphics.h"
#include "Async/DefaultGPUTasksExecutor.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Content.h"
#include "Engine/Platform/Windows/WindowsWindow.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Scripting/SoftObjectReference.h"

GPUPipelineState* GPUPipelineState::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreatePipelineState();
}

GPUPipelineState* GPUPipelineState::New()
{
    return GPUDevice::Instance->CreatePipelineState();
}

GPUPipelineState::Description GPUPipelineState::Description::Default =
{
    // Enable/disable depth write
    true,
    // Enable/disable depth test
    true,
    // DepthClipEnable
    true,
    // DepthFunc
    ComparisonFunc::Less,
    // Vertex shader
    nullptr,
    // Hull shader
    nullptr,
    // Domain shader
    nullptr,
    // Geometry shader
    nullptr,
    // Pixel shader
    nullptr,
    // Primitives topology
    PrimitiveTopologyType::Triangle,
    // True if use wireframe rendering
    false,
    // Primitives culling mode
    CullMode::Normal,
    // Colors blending mode
    BlendingMode::Opaque,
};

GPUPipelineState::Description GPUPipelineState::Description::DefaultNoDepth =
{
    // Enable/disable depth write
    false,
    // Enable/disable depth test
    false,
    // DepthClipEnable
    false,
    // DepthFunc
    ComparisonFunc::Less,
    // Vertex shader
    nullptr,
    // Hull shader
    nullptr,
    // Domain shader
    nullptr,
    // Geometry shader
    nullptr,
    // Pixel shader
    nullptr,
    // Primitives topology
    PrimitiveTopologyType::Triangle,
    // True if use wireframe rendering
    false,
    // Primitives culling mode
    CullMode::Normal,
    // Colors blending mode
    BlendingMode::Opaque,
};

GPUPipelineState::Description GPUPipelineState::Description::DefaultFullscreenTriangle =
{
    // Enable/disable depth write
    false,
    // Enable/disable depth test
    false,
    // DepthClipEnable
    false,
    // DepthFunc
    ComparisonFunc::Less,
    // Vertex shader
    nullptr,
    // Set to default quad VS via GPUDevice
    // Hull shader
    nullptr,
    // Domain shader
    nullptr,
    // Geometry shader
    nullptr,
    // Pixel shader
    nullptr,
    // Primitives topology
    PrimitiveTopologyType::Triangle,
    // True if use wireframe rendering
    false,
    // Primitives culling mode
    CullMode::TwoSided,
    // Colors blending mode
    BlendingMode::Opaque,
};

struct GPUDevice::PrivateData
{
    AssetReference<Shader> QuadShader;
    GPUPipelineState* PS_CopyLinear = nullptr;
    GPUPipelineState* PS_Clear = nullptr;
    GPUBuffer* FullscreenTriangleVB = nullptr;
    AssetReference<Material> DefaultMaterial;
    SoftObjectReference<Material> DefaultDeformableMaterial;
    AssetReference<Texture> DefaultNormalMap;
    AssetReference<Texture> DefaultWhiteTexture;
    AssetReference<Texture> DefaultBlackTexture;
};

GPUDevice* GPUDevice::Instance = nullptr;

GPUDevice::GPUDevice(RendererType type, ShaderProfile profile)
    : PersistentScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    , _state(DeviceState::Missing)
    , _isRendering(false)
    , _wasVSyncUsed(false)
    , _drawGpuEventIndex(0)
    , _rendererType(type)
    , _shaderProfile(profile)
    , _featureLevel(RenderTools::GetFeatureLevel(profile))
    , _res(New<PrivateData>())
    , TasksManager(this)
    , TotalGraphicsMemory(0)
    , QuadShader(nullptr)
    , CurrentTask(nullptr)
{
    ASSERT(_rendererType != RendererType::Unknown);
}

GPUDevice::~GPUDevice()
{
    Delete(_res);

    // Unlink
    ASSERT(GPUDevice::Instance == this);
    GPUDevice::Instance = nullptr;
}

bool GPUDevice::Init()
{
    LOG(Info, "Total graphics memory: {0}", Utilities::BytesToText(TotalGraphicsMemory));
    return false;
}

bool GPUDevice::LoadContent()
{
    // Load internal rendering shader for GPU device low-level impl
    _res->QuadShader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Quad"));
    if (_res->QuadShader == nullptr || _res->QuadShader->WaitForLoaded())
        return true;
    QuadShader = _res->QuadShader->GetShader();
    GPUPipelineState::Description::DefaultFullscreenTriangle.VS = QuadShader->GetVS("VS");
    _res->PS_CopyLinear = CreatePipelineState();
    GPUPipelineState::Description desc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    desc.PS = QuadShader->GetPS("PS_CopyLinear");
    if (_res->PS_CopyLinear->Init(desc))
        return true;
    _res->PS_Clear = CreatePipelineState();
    desc.PS = QuadShader->GetPS("PS_Clear");
    if (_res->PS_Clear->Init(desc))
        return true;

    // Create fullscreen triangle vertex buffer
    {
        // Create vertex buffer
        // @formatter:off
        static float vb[] =
        {
            //   XY            UV
            -1.0f, -1.0f,  0.0f,  1.0f,
            -1.0f,  3.0f,  0.0f, -1.0f,
             3.0f, -1.0f,  2.0f,  1.0f,
        };
        // @formatter:on
        _res->FullscreenTriangleVB = CreateBuffer(TEXT("QuadVB"));
        if (_res->FullscreenTriangleVB->Init(GPUBufferDescription::Vertex(sizeof(float) * 4, 3, vb)))
            return true;
    }

    // Load default material
    _res->DefaultMaterial = Content::LoadAsyncInternal<Material>(TEXT("Engine/DefaultMaterial"));
    if (_res->DefaultMaterial == nullptr)
        return true;
    _res->DefaultDeformableMaterial = Guid(0x639e12c0, 0x42d34bae, 0x89dd8b81, 0x7e1efc2d);

    // Load default normal map
    _res->DefaultNormalMap = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/NormalTexture"));
    if (_res->DefaultNormalMap == nullptr)
        return true;

    // Load default solid white
    _res->DefaultWhiteTexture = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/WhiteTexture"));
    if (_res->DefaultWhiteTexture == nullptr)
        return true;

    // Load default solid black
    _res->DefaultBlackTexture = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/BlackTexture"));
    if (_res->DefaultBlackTexture == nullptr)
        return true;

    return false;
}

void GPUDevice::preDispose()
{
    RenderTargetPool::Flush();

    // Release resources
    _res->DefaultMaterial = nullptr;
    _res->DefaultDeformableMaterial = nullptr;
    _res->DefaultNormalMap = nullptr;
    _res->DefaultWhiteTexture = nullptr;
    _res->DefaultBlackTexture = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_res->PS_CopyLinear);
    SAFE_DELETE_GPU_RESOURCE(_res->PS_Clear);
    SAFE_DELETE_GPU_RESOURCE(_res->FullscreenTriangleVB);

    // Release GPU resources memory and unlink from device
    // Note: after that no GPU resources should be used/created, only deleted
    Resources.OnDeviceDispose();
}

void GPUDevice::DrawBegin()
{
    // Set flag
    _isRendering = true;

    // Clear stats
    RenderTask::TasksDoneLastFrame = 0;
}

void GPUDevice::DrawEnd()
{
    PROFILE_CPU_NAMED("Present");

    // Check if use VSync
    bool useVSync = Graphics::UseVSync;
    if (CommandLine::Options.NoVSync.HasValue())
        useVSync = !CommandLine::Options.NoVSync.GetValue();
    else if (CommandLine::Options.VSync.HasValue())
        useVSync = CommandLine::Options.VSync.GetValue();

    // Find index of the last rendered window task (use vsync only on the last window)
    int32 lastWindowIndex = -1;
    for (int32 i = RenderTask::Tasks.Count() - 1; i >= 0; i--)
    {
        const auto task = RenderTask::Tasks[i];
        if (task && task->LastUsedFrame == Engine::FrameCount && task->SwapChain)
        {
            lastWindowIndex = i;
            break;
        }
    }

    // Call present on all used tasks
    int32 presentCount = 0;
    bool anyVSync = false;
    for (int32 i = 0; i < RenderTask::Tasks.Count(); i++)
    {
        const auto task = RenderTask::Tasks[i];
        if (task && task->LastUsedFrame == Engine::FrameCount && task->SwapChain)
        {
            bool vsync = useVSync;
            if (lastWindowIndex != i)
            {
                // Perform VSync only on the last window
                vsync = false;
            }
            else
            {
                // End profiler timer queries
#if COMPILE_WITH_PROFILER
                ProfilerGPU::OnPresent();
#endif
            }

            anyVSync |= vsync;
            task->OnPresent(vsync);
            presentCount++;
        }
    }

    // If no `Present` calls has been performed just execute GPU commands
    if (presentCount == 0)
    {
        // End profiler timer queries
#if COMPILE_WITH_PROFILER
        ProfilerGPU::OnPresent();
#endif
        GetMainContext()->Flush();
    }

    _wasVSyncUsed = anyVSync;
    _isRendering = false;

    RenderTargetPool::Flush();
}

void GPUDevice::RenderBegin()
{
#if COMPILE_WITH_PROFILER
    _drawGpuEventIndex = ProfilerGPU::BeginEvent(TEXT("Draw"));
#endif
}

void GPUDevice::RenderEnd()
{
#if COMPILE_WITH_PROFILER
    ProfilerGPU::EndEvent(_drawGpuEventIndex);
#endif
}

GPUTasksContext* GPUDevice::CreateTasksContext()
{
    return New<GPUTasksContext>(this);
}

GPUTasksExecutor* GPUDevice::CreateTasksExecutor()
{
    return New<DefaultGPUTasksExecutor>();
}

void GPUDevice::Draw()
{
    DrawBegin();

    auto context = GetMainContext();

    // Begin frame
    context->FrameBegin();
    RenderBegin();
    TasksManager.FrameBegin();
    Render2D::BeginFrame();

    // Perform actual drawing
    Engine::Draw();
    EngineService::OnDraw();
    RenderTask::DrawAll();

    // End frame
    Render2D::EndFrame();
    TasksManager.FrameEnd();
    RenderEnd();
    context->FrameEnd();

    DrawEnd();
}

void GPUDevice::Dispose()
{
    RenderList::CleanupCache();
    VideoOutputModes.Resize(0);
}

MaterialBase* GPUDevice::GetDefaultMaterial() const
{
    return _res->DefaultMaterial;
}

MaterialBase* GPUDevice::GetDefaultDeformableMaterial() const
{
    return _res->DefaultDeformableMaterial.Get();
}

GPUTexture* GPUDevice::GetDefaultNormalMap() const
{
    return _res->DefaultNormalMap ? _res->DefaultNormalMap->GetTexture() : nullptr;
}

GPUTexture* GPUDevice::GetDefaultWhiteTexture() const
{
    return _res->DefaultWhiteTexture ? _res->DefaultWhiteTexture->GetTexture() : nullptr;
}

GPUTexture* GPUDevice::GetDefaultBlackTexture() const
{
    return _res->DefaultBlackTexture ? _res->DefaultBlackTexture->GetTexture() : nullptr;
}

GPUPipelineState* GPUDevice::GetCopyLinearPS() const
{
    return _res->PS_CopyLinear;
}

GPUPipelineState* GPUDevice::GetClearPS() const
{
    return _res->PS_Clear;
}

GPUBuffer* GPUDevice::GetFullscreenTriangleVB() const
{
    return _res->FullscreenTriangleVB;
}
