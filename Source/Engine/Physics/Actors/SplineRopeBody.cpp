// Copyright (c) Wojciech Figat. All rights reserved.

#include "SplineRopeBody.h"
#include "Engine/Level/Actors/Spline.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"

SplineRopeBody::SplineRopeBody(const SpawnParams& params)
    : Actor(params)
{
}

void SplineRopeBody::Tick()
{
    if (!_spline || _spline->GetSplinePointsCount() < 2)
        return;
    PROFILE_CPU();
    PROFILE_MEM(Physics);

    // Cache data
    const Vector3 gravity = GetPhysicsScene()->GetGravity() * GravityScale;
    auto& keyframes = _spline->Curve.GetKeyframes();
    const Transform splineTransform = _spline->GetTransform();
    const int32 keyframesCount = keyframes.Count();
    const float substepTime = SubstepTime;
    // TODO: scale substep time based on distance to the camera to have better performance when rope is far away
    bool splineDirty = false;

    // Synchronize spline keyframes with simulated masses
    if (_masses.Count() > keyframesCount)
        _masses.Resize(keyframesCount);
    else
    {
        _masses.EnsureCapacity(keyframesCount);
        while (_masses.Count() < keyframesCount)
        {
            const int32 i = _masses.Count();
            auto& mass = _masses.AddOne();
            mass.Position = mass.PrevPosition = splineTransform.LocalToWorld(keyframes[i].Value.Translation);
            if (i != 0)
                mass.SegmentLength = Vector3::Distance(mass.PrevPosition, _masses[i - 1].PrevPosition);
            else
                mass.SegmentLength = 0.0f;
        }
    }
    {
        // Rope start
        auto& mass = _masses.First();
        mass.Position = GetPosition();
        mass.Unconstrained = false;
        if (splineTransform.LocalToWorld(keyframes.First().Value.Translation) != mass.Position)
            splineDirty = true;
    }
    bool syncPositions = true;
    if (splineTransform != _splineTransform)
    {
        _splineTransform = splineTransform;
        syncPositions = false;
        splineDirty = true;
    }
    if (syncPositions)
    {
        for (int32 i = 1; i < keyframesCount; i++)
        {
            auto& mass = _masses[i];
            mass.Unconstrained = true;
            mass.Position = splineTransform.LocalToWorld(keyframes[i].Value.Translation);
        }
    }
    if (AttachEnd)
    {
        // Rope end
        auto& mass = _masses.Last();
        mass.Position = AttachEnd->GetPosition();
        mass.Unconstrained = false;
        if (splineTransform.LocalToWorld(keyframes.Last().Value.Translation) != mass.Position)
            splineDirty = true;
    }

    // Perform simulation in substeps to have better stability
    _time += Time::Physics.DeltaTime.GetTotalSeconds();
    float stretchLimit = StretchLimit + 1;
    while (_time > substepTime)
    {
        // Verlet integration
        const Vector3 force = (gravity + AdditionalForce) * (substepTime * substepTime);
        for (int32 i = 0; i < keyframesCount; i++)
        {
            auto& mass = _masses[i];
            Vector3 position = mass.Position;
            if (mass.Unconstrained)
            {
                const Vector3 velocity = (mass.Position - mass.PrevPosition) * (1 - Drag);
                mass.Position += velocity + force;
            }
            mass.PrevPosition = position;
        }

        // Distance constraint
        for (int32 i = 1; i < keyframesCount; i++)
        {
            auto& massA = _masses[i - 1];
            auto& massB = _masses[i];
            Vector3 offset = massB.Position - massA.Position;
            const Real distance = offset.Length();
            const Real minDistance = massB.SegmentLength;
            const Real maxDistance = massB.SegmentLength * stretchLimit;
            if ((distance <= maxDistance && distance >= minDistance) || distance <= ZeroTolerance)
                continue;
            const Real scale = (distance - maxDistance) / distance;
            if (massA.Unconstrained && massB.Unconstrained)
            {
                offset *= scale * 0.5f;
                massA.Position += offset;
                massB.Position -= offset;
            }
            else if (massA.Unconstrained)
            {
                massA.Position += scale * offset;
            }
            else if (massB.Unconstrained)
            {
                massB.Position -= scale * offset;
            }
        }

        // Stiffness constraint
        if (EnableStiffness)
        {
            for (int32 i = 2; i < keyframesCount; i++)
            {
                auto& massA = _masses[i - 2];
                auto& massB = _masses[i];
                Vector3 offset = massB.Position - massA.Position;
                const Real distance = offset.Length();
                const Real maxDistance = massB.SegmentLength * stretchLimit * 2.0f;
                if (distance <= ZeroTolerance)
                    continue;
                const Real scale = (distance - maxDistance) / distance;
                if (massA.Unconstrained && massB.Unconstrained)
                {
                    offset *= scale * 0.5f;
                    massA.Position += offset;
                    massB.Position -= offset;
                }
                else if (massA.Unconstrained)
                {
                    massA.Position += scale * offset;
                }
                else if (massB.Unconstrained)
                {
                    massB.Position -= scale * offset;
                }
            }
        }

        _time -= substepTime;
        splineDirty = true;
    }

    // Update spline and relevant components (eg. spline model)
    if (splineDirty)
    {
        for (int32 i = 0; i < keyframesCount; i++)
            keyframes[i].Value.Translation = splineTransform.WorldToLocal(_masses[i].Position);

        _spline->UpdateSpline();
    }
}

void SplineRopeBody::OnEnable()
{
    GetScene()->Ticking.FixedUpdate.AddTick<SplineRopeBody, &SplineRopeBody::Tick>(this);

    Actor::OnEnable();
}

void SplineRopeBody::OnDisable()
{
    Actor::OnDisable();

    GetScene()->Ticking.FixedUpdate.RemoveTick(this);
}

void SplineRopeBody::OnParentChanged()
{
    Actor::OnParentChanged();

    _spline = Cast<Spline>(_parent);
    _splineTransform = _spline ? _spline->GetTransform() : Transform::Identity;
}

void SplineRopeBody::OnTransformChanged()
{
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
