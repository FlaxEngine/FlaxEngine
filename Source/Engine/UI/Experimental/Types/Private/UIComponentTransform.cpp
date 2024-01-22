//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../UIComponentTransform.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"

void UIComponentTransform::UpdateTransform()
{

    auto v1 = Pivot * Rect.Size;
    auto v2 = -v1;
    v1 += Rect.Location;

    // Scale and Shear
    Matrix3x3 m1 = Matrix3x3
    (
        1,(Shear.Y == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Shear.Y, -89.0f, 89.0f))))),0,
        (Shear.X == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Shear.X, -89.0f, 89.0f))))),1,0,
        0, 0, 1
    );

    float sin = Math::Sin(DegreesToRadians * Angle);
    float cos = Math::Cos(DegreesToRadians * Angle);

    m1.M11 = cos + (m1.M12 * -sin);
    m1.M12 = sin + (m1.M12 * cos);
    m1.M22 = (m1.M21 * sin) + cos;
    m1.M21 = (m1.M21 * cos) + sin;
    // Mix all the stuff
    m1.M31 = (v2.X * m1.M11) + (v2.Y * m1.M21) + v1.X;
    m1.M32 = (v2.X * m1.M12) + (v2.Y * m1.M22) + v1.Y;
    CachedTransformInv = m1;

    // Cache inverted transform
    Matrix3x3::Invert(CachedTransform, CachedTransformInv);
}
