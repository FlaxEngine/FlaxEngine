//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../UIComponentTransform.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"

void UIComponentTransform::UpdateTransform()
{
    // Actual pivot and negative pivot
    auto v1 = Pivot;
    auto v2 = -v1;
    v1 += Rect.Location;

    // Size and Shear
    Matrix3x3 m1 = Matrix3x3
    (
        Rect.Size.X,
        Rect.Size.X * (Shear.Y == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Shear.Y, -89.0f, 89.0f))))),
        0,
        Rect.Size.Y * (Shear.X == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Shear.X, -89.0f, 89.0f))))),
        Rect.Size.Y,
        0, 0, 0, 1
    );
    //rotate and mix
    float sin = Math::Sin(DegreesToRadians * Angle);
    float cos = Math::Cos(DegreesToRadians * Angle);
    m1.M11 = (Rect.Size.X * cos) + (m1.M12 * -sin);
    m1.M12 = (Rect.Size.X * sin) + (m1.M12 * cos);
    float m21 = (m1.M21 * cos) + (Rect.Size.Y * -sin);
    m1.M22 = (m1.M21 * sin) + (Rect.Size.Y * cos);
    m1.M21 = m21;

    // Mix all the stuff
    m1.M31 = (v2.X * m1.M11) + (v2.Y * m1.M21) + v1.X;
    m1.M32 = (v2.X * m1.M12) + (v2.Y * m1.M22) + v1.Y;
    CachedTransform = m1;

    // Cache inverted transform
    Matrix3x3::Invert(CachedTransform, CachedTransformInv);
}
