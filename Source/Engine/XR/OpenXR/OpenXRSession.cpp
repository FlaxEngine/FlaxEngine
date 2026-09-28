// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRSession.h"
#include "OpenXRInstance.h"
#include "OpenXRGraphicsD3D11.h"
#include "OpenXRConvert.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Level/LargeWorlds.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/XR/XRSettings.h"
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
        Array<GPUTexture*> Images;
        SceneRenderTask* Task = nullptr;
    };

    const XrViewConfigurationType ViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
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
    bool IsMirrorEnabled = false;
    bool WasCustomRendering = false;
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

    void DrawMirror(RenderTask* task, GPUContext* context)
    {
        const auto mainTask = (SceneRenderTask*)task;
        GPUTextureView* output = mainTask->GetOutputView();
        GPUTexture* image = Swapchains.HasItems() ? Swapchains[0].Task->Output : nullptr;
        if (output == nullptr || image == nullptr)
            return;
        PROFILE_CPU_NAMED("XR.Mirror");

        // Draw the left eye image to fill the main view output (keep the aspect ratio and crop the edges)
        const Viewport outputViewport = mainTask->GetOutputViewport();
        const float scale = Math::Max(outputViewport.Width / (float)image->Width(), outputViewport.Height / (float)image->Height());
        const float width = (float)image->Width() * scale;
        const float height = (float)image->Height() * scale;
        context->SetRenderTarget(output);
        context->FlushState(); // Unbind the eye image from the output before binding it as shader resource
        context->SetViewport(Viewport((outputViewport.Width - width) * 0.5f, (outputViewport.Height - height) * 0.5f, width, height));
        context->SetScissor(Rectangle(0, 0, outputViewport.Width, outputViewport.Height));
        context->Draw(image);
        context->ResetSR(); // The eye image is used as render target in the next frames
        context->ResetRenderTarget();
    }

    void SetupViewRender(RenderContext& renderContext)
    {
        // Use FXAA instead of TAA
        // TODO: support temporal anti-aliasing for each eye
        auto& antiAliasing = renderContext.List->Settings.AntiAliasing;
        if (antiAliasing.Mode == AntialiasingMode::TemporalAntialiasing)
        {
            antiAliasing.Mode = AntialiasingMode::FastApproximateAntialiasing;
            renderContext.List->Setup.UseTemporalAAJitter = false;
        }
    }

    void SetMirror(bool enable)
    {
        // Use the main view to mirror the headset image instead of rendering the scene again
        const auto mainTask = MainRenderTask::Instance;
        if (IsMirrorEnabled == enable || mainTask == nullptr)
            return;
        IsMirrorEnabled = enable;
        if (enable)
        {
            WasCustomRendering = mainTask->IsCustomRendering;
            mainTask->IsCustomRendering = true;
            mainTask->Render.Bind(DrawMirror);
        }
        else
        {
            mainTask->IsCustomRendering = WasCustomRendering;
            mainTask->Render.Unbind(DrawMirror);
        }
    }

    void DestroySession()
    {
        if (IsSessionRunning)
            StopFrameLoop();
        SetMirror(false);
        for (auto& swapchain : Swapchains)
        {
            SAFE_DELETE(swapchain.Task);
            SAFE_DELETE_GPU_RESOURCES(swapchain.Images);
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
        const float renderScale = XRSettings::Get()->RenderScale;
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
            swapchainInfo.width = (uint32)Math::Clamp(Math::RoundToInt((float)configViews[i].recommendedImageRectWidth * renderScale), 1, (int32)configViews[i].maxImageRectWidth);
            swapchainInfo.height = (uint32)Math::Clamp(Math::RoundToInt((float)configViews[i].recommendedImageRectHeight * renderScale), 1, (int32)configViews[i].maxImageRectHeight);
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
            swapchain.Width = (int32)swapchainInfo.width;
            swapchain.Height = (int32)swapchainInfo.height;
            if (OpenXRGraphicsD3D11::InitSwapchainImages(swapchain.Handle, format, swapchain.Width, swapchain.Height, swapchain.Images))
            {
                DestroySession();
                return true;
            }

            // Create task for rendering the view
            swapchain.Task = New<SceneRenderTask>();
            swapchain.Task->Enabled = false;
            swapchain.Task->Order = -100; // Render before the main view that is used to mirror the headset image
            swapchain.Task->SetupRender.Bind(SetupViewRender);
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
        SetMirror(false);

        // Release swapchain images
        for (auto& swapchain : Swapchains)
        {
            swapchain.Task->Enabled = false;
            swapchain.Task->Output = nullptr;
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

    // Use the main camera as the tracking space origin
    // TODO: add XR rig actor to control the tracking space origin
    const Camera* camera = Camera::GetMainCamera();
    const Transform origin = camera ? Transform(camera->GetPosition(), camera->GetOrientation()) : Transform::Identity;

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

        // Setup the view
        auto task = swapchain.Task;
        auto& view = task->View;
        if (camera)
        {
            view.Near = camera->GetNearPlane();
            view.Far = camera->GetFarPlane();
            view.RenderLayersMask = camera->RenderLayersMask;
            view.Flags = camera->RenderFlags;
            view.Mode = camera->RenderMode;
        }
        const Transform pose = origin.LocalToWorld(OpenXRConvert::ToTransform(Views[i].pose));
        LargeWorlds::UpdateOrigin(view.Origin, pose.Translation);
        view.Position = pose.Translation - view.Origin;
        view.Direction = pose.GetForward();
        Matrix viewMatrix, projectionMatrix;
        Matrix::LookAt(view.Position, view.Position + view.Direction, pose.GetUp(), viewMatrix);
        OpenXRConvert::ToProjection(Views[i].fov, view.Near, view.Far, projectionMatrix);
        view.SetUp(viewMatrix, projectionMatrix);

        // Disable motion blur (head movement would blur the whole view)
        view.Flags &= ~ViewFlags::MotionBlur;

        // Render to the swapchain image
        task->Output = swapchain.Images[imageIndex];
        task->Enabled = true;

        auto& projectionView = ProjectionViews[i];
        projectionView.pose = Views[i].pose;
        projectionView.fov = Views[i].fov;
        projectionView.subImage.swapchain = swapchain.Handle;
        projectionView.subImage.imageRect.offset = { 0, 0 };
        projectionView.subImage.imageRect.extent = { swapchain.Width, swapchain.Height };
    }
    HasProjectionLayer = true;
    SetMirror(true);
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
