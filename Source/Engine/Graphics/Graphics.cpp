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
#include "Engine/Profiler/ProfilerMemory.h"
#if PLATFORM_DESKTOP
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Platform/FileSystem.h"
#endif
#if USE_EDITOR
#include "Engine/Engine/Globals.h"
#else
#include "Engine/Render2D/Font.h"
#if PLATFORM_WINDOWS
#include "Engine/Core/Config/PlatformSettings.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/File.h"
#endif
#endif

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
float Graphics::GIProbesSpacing = 100;
bool Graphics::GICascadesBlending = false;
int32 Graphics::GlobalSurfaceAtlasResolution = 2048;
PostProcessSettings Graphics::PostProcessSettings;
bool Graphics::GammaColorSpace = true;
bool Graphics::SpreadWorkload = true;
#if !BUILD_RELEASE || USE_EDITOR
float Graphics::TestValue = 0.0f;
#endif
float Graphics::MotionVectors::MinObjectScreenSize = 0.02f;
float Graphics::Shadows::MinObjectPixelSize = 2.0f;
bool Graphics::PostProcessing::ColorGradingVolumeLUT = true;

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
    Graphics::GIProbesSpacing = GIProbesSpacing;
    Graphics::GICascadesBlending = GICascadesBlending;
    Graphics::GlobalSurfaceAtlasResolution = GlobalSurfaceAtlasResolution;
    Graphics::PostProcessSettings = ::PostProcessSettings();
    Graphics::PostProcessSettings.BlendWith(PostProcessSettings, 1.0f);
    Graphics::GammaColorSpace = GammaColorSpace;
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
    PROFILE_MEM(Graphics);

    // Create and initialize graphics device
    LOG_FLOOR();
    LOG(Info, "Creating Graphics Device...");
    PixelFormatExtensions::Init();
    GPUDevice* device = nullptr;

    // Clearing GPU shaders cache and PSOs storage to run with a empty-state as first run
#if PLATFORM_DESKTOP
    bool clearShadersCache = CommandLine::Get(TEXT("gpuclear"));

    // Auto-clear PSO/cache files on startup after game updates
#if PLATFORM_WINDOWS && !USE_EDITOR
    int32 shaderCacheVersion = PlatformSettings::Get()->ShaderCacheVersion, loadedCacheVersion = -1;
    if (shaderCacheVersion != 0)
    {
        const String shaderVersionPath = Globals::ProductLocalFolder / TEXT("ShaderVersion.cache");
        if (auto* shaderVersionFile = File::Open(shaderVersionPath, FileMode::OpenExisting, FileAccess::Read))
        {
            shaderVersionFile->Read(&loadedCacheVersion, sizeof(loadedCacheVersion));
            Delete(shaderVersionFile);
        }
        if (shaderCacheVersion != loadedCacheVersion)
        {
            // Version mismatch (eg. after game update), so clear driver cache and save the current version
            LOG(Info, "Loaded shaders cache version: {}, current version: {}", loadedCacheVersion, shaderCacheVersion);
            clearShadersCache = true;
            if (auto* shaderVersionFile = File::Open(shaderVersionPath, FileMode::CreateAlways, FileAccess::Write))
            {
                shaderVersionFile->Write(&shaderCacheVersion, sizeof(shaderCacheVersion));
                Delete(shaderVersionFile);
            }
            else
                LOG(Error, "Failed to save shaders cache version to '{}'", shaderVersionPath);
        }
    }
#endif

    if (clearShadersCache)
    {
        PROFILE_CPU_NAMED("Clear Shader Cache");
        LOG(Info, "Cleaning GPU shaders cache");
        auto ClearFolder = [](Array<String>& files, const String& folder)
        {
            files.Clear();
            FileSystem::DirectoryGetFiles(files, folder, TEXT("*"), DirectorySearchOption::TopDirectoryOnly);
            for (const String& file : files)
                FileSystem::DeleteFile(file);
        };
        Array<String> files;

#if USE_EDITOR
        // Editor shaders cache
        ClearFolder(files, GPU_CACHE_FOLDER / TEXT("Shaders"));
#endif
#if PLATFORM_WINDOWS
        String localAppData;
        FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, localAppData);

        // Disk Cleanup utility clears folder %LOCALAPPDATA%\D3DSCache
        String path = localAppData / TEXT("D3DSCache");
        if (FileSystem::DirectoryExists(path))
        {
            FileSystem::DeleteDirectory(path);
            FileSystem::CreateDirectory(path);
        }

        // Vulkan
        FileSystem::DeleteFile(GPU_CACHE_FOLDER / TEXT("VulkanPipeline.cache"));
        FileSystem::DeleteFile(GPU_CACHE_FOLDER / TEXT("VulkanValidation.cache"));

        // DirectX 12
        FileSystem::DeleteFile(GPU_CACHE_FOLDER / TEXT("D3D12Library.cache"));

        // NVIDIA
        ClearFolder(files, localAppData / TEXT("/NVIDIA/DXCache"));
        ClearFolder(files, localAppData / TEXT("/../LocalLow/NVIDIA/PerDriverVersion/DXCache"));

        // AMD
        ClearFolder(files, localAppData / TEXT("/AMD/DxCache"));
        ClearFolder(files, localAppData / TEXT("/AMD/DxcCache"));
        ClearFolder(files, localAppData / TEXT("/../LocalLow/AMD/DxCache"));
        ClearFolder(files, localAppData / TEXT("/../LocalLow/AMD/DxcCache"));

        // Intel
        ClearFolder(files, localAppData / TEXT("/../LocalLow/Intel/ShaderCache"));
#endif
    }
#endif

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
        extern GPUDevice* CreateGPUDevicePS4();
        if (!device)
            device = CreateGPUDevicePS4();
#endif
#if GRAPHICS_API_PS5
        extern GPUDevice* CreateGPUDevicePS5();
        if (!device)
            device = CreateGPUDevicePS5();
#endif
#if GRAPHICS_API_WEBGPU
        extern GPUDevice* CreateGPUDeviceWebGPU();
        if (!device)
            device = CreateGPUDeviceWebGPU();
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
#if !USE_EDITOR && BUILD_RELEASE && !PLATFORM_CONSOLE
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
    LOG_FLOOR();

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

#if PLATFORM_WEB && !GRAPHICS_API_WEBGPU
// Fix missing method when using Null backend on Web
void SetWebGPUTextureViewSampler(GPUTextureView* view, uint32 samplerType)
{
}
#endif
