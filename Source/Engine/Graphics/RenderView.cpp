// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "RenderView.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/RendererPass.h"
#include "RenderBuffers.h"
#include "RenderTask.h"

void RenderView::Prepare(RenderContext& renderContext)
{
    ASSERT(renderContext.List && renderContext.Buffers);

    const float width = static_cast<float>(renderContext.Buffers->GetWidth());
    const float height = static_cast<float>(renderContext.Buffers->GetHeight());

    // Check if use TAA (need to modify the projection matrix)
    Vector2 taaJitter;
    NonJitteredProjection = Projection;
    if (renderContext.List->Settings.AntiAliasing.Mode == AntialiasingMode::TemporalAntialiasing)
    {
        // Move to the next frame
        const int MaxSampleCount = 8;
        if (++TaaFrameIndex >= MaxSampleCount)
            TaaFrameIndex = 0;

        // Calculate jitter
        const float jitterSpread = renderContext.List->Settings.AntiAliasing.TAA_JitterSpread;
        const float jitterX = RendererUtils::TemporalHalton(TaaFrameIndex + 1, 2) * jitterSpread;
        const float jitterY = RendererUtils::TemporalHalton(TaaFrameIndex + 1, 3) * jitterSpread;
        taaJitter = Vector2(jitterX * 2.0f / width, jitterY * 2.0f / height);

        // Modify projection matrix
        Projection.Values[2][0] += taaJitter.X;
        Projection.Values[2][1] += taaJitter.Y;

        // Update matrices
        Matrix::Invert(Projection, IP);
        Frustum.SetMatrix(View, Projection);
        Frustum.GetInvMatrix(IVP);
        CullingFrustum = Frustum;
    }
    else
    {
        TaaFrameIndex = 0;
        taaJitter = Vector2::Zero;
    }

    renderContext.List->Init(renderContext);
    renderContext.LodProxyView = nullptr;

    PrepareCache(renderContext, width, height, taaJitter);
}

void RenderView::PrepareCache(RenderContext& renderContext, float width, float height, const Vector2& temporalAAJitter)
{
    // The same format used by the Flax common shaders and postFx materials
    ViewInfo = Vector4(1.0f / Projection.M11, 1.0f / Projection.M22, Far / (Far - Near), (-Far * Near) / (Far - Near) / Far);
    ScreenSize = Vector4(width, height, 1.0f / width, 1.0f / height);

    TemporalAAJitter.Z = TemporalAAJitter.X;
    TemporalAAJitter.W = TemporalAAJitter.Y;
    TemporalAAJitter.X = temporalAAJitter.X;
    TemporalAAJitter.Y = temporalAAJitter.Y;

    // Ortho views have issues with screen size LOD culling
    const float modelLODDistanceFactor = (renderContext.LodProxyView ? renderContext.LodProxyView->IsOrthographicProjection() : IsOrthographicProjection()) ? 100.0f : ModelLODDistanceFactor;
    ModelLODDistanceFactorSqrt = modelLODDistanceFactor * modelLODDistanceFactor;
}

void RenderView::SetFace(int32 faceIndex)
{
    static Vector3 directions[6] =
    {
        { Vector3::Right },
        { Vector3::Left },
        { Vector3::Up },
        { Vector3::Down },
        { Vector3::Forward },
        { Vector3::Backward },
    };
    static Vector3 ups[6] =
    {
        { Vector3::Up },
        { Vector3::Up },
        { Vector3::Backward },
        { Vector3::Forward },
        { Vector3::Up },
        { Vector3::Up },
    };
    ASSERT(faceIndex >= 0 && faceIndex < 6);

    // Create view matrix
    Direction = directions[faceIndex];
    Matrix::LookAt(Position, Position + Direction, ups[faceIndex], View);
    Matrix::Invert(View, IV);

    // Compute frustum matrix
    Frustum.SetMatrix(View, Projection);
    Matrix::Invert(ViewProjection(), IVP);
    CullingFrustum = Frustum;
}

void RenderView::CopyFrom(Camera* camera)
{
    Position = camera->GetPosition();
    Direction = camera->GetDirection();
    Near = camera->GetNearPlane();
    Far = camera->GetFarPlane();
    View = camera->GetView();
    Projection = camera->GetProjection();
    NonJitteredProjection = Projection;
    Frustum = camera->GetFrustum();
    Matrix::Invert(View, IV);
    Matrix::Invert(Projection, IP);
    Frustum.GetInvMatrix(IVP);
    CullingFrustum = Frustum;
    RenderLayersMask = camera->RenderLayersMask;
}

void RenderView::CopyFrom(Camera* camera, Viewport* viewport)
{
    Position = camera->GetPosition();
    Direction = camera->GetDirection();
    Near = camera->GetNearPlane();
    Far = camera->GetFarPlane();
    View = camera->GetView();
    camera->GetMatrices(View, Projection, *viewport);
    Frustum.SetMatrix(View, Projection);
    NonJitteredProjection = Projection;
    Matrix::Invert(View, IV);
    Matrix::Invert(Projection, IP);
    Frustum.GetInvMatrix(IVP);
    CullingFrustum = Frustum;
    RenderLayersMask = camera->RenderLayersMask;
}

DrawPass RenderView::GetShadowsDrawPassMask(ShadowsCastingMode shadowsMode) const
{
    switch (shadowsMode)
    {
    case ShadowsCastingMode::All:
        return DrawPass::All;
    case ShadowsCastingMode::DynamicOnly:
        return IsOfflinePass ? ~DrawPass::Depth : DrawPass::All;
    case ShadowsCastingMode::StaticOnly:
        return IsOfflinePass ? DrawPass::All : ~DrawPass::Depth;
    case ShadowsCastingMode::None:
        return ~DrawPass::Depth;
    default:
        return DrawPass::All;
    }
}
