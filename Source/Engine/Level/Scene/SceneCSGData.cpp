// Copyright (c) Wojciech Figat. All rights reserved.

#include "SceneCSGData.h"
#include "Engine/Core/Log.h"
#include "Engine/CSG/CSGBuilder.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/RawDataAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Physics/CollisionData.h"

using namespace CSG;

void Brush::OnBrushModified()
{
#if COMPILE_WITH_CSG_BUILDER
    const auto scene = GetBrushScene();
    if (scene && scene->IsDuringPlay())
    {
        Builder::OnBrushModified(this);
    }
#endif
}

SceneCSGData::SceneCSGData(Scene* scene)
    : _scene(scene)
    , BuildTime(0)
{
    Data.Loaded.Bind<SceneCSGData, &SceneCSGData::OnDataChanged>(this);
    Data.Changed.Bind<SceneCSGData, &SceneCSGData::OnDataChanged>(this);
}

void SceneCSGData::BuildCSG(float timeoutMs) const
{
#if COMPILE_WITH_CSG_BUILDER
    Builder::Build(_scene, timeoutMs);
#endif
}

bool SceneCSGData::HasData() const
{
    return Model && Data;
}

bool SceneCSGData::SurfaceData::Intersects(const Ray& ray, Real& distance, Vector3& normal)
{
    bool result = false;
    Real minDistance = MAX_Real;
    Vector3 minDistanceNormal = Vector3::Up;
    for (int32 i = 0; i < Triangles.Count(); i++)
    {
        auto& e = Triangles[i];
        if (CollisionsHelper::RayIntersectsTriangle(ray, e.V0, e.V1, e.V2, distance, normal) && distance < minDistance)
        {
            minDistance = distance;
            minDistanceNormal = normal;
            result = true;
        }
    }
    distance = minDistance;
    normal = minDistanceNormal;
    return result;
}

bool SceneCSGData::TryGetSurfaceData(const Guid& brushId, int32 brushSurfaceIndex, SurfaceData& outData)
{
    if (Data == nullptr || !Data->IsLoaded() || Data->Data.IsEmpty())
    {
        // Missing data or not loaded
        return false;
    }

    MemoryReadStream stream(Data->Data);

    // Check if has missing cache
    if (DataBrushLocations.IsEmpty())
    {
        int32 version;
        stream.ReadInt32(&version);
        if (version == 1)
        {
            int32 brushesCount;
            stream.ReadInt32(&brushesCount);
            if (brushesCount < 0)
            {
                // Invalid data
                return false;
            }
            DataBrushLocations.EnsureCapacity(brushesCount);
            for (int32 i = 0; i < brushesCount; i++)
            {
                Guid id;
                int32 pos;
                stream.Read(id);
                stream.ReadInt32(&pos);
                DataBrushLocations.Add(id, pos);
            }
        }
        else
        {
            // Unknown version
            LOG(Warning, "Unknwon version for scene CSG surface data (or corrupted file).");
            return false;
        }
    }

    // Find brush data
    int32 brushLocation;
    if (!DataBrushLocations.TryGet(brushId, brushLocation))
    {
        // Brush not found
        return false;
    }

    stream.SetPosition(brushLocation);

    // Skip leading surfaces data
    int32 trianglesCount;
    while (brushSurfaceIndex-- > 0)
    {
        stream.ReadInt32(&trianglesCount);
        if (trianglesCount < 0 || trianglesCount > 100)
        {
            // Invalid data
            return false;
        }
        stream.Move(trianglesCount * sizeof(Float3) * 3);
    }

    // Read surface data
    stream.ReadInt32(&trianglesCount);
    if (trianglesCount < 0 || trianglesCount > 100)
    {
        // Invalid data
        return false;
    }
    outData.Triangles.Clear();
    outData.Triangles.Resize(trianglesCount);
    Float3* src = stream.Move<Float3>(trianglesCount * 3);
    Vector3* dst = (Vector3*)outData.Triangles.Get();
    for (int32 i = 0; i < trianglesCount * 3; i++)
        *dst++ = *src++;
    return true;
}

void SceneCSGData::OnDataChanged()
{
    // Clear cache
    DataBrushLocations.Clear();
}

void SceneCSGData::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(SceneCSGData);

    SERIALIZE(Model);
    SERIALIZE(Data);
    SERIALIZE(CollisionData);
}

void SceneCSGData::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(Model);
    DESERIALIZE(Data);
    DESERIALIZE(CollisionData);
}
