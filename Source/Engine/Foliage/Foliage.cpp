// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Foliage.h"
#include "FoliageType.h"
#include "FoliageCluster.h"
#include "Engine/Core/Random.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Utilities/Encryption.h"

Foliage::Foliage(const SpawnParams& params)
    : Actor(params)
{
    _disableFoliageTypeEvents = false;
    Root = nullptr;
}

void Foliage::EnsureRoot()
{
    // Skip if root is already here or there is no instances at all
    if (Root || Instances.IsEmpty())
        return;
    ASSERT(Clusters.IsEmpty());

    PROFILE_CPU();

    // Calculate total bounds of valid instances
    BoundingBox totalBounds;
    {
        bool anyValid = false;
        // TODO: inline code and use SIMD
        BoundingBox box;
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (!FoliageTypes[i->Type].IsReady())
                continue;

            BoundingBox::FromSphere(i->Bounds, box);
            ASSERT(!i->Bounds.Center.IsNanOrInfinity());

            if (anyValid)
            {
                BoundingBox::Merge(totalBounds, box, totalBounds);
            }
            else
            {
                totalBounds = box;
                anyValid = true;
            }
        }

        // Skip if nothing is valid
        if (!anyValid)
            return;
    }
    ASSERT(!totalBounds.Minimum.IsNanOrInfinity() && !totalBounds.Maximum.IsNanOrInfinity());

    // Setup first and topmost cluster
    Clusters.Resize(1);
    Root = &Clusters[0];
    Root->Init(totalBounds);

    // Cache bounds
    _box = Root->Bounds;
    BoundingSphere::FromBox(_box, _sphere);
}

void Foliage::AddToCluster(FoliageCluster* cluster, FoliageInstance& instance)
{
    ASSERT(instance.Bounds.Radius > ZeroTolerance);
    ASSERT(cluster->Bounds.Intersects(instance.Bounds));

    // Find target cluster
    while (cluster->Children[0])
    {
#define CHECK_CHILD(idx) \
			if (cluster->Children[idx]->Bounds.Intersects(instance.Bounds)) \
			{ \
				cluster = cluster->Children[idx]; \
				continue; \
			}
        CHECK_CHILD(0);
        CHECK_CHILD(1);
        CHECK_CHILD(2);
        CHECK_CHILD(3);
#undef CHECK_CHILD
    }

    // Check if it's not full
    if (cluster->Instances.Count() != FOLIAGE_CLUSTER_CAPACITY)
    {
        // Insert into cluster
        cluster->Instances.Add(&instance);
    }
    else
    {
        // Subdivide cluster
        const int32 count = Clusters.Count();
        Clusters.Resize(count + 4);
        cluster->Children[0] = &Clusters[count + 0];
        cluster->Children[1] = &Clusters[count + 1];
        cluster->Children[2] = &Clusters[count + 2];
        cluster->Children[3] = &Clusters[count + 3];

        // Setup children
        const Vector3 min = cluster->Bounds.Minimum;
        const Vector3 max = cluster->Bounds.Maximum;
        const Vector3 size = cluster->Bounds.GetSize();
        cluster->Children[0]->Init(BoundingBox(min, min + size * Vector3(0.5f, 1.0f, 0.5f)));
        cluster->Children[1]->Init(BoundingBox(min + size * Vector3(0.5f, 0.0f, 0.5f), max));
        cluster->Children[2]->Init(BoundingBox(min + size * Vector3(0.5f, 0.0f, 0.0f), min + size * Vector3(1.0f, 1.0f, 0.5f)));
        cluster->Children[3]->Init(BoundingBox(min + size * Vector3(0.0f, 0.0f, 0.5f), min + size * Vector3(0.5f, 1.0f, 1.0f)));

        // Move instances to a proper cells
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            AddToCluster(cluster, *cluster->Instances[i]);
        }
        cluster->Instances.Clear();
        AddToCluster(cluster, instance);
    }
}

int32 Foliage::GetInstancesCount() const
{
    return Instances.Count();
}

FoliageInstance Foliage::GetInstance(int32 index) const
{
    return Instances[index];
}

int32 Foliage::GetFoliageTypesCount() const
{
    return FoliageTypes.Count();
}

FoliageType* Foliage::GetFoliageType(int32 index)
{
    CHECK_RETURN(index >= 0 && index < FoliageTypes.Count(), nullptr)
    return &FoliageTypes[index];
}

void Foliage::AddFoliageType(Model* model)
{
    // Ensure to have unique model
    CHECK(model);
    for (int32 i = 0; i < FoliageTypes.Count(); i++)
    {
        if (FoliageTypes[i].Model == model)
        {
            LOG(Error, "The given model is already used by other foliage type.");
            return;
        }
    }

    // Add
    _disableFoliageTypeEvents = true;
    auto& item = FoliageTypes.AddOne();
    _disableFoliageTypeEvents = false;

    // Setup
    item.Foliage = this;
    item.Index = FoliageTypes.Count() - 1;
    item.Model = model;
}

void Foliage::RemoveFoliageType(int32 index)
{
    // Remove instances using this foliage type
    if (FoliageTypes.Count() != 1)
    {
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Type == index)
            {
                Instances.Remove(i);
                --i;
            }
        }

        // Update all instances using foliage types with higher index to point into a valid type
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Type > index)
                i->Type--;
        }
    }
    else
    {
        Instances.Clear();
    }

    // Remove foliage instance type
    for (int32 i = index + 1; i < FoliageTypes.Count(); i++)
    {
        FoliageTypes[i].Index--;
    }
    auto& item = FoliageTypes[index];
    item.Model = nullptr;
    item.Entries.Release();
    FoliageTypes.RemoveAtKeepOrder(index);

    RebuildClusters();
}

int32 Foliage::GetFoliageTypeInstancesCount(int32 index) const
{
    PROFILE_CPU();

    int32 result = 0;

    for (auto i = Instances.Begin(); i.IsNotEnd(); i++)
    {
        if (i->Type == index)
            result++;
    }

    return result;
}

void Foliage::AddInstance(const FoliageInstance& instance)
{
    ASSERT(instance.Type >= 0 && instance.Type < FoliageTypes.Count());
    auto type = &FoliageTypes[instance.Type];

    // Add instance
    auto data = Instances.Add(instance);
    data->Bounds = BoundingSphere::Empty;
    data->Random = Random::Rand();
    data->CullDistance = type->CullDistance + type->CullDistanceRandomRange * data->Random;

    // Calculate foliage instance geometry transformation matrix
    Matrix matrix, world;
    GetLocalToWorldMatrix(world);
    data->Transform.GetWorld(matrix);
    Matrix::Multiply(matrix, world, data->World);
    data->DrawState.PrevWorld = data->World;

    // Validate foliage type model
    if (!type->IsReady())
        return;

    // Update bounds
    Vector3 corners[8];
    auto& meshes = type->Model->LODs[0].Meshes;
    for (int32 j = 0; j < meshes.Count(); j++)
    {
        meshes[j].GetCorners(corners);

        for (int32 k = 0; k < 8; k++)
        {
            Vector3::Transform(corners[k], data->World, corners[k]);
        }
        BoundingSphere meshBounds;
        BoundingSphere::FromPoints(corners, 8, meshBounds);
        ASSERT(meshBounds.Radius > ZeroTolerance);

        BoundingSphere::Merge(data->Bounds, meshBounds, data->Bounds);
    }
    data->Bounds.Radius += ZeroTolerance;
}

void Foliage::RemoveInstance(ChunkedArray<FoliageInstance, FOLIAGE_INSTANCE_CHUNKS_SIZE>::Iterator i)
{
    Instances.Remove(i);
}

void Foliage::SetInstanceTransform(int32 index, const Transform& value)
{
    auto& instance = Instances[index];
    auto type = &FoliageTypes[instance.Type];

    // Change transform
    instance.Transform = value;

    // Update world matrix
    Matrix matrix, world;
    GetLocalToWorldMatrix(world);
    instance.Transform.GetWorld(matrix);
    Matrix::Multiply(matrix, world, instance.World);

    // Update bounds
    instance.Bounds = BoundingSphere::Empty;
    if (!type->IsReady())
        return;
    Vector3 corners[8];
    auto& meshes = type->Model->LODs[0].Meshes;
    for (int32 j = 0; j < meshes.Count(); j++)
    {
        meshes[j].GetCorners(corners);

        for (int32 k = 0; k < 8; k++)
        {
            Vector3::Transform(corners[k], instance.World, corners[k]);
        }
        BoundingSphere meshBounds;
        BoundingSphere::FromPoints(corners, 8, meshBounds);
        ASSERT(meshBounds.Radius > ZeroTolerance);

        BoundingSphere::Merge(instance.Bounds, meshBounds, instance.Bounds);
    }
    instance.Bounds.Radius += ZeroTolerance;
}

void Foliage::OnFoliageTypeModelLoaded(int32 index)
{
    if (_disableFoliageTypeEvents)
        return;
    ASSERT(index >= 0 && index < FoliageTypes.Count());
    auto type = &FoliageTypes[index];
    ASSERT(type->IsReady());

    // TODO: maybe deffer OnFoliageTypeModelLoaded handling to game logic update because many foliage types may fire it during the same frame - save some CPU time

    PROFILE_CPU();

    // Update bounds for instances using this type
    bool hasAnyInstance = false;
    {
        PROFILE_CPU_NAMED("Update Bounds");

        Vector3 corners[8];
        auto& meshes = type->Model->LODs[0].Meshes;
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            if (instance.Type != index)
                continue;
            instance.Bounds = BoundingSphere::Empty;
            hasAnyInstance = true;

            // Include all meshes
            for (int32 j = 0; j < meshes.Count(); j++)
            {
                meshes[j].GetCorners(corners);

                for (int32 k = 0; k < 8; k++)
                {
                    Vector3::Transform(corners[k], instance.World, corners[k]);
                }
                BoundingSphere meshBounds;
                BoundingSphere::FromPoints(corners, 8, meshBounds);
                ASSERT(meshBounds.Radius > ZeroTolerance);

                BoundingSphere::Merge(instance.Bounds, meshBounds, instance.Bounds);
            }
        }
    }
    if (!hasAnyInstance)
        return;

    RebuildClusters();
}

void Foliage::RebuildClusters()
{
    PROFILE_CPU();

    // Remove previous clusters data
    Root = nullptr;
    Clusters.Clear();

    EnsureRoot();

    // Insert all instances to the clusters
    {
        PROFILE_CPU_NAMED("Create Clusters");

        const float globalDensityScale = GetGlobalDensityScale();
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            const float densityScale = type.UseDensityScaling ? globalDensityScale * type.DensityScalingScale : 1.0f;

            if (type.IsReady() && instance.Random < densityScale)
            {
                AddToCluster(Root, instance);
            }
        }
    }

    if (Root)
    {
        PROFILE_CPU_NAMED("Update Cache");

        Root->UpdateTotalBoundsAndCullDistance();
    }
}

void Foliage::UpdateCullDistance()
{
    PROFILE_CPU();

    {
        PROFILE_CPU_NAMED("Instances");

        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            instance.CullDistance = type.CullDistance + type.CullDistanceRandomRange * instance.Random;
        }
    }

    if (Root)
    {
        PROFILE_CPU_NAMED("Clusters");

        Root->UpdateCullDistance();
    }
}

static float GlobalDensityScale = 1.0f;

float Foliage::GetGlobalDensityScale()
{
    return GlobalDensityScale;
}

bool UpdateFoliageDensityScaling(Actor* actor)
{
    if (auto* foliage = dynamic_cast<Foliage*>(actor))
    {
        foliage->RebuildClusters();
    }

    return true;
}

void Foliage::SetGlobalDensityScale(float value)
{
    value = Math::Saturate(value);
    if (Math::NearEqual(value, GlobalDensityScale))
        return;

    PROFILE_CPU();

    GlobalDensityScale = value;

    Function<bool(Actor*)> f(UpdateFoliageDensityScaling);
    SceneQuery::TreeExecute(f);
}

bool Foliage::Intersects(const Ray& ray, float& distance, Vector3& normal, int32& instanceIndex)
{
    PROFILE_CPU();

    instanceIndex = -1;

    if (Root)
    {
        FoliageInstance* instance;
        if (Root->Intersects(this, ray, distance, normal, instance))
        {
            int32 j = 0;
            for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
            {
                if (&*i == instance)
                {
                    instanceIndex = j;
                    break;
                }
                j++;
            }

            return true;
        }
    }

    distance = MAX_float;
    normal = Vector3::Up;
    return false;
}

void Foliage::DrawCluster(RenderContext& renderContext, FoliageCluster* cluster, Mesh::DrawInfo& draw)
{
    // Skip clusters that around too far from view
    if (Vector3::Distance(renderContext.View.Position, cluster->TotalBoundsSphere.Center) - cluster->TotalBoundsSphere.Radius > cluster->MaxCullDistance)
        return;

    //DebugDraw::DrawBox(cluster->Bounds, Color::Red);

    // Draw visible children
    if (cluster->Children[0])
    {
#if BUILD_DEBUG
        // Don't store instances in non-leaf nodes
        ASSERT(cluster->Instances.IsEmpty());
#endif

#define DRAW_CLUSTER(idx) \
		if (renderContext.View.CullingFrustum.Intersects(cluster->Children[idx]->TotalBounds)) \
			DrawCluster(renderContext, cluster->Children[idx], draw)
        DRAW_CLUSTER(0);
        DRAW_CLUSTER(1);
        DRAW_CLUSTER(2);
        DRAW_CLUSTER(3);
#undef 	DRAW_CLUSTER
    }
    else
    {
        // Draw visible instances
        const auto frame = Engine::FrameCount;
        for (int32 i = 0; i < cluster->Instances.Count(); i++)
        {
            auto& instance = *cluster->Instances[i];
            auto& type = FoliageTypes[instance.Type];

            // Check if can draw this instance
            if (type._canDraw &&
                Vector3::Distance(renderContext.View.Position, instance.Bounds.Center) - instance.Bounds.Radius < instance.CullDistance &&
                renderContext.View.CullingFrustum.Intersects(instance.Bounds))
            {
                // Disable motion blur
                instance.DrawState.PrevWorld = instance.World;

                // Draw model
                draw.Lightmap = GetScene()->LightmapsData.GetReadyLightmap(instance.Lightmap.TextureIndex);
                draw.LightmapUVs = &instance.Lightmap.UVsArea;
                draw.Buffer = &type.Entries;
                draw.World = &instance.World;
                draw.DrawState = &instance.DrawState;
                draw.Bounds = instance.Bounds;
                draw.PerInstanceRandom = instance.Random;
                draw.DrawModes = type._drawModes;
                type.Model->Draw(renderContext, draw);

                //DebugDraw::DrawSphere(instance.Bounds, Color::YellowGreen);

                instance.DrawState.PrevFrame = frame;
            }
        }
    }
}

void Foliage::Draw(RenderContext& renderContext)
{
    // Skip if no instances spawned
    if (Instances.IsEmpty() || !Root)
        return;
    auto& view = renderContext.View;

    PROFILE_CPU();

    // Cache data per foliage instance type
    for (int32 i = 0; i < FoliageTypes.Count(); i++)
    {
        auto& type = FoliageTypes[i];

        const auto drawModes = static_cast<DrawPass>(type.DrawModes & view.Pass & (int32)view.GetShadowsDrawPassMask(type.ShadowsMode));
        type._canDraw = type.IsReady() && drawModes != DrawPass::None;
        type._drawModes = drawModes;

        if (type._canDraw)
        {
            for (int32 j = 0; j < type.Entries.Count(); j++)
            {
                auto& e = type.Entries[j];

                e.ReceiveDecals = type.ReceiveDecals != 0;
                e.ShadowsMode = type.ShadowsMode;
            }
        }
    }

    // Draw visible clusters
    Mesh::DrawInfo draw;
    draw.Flags = GetStaticFlags();
    draw.DrawModes = (DrawPass)(DrawPass::Default & view.Pass);
    draw.LODBias = 0;
    draw.ForcedLOD = -1;
    draw.VertexColors = nullptr;
    DrawCluster(renderContext, Root, draw);
}

void Foliage::DrawGeneric(RenderContext& renderContext)
{
    Draw(renderContext);
}

bool Foliage::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    int32 instanceIndex;
    return Intersects(ray, distance, normal, instanceIndex);
}

// Layout for encoded instance data (serialized as Base64 string)

static constexpr int32 GetInstanceBase64Size(int32 size)
{
    // 4 * (n / 3) -> align up to 4
    return (size * 4 / 3 + 3) & ~3;
}

// [Deprecated on 30.11.2019, expires on 30.11.2021]
struct InstanceEncoded1
{
    int32 Type;
    float Random;
    Transform Transform;

    static constexpr int32 Size = 48;
    static constexpr int32 Base64Size = GetInstanceBase64Size(Size);
};

struct InstanceEncoded2
{
    int32 Type;
    float Random;
    Transform Transform;
    LightmapEntry Lightmap;

    static const int32 Size = 68;
    static const int32 Base64Size = GetInstanceBase64Size(Size);
};

typedef InstanceEncoded2 InstanceEncoded;
static_assert(InstanceEncoded::Size == sizeof(InstanceEncoded), "Please update base64 buffer size to match the encoded instance buffer.");
static_assert(InstanceEncoded::Base64Size == GetInstanceBase64Size(sizeof(InstanceEncoded)), "Please update base64 buffer size to match the encoded instance buffer.");

void Foliage::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Foliage);

    if (FoliageTypes.IsEmpty())
        return;

    PROFILE_CPU();

    stream.JKEY("Foliage");
    stream.StartArray();
    for (int32 i = 0; i < FoliageTypes.Count(); i++)
    {
        stream.StartObject();
        FoliageTypes[i].Serialize(stream, nullptr);
        stream.EndObject();
    }
    stream.EndArray();

    stream.JKEY("Instances");
    stream.StartArray();
    InstanceEncoded enc;
    char base64[InstanceEncoded::Base64Size];
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        auto& instance = *i;

        enc.Type = instance.Type;
        enc.Random = instance.Random;
        enc.Transform = instance.Transform;
        enc.Lightmap = instance.Lightmap;

        Encryption::Base64Encode((const byte*)&enc, sizeof(enc), base64);

        stream.String(base64, InstanceEncoded::Base64Size);
    }
    stream.EndArray();
}

void Foliage::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    PROFILE_CPU();

    // Clear
    Root = nullptr;
    Instances.Release();
    Clusters.Release();
    FoliageTypes.Resize(0, false);

    // Deserialize foliage types
    int32 foliageTypesCount = 0;
    const auto& foliageTypesMember = stream.FindMember("Foliage");
    if (foliageTypesMember != stream.MemberEnd() && foliageTypesMember->value.IsArray())
    {
        foliageTypesCount = foliageTypesMember->value.Size();
    }
    if (foliageTypesCount)
    {
        const DeserializeStream& items = foliageTypesMember->value;;
        FoliageTypes.Resize(foliageTypesCount, false);
        for (int32 i = 0; i < foliageTypesCount; i++)
        {
            FoliageTypes[i].Foliage = this;
            FoliageTypes[i].Index = i;
            FoliageTypes[i].Deserialize((DeserializeStream&)items[i], modifier);
        }
    }

    // Skip if no foliage
    if (FoliageTypes.IsEmpty())
        return;

    // Deserialize foliage instances
    int32 foliageInstancesCount = 0;
    const auto& foliageInstancesMember = stream.FindMember("Instances");
    if (foliageInstancesMember != stream.MemberEnd() && foliageInstancesMember->value.IsArray())
    {
        foliageInstancesCount = foliageInstancesMember->value.Size();
    }
    if (foliageInstancesCount)
    {
        const DeserializeStream& items = foliageInstancesMember->value;
        Instances.Resize(foliageInstancesCount);

        if (modifier->EngineBuild <= 6189)
        {
            // [Deprecated on 30.11.2019, expires on 30.11.2021]
            InstanceEncoded1 enc;
            for (int32 i = 0; i < foliageInstancesCount; i++)
            {
                auto& instance = Instances[i];
                auto& item = items[i];

                const int32 length = item.GetStringLength();
                if (length != InstanceEncoded1::Base64Size)
                {
                    LOG(Warning, "Invalid foliage instance data size.");
                    continue;
                }
                Encryption::Base64Decode(item.GetString(), length, (byte*)&enc);

                instance.Type = enc.Type;
                instance.Random = enc.Random;
                instance.Transform = enc.Transform;
                instance.Lightmap = LightmapEntry();
            }
        }
        else
        {
            InstanceEncoded enc;
            for (int32 i = 0; i < foliageInstancesCount; i++)
            {
                auto& instance = Instances[i];
                auto& item = items[i];

                const int32 length = item.GetStringLength();
                if (length != InstanceEncoded::Base64Size)
                {
                    LOG(Warning, "Invalid foliage instance data size.");
                    continue;
                }
                Encryption::Base64Decode(item.GetString(), length, (byte*)&enc);

                instance.Type = enc.Type;
                instance.Random = enc.Random;
                instance.Transform = enc.Transform;
                instance.Lightmap = enc.Lightmap;
            }
        }

#if BUILD_DEBUG
        // Remove invalid instances
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Type < 0 || i->Type >= FoliageTypes.Count())
            {
                LOG(Warning, "Removing invalid foliage instance.");
                Instances.Remove(i);
                --i;
            }
        }
#endif

        // Update cull distance
        for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = FoliageTypes[instance.Type];
            instance.CullDistance = type.CullDistance + type.CullDistanceRandomRange * instance.Random;
        }
    }
}

void Foliage::OnEnable()
{
    GetScene()->Rendering.AddGeometry(this);

    // Base
    Actor::OnEnable();
}

void Foliage::OnDisable()
{
    GetScene()->Rendering.RemoveGeometry(this);

    // Base
    Actor::OnDisable();
}

void Foliage::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    PROFILE_CPU();

    // Update instances matrices and cached world bounds
    Vector3 corners[8];
    Matrix world, matrix;
    GetLocalToWorldMatrix(world);
    for (auto i = Instances.Begin(); i.IsNotEnd(); ++i)
    {
        auto& instance = *i;
        auto type = &FoliageTypes[instance.Type];

        // Update world matrix
        instance.Transform.GetWorld(matrix);
        Matrix::Multiply(matrix, world, instance.World);

        // Update bounds
        instance.Bounds = BoundingSphere::Empty;
        if (!type->IsReady())
            continue;
        auto& meshes = type->Model->LODs[0].Meshes;
        for (int32 j = 0; j < meshes.Count(); j++)
        {
            meshes[j].GetCorners(corners);

            for (int32 k = 0; k < 8; k++)
            {
                Vector3::Transform(corners[k], instance.World, corners[k]);
            }
            BoundingSphere meshBounds;
            BoundingSphere::FromPoints(corners, 8, meshBounds);

            BoundingSphere::Merge(instance.Bounds, meshBounds, instance.Bounds);
        }
    }

    RebuildClusters();
}
