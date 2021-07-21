// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SplineCollider.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Level/Actors/Spline.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Profiler/ProfilerCPU.h"
#if COMPILE_WITH_PHYSICS_COOKING
#include "Engine/Physics/CollisionCooking.h"
#include <ThirdParty/PhysX/PxPhysics.h>
#include <ThirdParty/PhysX/extensions/PxDefaultStreams.h>
#endif
#include <ThirdParty/PhysX/geometry/PxTriangleMesh.h>

SplineCollider::SplineCollider(const SpawnParams& params)
    : Collider(params)
{
    CollisionData.Changed.Bind<SplineCollider, &SplineCollider::OnCollisionDataChanged>(this);
    CollisionData.Loaded.Bind<SplineCollider, &SplineCollider::OnCollisionDataLoaded>(this);
}

Transform SplineCollider::GetPreTransform() const
{
    return _preTransform;
}

void SplineCollider::SetPreTransform(const Transform& value)
{
    if (_preTransform == value)
        return;
    _preTransform = value;
    UpdateGeometry();
}

void SplineCollider::ExtractGeometry(Array<Vector3>& vertexBuffer, Array<int32>& indexBuffer) const
{
    vertexBuffer.Add(_vertexBuffer);
    indexBuffer.Add(_indexBuffer);
}

void SplineCollider::OnCollisionDataChanged()
{
    // This should not be called during physics simulation, if it happened use write lock on physx scene
    ASSERT(!Physics::IsDuringSimulation());

    if (CollisionData)
    {
        // Ensure that collision asset is loaded (otherwise objects might fall though collider that is not yet loaded on play begin)
        CollisionData->WaitForLoaded();
    }

    UpdateGeometry();
}

void SplineCollider::OnCollisionDataLoaded()
{
    UpdateGeometry();
}

void SplineCollider::OnSplineUpdated()
{
    if (!_spline || !IsActiveInHierarchy() || _spline->GetSplinePointsCount() < 2 || !CollisionData || !CollisionData->IsLoaded())
    {
        _box = BoundingBox(_transform.Translation);
        BoundingSphere::FromBox(_box, _sphere);
        return;
    }

    UpdateGeometry();
}

bool SplineCollider::CanAttach(RigidBody* rigidBody) const
{
    return false;
}

bool SplineCollider::CanBeTrigger() const
{
    return false;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderView.h"

void SplineCollider::DrawPhysicsDebug(RenderView& view)
{
    if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        DebugDraw::DrawTriangles(_vertexBuffer, _indexBuffer, Color::CornflowerBlue, 0, true);
    else
        DebugDraw::DrawWireTriangles(_vertexBuffer, _indexBuffer, Color::GreenYellow * 0.8f, 0, true);
}

void SplineCollider::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_TRIANGLES_EX(_vertexBuffer, _indexBuffer, Color::GreenYellow, 0, false);

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool SplineCollider::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    // Use detailed hit
    if (_shape)
    {
        RayCastHit hitInfo;
        if (!RayCast(ray.Position, ray.Direction, hitInfo))
            return false;
        distance = hitInfo.Distance;
        normal = hitInfo.Normal;
        return true;
    }

    // Fallback to AABB
    return _box.Intersects(ray, distance, normal);
}

void SplineCollider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SplineCollider);

    SERIALIZE(CollisionData);
    SERIALIZE_MEMBER(PreTransform, _preTransform)
}

void SplineCollider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE(CollisionData);
    DESERIALIZE_MEMBER(PreTransform, _preTransform);
}

void SplineCollider::OnParentChanged()
{
    if (_spline)
    {
        _spline->SplineUpdated.Unbind<SplineCollider, &SplineCollider::OnSplineUpdated>(this);
    }

    // Base
    Collider::OnParentChanged();

    _spline = Cast<Spline>(_parent);
    if (_spline)
    {
        _spline->SplineUpdated.Bind<SplineCollider, &SplineCollider::OnSplineUpdated>(this);
    }

    OnSplineUpdated();
}

void SplineCollider::EndPlay()
{
    // Base
    Collider::EndPlay();

    // Cleanup
    if (_triangleMesh)
    {
        Physics::RemoveObject(_triangleMesh);
        _triangleMesh = nullptr;
    }
}

void SplineCollider::UpdateBounds()
{
    // Unused as bounds are updated during collision building
}

void SplineCollider::GetGeometry(PxGeometryHolder& geometry)
{
    // Reset bounds
    _box = BoundingBox(_transform.Translation);
    BoundingSphere::FromBox(_box, _sphere);

    // Skip if sth is missing
    if (!_spline || !IsActiveInHierarchy() || _spline->GetSplinePointsCount() < 2 || !CollisionData || !CollisionData->IsLoaded())
    {
        geometry.storeAny(PxSphereGeometry(0.001f));
        return;
    }
    PROFILE_CPU();

    // Extract collision geometry
    // TODO: cache memory allocation for dynamic colliders
    Array<Vector3> collisionVertices;
    Array<int32> collisionIndices;
    CollisionData->ExtractGeometry(collisionVertices, collisionIndices);
    if (collisionIndices.IsEmpty())
    {
        geometry.storeAny(PxSphereGeometry(0.001f));
        return;
    }

    // Apply local mesh transformation
    if (!_preTransform.IsIdentity())
    {
        for (int32 i = 0; i < collisionVertices.Count(); i++)
            collisionVertices[i] = _preTransform.LocalToWorld(collisionVertices[i]);
    }

    // Find collision geometry local bounds
    BoundingBox localModelBounds;
    localModelBounds.Minimum = localModelBounds.Maximum = collisionVertices[0];
    for (int32 i = 1; i < collisionVertices.Count(); i++)
    {
        Vector3 v = collisionVertices[i];
        localModelBounds.Minimum = Vector3::Min(localModelBounds.Minimum, v);
        localModelBounds.Maximum = Vector3::Max(localModelBounds.Maximum, v);
    }
    auto localModelBoundsSize = localModelBounds.GetSize();

    // Deform geometry over the spline
    const auto& keyframes = _spline->Curve.GetKeyframes();
    const int32 segments = keyframes.Count() - 1;
    _vertexBuffer.Resize(collisionVertices.Count() * segments);
    _indexBuffer.Resize(collisionIndices.Count() * segments);
    const Transform splineTransform = _spline->GetTransform();
    const Transform colliderTransform = GetTransform();
    Transform curveTransform, leftTangent, rightTangent;
    for (int32 segment = 0; segment < segments; segment++)
    {
        // Setup for the spline segment
        auto offsetVertices = segment * collisionVertices.Count();
        auto offsetIndices = segment * collisionIndices.Count();
        const auto& start = keyframes[segment];
        const auto& end = keyframes[segment + 1];
        const float length = end.Time - start.Time;
        AnimationUtils::GetTangent(start.Value, start.TangentOut, length, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, length, rightTangent);

        // Vertex buffer is deformed along the spline
        auto srcVertices = collisionVertices.Get();
        auto dstVertices = _vertexBuffer.Get() + offsetVertices;
        for (int32 i = 0; i < collisionVertices.Count(); i++)
        {
            Vector3 v = srcVertices[i];
            const float alpha = Math::Saturate((v.Z - localModelBounds.Minimum.Z) / localModelBoundsSize.Z);
            v.Z = alpha;

            // Evaluate transformation at the curve
            AnimationUtils::Bezier(start.Value, leftTangent, rightTangent, end.Value, alpha, curveTransform);

            // Apply spline direction (from position 1st derivative)
            Vector3 direction;
            AnimationUtils::BezierFirstDerivative(start.Value.Translation, leftTangent.Translation, rightTangent.Translation, end.Value.Translation, alpha, direction);
            direction.Normalize();
            Quaternion orientation;
            if (direction.IsZero())
                orientation = Quaternion::Identity;
            else if (Vector3::Dot(direction, Vector3::Up) >= 0.999f)
                Quaternion::RotationAxis(Vector3::Left, PI_HALF, orientation);
            else
                Quaternion::LookRotation(direction, Vector3::Cross(Vector3::Cross(direction, Vector3::Up), direction), orientation);
            curveTransform.Orientation = orientation * curveTransform.Orientation;

            // Transform vertex
            v = curveTransform.LocalToWorld(v);
            v = splineTransform.LocalToWorld(v);
            v = colliderTransform.WorldToLocal(v);

            dstVertices[i] = v;
        }

        // Index buffer is the same for every segment except it's shifted
        auto srcIndices = collisionIndices.Get();
        auto dstIndices = _indexBuffer.Get() + offsetIndices;
        for (int32 i = 0; i < collisionIndices.Count(); i++)
            dstIndices[i] = srcIndices[i] + offsetVertices;
    }

    // Prepare scale
    Vector3 scale = _cachedScale;
    scale.Absolute();
    const float minSize = 0.001f;
    scale = Vector3::Max(scale, minSize);

    // TODO: add support for cooking collision for static splines in editor and reusing it in game

#if COMPILE_WITH_PHYSICS_COOKING
    // Cook triangle mesh collision
    CollisionCooking::CookingInput cookingInput;
    cookingInput.VertexCount = _vertexBuffer.Count();
    cookingInput.VertexData = _vertexBuffer.Get();
    cookingInput.IndexCount = _indexBuffer.Count();
    cookingInput.IndexData = _indexBuffer.Get();
    cookingInput.Is16bitIndexData = false;
    BytesContainer collisionData;
    if (!CollisionCooking::CookTriangleMesh(cookingInput, collisionData))
    {
        // Create triangle mesh
        if (_triangleMesh)
        {
            Physics::RemoveObject(_triangleMesh);
            _triangleMesh = nullptr;
        }
        PxDefaultMemoryInputData input(collisionData.Get(), collisionData.Length());
        // TODO: try using getVerticesForModification for dynamic triangle mesh vertices updating when changing curve in the editor
        _triangleMesh = Physics::GetPhysics()->createTriangleMesh(input);
        if (!_triangleMesh)
        {
            LOG(Error, "Failed to create triangle mesh from collision data of {0}.", ToString());
            geometry.storeAny(PxSphereGeometry(0.001f));
            return;
        }

        // Transform vertices back to world space for debug shapes drawing and navmesh building
        for (int32 i = 0; i < _vertexBuffer.Count(); i++)
            _vertexBuffer[i] = colliderTransform.LocalToWorld(_vertexBuffer[i]);

        // Update bounds
        _box = P2C(_triangleMesh->getLocalBounds());
        Matrix splineWorld;
        colliderTransform.GetWorld(splineWorld);
        BoundingBox::Transform(_box, splineWorld, _box);
        BoundingSphere::FromBox(_box, _sphere);

        // Setup geometry
        PxTriangleMeshGeometry triangleMesh;
        triangleMesh.scale.scale = C2P(scale);
        triangleMesh.triangleMesh = _triangleMesh;
        geometry.storeAny(triangleMesh);

        // TODO: find a way of releasing _vertexBuffer and _indexBuffer for static colliders (note: ExtractGeometry usage for navmesh generation at runtime)

        return;
    }
#endif

    LOG(Error, "Cannot build collision data for {0} due to runtime collision cooking diabled.", ToString());
    geometry.storeAny(PxSphereGeometry(0.001f));
}
