// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Camera.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Content.h"
#include "Engine/Platform/Window.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#else
#include "Engine/Engine/Engine.h"
#endif

Array<Camera*> Camera::Cameras;
Camera* Camera::CutSceneCamera = nullptr;
Camera* Camera::OverrideMainCamera = nullptr;

Camera* Camera::GetMainCamera()
{
    if (OverrideMainCamera)
        return OverrideMainCamera;
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
    , _orthoScale(1.0f)
{
#if USE_EDITOR
    _previewModel.Loaded.Bind<Camera, &Camera::OnPreviewModelLoaded>(this);
#endif
}

void Camera::SetUsePerspective(bool value)
{
    if (_usePerspective != value)
    {
        _usePerspective = value;
        UpdateCache();
    }
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

void Camera::SetCustomAspectRatio(float value)
{
    value = Math::Clamp(value, 0.0f, 100.0f);
    if (_customAspectRatio != value)
    {
        _customAspectRatio = value;
        UpdateCache();
    }
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

void Camera::SetFarPlane(float value)
{
    value = Math::Max(value, _near + 1.0f);
    if (Math::NotNearEqual(_far, value))
    {
        _far = value;
        UpdateCache();
    }
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

void Camera::ProjectPoint(const Vector3& worldSpaceLocation, Vector2& screenSpaceLocation) const
{
    ProjectPoint(worldSpaceLocation, screenSpaceLocation, GetViewport());
}

void Camera::ProjectPoint(const Vector3& worldSpaceLocation, Vector2& screenSpaceLocation, const Viewport& viewport) const
{
    Matrix v, p, vp;
    GetMatrices(v, p, viewport);
    Matrix::Multiply(v, p, vp);
    Vector3 clipSpaceLocation;
    Vector3::Transform(worldSpaceLocation, vp, clipSpaceLocation);
    viewport.Project(worldSpaceLocation, vp, clipSpaceLocation);
    screenSpaceLocation = Vector2(clipSpaceLocation);
}

Ray Camera::ConvertMouseToRay(const Vector2& mousePosition) const
{
    return ConvertMouseToRay(mousePosition, GetViewport());
}

Ray Camera::ConvertMouseToRay(const Vector2& mousePosition, const Viewport& viewport) const
{
#if 1
    // Gather camera properties
    Matrix v, p, ivp;
    GetMatrices(v, p, viewport);
    Matrix::Multiply(v, p, ivp);
    ivp.Invert();

    // Create near and far points
    Vector3 nearPoint(mousePosition, 0.0f);
    Vector3 farPoint(mousePosition, 1.0f);
    viewport.Unproject(nearPoint, ivp, nearPoint);
    viewport.Unproject(farPoint, ivp, farPoint);

    // Create direction vector
    Vector3 direction = farPoint - nearPoint;
    direction.Normalize();

    return Ray(nearPoint, direction);
#else
    // Create near and far points
    Vector3 nearPoint, farPoint;
    Matrix ivp;
    _frustum.GetInvMatrix(&ivp);
    viewport.Unproject(Vector3(mousePosition, 0.0f), ivp, nearPoint);
    viewport.Unproject(Vector3(mousePosition, 1.0f), ivp, farPoint);

    // Create direction vector
    Vector3 direction = farPoint - nearPoint;
    direction.Normalize();

    // Return result
    return Ray(nearPoint, direction);
#endif
}

Viewport Camera::GetViewport() const
{
    Viewport result = Viewport(Vector2::Zero);

#if USE_EDITOR
    // Editor
    if (Editor::Managed)
        result.Size = Editor::Managed->GetGameWindowSize();
#else
	// game
	auto mainWin = Engine::MainWindow;
	if (mainWin)
	{
		const auto size = mainWin->GetClientSize();
		result.Size = size;
	}
#endif

    // Fallback to the default value
    if (result.Width <= ZeroTolerance)
        result.Size = Vector2(1280, 720);

    return result;
}

void Camera::GetMatrices(Matrix& view, Matrix& projection) const
{
    GetMatrices(view, projection, GetViewport());
}

void Camera::GetMatrices(Matrix& view, Matrix& projection, const Viewport& viewport) const
{
    // Create projection matrix
    if (_usePerspective)
    {
        const float aspect = _customAspectRatio <= 0.0f ? viewport.GetAspectRatio() : _customAspectRatio;
        Matrix::PerspectiveFov(_fov * DegreesToRadians, aspect, _near, _far, projection);
    }
    else
    {
        Matrix::Ortho(viewport.Width * _orthoScale, viewport.Height * _orthoScale, _near, _far, projection);
    }

    // Create view matrix
    const Vector3 direction = GetDirection();
    const Vector3 target = _transform.Translation + direction;
    Vector3 up;
    Vector3::Transform(Vector3::Up, GetOrientation(), up);
    Matrix::LookAt(_transform.Translation, target, up, view);
}

#if USE_EDITOR

void Camera::OnPreviewModelLoaded()
{
    _previewModelBuffer.Setup(_previewModel.Get());

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

bool Camera::IntersectsItselfEditor(const Ray& ray, float& distance)
{
    return _previewModelBox.Intersects(ray, distance);
}

bool Camera::HasContentLoaded() const
{
    return _previewModel == nullptr || _previewModel->IsLoaded();
}

void Camera::Draw(RenderContext& renderContext)
{
    if (renderContext.View.Flags & ViewFlags::EditorSprites && _previewModel && _previewModel->IsLoaded())
    {
        GeometryDrawStateData drawState;
        Mesh::DrawInfo draw;
        draw.Buffer = &_previewModelBuffer;
        draw.World = &_previewModelWorld;
        draw.DrawState = &drawState;
        draw.Lightmap = nullptr;
        draw.LightmapUVs = nullptr;
        draw.Flags = StaticFlags::Transform;
        draw.DrawModes = (DrawPass)(DrawPass::Default & renderContext.View.Pass);
        BoundingSphere::FromBox(_previewModelBox, draw.Bounds);
        draw.PerInstanceRandom = GetPerInstanceRandom();
        draw.LODBias = 0;
        draw.ForcedLOD = -1;
        draw.VertexColors = nullptr;

        _previewModel->Draw(renderContext, draw);
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
    // Update view and projection matrix
    GetMatrices(_view, _projection);

    // Update frustum and bounding box
    _frustum.SetMatrix(_view, _projection);
    _frustum.GetBox(_box);
    BoundingSphere::FromBox(_box, _sphere);

#if USE_EDITOR

    // Update editor preview model cache
    Matrix rot, world;
    _transform.GetWorld(world);
    Matrix::RotationY(PI * -0.5f, rot);
    Matrix::Multiply(rot, world, _previewModelWorld);

    // Calculate snap box for preview model
    if (_previewModel && _previewModel->IsLoaded())
    {
        _previewModelBox = _previewModel->GetBox(_previewModelWorld);
    }
    else
    {
        Vector3 min(-10.0f), max(10.0f);
        min = Vector3::Transform(min, _previewModelWorld);
        max = Vector3::Transform(max, _previewModelWorld);
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
}

void Camera::OnEnable()
{
    Cameras.Add(this);
#if USE_EDITOR
    GetSceneRendering()->AddCommonNoCulling(this);
#endif

    // Base
    Actor::OnEnable();
}

void Camera::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveCommonNoCulling(this);
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
