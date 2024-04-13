// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"

/// <summary>
/// Describes transformation in a 3D space.
/// </summary>
API_STRUCT() struct FLAXENGINE_API PhysicsTransform
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(PhysicsTransform);

    /// <summary>
    /// The translation vector of the transform.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(10), EditorDisplay(null, \"Position\"), ValueCategory(Utils.ValueCategory.Distance)")
        Vector3 Translation;

    /// <summary>
    /// The rotation of the transform.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(20), EditorDisplay(null, \"Rotation\"), ValueCategory(Utils.ValueCategory.Angle)")
        Quaternion Orientation;
#pragma region Constructors
    PhysicsTransform() :
        Translation(Vector3::Zero),
        Orientation(Quaternion::Identity)
    {}
    PhysicsTransform(const Vector3& InTranslation) : 
        Translation(InTranslation),
        Orientation(Quaternion::Identity)
    {}
    PhysicsTransform(const Vector3& InTranslation, Quaternion& InOrientation) : 
        Translation(InTranslation),
        Orientation(InOrientation)
    {}
    PhysicsTransform(const Transform& InTransform) : 
        Translation(InTransform.Translation),
        Orientation(InTransform.Orientation)
    {}
#pragma endregion
    static PhysicsTransform WorldToLocal(const PhysicsTransform& InWorld, const PhysicsTransform& InOtherWorld)
    {
        auto& inv = InWorld.Orientation.Conjugated();
        auto& T = inv * (InOtherWorld.Translation - InWorld.Translation);
        auto& Q = inv * InOtherWorld.Orientation;
        Q.Normalize();
        return PhysicsTransform{ T,Q };
    }

    static PhysicsTransform LocalToWorld(const PhysicsTransform& InWorld,const PhysicsTransform& InLocal)
    {
        auto& T = (InWorld.Orientation * InLocal.Translation) + InWorld.Translation;
        auto& Q = InWorld.Orientation * InLocal.Orientation;
        return PhysicsTransform{ T,Q };
    }

    Transform ToTransform(const Float3& InScale)
    {
        return Transform(Translation, Orientation, InScale);
    }
};
template<>
struct TIsPODType<PhysicsTransform>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(PhysicsTransform, "Translation:{0} Orientation:{1}", v.Translation, v.Orientation);
