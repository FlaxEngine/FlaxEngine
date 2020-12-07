// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "MeshCollider.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/Physics.h"
#include <ThirdParty/PhysX/PxShape.h>
#include <ThirdParty/PhysX/PxRigidActor.h>
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#endif

MeshCollider::MeshCollider(const SpawnParams& params)
    : Collider(params)
{
    CollisionData.Changed.Bind<MeshCollider, &MeshCollider::OnCollisionDataChanged>(this);
    CollisionData.Loaded.Bind<MeshCollider, &MeshCollider::OnCollisionDataLoaded>(this);
}

void MeshCollider::OnCollisionDataChanged()
{
    // This should not be called during physics simulation, if it happened use write lock on physx scene
    ASSERT(!Physics::IsDuringSimulation());

    if (CollisionData)
    {
        // Ensure that collision asset is loaded (otherwise objects might fall though collider that is not yet loaded on play begin)
        CollisionData->WaitForLoaded();
    }

    UpdateGeometry();
    UpdateBounds();
}

void MeshCollider::OnCollisionDataLoaded()
{
    UpdateGeometry();
    UpdateBounds();
}

bool MeshCollider::CanAttach(RigidBody* rigidBody) const
{
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    return type != CollisionDataType::TriangleMesh;
}

bool MeshCollider::CanBeTrigger() const
{
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    return type != CollisionDataType::TriangleMesh;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void MeshCollider::DrawPhysicsDebug(RenderView& view)
{
    if (CollisionData && CollisionData->IsLoaded())
    {
        const auto& debugLines = CollisionData->GetDebugLines();
        DEBUG_DRAW_LINES(Span<Vector3>(debugLines.Get(), debugLines.Count()), _transform.GetWorld(), Color::GreenYellow * 0.8f, 0, true);
    }
}

void MeshCollider::OnDebugDrawSelected()
{
    if (CollisionData && CollisionData->IsLoaded())
    {
        const auto& debugLines = CollisionData->GetDebugLines();
        DEBUG_DRAW_LINES(Span<Vector3>(debugLines.Get(), debugLines.Count()), _transform.GetWorld(), Color::GreenYellow, 0, false);
    }

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool MeshCollider::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
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

void MeshCollider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(MeshCollider);

    SERIALIZE(CollisionData);
}

void MeshCollider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE(CollisionData);
}

#if USE_EDITOR

void MeshCollider::OnEnable()
{
    GetSceneRendering()->AddPhysicsDebug<MeshCollider, &MeshCollider::DrawPhysicsDebug>(this);

    // Base
    Collider::OnEnable();
}

void MeshCollider::OnDisable()
{
    GetSceneRendering()->RemovePhysicsDebug<MeshCollider, &MeshCollider::DrawPhysicsDebug>(this);

    // Base
    Collider::OnDisable();
}

#endif

void MeshCollider::UpdateBounds()
{
    // Cache bounds
    BoundingBox box;
    if (CollisionData && CollisionData->IsLoaded())
        box = CollisionData->GetOptions().Box;
    else
        box = BoundingBox::Zero;
    BoundingBox::Transform(box, _transform.GetWorld(), _box);
    BoundingSphere::FromBox(_box, _sphere);
}

void MeshCollider::CreateShape()
{
    // Prepare scale
    Vector3 scale = GetScale();
    _cachedScale = scale;
    scale.Absolute();
    const float minSize = 0.001f;
    scale = Vector3::Max(scale, minSize);

    // Setup shape (based on type)
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    if (type == CollisionDataType::ConvexMesh)
    {
        // Convex mesh
        PxConvexMeshGeometry geometry;
        geometry.scale.scale = C2P(scale);
        geometry.convexMesh = CollisionData->GetConvex();
        CreateShapeBase(geometry);
    }
    else if (type == CollisionDataType::TriangleMesh)
    {
        // Triangle mesh
        PxTriangleMeshGeometry geometry;
        geometry.scale.scale = C2P(scale);
        geometry.triangleMesh = CollisionData->GetTriangle();
        CreateShapeBase(geometry);
    }
    else
    {
        // Dummy geometry
        const PxSphereGeometry geometry(0.01f);
        CreateShapeBase(geometry);
    }
}

void MeshCollider::UpdateGeometry()
{
    // Check if has no shape created
    if (_shape == nullptr)
        return;

    // Recreate shape if geometry has different type
    CollisionDataType type = CollisionDataType::None;
    if (CollisionData && CollisionData->IsLoaded())
        type = CollisionData->GetOptions().Type;
    if ((type == CollisionDataType::ConvexMesh && _shape->getGeometryType() != PxGeometryType::eCONVEXMESH)
        || (type == CollisionDataType::TriangleMesh && _shape->getGeometryType() != PxGeometryType::eTRIANGLEMESH)
        || (type == CollisionDataType::None && _shape->getGeometryType() != PxGeometryType::eSPHERE)
    )
    {
        // Detach from the actor
        auto actor = _shape->getActor();
        if (actor)
            actor->detachShape(*_shape);

        // Release shape
        Physics::RemoveCollider(this);
        _shape->release();
        _shape = nullptr;

        // Recreate shape
        CreateShape();

        // Reattach again (only if can, see CanAttach function)
        if (actor)
        {
            if (_staticActor != nullptr || type != CollisionDataType::TriangleMesh)
            {
                actor->attachShape(*_shape);
            }
            else
            {
                // Be static triangle mesh
                CreateStaticActor();
            }
        }

        return;
    }

    // Prepare scale
    Vector3 scale = GetScale();
    _cachedScale = scale;
    scale.Absolute();
    const float minSize = 0.001f;
    scale = Vector3::Max(scale, minSize);

    // Setup shape (based on type)
    if (type == CollisionDataType::ConvexMesh)
    {
        // Convex mesh
        PxConvexMeshGeometry geometry;
        geometry.scale.scale = C2P(scale);
        geometry.convexMesh = CollisionData->GetConvex();
        _shape->setGeometry(geometry);
    }
    else if (type == CollisionDataType::TriangleMesh)
    {
        // Triangle mesh
        PxTriangleMeshGeometry geometry;
        geometry.scale.scale = C2P(scale);
        geometry.triangleMesh = CollisionData->GetTriangle();
        _shape->setGeometry(geometry);
    }
    else
    {
        // Dummy geometry
        const PxSphereGeometry geometry(0.01f);
        _shape->setGeometry(geometry);
    }
}
