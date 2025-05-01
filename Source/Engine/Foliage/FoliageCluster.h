// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"

/// <summary>
/// Represents a single foliage cluster that contains a sub clusters organized in quad-tree or if it's a leaf node it contains a set of foliage instances.
/// </summary>
class FLAXENGINE_API FoliageCluster
{
public:
    /// <summary>
    /// The cluster bounds (in world space). Made of subdivided parent node in quad-tree.
    /// </summary>
    BoundingBox Bounds;

    /// <summary>
    /// The cached cluster total bounds (in world space). Made of attached instances bounds including children.
    /// </summary>
    BoundingBox TotalBounds;

    /// <summary>
    /// The cached cluster total bounds (in world space). Made of attached instances bounds including children.
    /// </summary>
    BoundingSphere TotalBoundsSphere;

    /// <summary>
    /// The maximum cull distance for the instances that are located in this cluster (including child clusters).
    /// </summary>
    float MaxCullDistance;

    /// <summary>
    /// The child clusters. If any element is valid then all are created.
    /// </summary>
    FoliageCluster* Children[4];

    /// <summary>
    /// The allocated foliage instances within this cluster.
    /// </summary>
    Array<FoliageInstance*, FixedAllocation<FOLIAGE_CLUSTER_CAPACITY>> Instances;

public:
    /// <summary>
    /// Initializes this instance.
    /// </summary>
    /// <param name="bounds">The bounds.</param>
    void Init(const BoundingBox& bounds);

    /// <summary>
    /// Updates the total bounds of the cluster and all child clusters and cull distance (as UpdateCullDistance does).
    /// </summary>
    void UpdateTotalBoundsAndCullDistance();

    /// <summary>
    /// Updates the cull distance for all foliage instances added to the cluster and its children.
    /// </summary>
    void UpdateCullDistance();

    /// <summary>
    /// Determines if there is an intersection between the current object or any it's child and a ray.
    /// </summary>
    /// <param name="foliage">The parent foliage actor.</param>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="instance">When the method completes, contains pointer of the foliage instance that is the closest to the ray.</param>
    /// <returns>True whether the two objects intersected, otherwise false.</returns>
    bool Intersects(Foliage* foliage, const Ray& ray, Real& distance, Vector3& normal, FoliageInstance*& instance);
};
