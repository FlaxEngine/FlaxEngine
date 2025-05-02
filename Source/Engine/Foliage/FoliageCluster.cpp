// Copyright (c) Wojciech Figat. All rights reserved.

#include "FoliageCluster.h"
#include "FoliageInstance.h"
#include "Foliage.h"

void FoliageCluster::Init(const BoundingBox& bounds)
{
    Bounds = bounds;
    TotalBounds = bounds;
    MaxCullDistance = 0.0f;

    Children[0] = nullptr;
    Children[1] = nullptr;
    Children[2] = nullptr;
    Children[3] = nullptr;

    Instances.Clear();
}

void FoliageCluster::UpdateTotalBoundsAndCullDistance()
{
    if (Children[0])
    {
        ASSERT(Instances.IsEmpty());

        Children[0]->UpdateTotalBoundsAndCullDistance();
        Children[1]->UpdateTotalBoundsAndCullDistance();
        Children[2]->UpdateTotalBoundsAndCullDistance();
        Children[3]->UpdateTotalBoundsAndCullDistance();

        TotalBounds = Children[0]->TotalBounds;
        BoundingBox::Merge(TotalBounds, Children[1]->TotalBounds, TotalBounds);
        BoundingBox::Merge(TotalBounds, Children[2]->TotalBounds, TotalBounds);
        BoundingBox::Merge(TotalBounds, Children[3]->TotalBounds, TotalBounds);

        MaxCullDistance = Children[0]->MaxCullDistance;
        MaxCullDistance = Math::Max(MaxCullDistance, Children[1]->MaxCullDistance);
        MaxCullDistance = Math::Max(MaxCullDistance, Children[2]->MaxCullDistance);
        MaxCullDistance = Math::Max(MaxCullDistance, Children[3]->MaxCullDistance);
    }
    else if (Instances.HasItems())
    {
        BoundingBox box;
        BoundingBox::FromSphere(Instances[0]->Bounds, TotalBounds);
        MaxCullDistance = Instances[0]->CullDistance;
        for (int32 i = 1; i < Instances.Count(); i++)
        {
            BoundingBox::FromSphere(Instances[i]->Bounds, box);
            BoundingBox::Merge(TotalBounds, box, TotalBounds);
            MaxCullDistance = Math::Max(MaxCullDistance, Instances[i]->CullDistance);
        }
    }
    else
    {
        TotalBounds = Bounds;
        MaxCullDistance = 0;
    }

    BoundingSphere::FromBox(TotalBounds, TotalBoundsSphere);
}

void FoliageCluster::UpdateCullDistance()
{
    if (Children[0])
    {
        Children[0]->UpdateCullDistance();
        Children[1]->UpdateCullDistance();
        Children[2]->UpdateCullDistance();
        Children[3]->UpdateCullDistance();

        MaxCullDistance = Children[0]->MaxCullDistance;
        MaxCullDistance = Math::Max(MaxCullDistance, Children[1]->MaxCullDistance);
        MaxCullDistance = Math::Max(MaxCullDistance, Children[2]->MaxCullDistance);
        MaxCullDistance = Math::Max(MaxCullDistance, Children[3]->MaxCullDistance);
    }
    else if (Instances.HasItems())
    {
        MaxCullDistance = Instances[0]->CullDistance;
        for (int32 i = 1; i < Instances.Count(); i++)
        {
            MaxCullDistance = Math::Max(MaxCullDistance, Instances[i]->CullDistance);
        }
    }
    else
    {
        MaxCullDistance = 0;
    }
}

bool FoliageCluster::Intersects(Foliage* foliage, const Ray& ray, Real& distance, Vector3& normal, FoliageInstance*& instance)
{
    bool result = false;
    Real minDistance = MAX_Real;
    Vector3 minDistanceNormal = Vector3::Up;
    FoliageInstance* minInstance = nullptr;

    if (Children[0])
    {
#define CHECK_CHILD(idx) \
		if (Children[idx]->TotalBounds.Intersects(ray) && Children[idx]->Intersects(foliage, ray, distance, normal, instance) && minDistance > distance) \
		{ \
			minDistanceNormal = normal; \
			minDistance = distance; \
			minInstance = instance; \
			result = true; \
		}
        CHECK_CHILD(0);
        CHECK_CHILD(1);
        CHECK_CHILD(2);
        CHECK_CHILD(3);
#undef CHECK_CHILD
    }
    else
    {
        Mesh* mesh;
        for (int32 i = 0; i < Instances.Count(); i++)
        {
            auto& ii = *Instances[i];
            auto& type = foliage->FoliageTypes[ii.Type];
            const Transform transform = foliage->GetTransform().LocalToWorld(ii.Transform);
            if (type.IsReady() && ii.Bounds.Intersects(ray) && type.Model->Intersects(ray, transform, distance, normal, &mesh) && minDistance > distance)
            {
                minDistanceNormal = normal;
                minDistance = distance;
                minInstance = &ii;
                result = true;
            }
        }
    }

    distance = minDistance;
    normal = minDistanceNormal;
    instance = minInstance;
    return result;
}
