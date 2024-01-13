// Writen by Nori_SC

#include "Engine/Render2D/Render2D.h"
#include "Engine/Core/Math/Rectangle.h"
//#include "Engine/UI/Experimental/Types/UIRenderTransform.h"
//#include "Engine/UI/Experimental/Types/ISlot.h"
//
//UIRenderTransform::UIRenderTransform(const SpawnParams& params) : ScriptingObject(params)
//{
//    Scale = Float2::One;
//    Shear = Float2::Zero;
//    Rotation = 0;
//}
//
// void UIRenderTransform::UpdateTransformCache(Float2 Location, Float2 Size,Float2 Povit)
//{
//    // Actual pivot and negative pivot
//    auto v1 = Povit * Size;
//    auto v2 = -v1;
//    v1 += Location;
//
//    //shear and scale
//    Matrix3x3 m1 = Matrix3x3
//    (
//    Scale.X,
//    Scale.X * (Shear.X == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Shear.X, -89.0f, 89.0f))))),
//    0,
//    Scale.Y * (Shear.Y == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Shear.Y, -89.0f, 89.0f))))),
//    Scale.Y,
//    0, 0, 0, 1
//    );
//    //rotate ,offset ,mix
//    float sin = Math::Sin(DegreesToRadians * Rotation);
//    float cos = Math::Cos(DegreesToRadians * Rotation);
//    m1.M11 = (Scale.X * cos) + (m1.M12 * -sin);
//    m1.M12 = (Scale.X * sin) + (m1.M12 * cos);
//    float m21 = (m1.M21 * cos) + (Scale.Y * -sin);
//    m1.M22 = (m1.M21 * sin) + (Scale.Y * cos);
//    m1.M21 = m21;
//    m1.M31 = (v2.X * m1.M11) + (v2.Y * m1.M21) + v1.X;
//    m1.M32 = (v2.X * m1.M12) + (v2.Y * m1.M22) + v1.Y;
//    _cachedTransform = m1;
//
//    // Cache inverted transform
//    _cachedTransformInv = Matrix3x3::Invert(_cachedTransform);
//}
//
// /// <summary>
// /// Check if transform is overlapping a point
// /// </summary>
// /// <param name="point">relative to object</param>
// /// <returns></returns>
//
// bool UIRenderTransform::Ovelaps(ISlotMinimal* slot, Float2 point)
// {
//     //transform point and include Transformation,Scale,Povit,Shear
//     _cachedTransform.Transform2DPoint(point - slot->GetDesiredLocation(), _cachedTransform, point);
//     return true;//Transformation.Contains(point);
// }
//
// void UIRenderTransform::DrawBorder(ISlotMinimal* slot, const Color& color, float thickness)
// {
//     Render2D::PushTransform(_cachedTransform);
//     Render2D::DrawRectangle(Rectangle(Float2::Zero, slot->GetDesiredSize()), color, thickness);
//     Render2D::PopTransform();
// }
