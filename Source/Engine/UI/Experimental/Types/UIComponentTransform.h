//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/ISerializable.h"

/// <summary>
/// Describes the standard transformation of a UIComponent
/// </summary>
API_STRUCT(Namespace = "FlaxEngine.Experimental.UI")
struct FLAXENGINE_API UIComponentTransform
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(UIComponentTransform);
private:
    Matrix3x3 CachedTransform = Matrix3x3::Identity;
    Matrix3x3 CachedTransformInv = Matrix3x3::Identity;
public:
    /// <summary>
    /// the Rectangle Constans Translation and Size
    /// </summary>
    API_FIELD() Rectangle Rect;

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
    /// <summary>
    /// Initializes a new instance of the <see cref="UIComponentTransform"/> struct.
    /// </summary>
    UIComponentTransform()
        : Rect(Rectangle::Empty)
        , Shear(Vector2::Zero)
        , Pivot(Vector2::One * 0.5f)
        , Angle(0){}    
    /// <summary>
    /// Initializes a new instance of the <see cref="UIComponentTransform"/> struct.
    /// </summary>
    /// <param name="InRectangle">The in rectangle.</param>
    /// <param name="InShear">The in shear.</param>
    /// <param name="InPivot">The in pivot.</param>
    /// <param name="InAngle">The in angle.</param>
    UIComponentTransform(const Rectangle& InRectangle, const Vector2& InShear, const Vector2& InPivot, float InAngle)
        : Rect(InRectangle)
        , Shear(InShear)
        , Pivot(InPivot)
        , Angle(InAngle){}

    /// <summary>
    /// Determines whether this instance is identity.
    /// </summary>
    /// <returns>
    ///   <c>true</c> if this instance is identity; otherwise, <c>false</c>.
    /// </returns>
    bool IsIdentity() const
    {
        const static UIComponentTransform Identity;

        return Identity == *this;
    }
    FORCE_INLINE bool operator==(const UIComponentTransform& Other) const
    {
        return Rect == Other.Rect && Shear == Other.Shear && Pivot == Other.Pivot && Angle == Other.Angle;
    }

    FORCE_INLINE bool operator!=(const UIComponentTransform& Other) const
    {
        return !(*this == Other);
    }


    void UpdateTransform();
};
