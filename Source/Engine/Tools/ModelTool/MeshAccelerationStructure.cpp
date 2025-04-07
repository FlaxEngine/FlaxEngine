// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL

#include "MeshAccelerationStructure.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Profiler/ProfilerCPU.h"

void MeshAccelerationStructure::BuildBVH(int32 node, int32 maxLeafSize, Array<byte>& scratch)
{
    auto& root = _bvh[node];
    ASSERT_LOW_LAYER(root.Leaf.IsLeaf);
    if (root.Leaf.TriangleCount <= maxLeafSize)
        return;

    // Spawn two leaves
    const int32 childIndex = _bvh.Count();
    _bvh.AddDefault(2);
    auto& left = _bvh.Get()[childIndex];
    auto& right = _bvh.Get()[childIndex + 1];
    left.Leaf.IsLeaf = 1;
    right.Leaf.IsLeaf = 1;
    left.Leaf.MeshIndex = root.Leaf.MeshIndex;
    right.Leaf.MeshIndex = root.Leaf.MeshIndex;

    // Mid-point splitting based on the largest axis
    const Float3 boundsSize = root.Bounds.GetSize();
    int32 axisCount = 0;
    int32 axis = 0;
RETRY:
    if (axisCount == 0)
    {
        // Pick the highest axis
        axis = 0;
        if (boundsSize.Y > boundsSize.X && boundsSize.Y >= boundsSize.Z)
            axis = 1;
        else if (boundsSize.Z > boundsSize.X)
            axis = 2;
    }
    else if (axisCount == 3)
    {
        // Failed to split
        _bvh.Resize(childIndex);
        return;
    }
    else
    {
        // Go to the next axis
        axis = (axis + 1) % 3;
    }
    const float midPoint = (float)(root.Bounds.Minimum.Raw[axis] + boundsSize.Raw[axis] * 0.5f);
    const Mesh& meshData = _meshes[root.Leaf.MeshIndex];
    const Float3* vb = meshData.VertexBuffer.Get<Float3>();
    int32 indexStart = root.Leaf.TriangleIndex * 3;
    int32 indexEnd = indexStart + root.Leaf.TriangleCount * 3;
    left.Leaf.TriangleCount = 0;
    right.Leaf.TriangleCount = 0;
    if (meshData.Use16BitIndexBuffer)
    {
        struct Tri
        {
            uint16 I0, I1, I2;
        };
        scratch.Resize(root.Leaf.TriangleCount * sizeof(Tri));
        auto dst = (Tri*)scratch.Get();
        auto ib16 = meshData.IndexBuffer.Get<uint16>();
        for (int32 i = indexStart; i < indexEnd;)
        {
            const Tri tri = { ib16[i++], ib16[i++], ib16[i++] };
            const float v0 = vb[tri.I0].Raw[axis];
            const float v1 = vb[tri.I1].Raw[axis];
            const float v2 = vb[tri.I2].Raw[axis];
            const float centroid = (v0 + v1 + v2) * 0.333f;
            if (centroid <= midPoint)
                dst[left.Leaf.TriangleCount++] = tri; // Left
            else
                dst[root.Leaf.TriangleCount - ++right.Leaf.TriangleCount] = tri; // Right
        }
        Platform::MemoryCopy(ib16 + indexStart, dst, root.Leaf.TriangleCount * 3 * sizeof(uint16));
        if (left.Leaf.TriangleCount == 0 || right.Leaf.TriangleCount == 0)
        {
            axisCount++;
            goto RETRY;
        }

        left.Bounds = BoundingBox(vb[dst[0].I0]);
        indexStart = 0;
        indexEnd = left.Leaf.TriangleCount * 3;
        for (int32 i = indexStart; i < indexEnd; i++)
            left.Bounds.Merge(vb[((uint16*)scratch.Get())[i]]);

        right.Bounds = BoundingBox(vb[dst[root.Leaf.TriangleCount - 1].I0]);
        indexStart = left.Leaf.TriangleCount;
        indexEnd = root.Leaf.TriangleCount * 3;
        for (int32 i = indexStart; i < indexEnd; i++)
            right.Bounds.Merge(vb[((uint16*)scratch.Get())[i]]);
    }
    else
    {
        struct Tri
        {
            uint32 I0, I1, I2;
        };
        scratch.Resize(root.Leaf.TriangleCount * sizeof(Tri));
        auto dst = (Tri*)scratch.Get();
        auto ib32 = meshData.IndexBuffer.Get<uint32>();
        for (int32 i = indexStart; i < indexEnd;)
        {
            const Tri tri = { ib32[i++], ib32[i++], ib32[i++] };
            const float v0 = vb[tri.I0].Raw[axis];
            const float v1 = vb[tri.I1].Raw[axis];
            const float v2 = vb[tri.I2].Raw[axis];
            const float centroid = (v0 + v1 + v2) * 0.333f;
            if (centroid <= midPoint)
                dst[left.Leaf.TriangleCount++] = tri; // Left
            else
                dst[root.Leaf.TriangleCount - ++right.Leaf.TriangleCount] = tri; // Right
        }
        Platform::MemoryCopy(ib32 + indexStart, dst, root.Leaf.TriangleCount * 3 * sizeof(uint32));
        if (left.Leaf.TriangleCount == 0 || right.Leaf.TriangleCount == 0)
        {
            axisCount++;
            goto RETRY;
        }

        left.Bounds = BoundingBox(vb[dst[0].I0]);
        indexStart = 0;
        indexEnd = left.Leaf.TriangleCount * 3;
        for (int32 i = indexStart; i < indexEnd; i++)
            left.Bounds.Merge(vb[((uint32*)scratch.Get())[i]]);

        right.Bounds = BoundingBox(vb[dst[root.Leaf.TriangleCount - 1].I0]);
        indexStart = left.Leaf.TriangleCount;
        indexEnd = root.Leaf.TriangleCount * 3;
        for (int32 i = indexStart; i < indexEnd; i++)
            right.Bounds.Merge(vb[((uint32*)scratch.Get())[i]]);
    }
    ASSERT_LOW_LAYER(left.Leaf.TriangleCount + right.Leaf.TriangleCount == root.Leaf.TriangleCount);
    left.Leaf.TriangleIndex = root.Leaf.TriangleIndex;
    right.Leaf.TriangleIndex = left.Leaf.TriangleIndex + left.Leaf.TriangleCount;

    // Convert into a node
    root.Node.IsLeaf = 0;
    root.Node.ChildIndex = childIndex;
    root.Node.ChildrenCount = 2;

    // Split children
    BuildBVH(childIndex, maxLeafSize, scratch);
    BuildBVH(childIndex + 1, maxLeafSize, scratch);
}

bool MeshAccelerationStructure::PointQueryBVH(int32 node, const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle) const
{
    const auto& root = _bvh[node];
    bool hit = false;
    if (root.Leaf.IsLeaf)
    {
        // Find closest triangle
        Vector3 p;
        const Mesh& meshData = _meshes[root.Leaf.MeshIndex];
        const Float3* vb = meshData.VertexBuffer.Get<Float3>();
        const int32 indexStart = root.Leaf.TriangleIndex * 3;
        const int32 indexEnd = indexStart + root.Leaf.TriangleCount * 3;
        if (meshData.Use16BitIndexBuffer)
        {
            const uint16* ib16 = meshData.IndexBuffer.Get<uint16>();
            for (int32 i = indexStart; i < indexEnd;)
            {
                Vector3 v0 = vb[ib16[i++]];
                Vector3 v1 = vb[ib16[i++]];
                Vector3 v2 = vb[ib16[i++]];
                CollisionsHelper::ClosestPointPointTriangle(point, v0, v1, v2, p);
                const Real distance = Vector3::Distance(point, p);
                if (distance < hitDistance)
                {
                    hitDistance = distance;
                    hitPoint = p;
                    hitTriangle = Triangle(v0, v1, v2);
                    hit = true;
                }
            }
        }
        else
        {
            const uint32* ib32 = meshData.IndexBuffer.Get<uint32>();
            for (int32 i = indexStart; i < indexEnd;)
            {
                Vector3 v0 = vb[ib32[i++]];
                Vector3 v1 = vb[ib32[i++]];
                Vector3 v2 = vb[ib32[i++]];
                CollisionsHelper::ClosestPointPointTriangle(point, v0, v1, v2, p);
                const Real distance = Vector3::Distance(point, p);
                if (distance < hitDistance)
                {
                    hitDistance = distance;
                    hitPoint = p;
                    hitTriangle = Triangle(v0, v1, v2);
                    hit = true;
                }
            }
        }
    }
    else
    {
        // Check all nested nodes
        for (uint32 i = 0; i < root.Node.ChildrenCount; i++)
        {
            const int32 index = root.Node.ChildIndex + i;
            if (_bvh[index].Bounds.Distance(point) >= hitDistance)
                continue;
            if (PointQueryBVH(index, point, hitDistance, hitPoint, hitTriangle))
                hit = true;
        }
    }
    return hit;
}

bool MeshAccelerationStructure::RayCastBVH(int32 node, const Ray& ray, Real& hitDistance, Vector3& hitNormal, Triangle& hitTriangle) const
{
    const auto& root = _bvh[node];
    if (!root.Bounds.Intersects(ray))
        return false;
    Vector3 normal;
    Real distance;
    bool hit = false;
    if (root.Leaf.IsLeaf)
    {
        // Ray cast along triangles in the leaf 
        const Mesh& meshData = _meshes[root.Leaf.MeshIndex];
        const Float3* vb = meshData.VertexBuffer.Get<Float3>();
        const int32 indexStart = root.Leaf.TriangleIndex * 3;
        const int32 indexEnd = indexStart + root.Leaf.TriangleCount * 3;
        if (meshData.Use16BitIndexBuffer)
        {
            const uint16* ib16 = meshData.IndexBuffer.Get<uint16>();
            for (int32 i = indexStart; i < indexEnd;)
            {
                Vector3 v0 = vb[ib16[i++]];
                Vector3 v1 = vb[ib16[i++]];
                Vector3 v2 = vb[ib16[i++]];
                if (CollisionsHelper::RayIntersectsTriangle(ray, v0, v1, v2, distance, normal) && distance < hitDistance)
                {
                    hitDistance = distance;
                    hitNormal = normal;
                    hitTriangle = Triangle(v0, v1, v2);
                    hit = true;
                }
            }
        }
        else
        {
            const uint32* ib32 = meshData.IndexBuffer.Get<uint32>();
            for (int32 i = indexStart; i < indexEnd;)
            {
                Vector3 v0 = vb[ib32[i++]];
                Vector3 v1 = vb[ib32[i++]];
                Vector3 v2 = vb[ib32[i++]];
                if (CollisionsHelper::RayIntersectsTriangle(ray, v0, v1, v2, distance, normal) && distance < hitDistance)
                {
                    hitDistance = distance;
                    hitNormal = normal;
                    hitTriangle = Triangle(v0, v1, v2);
                    hit = true;
                }
            }
        }
    }
    else
    {
        // Ray cast all child nodes
        Triangle triangle;
        for (uint32 i = 0; i < root.Node.ChildrenCount; i++)
        {
            const int32 index = root.Node.ChildIndex + i;
            distance = hitDistance;
            if (RayCastBVH(index, ray, distance, normal, triangle) && distance < hitDistance)
            {
                hitDistance = distance;
                hitNormal = normal;
                hitTriangle = triangle;
                hit = true;
            }
        }
    }
    return hit;
}

void MeshAccelerationStructure::Add(Model* model, int32 lodIndex)
{
    PROFILE_CPU();
    lodIndex = Math::Clamp(lodIndex, model->HighestResidentLODIndex(), model->LODs.Count() - 1);
    ModelLOD& lod = model->LODs[lodIndex];
    _meshes.EnsureCapacity(_meshes.Count() + lod.Meshes.Count());
    bool failed = false;
    for (int32 i = 0; i < lod.Meshes.Count(); i++)
    {
        auto& mesh = lod.Meshes[i];
        const MaterialSlot& materialSlot = model->MaterialSlots[mesh.GetMaterialSlotIndex()];
        if (materialSlot.Material && !materialSlot.Material->WaitForLoaded())
        {
            // Skip transparent materials
            if (materialSlot.Material->GetInfo().BlendMode != MaterialBlendMode::Opaque)
                continue;
        }

        auto& meshData = _meshes.AddOne();
        if (model->IsVirtual())
        {
            meshData.Indices = mesh.GetTriangleCount() * 3;
            meshData.Vertices = mesh.GetVertexCount();
            failed |= mesh.DownloadDataGPU(MeshBufferType::Index, meshData.IndexBuffer);
            failed |= mesh.DownloadDataGPU(MeshBufferType::Vertex0, meshData.VertexBuffer);
        }
        else
        {
            failed |= mesh.DownloadDataCPU(MeshBufferType::Index, meshData.IndexBuffer, meshData.Indices);
            failed |= mesh.DownloadDataCPU(MeshBufferType::Vertex0, meshData.VertexBuffer, meshData.Vertices);
        }
        if (failed)
            return;
        if (!meshData.IndexBuffer.IsAllocated() && meshData.IndexBuffer.Length() != 0)
        {
            // BVH nodes modifies index buffer (sorts data in-place) so clone it
            meshData.IndexBuffer.Copy(meshData.IndexBuffer.Get(), meshData.IndexBuffer.Length());
        }
        meshData.Use16BitIndexBuffer = mesh.Use16BitIndexBuffer();
        meshData.Bounds = mesh.GetBox();
    }
}

void MeshAccelerationStructure::Add(const ModelData* modelData, int32 lodIndex, bool copy)
{
    PROFILE_CPU();
    lodIndex = Math::Clamp(lodIndex, 0, modelData->LODs.Count() - 1);
    const ModelLodData& lod = modelData->LODs[lodIndex];
    _meshes.EnsureCapacity(_meshes.Count() + lod.Meshes.Count());
    for (int32 i = 0; i < lod.Meshes.Count(); i++)
    {
        MeshData* mesh = lod.Meshes[i];
        const MaterialSlotEntry& materialSlot = modelData->Materials[mesh->MaterialSlotIndex];
        auto material = Content::LoadAsync<MaterialBase>(materialSlot.AssetID);
        if (material && !material->WaitForLoaded())
        {
            // Skip transparent materials
            if (material->GetInfo().BlendMode != MaterialBlendMode::Opaque)
                continue;
        }

        auto& meshData = _meshes.AddOne();
        meshData.Indices = mesh->Indices.Count();
        meshData.Vertices = mesh->Positions.Count();
        if (copy)
        {
            meshData.IndexBuffer.Copy((const byte*)mesh->Indices.Get(), meshData.Indices * sizeof(uint32));
            meshData.VertexBuffer.Copy((const byte*)mesh->Positions.Get(), meshData.Vertices * sizeof(Float3));
        }
        else
        {
            meshData.IndexBuffer.Link((const byte*)mesh->Indices.Get(), meshData.Indices * sizeof(uint32));
            meshData.VertexBuffer.Link((const byte*)mesh->Positions.Get(), meshData.Vertices * sizeof(Float3));
        }
        meshData.Use16BitIndexBuffer = false;
        mesh->CalculateBox(meshData.Bounds);
    }
}

void MeshAccelerationStructure::Add(Float3* vb, int32 vertices, void* ib, int32 indices, bool use16BitIndex, bool copy)
{
    auto& meshData = _meshes.AddOne();
    if (copy)
    {
        meshData.VertexBuffer.Copy((const byte*)vb, vertices * sizeof(Float3));
    }
    else
    {
        meshData.VertexBuffer.Link((const byte*)vb, vertices * sizeof(Float3));
    }
    meshData.IndexBuffer.Copy((const byte*)ib, indices * (use16BitIndex ? sizeof(uint16) : sizeof(uint32)));
    meshData.Vertices = vertices;
    meshData.Indices = indices;
    meshData.Use16BitIndexBuffer = use16BitIndex;
}

void MeshAccelerationStructure::BuildBVH(int32 maxLeafSize)
{
    if (_meshes.Count() == 0)
        return;
    PROFILE_CPU();

    // Estimate memory usage
    int32 trianglesCount = 0;
    for (const Mesh& meshData : _meshes)
        trianglesCount += meshData.Indices / 3;
    _bvh.Clear();
    _bvh.EnsureCapacity(trianglesCount / maxLeafSize);

    // Init with the root node and all meshes as leaves
    auto& root = _bvh.AddOne();
    root.Node.IsLeaf = 0;
    root.Node.ChildIndex = 1;
    root.Node.ChildrenCount = _meshes.Count();
    root.Bounds = _meshes[0].Bounds;
    for (int32 i = 0; i < _meshes.Count(); i++)
    {
        const Mesh& meshData = _meshes[i];
        auto& child = _bvh.AddOne();
        child.Leaf.IsLeaf = 1;
        child.Leaf.MeshIndex = i;
        child.Leaf.TriangleIndex = 0;
        child.Leaf.TriangleCount = meshData.Indices / 3;
        child.Bounds = meshData.Bounds;
        BoundingBox::Merge(root.Bounds, meshData.Bounds, root.Bounds);
    }

    // Sub-divide mesh nodes into smaller leaves
    Array<byte> scratch;
    for (int32 i = 0; i < _meshes.Count(); i++)
        BuildBVH(i + 1, maxLeafSize, scratch);
}

bool MeshAccelerationStructure::PointQuery(const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle, Real maxDistance) const
{
    hitDistance = maxDistance >= MAX_Real ? maxDistance : maxDistance * maxDistance;
    bool hit = false;

    // BVH
    if (_bvh.Count() != 0)
    {
        Array<int32, InlinedAllocation<32>> stack;
        stack.Push(0);
        while (stack.HasItems())
        {
            const int32 node = stack.Pop();
            auto& root = _bvh[node];

            // Skip too far nodes
            if (root.Bounds.Distance(point) >= hitDistance)
                continue;

            if (root.Leaf.IsLeaf)
            {
                // Check this leaf
                hit |= PointQueryBVH(node, point, hitDistance, hitPoint, hitTriangle);
            }
            else
            {
                // Check this node children
                for (uint32 i = 0; i < root.Node.ChildrenCount; i++)
                    stack.Push(root.Node.ChildIndex + i);
            }
        }
        //hit = PointQueryBVH(0, point, hitDistance, hitPoint, hitTriangle);
        return hit;
    }

    // Brute-force
    {
        Vector3 p;
        for (const Mesh& meshData : _meshes)
        {
            const Float3* vb = meshData.VertexBuffer.Get<Float3>();
            if (meshData.Use16BitIndexBuffer)
            {
                const uint16* ib16 = meshData.IndexBuffer.Get<uint16>();
                for (int32 i = 0; i < meshData.Indices;)
                {
                    Vector3 v0 = vb[ib16[i++]];
                    Vector3 v1 = vb[ib16[i++]];
                    Vector3 v2 = vb[ib16[i++]];
                    CollisionsHelper::ClosestPointPointTriangle(point, v0, v1, v2, p);
                    const Real distance = Vector3::DistanceSquared(point, p);
                    if (distance < hitDistance)
                    {
                        hitDistance = distance;
                        hitPoint = p;
                        hitTriangle = Triangle(v0, v1, v2);
                        hit = true;
                    }
                }
            }
            else
            {
                const uint32* ib32 = meshData.IndexBuffer.Get<uint32>();
                for (int32 i = 0; i < meshData.Indices;)
                {
                    Vector3 v0 = vb[ib32[i++]];
                    Vector3 v1 = vb[ib32[i++]];
                    Vector3 v2 = vb[ib32[i++]];
                    CollisionsHelper::ClosestPointPointTriangle(point, v0, v1, v2, p);
                    const Real distance = Vector3::DistanceSquared(point, p);
                    if (distance < hitDistance)
                    {
                        hitDistance = distance;
                        hitPoint = p;
                        hitTriangle = Triangle(v0, v1, v2);
                        hit = true;
                    }
                }
            }
        }
        if (hit)
            hitDistance = Math::Sqrt(hitDistance);
        return hit;
    }
}

bool MeshAccelerationStructure::RayCast(const Ray& ray, Real& hitDistance, Vector3& hitNormal, Triangle& hitTriangle, Real maxDistance) const
{
    hitDistance = maxDistance;

    // BVH
    if (_bvh.Count() != 0)
    {
        return RayCastBVH(0, ray, hitDistance, hitNormal, hitTriangle);
    }

    // Brute-force
    {
        Vector3 normal;
        Real distance;
        bool hit = false;
        for (const Mesh& meshData : _meshes)
        {
            if (!meshData.Bounds.Intersects(ray))
                continue;
            const Float3* vb = meshData.VertexBuffer.Get<Float3>();
            if (meshData.Use16BitIndexBuffer)
            {
                const uint16* ib16 = meshData.IndexBuffer.Get<uint16>();
                for (int32 i = 0; i < meshData.Indices;)
                {
                    Vector3 v0 = vb[ib16[i++]];
                    Vector3 v1 = vb[ib16[i++]];
                    Vector3 v2 = vb[ib16[i++]];
                    if (CollisionsHelper::RayIntersectsTriangle(ray, v0, v1, v2, distance, normal) && distance < hitDistance)
                    {
                        hitDistance = distance;
                        hitNormal = normal;
                        hitTriangle = Triangle(v0, v1, v2);
                        hit = true;
                    }
                }
            }
            else
            {
                const uint32* ib32 = meshData.IndexBuffer.Get<uint32>();
                for (int32 i = 0; i < meshData.Indices;)
                {
                    Vector3 v0 = vb[ib32[i++]];
                    Vector3 v1 = vb[ib32[i++]];
                    Vector3 v2 = vb[ib32[i++]];
                    if (CollisionsHelper::RayIntersectsTriangle(ray, v0, v1, v2, distance, normal) && distance < hitDistance)
                    {
                        hitDistance = distance;
                        hitNormal = normal;
                        hitTriangle = Triangle(v0, v1, v2);
                        hit = true;
                    }
                }
            }
        }
        return hit;
    }
}

#endif
