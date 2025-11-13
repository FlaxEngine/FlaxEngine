// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL

#include "MeshAccelerationStructure.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Profiler/ProfilerCPU.h"

PACK_STRUCT(struct GPUBVH {
    Float3 BoundsMin;
    uint32 Index;
    Float3 BoundsMax;
    int32 Count; // Negative for non-leaf nodes
});
static_assert(sizeof(GPUBVH) == sizeof(Float4) * 2, "Invalid BVH structure size for GPU.");

void MeshAccelerationStructure::BuildBVH(int32 node, BVHBuild& build)
{
    auto& root = _bvh[node];
    ASSERT_LOW_LAYER(root.Leaf.IsLeaf);
    if (build.MaxLeafSize > 0 && root.Leaf.TriangleCount <= build.MaxLeafSize)
        return;
    if (build.MaxDepth > 0 && build.NodeDepth >= build.MaxDepth)
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
        build.Scratch.Resize(root.Leaf.TriangleCount * sizeof(Tri));
        auto dst = (Tri*)build.Scratch.Get();
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
            left.Bounds.Merge(vb[((uint16*)build.Scratch.Get())[i]]);

        right.Bounds = BoundingBox(vb[dst[root.Leaf.TriangleCount - 1].I0]);
        indexStart = left.Leaf.TriangleCount;
        indexEnd = root.Leaf.TriangleCount * 3;
        for (int32 i = indexStart; i < indexEnd; i++)
            right.Bounds.Merge(vb[((uint16*)build.Scratch.Get())[i]]);
    }
    else
    {
        struct Tri
        {
            uint32 I0, I1, I2;
        };
        build.Scratch.Resize(root.Leaf.TriangleCount * sizeof(Tri));
        auto dst = (Tri*)build.Scratch.Get();
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
            left.Bounds.Merge(vb[((uint32*)build.Scratch.Get())[i]]);

        right.Bounds = BoundingBox(vb[dst[root.Leaf.TriangleCount - 1].I0]);
        indexStart = left.Leaf.TriangleCount;
        indexEnd = root.Leaf.TriangleCount * 3;
        for (int32 i = indexStart; i < indexEnd; i++)
            right.Bounds.Merge(vb[((uint32*)build.Scratch.Get())[i]]);
    }
    ASSERT_LOW_LAYER(left.Leaf.TriangleCount + right.Leaf.TriangleCount == root.Leaf.TriangleCount);
    left.Leaf.TriangleIndex = root.Leaf.TriangleIndex;
    right.Leaf.TriangleIndex = left.Leaf.TriangleIndex + left.Leaf.TriangleCount;
    build.MaxNodeTriangles = Math::Max(build.MaxNodeTriangles, (int32)right.Leaf.TriangleCount);
    build.MaxNodeTriangles = Math::Max(build.MaxNodeTriangles, (int32)right.Leaf.TriangleCount);

    // Convert into a node
    root.Node.IsLeaf = 0;
    root.Node.ChildIndex = childIndex;
    root.Node.ChildrenCount = 2;

    // Split children
    build.NodeDepth++;
    build.MaxNodeDepth = Math::Max(build.NodeDepth, build.MaxNodeDepth);
    BuildBVH(childIndex, build);
    BuildBVH(childIndex + 1, build);
    build.NodeDepth--;
}

bool MeshAccelerationStructure::PointQueryBVH(int32 node, const Vector3& point, Real& hitDistance, Vector3& hitPoint, Triangle& hitTriangle) const
{
    const auto& root = _bvh[node];
    bool hit = false;
    if (root.Leaf.IsLeaf)
    {
        // Find closest triangle
        Vector3 p;
        const Mesh& meshData = _meshes.Get()[root.Leaf.MeshIndex];
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
        const Mesh& meshData = _meshes.Get()[root.Leaf.MeshIndex];
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

MeshAccelerationStructure::~MeshAccelerationStructure()
{
    for (auto& e : _meshes)
    {
        if (e.Asset)
            e.Asset->RemoveReference();
    }
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
        meshData.Asset = model;
        model->AddReference();
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
        meshData.Asset = nullptr;
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
    ASSERT(vertices % 3 == 0);
    auto& meshData = _meshes.AddOne();
    meshData.Asset = nullptr;
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
    BoundingBox::FromPoints(meshData.VertexBuffer.Get<Float3>(), vertices, meshData.Bounds);
}

void MeshAccelerationStructure::MergeMeshes(bool force16BitIndexBuffer)
{
    if (_meshes.Count() == 0)
        return;
    if (_meshes.Count() == 1 && (!force16BitIndexBuffer || !_meshes[0].Use16BitIndexBuffer))
        return;
    PROFILE_CPU();
    auto meshes = _meshes;
    _meshes.Clear();
    _meshes.Resize(1);
    auto& mesh = _meshes[0];
    mesh.Asset = nullptr;
    mesh.Use16BitIndexBuffer = true;
    mesh.Indices = 0;
    mesh.Vertices = 0;
    mesh.Bounds = meshes[0].Bounds;
    for (auto& e : meshes)
    {
        if (!e.Use16BitIndexBuffer)
            mesh.Use16BitIndexBuffer = false;
        mesh.Vertices += e.Vertices;
        mesh.Indices += e.Indices;
        BoundingBox::Merge(mesh.Bounds, e.Bounds, mesh.Bounds);
    }
    mesh.Use16BitIndexBuffer &= mesh.Indices <= MAX_uint16 && !force16BitIndexBuffer;
    mesh.VertexBuffer.Allocate(mesh.Vertices * sizeof(Float3));
    mesh.IndexBuffer.Allocate(mesh.Indices * sizeof(uint32));
    int32 vertexCounter = 0, indexCounter = 0;
    for (auto& e : meshes)
    {
        Platform::MemoryCopy(mesh.VertexBuffer.Get() + vertexCounter * sizeof(Float3), e.VertexBuffer.Get(), e.Vertices * sizeof(Float3));
        if (e.Use16BitIndexBuffer)
        {
            for (int32 i = 0; i < e.Indices; i++)
            {
                uint16 index = ((uint16*)e.IndexBuffer.Get())[i];
                ((uint32*)mesh.IndexBuffer.Get())[indexCounter + i] = vertexCounter + index;
            }
        }
        else
        {
            for (int32 i = 0; i < e.Indices; i++)
            {
                uint16 index = ((uint32*)e.IndexBuffer.Get())[i];
                ((uint32*)mesh.IndexBuffer.Get())[indexCounter + i] = vertexCounter + index;
            }
        }
        vertexCounter += e.Vertices;
        indexCounter += e.Indices;
        if (e.Asset)
            e.Asset->RemoveReference();
    }
}

void MeshAccelerationStructure::BuildBVH(int32 maxLeafSize, int32 maxDepth)
{
    if (_meshes.Count() == 0)
        return;
    PROFILE_CPU();

    BVHBuild build;
    build.MaxLeafSize = maxLeafSize;
    build.MaxDepth = maxDepth;

    // Estimate memory usage
    int32 trianglesCount = 0;
    for (const Mesh& meshData : _meshes)
        trianglesCount += meshData.Indices / 3;
    _bvh.Clear();
    _bvh.EnsureCapacity(trianglesCount / Math::Max(maxLeafSize, 16));

    // Skip using root node if BVH contains only one mesh
    if (_meshes.Count() == 1)
    {
        const Mesh& meshData = _meshes.First();
        auto& child = _bvh.AddOne();
        child.Leaf.IsLeaf = 1;
        child.Leaf.MeshIndex = 0;
        child.Leaf.TriangleIndex = 0;
        child.Leaf.TriangleCount = meshData.Indices / 3;
        child.Bounds = meshData.Bounds;
        Array<byte> scratch;
        BuildBVH(0, build);
    }
    else
    {
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
        build.MaxNodeDepth = build.MaxDepth = 2;
        Array<byte> scratch;
        for (int32 i = 0; i < _meshes.Count(); i++)
            BuildBVH(i + 1, build);
        build.NodeDepth = 0;
    }

    LOG(Info, "BVH nodes: {}, max depth: {}, max triangles: {}", _bvh.Count(), build.MaxNodeDepth, build.MaxNodeTriangles);
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

MeshAccelerationStructure::GPU::~GPU()
{
    SAFE_DELETE_GPU_RESOURCE(BVHBuffer);
    SAFE_DELETE_GPU_RESOURCE(VertexBuffer);
    SAFE_DELETE_GPU_RESOURCE(IndexBuffer);
}

MeshAccelerationStructure::GPU::operator bool() const
{
    // Index buffer is initialized as last one so all other buffers are fine too
    return IndexBuffer && IndexBuffer->GetSize() != 0;
}

MeshAccelerationStructure::GPU MeshAccelerationStructure::ToGPU()
{
    PROFILE_CPU();
    GPU gpu;

    // GPU BVH operates on a single mesh with 32-bit indices
    MergeMeshes(true);

    // Construct BVH
    const int32 BVH_STACK_SIZE = 32; // This must match HLSL shader
    BuildBVH(0, BVH_STACK_SIZE);

    // Upload BVH
    {
        Array<GPUBVH> bvhData;
        bvhData.Resize(_bvh.Count());
        for (int32 i = 0; i < _bvh.Count(); i++)
        {
            const auto& src = _bvh.Get()[i];
            auto& dst = bvhData.Get()[i];
            dst.BoundsMin = src.Bounds.Minimum;
            dst.BoundsMax = src.Bounds.Maximum;
            if (src.Leaf.IsLeaf)
            {
                dst.Index = src.Leaf.TriangleIndex * 3;
                dst.Count = src.Leaf.TriangleCount * 3;
            }
            else
            {
                dst.Index = src.Node.ChildIndex;
                dst.Count = -(int32)src.Node.ChildrenCount; // Mark as non-leaf
                ASSERT(src.Node.ChildrenCount == 2); // GPU shader is hardcoded for 2 children per node
            }
        }
        gpu.BVHBuffer = GPUBuffer::New();
        auto desc =GPUBufferDescription::Structured(_bvh.Count(), sizeof(GPUBVH));
        desc.InitData = bvhData.Get();
        if (gpu.BVHBuffer->Init(desc))
            return gpu;
    }

    // Upload vertex buffer
    {
        const Mesh& mesh = _meshes[0];
        gpu.VertexBuffer = GPUBuffer::New();
        auto desc = GPUBufferDescription::Raw(mesh.Vertices * sizeof(Float3), GPUBufferFlags::ShaderResource);
        desc.InitData = mesh.VertexBuffer.Get();
        if (gpu.VertexBuffer->Init(desc))
            return gpu;
    }

    // Upload index buffer
    {
        const Mesh& mesh = _meshes[0];
        gpu.IndexBuffer = GPUBuffer::New();
        auto desc = GPUBufferDescription::Raw(mesh.Indices * sizeof(uint32), GPUBufferFlags::ShaderResource);
        desc.InitData = mesh.IndexBuffer.Get();
        gpu.IndexBuffer->Init(desc);
    }

    return gpu;
}

#endif
