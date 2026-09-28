// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRSession.h"
#include "OpenXRInstance.h"
#include "OpenXRGraphicsD3D11.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/OpenXR/openxr/openxr_reflection.h>

// Time (in seconds) between attempts to create the session when the head-mounted display is not available
#define OPENXR_SESSION_RETRY_INTERVAL 2.0

namespace
{
    struct Swapchain
    {
        XrSwapchain Handle = XR_NULL_HANDLE;
        int32 Width = 0;
        int32 Height = 0;
        bool IsAcquired = false;
        Array<void*> Images;
    };

    const XrViewConfigurationType ViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    const Color ClearColor(0.1f, 0.3f, 0.6f);
    XrSession Session = XR_NULL_HANDLE;
    XrSpace Space = XR_NULL_HANDLE;
    XrEnvironmentBlendMode BlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    Array<Swapchain> Swapchains;
    Array<XrView> Views;
    Array<XrCompositionLayerProjectionView> ProjectionViews;
    XrFrameState FrameState = { XR_TYPE_FRAME_STATE };
    bool IsSessionRunning = false;
    bool IsSessionExiting = false;
    bool IsFrameWaited = false;
    bool IsFrameBegun = false;
    bool HasProjectionLayer = false;
    bool CreateFailedLogged = false;
    double NextCreateTime = 0.0;

#define XRSTATE(x, value) case x: return TEXT(#x);

    const Char* GetStateString(XrSessionState state)
    {
        switch (state)
        {
        XR_LIST_ENUM_XrSessionState(XRSTATE)
        default:
            return TEXT("XR_SESSION_STATE_UNKNOWN");
        }
    }

#undef XRSTATE

    void StopFrameLoop()
    {
        IsSessionRunning = false;
        IsFrameWaited = false;
        Engine::FramePacer.Unbind();
    }

    void DestroySession()
    {
        if (IsSessionRunning)
            StopFrameLoop();
        for (auto& swapchain : Swapchains)
        {
            OpenXRGraphicsD3D11::ReleaseSwapchainImages(swapchain.Images);
            if (swapchain.Handle != XR_NULL_HANDLE)
                xrDestroySwapchain(swapchain.Handle);
        }
        Swapchains.Clear();
        Views.Clear();
        ProjectionViews.Clear();
        if (Space != XR_NULL_HANDLE)
        {
            xrDestroySpace(Space);
            Space = XR_NULL_HANDLE;
        }
        if (Session != XR_NULL_HANDLE)
        {
            xrDestroySession(Session);
            Session = XR_NULL_HANDLE;
        }
        IsFrameBegun = false;
        HasProjectionLayer = false;
    }

    bool CreateSession()
    {
        // Get head-mounted display system (the headset can be connected later)
        if (OpenXRInstance::SystemId == XR_NULL_SYSTEM_ID && OpenXRInstance::InitSystem())
            return true;
        const XrInstance instance = OpenXRInstance::Instance;
        const XrSystemId systemId = OpenXRInstance::SystemId;

        // Check if graphics device can be used by the runtime
        if (OpenXRGraphicsD3D11::SetupDevice(instance, systemId))
        {
            OpenXRInstance::Dispose();
            return true;
        }

        // Create session
        XrSessionCreateInfo createInfo = { XR_TYPE_SESSION_CREATE_INFO };
        createInfo.next = OpenXRGraphicsD3D11::GetGraphicsBinding();
        createInfo.systemId = systemId;
        XrResult result = xrCreateSession(instance, &createInfo, &Session);
        if (XR_FAILED(result))
        {
            if (!CreateFailedLogged)
            {
                CreateFailedLogged = true;
                LOG(Warning, "Failed to create OpenXR session ({0})", OpenXRInstance::GetResultString(result));
            }
            Session = XR_NULL_HANDLE;
            return true;
        }
        CreateFailedLogged = false;

        // Create reference space (stage if supported, otherwise local)
        uint32 spacesCount = 0;
        Array<XrReferenceSpaceType> spaces;
        result = xrEnumerateReferenceSpaces(Session, 0, &spacesCount, nullptr);
        if (XR_SUCCEEDED(result))
        {
            spaces.Resize(spacesCount);
            result = xrEnumerateReferenceSpaces(Session, spacesCount, &spacesCount, spaces.Get());
        }
        XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceInfo.referenceSpaceType = spaces.Contains(XR_REFERENCE_SPACE_TYPE_STAGE) ? XR_REFERENCE_SPACE_TYPE_STAGE : XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
        if (XR_SUCCEEDED(result))
            result = xrCreateReferenceSpace(Session, &spaceInfo, &Space);
        if (XR_FAILED(result))
        {
            LOG(Warning, "Failed to create OpenXR reference space ({0})", OpenXRInstance::GetResultString(result));
            DestroySession();
            return true;
        }

        // Get environment blend mode
        uint32 blendModesCount = 0;
        Array<XrEnvironmentBlendMode> blendModes;
        result = xrEnumerateEnvironmentBlendModes(instance, systemId, ViewConfigurationType, 0, &blendModesCount, nullptr);
        if (XR_SUCCEEDED(result))
        {
            blendModes.Resize(blendModesCount);
            result = xrEnumerateEnvironmentBlendModes(instance, systemId, ViewConfigurationType, blendModesCount, &blendModesCount, blendModes.Get());
        }
        BlendMode = XR_SUCCEEDED(result) && blendModes.HasItems() ? blendModes[0] : XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

        // Get views
        uint32 viewsCount = 0;
        Array<XrViewConfigurationView> configViews;
        result = xrEnumerateViewConfigurationViews(instance, systemId, ViewConfigurationType, 0, &viewsCount, nullptr);
        if (XR_SUCCEEDED(result))
        {
            configViews.Resize(viewsCount);
            for (auto& configView : configViews)
                configView = { XR_TYPE_VIEW_CONFIGURATION_VIEW };
            result = xrEnumerateViewConfigurationViews(instance, systemId, ViewConfigurationType, viewsCount, &viewsCount, configViews.Get());
        }
        if (XR_FAILED(result) || viewsCount == 0)
        {
            LOG(Warning, "Failed to get OpenXR views ({0})", OpenXRInstance::GetResultString(result));
            DestroySession();
            return true;
        }

        // Get swapchain format
        uint32 formatsCount = 0;
        Array<int64> formats;
        result = xrEnumerateSwapchainFormats(Session, 0, &formatsCount, nullptr);
        if (XR_SUCCEEDED(result))
        {
            formats.Resize(formatsCount);
            result = xrEnumerateSwapchainFormats(Session, formatsCount, &formatsCount, formats.Get());
        }
        if (XR_FAILED(result))
        {
            LOG(Warning, "Failed to get OpenXR swapchain formats ({0})", OpenXRInstance::GetResultString(result));
            DestroySession();
            return true;
        }
        const int64 format = OpenXRGraphicsD3D11::SelectSwapchainFormat(formats);

        // Create swapchain for each view
        Swapchains.Resize(viewsCount);
        Views.Resize(viewsCount);
        ProjectionViews.Resize(viewsCount);
        for (uint32 i = 0; i < viewsCount; i++)
        {
            auto& swapchain = Swapchains[i];
            XrSwapchainCreateInfo swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
            swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
            swapchainInfo.format = format;
            swapchainInfo.sampleCount = 1;
            swapchainInfo.width = configViews[i].recommendedImageRectWidth;
            swapchainInfo.height = configViews[i].recommendedImageRectHeight;
            swapchainInfo.faceCount = 1;
            swapchainInfo.arraySize = 1;
            swapchainInfo.mipCount = 1;
            result = xrCreateSwapchain(Session, &swapchainInfo, &swapchain.Handle);
            if (XR_FAILED(result))
            {
                LOG(Warning, "Failed to create OpenXR swapchain ({0})", OpenXRInstance::GetResultString(result));
                swapchain.Handle = XR_NULL_HANDLE;
                DestroySession();
                return true;
            }
            if (OpenXRGraphicsD3D11::InitSwapchainImages(swapchain.Handle, format, swapchain.Images))
            {
                DestroySession();
                return true;
            }
            swapchain.Width = (int32)swapchainInfo.width;
            swapchain.Height = (int32)swapchainInfo.height;
            Views[i] = { XR_TYPE_VIEW };
            ProjectionViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
        }

        LOG(Info, "OpenXR session created. Views: {0}, Size: {1}x{2}, Format: {3}, Reference Space: {4}", viewsCount, Swapchains[0].Width, Swapchains[0].Height, format, spaceInfo.referenceSpaceType == XR_REFERENCE_SPACE_TYPE_STAGE ? TEXT("Stage") : TEXT("Local"));
        return false;
    }

    void OnSessionStateChanged(XrSessionState state)
    {
        LOG(Info, "OpenXR session state: {0}", GetStateString(state));
        switch (state)
        {
        case XR_SESSION_STATE_READY:
        {
            // Start the frame loop
            XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
            beginInfo.primaryViewConfigurationType = ViewConfigurationType;
            const XrResult result = xrBeginSession(Session, &beginInfo);
            if (XR_FAILED(result))
            {
                LOG(Warning, "Failed to begin OpenXR session ({0})", OpenXRInstance::GetResultString(result));
                break;
            }
            IsSessionRunning = true;
            Engine::FramePacer.Bind<&OpenXRSession::WaitFrame>();
            break;
        }
        case XR_SESSION_STATE_STOPPING:
            // Stop the frame loop (eg. headset got taken off)
            StopFrameLoop();
            xrEndSession(Session);
            break;
        case XR_SESSION_STATE_LOSS_PENDING:
            // Recreate session later (eg. headset got disconnected)
            DestroySession();
            OpenXRInstance::SystemId = XR_NULL_SYSTEM_ID;
            NextCreateTime = Platform::GetTimeSeconds() + OPENXR_SESSION_RETRY_INTERVAL;
            break;
        case XR_SESSION_STATE_EXITING:
            // Runtime requested to exit (eg. user closed the app from the runtime dashboard)
            DestroySession();
            IsSessionExiting = true;
            break;
        default:
            break;
        }
    }

    bool BeginFrame()
    {
        XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
        const XrResult result = xrBeginFrame(Session, &beginInfo);
        IsFrameBegun = XR_SUCCEEDED(result);
        HasProjectionLayer = false;
        return IsFrameBegun;
    }

    void EndFrame()
    {
        IsFrameBegun = false;

        // Release swapchain images
        for (auto& swapchain : Swapchains)
        {
            if (swapchain.IsAcquired)
            {
                swapchain.IsAcquired = false;
                XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
                xrReleaseSwapchainImage(swapchain.Handle, &releaseInfo);
            }
        }

        // Submit projection layer (skipped if frame was not rendered)
        XrCompositionLayerProjection layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        layer.space = Space;
        layer.viewCount = (uint32)ProjectionViews.Count();
        layer.views = ProjectionViews.Get();
        const XrCompositionLayerBaseHeader* layers[] = { (XrCompositionLayerBaseHeader*)&layer };
        XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
        endInfo.displayTime = FrameState.predictedDisplayTime;
        endInfo.environmentBlendMode = BlendMode;
        endInfo.layerCount = HasProjectionLayer ? 1 : 0;
        endInfo.layers = layers;
        xrEndFrame(Session, &endInfo);
        HasProjectionLayer = false;
    }
}

void OpenXRSession::Update()
{
    const XrInstance instance = OpenXRInstance::Instance;
    if (instance == XR_NULL_HANDLE)
        return;
    PROFILE_CPU();

    // Process runtime events
    XrEventDataBuffer event = { XR_TYPE_EVENT_DATA_BUFFER };
    while (xrPollEvent(instance, &event) == XR_SUCCESS)
    {
        switch (event.type)
        {
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            // Runtime is shutting down (eg. SteamVR got closed)
            LOG(Warning, "XR unavailable, OpenXR runtime is shutting down");
            DestroySession();
            OpenXRInstance::Dispose();
            return;
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            const auto& stateChanged = *(XrEventDataSessionStateChanged*)&event;
            if (stateChanged.session == Session)
                OnSessionStateChanged(stateChanged.state);
            break;
        }
        default:
            break;
        }
        event = { XR_TYPE_EVENT_DATA_BUFFER };
    }

    // Create session once the head-mounted display is available
    const double time = Platform::GetTimeSeconds();
    if (Session == XR_NULL_HANDLE && !IsSessionExiting && time >= NextCreateTime)
    {
        NextCreateTime = time + OPENXR_SESSION_RETRY_INTERVAL;
        CreateSession();
    }
}

void OpenXRSession::Draw()
{
    if (!IsFrameWaited)
        return;
    PROFILE_CPU();
    IsFrameWaited = false;
    if (!BeginFrame() || !FrameState.shouldRender)
        return;

    // Locate views at the predicted display time
    XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
    locateInfo.viewConfigurationType = ViewConfigurationType;
    locateInfo.displayTime = FrameState.predictedDisplayTime;
    locateInfo.space = Space;
    XrViewState viewState = { XR_TYPE_VIEW_STATE };
    uint32 viewsCount = 0;
    XrResult result = xrLocateViews(Session, &locateInfo, &viewState, (uint32)Views.Count(), &viewsCount, Views.Get());
    if (XR_FAILED(result) || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
        return;

    // Render each view
    for (int32 i = 0; i < Swapchains.Count(); i++)
    {
        auto& swapchain = Swapchains[i];
        uint32 imageIndex = 0;
        XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        result = xrAcquireSwapchainImage(swapchain.Handle, &acquireInfo, &imageIndex);
        if (XR_FAILED(result))
            return;
        XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        result = xrWaitSwapchainImage(swapchain.Handle, &waitInfo);
        if (XR_FAILED(result))
            return;
        swapchain.IsAcquired = true;

        // TODO: render the scene from the view
        OpenXRGraphicsD3D11::ClearSwapchainImage(swapchain.Images[imageIndex], ClearColor);

        auto& projectionView = ProjectionViews[i];
        projectionView.pose = Views[i].pose;
        projectionView.fov = Views[i].fov;
        projectionView.subImage.swapchain = swapchain.Handle;
        projectionView.subImage.imageRect.offset = { 0, 0 };
        projectionView.subImage.imageRect.extent = { swapchain.Width, swapchain.Height };
    }
    HasProjectionLayer = true;
}

void OpenXRSession::PostDraw()
{
    if (!IsFrameBegun)
        return;
    PROFILE_CPU();
    EndFrame();
}

void OpenXRSession::WaitFrame()
{
    if (!IsSessionRunning)
        return;

    // End the previous frame if it was not drawn (xrWaitFrame requires the previous frame to be begun)
    if (IsFrameWaited)
    {
        IsFrameWaited = false;
        if (BeginFrame())
            EndFrame();
    }

    PROFILE_CPU();
    FrameState = { XR_TYPE_FRAME_STATE };
    XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
    const XrResult result = xrWaitFrame(Session, &waitInfo, &FrameState);
    IsFrameWaited = XR_SUCCEEDED(result);
}

void OpenXRSession::Dispose()
{
    DestroySession();
    IsSessionExiting = false;
    CreateFailedLogged = false;
    NextCreateTime = 0.0;
}

bool OpenXRSession::IsRunning()
{
    return IsSessionRunning;
}

#endif
