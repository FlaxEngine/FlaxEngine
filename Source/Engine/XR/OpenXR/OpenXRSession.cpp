// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRSession.h"
#include "OpenXRInstance.h"
#include "OpenXRInput.h"
#include "OpenXRGraphicsD3D11.h"
#include "OpenXRConvert.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Level/LargeWorlds.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/XR/XR.h"
#include "Engine/XR/XRSettings.h"
#include <ThirdParty/OpenXR/openxr/openxr_reflection.h>

// Time (in seconds) between attempts to create the session when the head-mounted display is not available
#define OPENXR_SESSION_RETRY_INTERVAL 2.0

namespace
{
    struct Swapchain
    {
        XrSwapchain Handle = XR_NULL_HANDLE;
        XrSwapchain DepthHandle = XR_NULL_HANDLE;
        int32 Width = 0;
        int32 Height = 0;
        bool IsAcquired = false;
        bool IsDepthAcquired = false;
        bool HasDepth = false;
        Array<GPUTexture*> Images;
        Array<GPUTexture*> DepthImages;
        GPUTexture* DepthOutput = nullptr;
        SceneRenderTask* Task = nullptr;
    };

    const XrViewConfigurationType ViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    XrSession Session = XR_NULL_HANDLE;
    XrSpace Space = XR_NULL_HANDLE;
    XrEnvironmentBlendMode BlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    Array<Swapchain> Swapchains;
    Array<XrView> Views;
    Array<XrCompositionLayerProjectionView> ProjectionViews;
    Array<XrCompositionLayerDepthInfoKHR> DepthInfos;
    RenderView ProxyView;
    PFN_xrGetDisplayRefreshRateFB GetDisplayRefreshRateFB = nullptr;
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

    bool AcquireImage(XrSwapchain swapchain, bool& isAcquired, uint32& imageIndex)
    {
        XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        XrResult result = xrAcquireSwapchainImage(swapchain, &acquireInfo, &imageIndex);
        if (XR_FAILED(result))
            return true;
        isAcquired = true;
        PROFILE_CPU_NAMED("xrWaitSwapchainImage");
        XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        result = xrWaitSwapchainImage(swapchain, &waitInfo);
        return XR_FAILED(result);
    }

    void ReleaseImage(XrSwapchain swapchain, bool& isAcquired)
    {
        if (!isAcquired)
            return;
        isAcquired = false;
        XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        xrReleaseSwapchainImage(swapchain, &releaseInfo);
    }

    void CopyViewDepth(RenderTask* task, GPUContext* context)
    {
        for (auto& swapchain : Swapchains)
        {
            if (swapchain.Task != task)
                continue;

            // Copy the scene depth to the depth swapchain image
            GPUTexture* depthBuffer = swapchain.Task->Buffers->DepthBuffer;
            GPUTexture* depthOutput = swapchain.DepthOutput;
            if (depthOutput && depthBuffer->Size() == depthOutput->Size() && depthBuffer->Format() == depthOutput->Format())
            {
                PROFILE_CPU_NAMED("XR.CopyDepth");
                context->CopyResource(depthOutput, depthBuffer);
                swapchain.HasDepth = true;
            }
            break;
        }
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

    void SetupProxyView()
    {
        // Use the orientation of the first view and the center between the views
        const RenderView& firstView = Swapchains[0].Task->View;
        Float3 center = Float3::Zero;
        for (const auto& swapchain : Swapchains)
            center += swapchain.Task->View.Position;
        center /= (float)Swapchains.Count();
        const Float3 up = firstView.IV.GetUp();
        Matrix viewMatrix, projectionMatrix;
        Matrix::LookAt(center, center + firstView.Direction, up, viewMatrix);

        // Get the frustum sides that contain the frustums of all views (as tangents of the angles)
        float tanLeft = 0.01f, tanRight = 0.01f, tanDown = 0.01f, tanUp = 0.01f, depthScale = 1.0f;
        for (int32 i = 0; i < Swapchains.Count(); i++)
        {
            const XrFovf& fov = Views[i].fov;
            const Float3 corners[4] =
            {
                Float3(Math::Tan(fov.angleLeft), Math::Tan(fov.angleUp), 1.0f),
                Float3(Math::Tan(fov.angleRight), Math::Tan(fov.angleUp), 1.0f),
                Float3(Math::Tan(fov.angleRight), Math::Tan(fov.angleDown), 1.0f),
                Float3(Math::Tan(fov.angleLeft), Math::Tan(fov.angleDown), 1.0f),
            };
            for (const Float3& corner : corners)
            {
                Float3 direction;
                Float3::TransformNormal(corner, Swapchains[i].Task->View.IV, direction);
                Float3::TransformNormal(direction, viewMatrix, direction);
                depthScale = Math::Max(depthScale, direction.Z);
                direction /= Math::Max(direction.Z, ZeroTolerance);
                tanLeft = Math::Max(tanLeft, -direction.X);
                tanRight = Math::Max(tanRight, direction.X);
                tanDown = Math::Max(tanDown, -direction.Y);
                tanUp = Math::Max(tanUp, direction.Y);
            }
        }

        // Move the view back so the view positions are inside its frustum
        float offset = 0.0f;
        for (const auto& swapchain : Swapchains)
        {
            const Float3 position = Float3::Transform(swapchain.Task->View.Position, viewMatrix);
            offset = Math::Max(offset, Math::Max(position.X / tanRight, -position.X / tanLeft, position.Y / tanUp, -position.Y / tanDown) - position.Z);
        }

        // Setup view (models LOD, shadow cascades and particles sorting are the same in all views)
        ProxyView = firstView;
        ProxyView.Position = center - firstView.Direction * offset;
        ProxyView.Far = offset + firstView.Far * depthScale;
        Matrix::LookAt(ProxyView.Position, ProxyView.Position + ProxyView.Direction, up, viewMatrix);
        Matrix::PerspectiveOffCenter(-tanLeft * ProxyView.Near, tanRight * ProxyView.Near, -tanDown * ProxyView.Near, tanUp * ProxyView.Near, ProxyView.Near, ProxyView.Far, projectionMatrix);
        ProxyView.SetUp(viewMatrix, projectionMatrix);

        // Cull the first view with the frustum of all views so its shadow maps contain all the lights visible in the other views
        Swapchains[0].Task->View.CullingFrustum = ProxyView.Frustum;
    }

#if COMPILE_WITH_DEBUG_DRAW
    void DrawViewDebug(GPUContext* context, RenderContext& renderContext)
    {
        // Draw debug shapes if enabled for the view (eg. via camera render flags)
        if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::DebugDraw))
            DebugDraw::Draw(renderContext, renderContext.Task->GetOutputView());
    }
#endif

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
            SAFE_DELETE_GPU_RESOURCES(swapchain.DepthImages);
            if (swapchain.Handle != XR_NULL_HANDLE)
                xrDestroySwapchain(swapchain.Handle);
            if (swapchain.DepthHandle != XR_NULL_HANDLE)
                xrDestroySwapchain(swapchain.DepthHandle);
        }
        Swapchains.Clear();
        Views.Clear();
        ProjectionViews.Clear();
        DepthInfos.Clear();
        GetDisplayRefreshRateFB = nullptr;
        OpenXRInput::DisposeSession();
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

        // Setup controllers input (session can be used without it)
        if (!OpenXRInput::IsInitialized())
            OpenXRInput::Init(instance);
        OpenXRInput::InitSession(Session, Space);

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
        const int64 depthFormat = XRSettings::Get()->SubmitDepth && OpenXRInstance::SupportsDepthLayer ? OpenXRGraphicsD3D11::SelectDepthSwapchainFormat(formats) : 0;

        // Create swapchain for each view
        const float renderScale = XRSettings::Get()->RenderScale;
        Swapchains.Resize(viewsCount);
        Views.Resize(viewsCount);
        ProjectionViews.Resize(viewsCount);
        DepthInfos.Resize(viewsCount);
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
            if (OpenXRGraphicsD3D11::InitSwapchainImages(swapchain.Handle, format, swapchain.Width, swapchain.Height, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget, swapchain.Images))
            {
                DestroySession();
                return true;
            }

            // Create depth swapchain for the view (used by the runtime to improve the reprojection)
            if (depthFormat != 0)
            {
                swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                swapchainInfo.format = depthFormat;
                result = xrCreateSwapchain(Session, &swapchainInfo, &swapchain.DepthHandle);
                if (XR_FAILED(result))
                {
                    LOG(Warning, "Failed to create OpenXR depth swapchain ({0})", OpenXRInstance::GetResultString(result));
                    swapchain.DepthHandle = XR_NULL_HANDLE;
                    DestroySession();
                    return true;
                }
                if (OpenXRGraphicsD3D11::InitSwapchainImages(swapchain.DepthHandle, depthFormat, swapchain.Width, swapchain.Height, GPUTextureFlags::DepthStencil, swapchain.DepthImages))
                {
                    DestroySession();
                    return true;
                }
            }

            // Create task for rendering the view
            swapchain.Task = New<SceneRenderTask>();
            swapchain.Task->Enabled = false;
            swapchain.Task->Order = -100 + (int32)i; // Render before the main view that is used to mirror the headset image (first view goes first)
            if (depthFormat != 0)
                swapchain.Task->Render.Bind(CopyViewDepth);
#if COMPILE_WITH_DEBUG_DRAW
            swapchain.Task->PostRender.Bind(DrawViewDebug);
#endif
            if (viewsCount > 1)
            {
                // Use the same models LOD and shadow projections in all views
                swapchain.Task->ProxyView = &ProxyView;
            }
            if (i > 0)
            {
                // Share the effects state (eg. global illumination), the shadow maps and the exposure of the first view so both eyes look the same
                swapchain.Task->Buffers->LinkedCustomBuffers = Swapchains[0].Task->Buffers;
                swapchain.Task->Buffers->LinkedShadows = Swapchains[0].Task->Buffers;
                swapchain.Task->Buffers->LinkedEyeAdaptation = Swapchains[0].Task->Buffers;
            }
            Views[i] = { XR_TYPE_VIEW };
            ProjectionViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
            DepthInfos[i] = { XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR };
        }

        LOG(Info, "OpenXR session created. Views: {0}, Size: {1}x{2}, Format: {3}, Depth Format: {4}, Reference Space: {5}", viewsCount, Swapchains[0].Width, Swapchains[0].Height, format, depthFormat, spaceInfo.referenceSpaceType == XR_REFERENCE_SPACE_TYPE_STAGE ? TEXT("Stage") : TEXT("Local"));
        return false;
    }

    void InitDisplayRefreshRate()
    {
        // Get extension functions
        const XrInstance instance = OpenXRInstance::Instance;
        PFN_xrEnumerateDisplayRefreshRatesFB enumerateDisplayRefreshRates = nullptr;
        PFN_xrRequestDisplayRefreshRateFB requestDisplayRefreshRate = nullptr;
        xrGetInstanceProcAddr(instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)&GetDisplayRefreshRateFB);
        xrGetInstanceProcAddr(instance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction*)&enumerateDisplayRefreshRates);
        xrGetInstanceProcAddr(instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction*)&requestDisplayRefreshRate);
        if (GetDisplayRefreshRateFB == nullptr || enumerateDisplayRefreshRates == nullptr || requestDisplayRefreshRate == nullptr)
        {
            GetDisplayRefreshRateFB = nullptr;
            return;
        }

        // Get refresh rates supported by the headset
        uint32 ratesCount = 0;
        Array<float> rates;
        XrResult result = enumerateDisplayRefreshRates(Session, 0, &ratesCount, nullptr);
        if (XR_SUCCEEDED(result))
        {
            rates.Resize(ratesCount);
            result = enumerateDisplayRefreshRates(Session, ratesCount, &ratesCount, rates.Get());
        }
        float rate = 0.0f;
        if (XR_SUCCEEDED(result))
            result = GetDisplayRefreshRateFB(Session, &rate);
        if (XR_FAILED(result) || rates.IsEmpty())
        {
            LOG(Warning, "Failed to get OpenXR display refresh rates ({0})", OpenXRInstance::GetResultString(result));
            return;
        }
        String ratesText;
        for (const float e : rates)
        {
            if (ratesText.HasChars())
                ratesText += TEXT(", ");
            ratesText += String::Format(TEXT("{0}"), e);
        }
        LOG(Info, "OpenXR display refresh rate: {0} Hz, Supported: {1}", rate, ratesText);

        // Request the refresh rate from the settings (use the closest one supported by the headset)
        const float requestedRate = XRSettings::Get()->DisplayRefreshRate;
        if (requestedRate <= 0.0f)
            return;
        float closestRate = rates[0];
        for (const float e : rates)
        {
            if (Math::Abs(e - requestedRate) < Math::Abs(closestRate - requestedRate))
                closestRate = e;
        }
        if (Math::NearEqual(closestRate, rate))
            return;
        result = requestDisplayRefreshRate(Session, closestRate);
        if (XR_FAILED(result))
            LOG(Warning, "Failed to request OpenXR display refresh rate {0} Hz ({1})", closestRate, OpenXRInstance::GetResultString(result));
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
            if (OpenXRInstance::SupportsDisplayRefreshRate)
                InitDisplayRefreshRate();
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
        PROFILE_CPU_NAMED("xrBeginFrame");
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
        for (int32 i = 0; i < Swapchains.Count(); i++)
        {
            auto& swapchain = Swapchains[i];
            swapchain.Task->Enabled = false;
            swapchain.Task->Output = nullptr;
            swapchain.DepthOutput = nullptr;
            ReleaseImage(swapchain.Handle, swapchain.IsAcquired);
            ReleaseImage(swapchain.DepthHandle, swapchain.IsDepthAcquired);

            // Submit depth only if it was rendered
            ProjectionViews[i].next = swapchain.HasDepth ? &DepthInfos[i] : nullptr;
            swapchain.HasDepth = false;
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
        PROFILE_CPU_NAMED("xrEndFrame");
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
        case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
        {
            const auto& refreshRateChanged = *(XrEventDataDisplayRefreshRateChangedFB*)&event;
            LOG(Info, "OpenXR display refresh rate changed to {0} Hz", refreshRateChanged.toDisplayRefreshRate);
            break;
        }
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
            OpenXRInput::LogInteractionProfiles();
            break;
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

    // Locate controllers again for rendering
    OpenXRInput::LocatePoses(FrameState.predictedDisplayTime);

    const Camera* camera = Camera::GetMainCamera();
    const Transform origin = XR::GetTrackingOrigin();

    // Render each view
    for (int32 i = 0; i < Swapchains.Count(); i++)
    {
        auto& swapchain = Swapchains[i];
        uint32 imageIndex = 0, depthImageIndex = 0;
        if (AcquireImage(swapchain.Handle, swapchain.IsAcquired, imageIndex))
            return;
        if (swapchain.DepthHandle != XR_NULL_HANDLE && AcquireImage(swapchain.DepthHandle, swapchain.IsDepthAcquired, depthImageIndex))
            return;

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
        if (i == 0)
            LargeWorlds::UpdateOrigin(view.Origin, pose.Translation);
        else
            view.Origin = Swapchains[0].Task->View.Origin; // Use the same origin in all views to share the shadow maps
        view.Position = pose.Translation - view.Origin;
        view.Direction = pose.GetForward();
        Matrix viewMatrix, projectionMatrix;
        Matrix::LookAt(view.Position, view.Position + view.Direction, pose.GetUp(), viewMatrix);
        OpenXRConvert::ToProjection(Views[i].fov, view.Near, view.Far, projectionMatrix);
        view.SetUp(viewMatrix, projectionMatrix);

        // Disable effects that don't work in the head-mounted display (head movement would blur the whole view, focus doesn't follow the eyes, flares differ between the eyes)
        view.Flags &= ~(ViewFlags::MotionBlur | ViewFlags::DepthOfField | ViewFlags::LensFlares);

        // Render to the swapchain image
        task->Output = swapchain.Images[imageIndex];
        task->Enabled = true;

        // Drop the history of the temporal effects if the view was not rendered in the previous frame (eg. headset was taken off)
        if (task->LastUsedFrame + 1 < Engine::FrameCount)
            task->IsCameraCut = true;

        auto& projectionView = ProjectionViews[i];
        projectionView.pose = Views[i].pose;
        projectionView.fov = Views[i].fov;
        projectionView.subImage.swapchain = swapchain.Handle;
        projectionView.subImage.imageRect.offset = { 0, 0 };
        projectionView.subImage.imageRect.extent = { swapchain.Width, swapchain.Height };

        // Setup depth (copied from the scene depth buffer after rendering)
        if (swapchain.DepthHandle != XR_NULL_HANDLE)
        {
            swapchain.DepthOutput = swapchain.DepthImages[depthImageIndex];
            auto& depthInfo = DepthInfos[i];
            depthInfo.subImage.swapchain = swapchain.DepthHandle;
            depthInfo.subImage.imageRect = projectionView.subImage.imageRect;
            depthInfo.minDepth = 0.0f;
            depthInfo.maxDepth = 1.0f;
            depthInfo.nearZ = view.Near * UNITS_TO_METERS_SCALE;
            depthInfo.farZ = view.Far * UNITS_TO_METERS_SCALE;
        }
    }
    if (Swapchains.Count() > 1)
        SetupProxyView();
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

    PROFILE_CPU_NAMED("xrWaitFrame");
    FrameState = { XR_TYPE_FRAME_STATE };
    XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
    const XrResult result = xrWaitFrame(Session, &waitInfo, &FrameState);
    IsFrameWaited = XR_SUCCEEDED(result);

    // Get controllers input for the frame
    if (IsFrameWaited)
        OpenXRInput::Update(FrameState.predictedDisplayTime);
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

float OpenXRSession::GetDisplayRefreshRate()
{
    if (!IsSessionRunning)
        return 0.0f;
    float rate = 0.0f;
    if (GetDisplayRefreshRateFB && XR_SUCCEEDED(GetDisplayRefreshRateFB(Session, &rate)))
        return rate;

    // Use the predicted time between frames
    return FrameState.predictedDisplayPeriod > 0 ? (float)(1000000000.0 / (double)FrameState.predictedDisplayPeriod) : 0.0f;
}

#endif
