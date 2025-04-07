// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Scripting/ScriptingType.h"

struct PhysicsStatistics;
class PhysicsColliderActor;
class PhysicsScene;
class PhysicalMaterial;
class Joint;
class Collider;
class CollisionData;

/// <summary>
/// Enumeration that determines the way in which two material properties will be combined to yield a friction or restitution coefficient for a collision.
/// </summary>
/// <remarks>
/// Physics doesn't have any inherent combinations because the coefficients are determined empirically on a case by case basis.
/// However, simulating this with a pairwise lookup table is often impractical.
/// The effective combine mode for the pair is maximum(material0.combineMode, material1.combineMode).
/// </remarks>
API_ENUM() enum class PhysicsCombineMode
{
    /// <summary>
    /// Uses the average value of the touching materials: (a+b)/2.
    /// </summary>
    Average = 0,

    /// <summary>
    /// Uses the smaller value of the touching materials: min(a,b)
    /// </summary>
    Minimum = 1,

    /// <summary>
    /// Multiplies the values of the touching materials: a*b
    /// </summary>
    Multiply = 2,

    /// <summary>
    /// Uses the larger value of the touching materials: max(a, b)
    /// </summary>
    Maximum = 3,
};

/// <summary>
/// Force mode type determines the exact operation that is carried out when applying the force on a rigidbody.
/// </summary>
API_ENUM() enum class ForceMode
{
    /// <summary>
    /// Add a continuous force to the rigidbody, using its mass. The parameter has unit of mass * distance / time^2, i.e. a force.
    /// </summary>
    Force,

    /// <summary>
    /// Add an instant force impulse to the rigidbody, using its mass. The parameter has unit of mass * distance / time.
    /// </summary>
    Impulse,

    /// <summary>
    /// Add an instant velocity change to the rigidbody, ignoring its mass. The parameter has unit of distance / time, i.e. the effect is mass independent: a velocity change.	
    /// </summary>
    VelocityChange,

    /// <summary>
    /// Add a continuous acceleration to the rigidbody, ignoring its mass. The parameter has unit of distance / time^2, i.e. an acceleration. It gets treated just like a force except the mass is not divided out before integration.
    /// </summary>
    Acceleration
};

/// <summary>
/// Dynamic rigidbodies movement and rotation locking flags. Provide a mechanism to lock motion along/around a specific axis or set of axes to constrain object motion.
/// </summary>
API_ENUM(Attributes="Flags") enum class RigidbodyConstraints
{
    /// <summary>
    /// No constraints.
    /// </summary>
    None = 0,

    /// <summary>
    /// Freeze motion along the X-axis.
    /// </summary>
    LockPositionX = 1 << 0,

    /// <summary>
    /// Freeze motion along the Y-axis.
    /// </summary>
    LockPositionY = 1 << 1,

    /// <summary>
    /// Freeze motion along the Z-axis.
    /// </summary>
    LockPositionZ = 1 << 2,

    /// <summary>
    /// Freeze rotation along the X-axis.
    /// </summary>
    LockRotationX = 1 << 3,

    /// <summary>
    /// Freeze rotation along the Y-axis.
    /// </summary>
    LockRotationY = 1 << 4,

    /// <summary>
    /// Freeze rotation along the Z-axis.
    /// </summary>
    LockRotationZ = 1 << 5,

    /// <summary>
    /// Freeze motion along all axes.
    /// </summary>
    LockPosition = LockPositionX | LockPositionY | LockPositionZ,

    /// <summary>
    /// Freeze rotation along all axes.
    /// </summary>
    LockRotation = LockRotationX | LockRotationY | LockRotationZ,

    /// <summary>
    /// Freeze rotation and motion along all axes.
    /// </summary>
    LockAll = LockPosition | LockRotation,
};

DECLARE_ENUM_OPERATORS(RigidbodyConstraints);

/// <summary>
/// Raycast hit result data.
/// </summary>
API_STRUCT(NoDefault) struct RayCastHit
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(RayCastHit);

    /// <summary>
    /// The collider that was hit.
    /// </summary>
    API_FIELD() PhysicsColliderActor* Collider = nullptr;

    /// <summary>
    /// The physical material of the surface that was hit.
    /// </summary>
    API_FIELD() PhysicalMaterial* Material = nullptr;

    /// <summary>
    /// The normal of the surface the ray hit.
    /// </summary>
    API_FIELD() Vector3 Normal;

    /// <summary>
    /// The distance from the ray's origin to the hit location.
    /// </summary>
    API_FIELD() float Distance;

    /// <summary>
    /// The point in the world space where ray hit the collider.
    /// </summary>
    API_FIELD() Vector3 Point;

    /// <summary>
    /// The index of the face that was hit. Valid only for convex mesh (polygon index), triangle mesh (triangle index) and height field (triangle index).
    /// </summary>
    /// <seealso cref="CollisionData.GetModelTriangle" />
    API_FIELD() uint32 FaceIndex;

    /// <summary>
    /// The barycentric coordinates of hit triangle. Valid only for triangle mesh and height field.
    /// </summary>
    API_FIELD() Float2 UV;
};

/// <summary>
/// Physics collision shape variant for different shapes such as box, sphere, capsule.
/// </summary>
struct FLAXENGINE_API CollisionShape
{
    enum class Types : uint8
    {
        Sphere,
        Box,
        Capsule,
        ConvexMesh,
        TriangleMesh,
        HeightField,
    };

    Types Type;

    union
    {
        struct
        {
            float Radius;
        } Sphere;

        struct
        {
            float HalfExtents[3];
        } Box;

        struct
        {
            float Radius;
            float HalfHeight;
        } Capsule;

        struct
        {
            void* ConvexMesh;
            float Scale[3];
        } ConvexMesh;

        struct
        {
            void* TriangleMesh;
            float Scale[3];
        } TriangleMesh;

        struct
        {
            void* HeightField;
            float HeightScale;
            float RowScale;
            float ColumnScale;
        } HeightField;
    };

    void SetSphere(float radius)
    {
        Type = Types::Sphere;
        Sphere.Radius = radius;
    }

    void SetBox(float halfExtents[3])
    {
        Type = Types::Box;
        Box.HalfExtents[0] = halfExtents[0];
        Box.HalfExtents[1] = halfExtents[1];
        Box.HalfExtents[2] = halfExtents[2];
    }

    void SetCapsule(float radius, float halfHeight)
    {
        Type = Types::Capsule;
        Capsule.Radius = radius;
        Capsule.HalfHeight = halfHeight;
    }

    void SetConvexMesh(void* contextMesh, float scale[3])
    {
        Type = Types::ConvexMesh;
        ConvexMesh.ConvexMesh = contextMesh;
        ConvexMesh.Scale[0] = scale[0];
        ConvexMesh.Scale[1] = scale[1];
        ConvexMesh.Scale[2] = scale[2];
    }

    void SetTriangleMesh(void* triangleMesh, float scale[3])
    {
        Type = Types::TriangleMesh;
        TriangleMesh.TriangleMesh = triangleMesh;
        TriangleMesh.Scale[0] = scale[0];
        TriangleMesh.Scale[1] = scale[1];
        TriangleMesh.Scale[2] = scale[2];
    }

    void SetHeightField(void* heightField, float heightScale, float rowScale, float columnScale)
    {
        Type = Types::HeightField;
        HeightField.HeightField = heightField;
        HeightField.HeightScale = heightScale;
        HeightField.RowScale = rowScale;
        HeightField.ColumnScale = columnScale;
    }
};
