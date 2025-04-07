// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUDevice.h"
#include "RenderTargetPool.h"
#include "GPUPipelineState.h"
#include "GPUResourceProperty.h"
#include "GPUSwapChain.h"
#include "RenderTask.h"
#include "RenderTools.h"
#include "Graphics.h"
#include "Shaders/GPUShader.h"
#include "Shaders/GPUVertexLayout.h"
#include "Async/DefaultGPUTasksExecutor.h"
#include "Async/GPUTasksManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/SoftAssetReference.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Scripting/Enums.h"

GPUResourcePropertyBase::~GPUResourcePropertyBase()
{
    const auto e = _resource;
    if (e)
    {
        _resource = nullptr;
        e->Releasing.Unbind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
    }
}

void GPUResourcePropertyBase::OnSet(GPUResource* resource)
{
    auto e = _resource;
    if (e != resource)
    {
        if (e)
            e->Releasing.Unbind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
        _resource = e = resource;
        if (e)
            e->Releasing.Bind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
    }
}

void GPUResourcePropertyBase::OnReleased()
{
    auto e = _resource;
    if (e)
    {
        _resource = nullptr;
        e->Releasing.Unbind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
    }
}

GPUPipelineState* GPUPipelineState::Spawn(const SpawnParams& params)
{
    return GPUDevice::Instance->CreatePipelineState();
}

GPUPipelineState* GPUPipelineState::New()
{
    return GPUDevice::Instance->CreatePipelineState();
}

GPUPipelineState::GPUPipelineState()
    : GPUResource(SpawnParams(Guid::New(), TypeInitializer))
{
}

bool GPUPipelineState::Init(const Description& desc)
{
    // Cache description in debug builds
#if BUILD_DEBUG
    DebugDesc = desc;
#endif

    // Cache shader stages usage flags for pipeline state
    _meta.InstructionsCount = 0;
    _meta.UsedCBsMask = 0;
    _meta.UsedSRsMask = 0;
    _meta.UsedUAsMask = 0;
#define CHECK_STAGE(stage) \
	if (desc.stage) { \
		_meta.UsedCBsMask |= desc.stage->GetBindings().UsedCBsMask; \
		_meta.UsedSRsMask |= desc.stage->GetBindings().UsedSRsMask; \
		_meta.UsedUAsMask |= desc.stage->GetBindings().UsedUAsMask; \
	}
    CHECK_STAGE(VS);
    CHECK_STAGE(HS);
    CHECK_STAGE(DS);
    CHECK_STAGE(GS);
    CHECK_STAGE(PS);
#undef CHECK_STAGE

#if USE_EDITOR
    // Estimate somehow performance cost of this pipeline state for the content profiling
    const int32 textureLookupCost = 20;
    const int32 tessCost = 300;
    Complexity = Utilities::CountBits(_meta.UsedSRsMask) * textureLookupCost;
    if (desc.PS)
        Complexity += desc.PS->GetBindings().InstructionsCount;
    if (desc.HS || desc.DS)
        Complexity += tessCost;
    if (desc.DepthWriteEnable)
        Complexity += 5;
    if (desc.DepthEnable)
        Complexity += 5;
    if (desc.BlendMode.BlendEnable)
        Complexity += 20;
#endif

    return false;
}

GPUResourceType GPUPipelineState::GetResourceType() const
{
    return GPUResourceType::PipelineState;
}

// @formatter:off
GPUPipelineState::Description GPUPipelineState::Description::Default =
{
    true, // DepthEnable
    true, // DepthWriteEnable
    true, // DepthClipEnable
    ComparisonFunc::Less, // DepthFunc
    false, // StencilEnable
    0xff, // StencilReadMask
    0xff, // StencilWriteMask
    ComparisonFunc::Always, // StencilFunc
    StencilOperation::Keep, // StencilFailOp
    StencilOperation::Keep, // StencilDepthFailOp
    StencilOperation::Keep, // StencilPassOp
    nullptr, // VS
    nullptr, // HS
    nullptr, // DS
    nullptr, // GS
    nullptr, // PS
    PrimitiveTopologyType::Triangle, // PrimitiveTopology
    false, // Wireframe
    CullMode::Normal, // CullMode
    BlendingMode::Opaque, // BlendMode
};

GPUPipelineState::Description GPUPipelineState::Description::DefaultNoDepth =
{
    false, // DepthEnable
    false, // DepthWriteEnable
    false, // DepthClipEnable
    ComparisonFunc::Less, // DepthFunc
    false, // StencilEnable
    0xff, // StencilReadMask
    0xff, // StencilWriteMask
    ComparisonFunc::Always, // StencilFunc
    StencilOperation::Keep, // StencilFailOp
    StencilOperation::Keep, // StencilDepthFailOp
    StencilOperation::Keep, // StencilPassOp
    nullptr, // VS
    nullptr, // HS
    nullptr, // DS
    nullptr, // GS
    nullptr, // PS
    PrimitiveTopologyType::Triangle, // PrimitiveTopology
    false, // Wireframe
    CullMode::Normal, // CullMode
    BlendingMode::Opaque, // BlendMode
};

GPUPipelineState::Description GPUPipelineState::Description::DefaultFullscreenTriangle =
{
    false, // DepthEnable
    false, // DepthWriteEnable
    false, // DepthClipEnable
    ComparisonFunc::Less, // DepthFunc
    false, // StencilEnable
    0xff, // StencilReadMask
    0xff, // StencilWriteMask
    ComparisonFunc::Always, // StencilFunc
    StencilOperation::Keep, // StencilFailOp
    StencilOperation::Keep, // StencilDepthFailOp
    StencilOperation::Keep, // StencilPassOp
    nullptr, // VS (Set to default quad VS via GPUDevice)
    nullptr, // HS
    nullptr, // DS
    nullptr, // GS
    nullptr, // PS
    PrimitiveTopologyType::Triangle, // PrimitiveTopology
    false, // Wireframe
    CullMode::TwoSided, // CullMode
    BlendingMode::Opaque, // BlendMode
};
// @formatter:on

GPUResource::GPUResource()
    : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
{
}

GPUResource::GPUResource(const SpawnParams& params)
    : ScriptingObject(params)
{
}

GPUResource::~GPUResource()
{
#if !BUILD_RELEASE && GPU_ENABLE_RESOURCE_NAMING
    if (_memoryUsage != 0)
        LOG(Error, "{0} '{1}' has not been disposed before destruction", ScriptingObject::ToString(), GetName());
#endif
#if GPU_ENABLE_RESOURCE_NAMING
    if (_namePtr)
        Platform::Free(_namePtr);
#endif
}

uint64 GPUResource::GetMemoryUsage() const
{
    return _memoryUsage;
}

static_assert((GPU_ENABLE_RESOURCE_NAMING) == (!BUILD_RELEASE), "Update build condition on around GPUResource Name property getter/setter.");

#if GPU_ENABLE_RESOURCE_NAMING

StringView GPUResource::GetName() const
{
    return StringView(_namePtr, _nameSize);
}

void GPUResource::SetName(const StringView& name)
{
    if (_nameCapacity < name.Length() + 1)
    {
        if (_namePtr)
            Platform::Free(_namePtr);
        _nameCapacity = name.Length() + 1;
        _namePtr = (Char*)Platform::Allocate(_nameCapacity * sizeof(Char), 16);
    }
    _nameSize = name.Length();
    if (name.HasChars())
    {
        Platform::MemoryCopy(_namePtr, name.Get(), _nameSize * sizeof(Char));
        _namePtr[_nameSize] = 0;
    }
}

#endif

void GPUResource::ReleaseGPU()
{
    if (_memoryUsage != 0)
    {
        Releasing();
        OnReleaseGPU();
        _memoryUsage = 0;
    }
}

void GPUResource::OnDeviceDispose()
{
    // By default we want to release resource data but keep it alive
    ReleaseGPU();
}

void GPUResource::OnReleaseGPU()
{
}

String GPUResource::ToString() const
{
#if GPU_ENABLE_RESOURCE_NAMING
    if (_namePtr)
        return String(_namePtr, _nameSize);
#endif
    return ScriptingObject::ToString();
}

void GPUResource::OnDeleteObject()
{
    ReleaseGPU();

    ScriptingObject::OnDeleteObject();
}

double GPUResourceView::DummyLastRenderTime = -1;

struct GPUDevice::PrivateData
{
    AssetReference<Shader> QuadShader;
    GPUPipelineState* PS_CopyLinear = nullptr;
    GPUPipelineState* PS_Clear = nullptr;
    GPUPipelineState* PS_DecodeYUY2 = nullptr;
    GPUPipelineState* PS_DecodeNV12 = nullptr;
    GPUBuffer* FullscreenTriangleVB = nullptr;
    AssetReference<Material> DefaultMaterial;
    SoftAssetReference<Material> DefaultDeformableMaterial;
    AssetReference<Texture> DefaultNormalMap;
    AssetReference<Texture> DefaultWhiteTexture;
    AssetReference<Texture> DefaultBlackTexture;
    GPUTasksManager TasksManager;
};

GPUDevice* GPUDevice::Instance = nullptr;

void GPUDevice::OnRequestingExit()
{
    if (Engine::FatalError != FatalErrorType::GPUCrash && 
        Engine::FatalError != FatalErrorType::GPUHang && 
        Engine::FatalError != FatalErrorType::GPUOutOfMemory)
        return;
    // TODO: get and log actual GPU memory used by the engine (API-specific)
    DumpResourcesToLog();
}

GPUDevice::GPUDevice(RendererType type, ShaderProfile profile)
    : ScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    , _state(DeviceState::Missing)
    , _isRendering(false)
    , _wasVSyncUsed(false)
    , _drawGpuEventIndex(0)
    , _rendererType(type)
    , _shaderProfile(profile)
    , _featureLevel(RenderTools::GetFeatureLevel(profile))
    , _res(New<PrivateData>())
    , _resources(1024)
    , TotalGraphicsMemory(0)
    , IsDebugToolAttached(false)
    , QuadShader(nullptr)
    , CurrentTask(nullptr)
{
    ASSERT_LOW_LAYER(_rendererType != RendererType::Unknown);
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
    // Clamp texture limits (eg. if driver reports higher value)
    Limits.MaximumTexture1DSize = Math::Min(Limits.MaximumTexture1DSize, GPU_MAX_TEXTURE_SIZE);
    Limits.MaximumTexture2DSize = Math::Min(Limits.MaximumTexture2DSize, GPU_MAX_TEXTURE_SIZE);
    Limits.MaximumTexture3DSize = Math::Min(Limits.MaximumTexture3DSize, GPU_MAX_TEXTURE_SIZE);
    Limits.MaximumTextureCubeSize = Math::Min(Limits.MaximumTextureCubeSize, GPU_MAX_TEXTURE_SIZE);
    Limits.MaximumTexture1DArraySize = Math::Min(Limits.MaximumTexture1DArraySize, GPU_MAX_TEXTURE_ARRAY_SIZE);
    Limits.MaximumTexture2DArraySize = Math::Min(Limits.MaximumTexture2DArraySize, GPU_MAX_TEXTURE_ARRAY_SIZE);
    Limits.MaximumMipLevelsCount = Math::Min(Limits.MaximumMipLevelsCount, GPU_MAX_TEXTURE_MIP_LEVELS);

    _res->TasksManager.SetExecutor(CreateTasksExecutor());
    LOG(Info, "Total graphics memory: {0}", Utilities::BytesToText(TotalGraphicsMemory));
    if (!Limits.HasCompute)
        LOG(Warning, "Compute Shaders are not supported");
    Engine::RequestingExit.Bind<GPUDevice, &GPUDevice::OnRequestingExit>(this);
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
        auto layout = GPUVertexLayout::Get({
            { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32_Float },
            { VertexElement::Types::TexCoord, 0, 8, 0, PixelFormat::R32G32_Float },
        });
        if (_res->FullscreenTriangleVB->Init(GPUBufferDescription::Vertex(layout, 16, 3, vb)))
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

bool GPUDevice::CanDraw()
{
    return true;
}

void GPUDevice::AddResource(GPUResource* resource)
{
    _resourcesLock.Lock();
    ASSERT(resource && !_resources.Contains(resource));
    _resources.Add(resource);
    _resourcesLock.Unlock();
}

void GPUDevice::RemoveResource(GPUResource* resource)
{
    _resourcesLock.Lock();
    ASSERT(resource && _resources.Contains(resource));
    _resources.Remove(resource);
    _resourcesLock.Unlock();
}

void GPUDevice::DumpResourcesToLog() const
{
    StringBuilder output;
    _resourcesLock.Lock();

    output.AppendFormat(TEXT("GPU Resources dump. Count: {0}, total GPU memory used: {1}"), _resources.Count(), Utilities::BytesToText(GetMemoryUsage()));
    output.AppendLine();
    output.AppendLine();

    const bool printTypes[(int32)GPUResourceType::MAX] =
    {
        true, // RenderTarget
        true, // Texture
        true, // CubeTexture
        true, // VolumeTexture
        true, // Buffer
        true, // Shader
        false, // PipelineState
        false, // Descriptor
        false, // Query
        false, // Sampler
    };
    for (int32 typeIndex = 0; typeIndex < (int32)GPUResourceType::MAX; typeIndex++)
    {
        const auto type = static_cast<GPUResourceType>(typeIndex);
        const auto printType = printTypes[typeIndex];

        output.AppendFormat(TEXT("Group: {0}s"), ScriptingEnum::ToString(type));
        output.AppendLine();

        int32 count = 0;
        uint64 memUsage = 0;
        for (int32 i = 0; i < _resources.Count(); i++)
        {
            const GPUResource* resource = _resources[i];
            if (resource->GetResourceType() == type && resource->GetMemoryUsage() != 0)
            {
                count++;
                memUsage += resource->GetMemoryUsage();
                auto str = resource->ToString();
                if (str.HasChars() && printType)
                {
                    output.Append(TEXT('\t'));
                    output.Append(str);
                    output.AppendLine();
                }
            }
        }

        output.AppendFormat(TEXT("Total count: {0}, memory usage: {1}"), count, Utilities::BytesToText(memUsage));
        output.AppendLine();
        output.AppendLine();
    }

    _resourcesLock.Unlock();
    LOG_STR(Info, output.ToStringView());
}

extern void ClearVertexLayoutCache();

void GPUDevice::preDispose()
{
    Locker.Lock();
    RenderTargetPool::Flush();

    // Release resources
    _res->DefaultMaterial = nullptr;
    _res->DefaultDeformableMaterial = nullptr;
    _res->DefaultNormalMap = nullptr;
    _res->DefaultWhiteTexture = nullptr;
    _res->DefaultBlackTexture = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_res->PS_CopyLinear);
    SAFE_DELETE_GPU_RESOURCE(_res->PS_Clear);
    SAFE_DELETE_GPU_RESOURCE(_res->PS_DecodeYUY2);
    SAFE_DELETE_GPU_RESOURCE(_res->FullscreenTriangleVB);
    ClearVertexLayoutCache();

    Locker.Unlock();

    // Release GPU resources memory and unlink from device
    // Note: after that no GPU resources should be used/created, only deleted
    _resourcesLock.Lock();
    for (int32 i = _resources.Count() - 1; i >= 0 && i < _resources.Count(); i--)
    {
        _resources[i]->OnDeviceDispose();
    }
    _resources.Clear();
    _resourcesLock.Unlock();
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
        if (task && task->LastUsedFrame == Engine::FrameCount && task->SwapChain && task->SwapChain->IsReady())
        {
            lastWindowIndex = i;
            break;
        }
    }

    // Call present on all used tasks
    int32 presentCount = 0;
    bool anyVSync = false;
#if COMPILE_WITH_PROFILER
    const double presentStart = Platform::GetTimeSeconds();
#endif
    for (int32 i = 0; i < RenderTask::Tasks.Count(); i++)
    {
        const auto task = RenderTask::Tasks[i];
        if (task && task->LastUsedFrame == Engine::FrameCount && task->SwapChain && task->SwapChain->IsReady())
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
#if COMPILE_WITH_PROFILER
    const double presentEnd = Platform::GetTimeSeconds();
    ProfilerGPU::OnPresentTime((float)((presentEnd - presentStart) * 1000.0));
#endif

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
    _res->TasksManager.FrameBegin();
    Render2D::BeginFrame();

    // Perform actual drawing
    Engine::Draw();
    EngineService::OnDraw();
    RenderTask::DrawAll();

    // End frame
    Render2D::EndFrame();
    _res->TasksManager.FrameEnd();
    RenderEnd();
    context->FrameEnd();

    DrawEnd();
}

void GPUDevice::Dispose()
{
    RenderList::CleanupCache();
    VideoOutputModes.Resize(0);
}

uint64 GPUDevice::GetMemoryUsage() const
{
    uint64 result = 0;
    _resourcesLock.Lock();
    for (int32 i = 0; i < _resources.Count(); i++)
        result += _resources[i]->GetMemoryUsage();
    _resourcesLock.Unlock();
    return result;
}

Array<GPUResource*> GPUDevice::GetResources() const
{
    _resourcesLock.Lock();
    Array<GPUResource*> result = _resources;
    _resourcesLock.Unlock();
    return result;
}

GPUTasksManager* GPUDevice::GetTasksManager() const
{
    return &_res->TasksManager;
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

GPUPipelineState* GPUDevice::GetDecodeYUY2PS() const
{
    if (_res->PS_DecodeYUY2 == nullptr)
    {
        auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = QuadShader->GetPS("PS_DecodeYUY2");
        _res->PS_DecodeYUY2 = const_cast<GPUDevice*>(this)->CreatePipelineState();
        _res->PS_DecodeYUY2->Init(psDesc);
    }
    return _res->PS_DecodeYUY2;
}

GPUPipelineState* GPUDevice::GetDecodeNV12PS() const
{
    if (_res->PS_DecodeNV12 == nullptr)
    {
        auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = QuadShader->GetPS("PS_DecodeNV12");
        _res->PS_DecodeNV12 = const_cast<GPUDevice*>(this)->CreatePipelineState();
        _res->PS_DecodeNV12->Init(psDesc);
    }
    return _res->PS_DecodeNV12;
}

GPUBuffer* GPUDevice::GetFullscreenTriangleVB() const
{
    return _res->FullscreenTriangleVB;
}
