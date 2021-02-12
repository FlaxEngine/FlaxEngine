// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SplineRopeBody.h"
#include "Engine/Level/Actors/Spline.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"

SplineRopeBody::SplineRopeBody(const SpawnParams& params)
    : Actor(params)
{
}

void SplineRopeBody::Tick()
{
    if (!_spline || _spline->GetSplinePointsCount() < 2)
        return;
    PROFILE_CPU();

    // Cache data
    const Vector3 gravity = Physics::GetGravity() * GravityScale;
    auto& keyframes = _spline->Curve.GetKeyframes();
    const Transform splineTransform = _spline->GetTransform();
    const int32 keyframesCount = keyframes.Count();
    const float substepTime = SubstepTime;
    const float substepTimeSqr = substepTime * substepTime;
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
            mass.PrevPosition = splineTransform.LocalToWorld(keyframes[i].Value.Translation);
            if (i != 0)
                mass.SegmentLength = Vector3::Distance(mass.PrevPosition, _masses[i - 1].PrevPosition);
            else
                mass.SegmentLength = 0.0f;
        }
    }
    {
        // Rope start
        auto& mass = _masses.First();
        mass.Position = mass.PrevPosition = GetPosition();
        mass.Unconstrained = false;
        if (splineTransform.LocalToWorld(keyframes.First().Value.Translation) != mass.Position)
            splineDirty = true;
    }
    for (int32 i = 1; i < keyframesCount; i++)
    {
        auto& mass = _masses[i];
        mass.Unconstrained = true;
        mass.Position = splineTransform.LocalToWorld(keyframes[i].Value.Translation);
    }
    if (AttachEnd)
    {
        // Rope end
        auto& mass = _masses.Last();
        mass.Position = mass.PrevPosition = AttachEnd->GetPosition();
        mass.Unconstrained = false;
        if (splineTransform.LocalToWorld(keyframes.Last().Value.Translation) != mass.Position)
            splineDirty = true;
    }

    // Perform simulation in substeps to have better stability
    _time += Time::Update.DeltaTime.GetTotalSeconds();
    while (_time > substepTime)
    {
        // Verlet integration
        // [Reference: https://en.wikipedia.org/wiki/Verlet_integration]
        const Vector3 force = gravity + AdditionalForce;
        for (int32 i = 0; i < keyframesCount; i++)
        {
            auto& mass = _masses[i];
            if (mass.Unconstrained)
            {
                const Vector3 velocity = mass.Position - mass.PrevPosition;
                mass.PrevPosition = mass.Position;
                mass.Position = mass.Position + velocity + (substepTimeSqr * force);
                keyframes[i].Value.Translation = splineTransform.WorldToLocal(mass.Position);
            }
        }

        // Distance constraint
        for (int32 i = 1; i < keyframesCount; i++)
        {
            auto& massA = _masses[i - 1];
            auto& massB = _masses[i];
            Vector3 offset = massB.Position - massA.Position;
            const float distance = offset.Length();
            const float scale = (distance - massB.SegmentLength) / Math::Max(distance, ZeroTolerance);
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
                const float distance = offset.Length();
                const float scale = (distance - massB.SegmentLength * 2.0f) / Math::Max(distance, ZeroTolerance);
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
}

void SplineRopeBody::OnTransformChanged()
{
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
