// Copyright (c) Wojciech Figat. All rights reserved.

#include "Graphics.h"
#include "GPUDevice.h"
#include "PixelFormatExtensions.h"
#include "Async/GPUTasksManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerGPU.h"
#include "Engine/Render2D/Font.h"

bool Graphics::UseVSync = false;
Quality Graphics::AAQuality = Quality::Medium;
Quality Graphics::SSRQuality = Quality::Medium;
Quality Graphics::SSAOQuality = Quality::Medium;
Quality Graphics::VolumetricFogQuality = Quality::High;
Quality Graphics::ShadowsQuality = Quality::Medium;
Quality Graphics::ShadowMapsQuality = Quality::Medium;
float Graphics::ShadowUpdateRate = 1.0f;
bool Graphics::AllowCSMBlending = false;
Quality Graphics::GlobalSDFQuality = Quality::High;
Quality Graphics::GIQuality = Quality::High;
bool Graphics::GICascadesBlending = false;
PostProcessSettings Graphics::PostProcessSettings;
bool Graphics::SpreadWorkload = true;

#if GRAPHICS_API_NULL
extern GPUDevice* CreateGPUDeviceNull();
#endif
#if GRAPHICS_API_VULKAN
extern GPUDevice* CreateGPUDeviceVulkan();
#endif
#if GRAPHICS_API_DIRECTX11
extern GPUDevice* CreateGPUDeviceDX11();
#endif
#if GRAPHICS_API_DIRECTX12
extern GPUDevice* CreateGPUDeviceDX12();
#endif
#if GRAPHICS_API_PS4
extern GPUDevice* CreateGPUDevicePS4();
#endif
#if GRAPHICS_API_PS5
extern GPUDevice* CreateGPUDevicePS5();
#endif

class GraphicsService : public EngineService
{
public:
    GraphicsService()
        : EngineService(TEXT("Graphics"), -40)
    {
    }

    bool Init() override;
    void BeforeExit() override;
    void Dispose() override;
};

GraphicsService GraphicsServiceInstance;

void GraphicsSettings::Apply()
{
    Graphics::UseVSync = UseVSync;
    Graphics::AAQuality = AAQuality;
    Graphics::SSRQuality = SSRQuality;
    Graphics::SSAOQuality = SSAOQuality;
    Graphics::VolumetricFogQuality = VolumetricFogQuality;
    Graphics::ShadowsQuality = ShadowsQuality;
    Graphics::ShadowMapsQuality = ShadowMapsQuality;
    Graphics::AllowCSMBlending = AllowCSMBlending;
    Graphics::GlobalSDFQuality = GlobalSDFQuality;
    Graphics::GIQuality = GIQuality;
    Graphics::GICascadesBlending = GICascadesBlending;
    Graphics::PostProcessSettings = ::PostProcessSettings();
    Graphics::PostProcessSettings.BlendWith(PostProcessSettings, 1.0f);
#if !USE_EDITOR // OptionsModule handles fallback fonts in Editor
    Font::FallbackFonts = FallbackFonts;
#endif
}

void Graphics::DisposeDevice()
{
    if (GPUDevice::Instance)
    {
        // Clean any danging pointer to last task (might stay if engine is disposing after crash)
        GPUDevice::Instance->CurrentTask = nullptr;

        GPUDevice::Instance->Dispose();
        LOG_FLUSH();
        Delete(GPUDevice::Instance);
        GPUDevice::Instance = nullptr;
    }
}

bool GraphicsService::Init()
{
    ASSERT(GPUDevice::Instance == nullptr);

    // Create and initialize graphics device
    Log::Logger::WriteFloor();
    LOG(Info, "Creating Graphics Device...");
    PixelFormatExtensions::Init();
    GPUDevice* device = nullptr;

    // Null
    if (!device && CommandLine::Options.Null.IsTrue())
    {
#if GRAPHICS_API_NULL
        device = CreateGPUDeviceNull();
#else
        LOG(Warning, "Null backend not available");
#endif
    }

    // Vulkan
    if (!device && CommandLine::Options.Vulkan.IsTrue())
    {
#if GRAPHICS_API_VULKAN
        device = CreateGPUDeviceVulkan();
#else
        LOG(Warning, "Vulkan backend not available");
#endif
    }

    // DirectX 12
    if (!device && CommandLine::Options.D3D12.IsTrue())
    {
#if GRAPHICS_API_DIRECTX12
        if (Platform::IsWindows10())
        {
            device = CreateGPUDeviceDX12();
        }
#else
        LOG(Warning, "DirectX 12 backend not available");
#endif
    }

    // DirectX 11 and DirectX 10
    if (!device && (CommandLine::Options.D3D11.IsTrue() || CommandLine::Options.D3D10.IsTrue()))
    {
#if GRAPHICS_API_DIRECTX11
        device = CreateGPUDeviceDX11();
#else
        LOG(Warning, "DirectX 11 backend not available");
#endif
    }

    // Platform default
    if (!device)
    {
#if GRAPHICS_API_DIRECTX11
        if (!device)
            device = CreateGPUDeviceDX11();
#endif
#if GRAPHICS_API_DIRECTX12
        if (!device && Platform::IsWindows10())
            device = CreateGPUDeviceDX12();
#endif
#if GRAPHICS_API_VULKAN
        if (!device)
            device = CreateGPUDeviceVulkan();
#endif
#if GRAPHICS_API_PS4
        if (!device)
            device = CreateGPUDevicePS4();
#endif
#if GRAPHICS_API_PS5
        if (!device)
            device = CreateGPUDevicePS5();
#endif
    }

    // Null as a fallback
#if GRAPHICS_API_NULL
    if (!device)
        device = CreateGPUDeviceNull();
#endif

    if (device == nullptr)
    {
        return true;
    }
    GPUDevice::Instance = device;
    LOG(Info, "GPU Device created: {}", device->GetAdapter()->GetDescription());
    LOG(Info, "Renderer: {}, Shader Profile: {}, Feature Level: {}, Driver: {}",
        ::ToString(device->GetRendererType()),
        ::ToString(device->GetShaderProfile()),
        ::ToString(device->GetFeatureLevel()),
        device->GetAdapter()->GetDriverVersion().ToString()
    );

    // Initialize
    if (device->IsDebugToolAttached
#if USE_EDITOR || !BUILD_RELEASE
        || CommandLine::Options.ShaderProfile.IsTrue()
#endif
#if USE_EDITOR
        || CommandLine::Options.ShaderDebug.IsTrue()
#endif
        )
    {
#if !USE_EDITOR && BUILD_RELEASE && !PLATFORM_LINUX // IsDebugToolAttached seams to be enabled on many Linux machines via VK_EXT_tooling_info
        // Block graphics debugging to protect contents
        Platform::Fatal(TEXT("Graphics debugger attached."));
#endif
#if COMPILE_WITH_PROFILER
        // Auto-enable GPU events
        ProfilerGPU::EventsEnabled = true;
#endif
    }
    if (device->LoadContent())
    {
        return true;
    }
    Log::Logger::WriteFloor();

    return false;
}

void GraphicsService::BeforeExit()
{
    if (GPUDevice::Instance)
    {
        // Start disposing
        GPUDevice::Instance->GetTasksManager()->Dispose();
    }
}

void GraphicsService::Dispose()
{
    // Device is disposed AFTER Content (faster and safer because there is no assets so there is less gpu resources to cleanup)
}
