// Writen by Nori_SC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"

/// <summary>
/// Describes the standard transformation of a UIComponent
/// </summary>
API_STRUCT(Namespace = "FlaxEngine.Experimental.UI")
struct FLAXENGINE_API UIComponentTransform
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(UIComponentTransform);
public:

    /// <summary>
    /// The amount to translate the UIComponent
    /// </summary>
    API_FIELD() Vector2 Translation;

    /// <summary>
    /// The scale to apply to the UIComponent
    /// </summary>
    API_FIELD() Vector2 Size;

    /// <summary>
    /// The amount to shear the UIComponent
    /// </summary>
    API_FIELD() Vector2 Shear;

    /// <summary>
    /// The angle in degrees to rotate
    /// </summary>
    API_FIELD() float Angle;

    /// <summary>
    /// The render transform pivot controls the location about which transforms are applied.
    /// This value is a normalized coordinate about which things like rotations will occur.
    /// </summary>
    API_FIELD() Vector2 Pivot;
public:
    UIComponentTransform()
        : Translation(Vector2::Zero)
        , Size(Vector2::One)
        , Shear(Vector2::Zero)
        , Pivot(Vector2::One * 0.5f)
        , Angle(0)
    {
    }
    
    UIComponentTransform(const Vector2& InTranslation, const Vector2& InScale, const Vector2& InShear, const Vector2& InPivot, float InAngle)
        : Translation(InTranslation)
        , Size(InScale)
        , Shear(InShear)
        , Pivot(InPivot)
        , Angle(InAngle)
    {
    }

    bool IsIdentity() const
    {
        const static UIComponentTransform Identity;

        return Identity == *this;
    }

    FORCE_INLINE bool operator==(const UIComponentTransform& Other) const
    {
        return Translation == Other.Translation && Size == Other.Size && Shear == Other.Shear && Pivot == Other.Pivot && Angle == Other.Angle;
    }

    FORCE_INLINE bool operator!=(const UIComponentTransform& Other) const
    {
        return !(*this == Other);
    }

};
