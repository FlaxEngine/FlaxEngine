// Copyright (c) Wojciech Figat. All rights reserved.

#include "XR.h"
#include "XRSettings.h"
#include "XRController.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Level/Actors/Camera.h"
#if XR_API_OPENXR
#include "OpenXR/OpenXRInstance.h"
#include "OpenXR/OpenXRInput.h"
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

Transform XR::GetTrackingOrigin()
{
    const Camera* camera = Camera::GetMainCamera();
    return camera ? Transform(camera->GetPosition(), camera->GetOrientation()) : Transform::Identity;
}

bool XR::GetHeadPose(Transform& pose)
{
#if XR_API_OPENXR
    if (OpenXRSession::IsRunning())
        return OpenXRInput::GetHeadPose(pose);
#endif
    return false;
}

bool XR::GetHandPose(XRHand hand, XRPoseType type, Transform& pose)
{
#if XR_API_OPENXR
    if (OpenXRSession::IsRunning())
        return OpenXRInput::GetHandPose(hand, type, pose);
#endif
    return false;
}

void XR::Vibrate(XRHand hand, float amplitude, float duration, float frequency)
{
#if XR_API_OPENXR
    if (OpenXRSession::IsRunning())
        OpenXRInput::Vibrate(hand, amplitude, duration, frequency);
#endif
}

Gamepad* XR::GetGamepad()
{
#if XR_API_OPENXR
    return OpenXRInput::GetGamepad();
#else
    return nullptr;
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
    XRController::UpdateAll();
}

void XRService::Draw()
{
#if XR_API_OPENXR
    OpenXRSession::Draw();
#endif

    // Use the latest poses for rendering
    XRController::UpdateAll();
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
