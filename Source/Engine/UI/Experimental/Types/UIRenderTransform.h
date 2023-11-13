#pragma once
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Serialization/Serialization.h"

/// <summary>
/// 2D Transform Component spectfic to UI
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API UIRenderTransform : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UIRenderTransform);

private:
    Matrix3x3 _cachedTransform;
    Matrix3x3 _cachedTransformInv;
public:
    /// <summary>
    /// location (coordinates of the upper-left corner)
    /// </summary>
    API_FIELD()
        Float2 Transformation;

    /// <summary>
    /// Shear
    /// </summary>
    API_FIELD()
        Float2 Scale;

    /// <summary>
    /// Shear
    /// </summary>
    API_FIELD()
        Float2 Shear;
    /// <summary>
    /// Rotation
    /// </summary>
    API_FIELD()
        float Rotation;

    /// <summary>
    /// Update Transform Matrix3x3 call it if any component is changed
    /// </summary>
    API_FUNCTION() void UpdateTransformCache(Float2 Location, Float2 Size, Float2 Povit);

    /// <summary>
    /// Check if transform is overlapping a point
    /// </summary>
    /// <param name="point">relative to object</param>
    /// <returns></returns>
    API_FUNCTION() bool Ovelaps(class ISlotMinimal* slot,Float2 point);
    /// <summary>
    /// Draws a rectangle borders.
    /// </summary>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to use.</param>
    API_FUNCTION() void DrawBorder(class ISlotMinimal* element, const Color& color, float thickness = 1.0f);

};


namespace Serialization
{
    inline bool ShouldSerialize(const UIRenderTransform& v, const void* otherObj)
    {
        auto other = static_cast<const UIRenderTransform*>(otherObj);
        // This can detect if value is the same as other object (if not null) and skip serialization
        return true;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const UIRenderTransform& v, const void* otherObj)
    {
        SERIALIZE_GET_OTHER_OBJ(UIRenderTransform);
        auto Scale    =v.Scale   ;
        auto Shear    =v.Shear   ;
        auto Rotation =v.Rotation;
        SERIALIZE_MEMBER(Scale,    Scale   );
        SERIALIZE_MEMBER(Shear,    Shear   );
        SERIALIZE_MEMBER(Rotation, Rotation);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, UIRenderTransform& v, ISerializeModifier* modifier)
    {
        // Populate data with values from stream
        DESERIALIZE_MEMBER(Scale   ,v.Scale   );
        DESERIALIZE_MEMBER(Shear   ,v.Shear   );
        DESERIALIZE_MEMBER(Rotation,v.Rotation);
    }
}
