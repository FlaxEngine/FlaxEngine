// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Camera.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Content.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Level/Scene/SceneRendering.h"
#else
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Window.h"
#endif

Array<Camera*> Camera::Cameras;
Camera* Camera::CutSceneCamera = nullptr;
ScriptingObjectReference<Camera> Camera::OverrideMainCamera;

Camera* Camera::GetMainCamera()
{
    Camera* overrideMainCamera = OverrideMainCamera.Get();
    if (overrideMainCamera)
        return overrideMainCamera;
    if (CutSceneCamera)
        return CutSceneCamera;
    return Cameras.HasItems() ? Cameras.First() : nullptr;
}

Camera::Camera(const SpawnParams& params)
    : Actor(params)
    , _usePerspective(true)
    , _fov(60.0f)
    , _customAspectRatio(0.0f)
    , _near(10.0f)
    , _far(40000.0f)
    , _orthoSize(0.0f)
    , _orthoScale(1.0f)
{
#if USE_EDITOR
    _previewModel.Loaded.Bind<Camera, &Camera::OnPreviewModelLoaded>(this);
#endif
}

bool Camera::GetUsePerspective() const
{
    return _usePerspective;
}

void Camera::SetUsePerspective(bool value)
{
    if (_usePerspective != value)
    {
        _usePerspective = value;
        UpdateCache();
    }
}

float Camera::GetFieldOfView() const
{
    return _fov;
}

void Camera::SetFieldOfView(float value)
{
    value = Math::Clamp(value, 1.0f, 179.9f);
    if (Math::NotNearEqual(_fov, value))
    {
        _fov = value;
        UpdateCache();
    }
}

float Camera::GetCustomAspectRatio() const
{
    return _customAspectRatio;
}

void Camera::SetCustomAspectRatio(float value)
{
    value = Math::Clamp(value, 0.0f, 100.0f);
    if (Math::NotNearEqual(_customAspectRatio, value))
    {
        _customAspectRatio = value;
        UpdateCache();
    }
}

float Camera::GetNearPlane() const
{
    return _near;
}

void Camera::SetNearPlane(float value)
{
    value = Math::Clamp(value, 0.001f, _far - 1.0f);
    if (Math::NotNearEqual(_near, value))
    {
        _near = value;
        UpdateCache();
    }
}

float Camera::GetFarPlane() const
{
    return _far;
}

void Camera::SetFarPlane(float value)
{
    value = Math::Max(value, _near + 1.0f);
    if (Math::NotNearEqual(_far, value))
    {
        _far = value;
        UpdateCache();
    }
}

float Camera::GetOrthographicSize() const
{
    return _orthoSize;
}

void Camera::SetOrthographicSize(float value)
{
    value = Math::Clamp(value, 0.0f, 1000000.0f);
    if (Math::NotNearEqual(_orthoSize, value))
    {
        _orthoSize = value;
        UpdateCache();
    }
}

float Camera::GetOrthographicScale() const
{
    return _orthoScale;
}

void Camera::SetOrthographicScale(float value)
{
    value = Math::Clamp(value, 0.0001f, 1000000.0f);
    if (Math::NotNearEqual(_orthoScale, value))
    {
        _orthoScale = value;
        UpdateCache();
    }
}

void Camera::ProjectPoint(const Vector3& worldSpaceLocation, Float2& gameWindowSpaceLocation) const
{
    ProjectPoint(worldSpaceLocation, gameWindowSpaceLocation, GetViewport());
}

void Camera::ProjectPoint(const Vector3& worldSpaceLocation, Float2& cameraViewportSpaceLocation, const Viewport& viewport) const
{
    Matrix v, p, vp;
    GetMatrices(v, p, viewport);
    Matrix::Multiply(v, p, vp);
    Vector3 clipSpaceLocation;
    Vector3::Transform(worldSpaceLocation, vp, clipSpaceLocation);
    viewport.Project(worldSpaceLocation, vp, clipSpaceLocation);
    cameraViewportSpaceLocation = Float2(clipSpaceLocation);
}

void Camera::UnprojectPoint(const Float2& gameWindowSpaceLocation, float depth, Vector3& worldSpaceLocation) const
{
    UnprojectPoint(gameWindowSpaceLocation, depth, worldSpaceLocation, GetViewport());
}

void Camera::UnprojectPoint(const Float2& cameraViewportSpaceLocation, float depth, Vector3& worldSpaceLocation, const Viewport& viewport) const
{
    Matrix v, p, ivp;
    GetMatrices(v, p, viewport);
    Matrix::Multiply(v, p, ivp);
    ivp.Invert();
    viewport.Unproject(Vector3(cameraViewportSpaceLocation, depth), ivp, worldSpaceLocation);
}

bool Camera::IsPointOnView(const Vector3& worldSpaceLocation) const
{
    Vector3 cameraUp = GetTransform().GetUp();
    Vector3 cameraForward = GetTransform().GetForward();
    Vector3 directionToPosition = (worldSpaceLocation - GetPosition()).GetNormalized();
    if (Vector3::Dot(cameraForward, directionToPosition) < 0)
        return false;

    Quaternion lookAt = Quaternion::LookRotation(directionToPosition, cameraUp);
    Vector3 lookAtDirection = lookAt * Vector3::Forward;
    Vector3 newWorldLocation = GetPosition() + lookAtDirection;

    Float2 windowSpace;
    const Viewport viewport = GetViewport();
    ProjectPoint(newWorldLocation, windowSpace, viewport);

    return windowSpace.X >= 0 && windowSpace.X <= viewport.Size.X && windowSpace.Y >= 0 && windowSpace.Y <= viewport.Size.Y;
}

Ray Camera::ConvertMouseToRay(const Float2& mousePosition) const
{
    return ConvertMouseToRay(mousePosition, GetViewport());
}

Ray Camera::ConvertMouseToRay(const Float2& mousePosition, const Viewport& viewport) const
{
    Vector3 position = GetPosition();
    if (viewport.Width < ZeroTolerance || viewport.Height < ZeroTolerance)
        return Ray(position, GetDirection());

    // Use different logic in orthographic projection
    if (!_usePerspective)
    {
        Float2 screenPosition(mousePosition.X / viewport.Width - 0.5f, -mousePosition.Y / viewport.Height + 0.5f);
        Quaternion orientation = GetOrientation();
        Float3 direction = orientation * Float3::Forward;
        const float orthoSize = (_orthoSize > 0.0f ? _orthoSize : viewport.Height) * _orthoScale;
        const float aspect = _customAspectRatio <= 0.0f ? viewport.GetAspectRatio() : _customAspectRatio;
        Vector3 rayOrigin(screenPosition.X * orthoSize * aspect, screenPosition.Y * orthoSize, 0);
        rayOrigin = position + Vector3::Transform(rayOrigin, orientation);
        rayOrigin += direction * _near;
        return Ray(rayOrigin, direction);
    }

    // Create view frustum
    Matrix v, p, ivp;
    GetMatrices(v, p, viewport);
    Matrix::Multiply(v, p, ivp);
    ivp.Invert();

    // Create near and far points
    Vector3 nearPoint(mousePosition, _near);
    Vector3 farPoint(mousePosition, _far);
    viewport.Unproject(nearPoint, ivp, nearPoint);
    viewport.Unproject(farPoint, ivp, farPoint);

    Vector3 dir = Vector3::Normalize(farPoint - nearPoint);
    if (dir.IsZero())
        return Ray::Identity;
    return Ray(nearPoint, dir);
}

Viewport Camera::GetViewport() const
{
    Viewport result = Viewport(Float2::Zero);
    float dpiScale = Platform::GetDpiScale();

#if USE_EDITOR
    // Editor
    if (Editor::Managed)
    {
        result.Size = Editor::Managed->GetGameWindowSize();
        if (auto* window = Editor::Managed->GetGameWindow())
            dpiScale = window->GetDpiScale();
    }
#else
	// Game
	auto mainWin = Engine::MainWindow;
	if (mainWin)
	{
		const auto size = mainWin->GetClientSize();
		result.Size = size;
        dpiScale = mainWin->GetDpiScale();
	}
#endif

    // Remove DPI scale (game viewport coords are unscaled)
    result.Size /= dpiScale;

    // Fallback to the default value
    if (result.Size.MinValue() <= ZeroTolerance)
        result.Size = Float2(1280, 720);

    return result;
}

void Camera::GetMatrices(Matrix& view, Matrix& projection) const
{
    GetMatrices(view, projection, GetViewport(), Vector3::Zero);
}

void Camera::GetMatrices(Matrix& view, Matrix& projection, const Viewport& viewport) const
{
    GetMatrices(view, projection, viewport, Vector3::Zero);
}

void Camera::GetMatrices(Matrix& view, Matrix& projection, const Viewport& viewport, const Vector3& origin) const
{
    // Create projection matrix
    const float aspect = _customAspectRatio <= 0.0f ? viewport.GetAspectRatio() : _customAspectRatio;
    if (_usePerspective)
    {
        Matrix::PerspectiveFov(_fov * DegreesToRadians, aspect, _near, _far, projection);
    }
    else
    {
        const float orthoSize = (_orthoSize > 0.0f ? _orthoSize : viewport.Height) * _orthoScale;
        Matrix::Ortho(orthoSize * aspect, orthoSize, _near, _far, projection);
    }

    // Create view matrix
    const Float3 direction = GetDirection();
    const Float3 position = _transform.Translation - origin;
    const Float3 target = position + direction;
    Float3 up;
    Float3::Transform(Float3::Up, GetOrientation(), up);
    Matrix::LookAt(position, target, up, view);
}

#if USE_EDITOR

void Camera::OnPreviewModelLoaded()
{
    _previewModelBuffer.Setup(_previewModel.Get());
    if (_previewModelBuffer.Count() > 0)
        _previewModelBuffer.At(0).ReceiveDecals = false;

    UpdateCache();
}

void Camera::BeginPlay(SceneBeginData* data)
{
    _previewModel = Content::LoadAsyncInternal<Model>(TEXT("Editor/Camera/O_Camera"));

    // Base
    Actor::BeginPlay(data);
}

BoundingBox Camera::GetEditorBox() const
{
    const Vector3 size(100);
    const Vector3 pos = _transform.Translation + _transform.Orientation * Vector3::Forward * 30.0f;
    return BoundingBox(pos - size, pos + size);
}

bool Camera::IntersectsItselfEditor(const Ray& ray, Real& distance)
{
    return _previewModelBox.Intersects(ray, distance);
}

bool Camera::HasContentLoaded() const
{
    return _previewModel == nullptr || _previewModel->IsLoaded();
}

void Camera::Draw(RenderContext& renderContext)
{
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::EditorSprites)
        && renderContext.View.CullingFrustum.Intersects(_previewModelBox)
        && _previewModel
        && _previewModel->IsLoaded())
    {
        Matrix rot, tmp, world;
        renderContext.View.GetWorldMatrix(_transform, tmp);
        Matrix::RotationY(PI * -0.5f, rot);
        Matrix::Multiply(rot, tmp, world);
        GeometryDrawStateData drawState;
        Mesh::DrawInfo draw;
        draw.Buffer = &_previewModelBuffer;
        draw.World = &world;
        draw.DrawState = &drawState;
        draw.Deformation = nullptr;
        draw.Lightmap = nullptr;
        draw.LightmapUVs = nullptr;
        draw.Flags = StaticFlags::Transform;
        draw.DrawModes = (DrawPass::Depth | DrawPass::GBuffer | DrawPass::Forward) & renderContext.View.Pass;
        BoundingSphere::FromBox(_previewModelBox, draw.Bounds);
        draw.Bounds.Center -= renderContext.View.Origin;
        draw.PerInstanceRandom = GetPerInstanceRandom();
        draw.LODBias = 0;
        draw.ForcedLOD = -1;
        draw.SortOrder = 0;
        draw.VertexColors = nullptr;
        if (draw.DrawModes != DrawPass::None)
        {
            _previewModel->Draw(renderContext, draw);
        }
    }
    // Load preview model if it doesnt exist. Ex: prefabs
    else if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::EditorSprites) && !_previewModel)
    {
        _previewModel = Content::LoadAsyncInternal<Model>(TEXT("Editor/Camera/O_Camera"));
    }
}

#include "Engine/Debug/DebugDraw.h"

void Camera::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_FRUSTUM(_frustum, Color::White, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Camera::UpdateCache()
{
    // Calculate view and projection matrices
    Matrix view, projection;
    GetMatrices(view, projection);

    // Update frustum and bounding box
    _frustum.SetMatrix(view, projection);
    _frustum.GetBox(_box);
    BoundingSphere::FromBox(_box, _sphere);

#if USE_EDITOR

    // Update editor preview model cache
    Matrix rot, tmp, world;
    GetLocalToWorldMatrix(tmp);
    Matrix::RotationY(PI * -0.5f, rot);
    Matrix::Multiply(rot, tmp, world);

    // Calculate snap box for preview model
    if (_previewModel && _previewModel->IsLoaded())
    {
        _previewModelBox = _previewModel->GetBox(world);
    }
    else
    {
        Vector3 min(-10.0f), max(10.0f);
        min = Vector3::Transform(min, world);
        max = Vector3::Transform(max, world);
        _previewModelBox = BoundingBox(min, max);
    }

    // Extend culling bounding box
    BoundingBox::Merge(_box, _previewModelBox, _box);
    BoundingSphere::FromBox(_box, _sphere);

#endif
}

void Camera::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Camera);

    SERIALIZE_MEMBER(UsePerspective, _usePerspective);
    SERIALIZE_MEMBER(FOV, _fov);
    SERIALIZE_MEMBER(CustomAspectRatio, _customAspectRatio);
    SERIALIZE_MEMBER(Near, _near);
    SERIALIZE_MEMBER(Far, _far);
    SERIALIZE_MEMBER(OrthoScale, _orthoScale);
    SERIALIZE(RenderLayersMask);
    SERIALIZE(RenderFlags);
    SERIALIZE(RenderMode);
}

void Camera::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(UsePerspective, _usePerspective);
    DESERIALIZE_MEMBER(FOV, _fov);
    DESERIALIZE_MEMBER(CustomAspectRatio, _customAspectRatio);
    DESERIALIZE_MEMBER(Near, _near);
    DESERIALIZE_MEMBER(Far, _far);
    DESERIALIZE_MEMBER(OrthoScale, _orthoScale);
    DESERIALIZE(RenderLayersMask);
    DESERIALIZE(RenderFlags);
    DESERIALIZE(RenderMode);
}

void Camera::OnEnable()
{
    Cameras.Add(this);
#if USE_EDITOR
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#endif

    // Base
    Actor::OnEnable();
}

void Camera::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
#endif
    Cameras.Remove(this);
    if (CutSceneCamera == this)
        CutSceneCamera = nullptr;

    // Base
    Actor::OnDisable();
}

void Camera::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateCache();
}
