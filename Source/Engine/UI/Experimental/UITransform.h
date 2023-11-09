#pragma once
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Serialization/Serialization.h"
/// <summary>
/// Transform
/// </summary>
API_CLASS()
class FLAXENGINE_API UITransform : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UITransform);

private:
    Matrix3x3 _cachedTransform;
    Matrix3x3 _cachedTransformInv;
public:
    /// <summary>
    /// location (coordinates of the upper-left corner)
    /// </summary>
    API_FIELD()
        Float2 Location;
    /// <summary>
    /// size
    /// </summary>
    API_FIELD()
        Float2 Size;
    /// <summary>
    /// Scale
    /// </summary>
    API_FIELD()
        Float2 Scale = Float2::One;
    /// <summary>
    /// Povit
    /// </summary>
    API_FIELD()
        Float2 Povit = Float2::One * 0.5f;
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
    API_FUNCTION() void UpdateTransformCache();

    /// <summary>
    /// Check if transform is overlapping a point
    /// </summary>
    /// <param name="point">relative to object</param>
    /// <returns></returns>
    API_FUNCTION() bool Ovelaps(Float2 point);
    /// <summary>
    /// Draws a rectangle borders.
    /// </summary>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to use.</param>
    API_FUNCTION() void DrawBorder(const Color& color, float thickness = 1.0f);

};


namespace Serialization
{
    inline bool ShouldSerialize(const UITransform& v, const void* otherObj)
    {
        auto other = static_cast<const UITransform*>(otherObj);
        // This can detect if value is the same as other object (if not null) and skip serialization
        return true;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const UITransform& v, const void* otherObj)
    {
        SERIALIZE_GET_OTHER_OBJ(UITransform);
        auto Location =v.Location;
        auto Size     =v.Size    ;
        auto Scale    =v.Scale   ;
        auto Povit    =v.Povit   ;
        auto Shear    =v.Shear   ;
        auto Rotation =v.Rotation;
        SERIALIZE_MEMBER(Location, Location);
        SERIALIZE_MEMBER(Size,     Size    );
        SERIALIZE_MEMBER(Scale,    Scale   );
        SERIALIZE_MEMBER(Povit,    Povit   );
        SERIALIZE_MEMBER(Shear,    Shear   );
        SERIALIZE_MEMBER(Rotation, Rotation);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, UITransform& v, ISerializeModifier* modifier)
    {
        // Populate data with values from stream
        DESERIALIZE_MEMBER(Location,v.Location);
        DESERIALIZE_MEMBER(Size    ,v.Size    );
        DESERIALIZE_MEMBER(Scale   ,v.Scale   );
        DESERIALIZE_MEMBER(Povit   ,v.Povit   );
        DESERIALIZE_MEMBER(Shear   ,v.Shear   );
        DESERIALIZE_MEMBER(Rotation,v.Rotation);
        v.UpdateTransformCache();
    }
}
