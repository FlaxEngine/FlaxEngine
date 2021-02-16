// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Enums.h"
#include "Engine/Core/Config.h"

namespace physx
{
    class PxScene;
    class PxConvexMesh;
    class PxTriangleMesh;
    class PxCooking;
    class PxPhysics;
    class PxVec3;
    class PxVec4;
    class PxTransform;
    class PxJoint;
    class PxMat44;
    class PxCpuDispatcher;
    class PxGpuDispatcher;
    class PxSimulationEventCallback;
    struct PxActiveTransform;
    class PxActor;
    class PxRigidActor;
    class PxRigidDynamic;
    class PxRigidStatic;
    class PxFoundation;
    class PxShape;
    class PxGeometry;
    class PxGeometryHolder;
    class PxProfileZoneManager;
    class PxMaterial;
    class PxPvd;
    class PxBase;
    class PxTolerancesScale;
    class PxBaseTask;
    class PxControllerManager;
    class PxController;
    class PxCapsuleController;
    class PxQueryFilterCallback;
    class PxHeightField;
    struct PxFilterData;
    struct PxRaycastHit;
    struct PxSweepHit;
}

using namespace physx;

#define RELEASE_PHYSX(x) if(x) { (x)->release(); x = nullptr; }

// Global pointer to PhysX SDK object
extern PxPhysics* CPhysX;

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
