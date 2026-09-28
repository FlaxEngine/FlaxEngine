// Copyright (c) Wojciech Figat. All rights reserved.

#include "XR.h"
#include "XRSettings.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#if XR_API_OPENXR
#include "OpenXR/OpenXRInstance.h"
#include "OpenXR/OpenXRSession.h"
#endif

class XRService : public EngineService
{
public:
    XRService()
        : EngineService(TEXT("XR"), -45)
    {
    }

    bool Init() override;
    void Update() override;
    void Draw() override;
    void BeforeExit() override;
    void Dispose() override;
};

XRService XRServiceInstance;

bool XR::IsAvailable()
{
#if XR_API_OPENXR
    return OpenXRInstance::IsReady();
#else
    return false;
#endif
}

bool XR::IsActive()
{
#if XR_API_OPENXR
    return OpenXRSession::IsRunning();
#else
    return false;
#endif
}

StringView XR::GetRuntimeName()
{
#if XR_API_OPENXR
    return OpenXRInstance::RuntimeName;
#else
    return StringView::Empty;
#endif
}

StringView XR::GetRuntimeVersion()
{
#if XR_API_OPENXR
    return OpenXRInstance::RuntimeVersion;
#else
    return StringView::Empty;
#endif
}

StringView XR::GetSystemName()
{
#if XR_API_OPENXR
    return OpenXRInstance::SystemName;
#else
    return StringView::Empty;
#endif
}

float XR::GetDisplayRefreshRate()
{
#if XR_API_OPENXR
    return OpenXRSession::GetDisplayRefreshRate();
#else
    return 0.0f;
#endif
}

bool XRService::Init()
{
    // Check if use XR
    bool useXR = XRSettings::Get()->Enabled;
    if (CommandLine::Options.NoXR.HasValue())
        useXR = !CommandLine::Options.NoXR.GetValue();
    else if (CommandLine::Options.XR.HasValue())
        useXR = CommandLine::Options.XR.GetValue();
    if (!useXR)
        return false;

#if XR_API_OPENXR
    // Missing runtime or head-mounted display is not an error (XR stays unavailable)
    OpenXRInstance::Init();
    Engine::PostDraw.Bind<&OpenXRSession::PostDraw>();
#else
    LOG(Info, "XR unavailable (not supported on this platform)");
#endif
    return false;
}

void XRService::Update()
{
#if XR_API_OPENXR
    OpenXRSession::Update();
#endif
}

void XRService::Draw()
{
#if XR_API_OPENXR
    OpenXRSession::Draw();
#endif
}

void XRService::BeforeExit()
{
#if XR_API_OPENXR
    // Destroy session before the graphics device
    Engine::PostDraw.Unbind<&OpenXRSession::PostDraw>();
    OpenXRSession::Dispose();
#endif
}

void XRService::Dispose()
{
#if XR_API_OPENXR
    OpenXRInstance::Dispose();
#endif
}
