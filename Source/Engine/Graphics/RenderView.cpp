// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "RenderView.h"
#include "Engine/Level/LargeWorlds.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Core/Math/Double4x4.h"
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
    Float2 taaJitter;
    NonJitteredProjection = Projection;
    IsTaaResolved = false;
    if (renderContext.List->Setup.UseTemporalAAJitter)
    {
        // Move to the next frame
        const int MaxSampleCount = 8;
        if (++TaaFrameIndex >= MaxSampleCount)
            TaaFrameIndex = 0;

        // Calculate jitter
        const float jitterSpread = renderContext.List->Settings.AntiAliasing.TAA_JitterSpread;
        const float jitterX = (RendererUtils::TemporalHalton(TaaFrameIndex + 1, 2) - 0.5f) * jitterSpread;
        const float jitterY = (RendererUtils::TemporalHalton(TaaFrameIndex + 1, 3) - 0.5f) * jitterSpread;
        taaJitter = Float2(jitterX * 2.0f / width, jitterY * 2.0f / height);

        // Modify projection matrix
        if (IsOrthographicProjection())
        {
            // TODO: jitter otho matrix in a proper way
        }
        else
        {
            Projection.Values[2][0] += taaJitter.X;
            Projection.Values[2][1] += taaJitter.Y;
        }

        // Update matrices
        Matrix::Invert(Projection, IP);
        Frustum.SetMatrix(View, Projection);
        Frustum.GetInvMatrix(IVP);
        CullingFrustum = Frustum;
    }
    else
    {
        TaaFrameIndex = 0;
        taaJitter = Float2::Zero;
    }

    renderContext.List->Init(renderContext);
    renderContext.LodProxyView = nullptr;

    PrepareCache(renderContext, width, height, taaJitter);
}

void RenderView::PrepareCache(const RenderContext& renderContext, float width, float height, const Float2& temporalAAJitter, const RenderView* mainView)
{
    // The same format used by the Flax common shaders and postFx materials
    ViewInfo = Float4(1.0f / Projection.M11, 1.0f / Projection.M22, Far / (Far - Near), (-Far * Near) / (Far - Near) / Far);
    ScreenSize = Float4(width, height, 1.0f / width, 1.0f / height);

    TemporalAAJitter.Z = TemporalAAJitter.X;
    TemporalAAJitter.W = TemporalAAJitter.Y;
    TemporalAAJitter.X = temporalAAJitter.X;
    TemporalAAJitter.Y = temporalAAJitter.Y;

    WorldPosition = Origin + Position;

    ModelLODDistanceFactorSqrt = ModelLODDistanceFactor * ModelLODDistanceFactor;

    // Setup main view render info
    if (!mainView)
        mainView = this;
    MainViewProjection = mainView->ViewProjection();
    MainScreenSize = mainView->ScreenSize;
}

void RenderView::UpdateCachedData()
{
    Matrix::Invert(View, IV);
    Matrix::Invert(Projection, IP);
    Matrix viewProjection;
    Matrix::Multiply(View, Projection, viewProjection);
    Frustum.SetMatrix(viewProjection);
    Matrix::Invert(viewProjection, IVP);
    CullingFrustum = Frustum;
    NonJitteredProjection = Projection;
}

void RenderView::SetUp(const Matrix& viewProjection)
{
    // Copy data
    Matrix::Invert(viewProjection, IVP);
    Frustum.SetMatrix(viewProjection);
    CullingFrustum = Frustum;
}

void RenderView::SetUp(const Matrix& view, const Matrix& projection)
{
    // Copy data
    Projection = projection;
    NonJitteredProjection = projection;
    View = view;
    Matrix::Invert(View, IV);
    Matrix::Invert(Projection, IP);

    // Compute matrix
    Matrix viewProjection;
    Matrix::Multiply(View, Projection, viewProjection);
    Matrix::Invert(viewProjection, IVP);
    Frustum.SetMatrix(viewProjection);
    CullingFrustum = Frustum;
}

void RenderView::SetUpCube(float nearPlane, float farPlane, const Float3& position)
{
    // Copy data
    Near = nearPlane;
    Far = farPlane;
    Position = position;

    // Create projection matrix
    Matrix::PerspectiveFov(PI_OVER_2, 1.0f, nearPlane, farPlane, Projection);
    NonJitteredProjection = Projection;
    Matrix::Invert(Projection, IP);
}

void RenderView::SetFace(int32 faceIndex)
{
    static Float3 directions[6] =
    {
        { Float3::Right },
        { Float3::Left },
        { Float3::Up },
        { Float3::Down },
        { Float3::Forward },
        { Float3::Backward },
    };
    static Float3 ups[6] =
    {
        { Float3::Up },
        { Float3::Up },
        { Float3::Backward },
        { Float3::Forward },
        { Float3::Up },
        { Float3::Up },
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

void RenderView::SetProjector(float nearPlane, float farPlane, const Float3& position, const Float3& direction, const Float3& up, float angle)
{
    // Copy data
    Near = nearPlane;
    Far = farPlane;
    Position = position;

    // Create projection matrix
    Matrix::PerspectiveFov(angle * DegreesToRadians, 1.0f, nearPlane, farPlane, Projection);
    NonJitteredProjection = Projection;
    Matrix::Invert(Projection, IP);

    // Create view matrix
    Direction = direction;
    Matrix::LookAt(Position, Position + Direction, up, View);
    Matrix::Invert(View, IV);

    // Compute frustum matrix
    Frustum.SetMatrix(View, Projection);
    Matrix::Invert(ViewProjection(), IVP);
    CullingFrustum = Frustum;
}

void RenderView::CopyFrom(const Camera* camera, const Viewport* viewport)
{
    const Vector3 cameraPos = camera->GetPosition();
    LargeWorlds::UpdateOrigin(Origin, cameraPos);
    Position = cameraPos - Origin;
    Direction = camera->GetDirection();
    Near = camera->GetNearPlane();
    Far = camera->GetFarPlane();
    camera->GetMatrices(View, Projection, viewport ? *viewport : camera->GetViewport(), Origin);
    Frustum.SetMatrix(View, Projection);
    NonJitteredProjection = Projection;
    Matrix::Invert(View, IV);
    Matrix::Invert(Projection, IP);
    Frustum.GetInvMatrix(IVP);
    CullingFrustum = Frustum;
    RenderLayersMask = camera->RenderLayersMask;
    Flags = camera->RenderFlags;
    Mode = camera->RenderMode;
}

void RenderView::GetWorldMatrix(const Transform& transform, Matrix& world) const
{
    const Float3 translation = transform.Translation - Origin;
    Matrix::Transformation(transform.Scale, transform.Orientation, translation, world);
}

void RenderView::GetWorldMatrix(Double4x4& world) const
{
    world.M41 -= Origin.X;
    world.M42 -= Origin.Y;
    world.M43 -= Origin.Z;
}

TaaJitterRemoveContext::TaaJitterRemoveContext(const RenderView& view)
{
    if (view.IsTaaResolved)
    {
        // Cancel-out sub-pixel jitter when drawing geometry after TAA has been resolved
        _view = (RenderView*)&view;
        _prevProjection = view.Projection;
        _prevNonJitteredProjection = view.NonJitteredProjection;
        _view->Projection = _prevNonJitteredProjection;
        _view->UpdateCachedData();
    }
}

TaaJitterRemoveContext::~TaaJitterRemoveContext()
{
    if (_view)
    {
        // Restore projection
        _view->Projection = _prevProjection;
        _view->UpdateCachedData();
        _view->NonJitteredProjection = _prevNonJitteredProjection;
    }
}
