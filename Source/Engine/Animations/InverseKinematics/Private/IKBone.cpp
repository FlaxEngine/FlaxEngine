//writen by https://github.com/NoriteSC

#include "Engine/Animations/InverseKinematics/IKBone.h"
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Color.h"

IKBone::IKBone(const SpawnParams& params) : ScriptingObject(params)
{

}
void IKBone::SnapTaillTo(const Vector3& newLocation)
{
    Vector3 delta = newLocation - Head;
    Head += delta;
    Taill += delta;
}

void IKBone::SnapHeadTo(const Vector3& newLocation)
{
    Vector3 delta = newLocation - Taill;
    Head += delta;
    Taill += delta;
}
void IKBone::SetOrientation(const Quaternion& Oritentacion)
{
    this->Oritentacion = Oritentacion * Quaternion::Invert(ParentOritentacion);
    Vector3 e = this->Oritentacion.GetEuler();
    if (X.Active)
        e.X = Math::Clamp(e.X, X.Min, X.Max);
    if (Y.Active)
        e.Y = Math::Clamp(e.Y, Y.Min, Y.Max);
    if (Z.Active)
        e.Z = Math::Clamp(e.Z, Z.Min, Z.Max);

    Roll = e.Z;
    this->Oritentacion = Quaternion::Euler(e) * ParentOritentacion;
    Taill = ((this->Oritentacion * Vector3::Forward) * Lenght) + Head;
}
void IKBone::SetOrientationUnconstrained(const Quaternion& Oritentacion)
{
    Taill = ((Oritentacion * Vector3::Forward) * Lenght) + Head;
}
Vector3 IKBone::GetDirection()
{
    return Taill - Head;
}

void IKBone::Draw()
{
#if USE_EDITOR
    auto v = Lenght * 0.075f;
    auto pe = ParentOritentacion.GetEuler();
    //var e = Oritentacion.EulerAngles;
    float a;
    if (X.Active)
    {
        a = (X.Max - X.Min) * DegreesToRadians;
        Quaternion xq = Quaternion::Euler(pe.X, pe.Y, pe.Z) * Quaternion::Euler(0, -90, X.Min);
        DebugDraw::DrawArc(Head, xq, v, a, Color::Red.RGBMultiplied(0.8f).AlphaMultiplied(0.1f));
        DebugDraw::DrawWireArc(Head, xq, v, a, Color::Red);
    }
    if (Y.Active)
    {
        a = (Y.Max - Y.Min) * DegreesToRadians;
        auto yq = Quaternion::Euler(pe.X, pe.Y, pe.Z) * Quaternion::Euler(90, -90, Y.Min);
        DebugDraw::DrawArc(Head, yq, v, a, Color::Green.RGBMultiplied(0.8f).AlphaMultiplied(0.1f));
        DebugDraw::DrawWireArc(Head, yq, v, a, Color::Green);
    }
    if (Z.Active)
    {
        a = (Z.Max - Z.Min) * DegreesToRadians;
        auto zq = Quaternion::Euler(pe.X, pe.Y, pe.Z) * Quaternion::Euler(0, 0, 90 + Z.Min);
        DebugDraw::DrawArc(Head, zq, v, a, Color::Blue.RGBMultiplied(0.8f).AlphaMultiplied(0.1f));
        DebugDraw::DrawWireArc(Head, zq, v, a, Color::Blue);
    }
    DrawOctahedralBone(Head, Taill, Roll, DebugColor);
#endif
}


void IKBone::DrawOctahedralBone(const Vector3& Head, const Vector3& Taill, float Roll,const Color& Color)
{
#if USE_EDITOR
    auto distance = Vector3::Distance(Head, Taill);
    auto v = distance * 0.05f;
    DebugDraw::DrawWireSphere(BoundingSphere(Head, v), Color);
    DebugDraw::DrawWireSphere(BoundingSphere(Taill, v), Color);
    Vector3 fwd = (Taill - Head).GetNormalized();
    Quaternion q;
    Quaternion::RotationAxis(Vector3::Forward, Roll * DegreesToRadians, q);
    q * Quaternion::FromDirection(fwd);
    const float BoneSize = 0.1f;
    Vector3 p1 = (q * Vector3(BoneSize, BoneSize, BoneSize) * distance) + Head;
    Vector3 p2 = (q * Vector3(-BoneSize, BoneSize, BoneSize) * distance) + Head;
    Vector3 p3 = (q * Vector3(BoneSize, -BoneSize, BoneSize) * distance) + Head;
    Vector3 p4 = (q * Vector3(-BoneSize, -BoneSize, BoneSize) * distance) + Head;

    //Draw bottom
    DebugDraw::DrawWireTriangle(p1, p2, Taill, Color);
    DebugDraw::DrawWireTriangle(p2, p4, Taill, Color);
    DebugDraw::DrawWireTriangle(p3, p4, Taill, Color);
    DebugDraw::DrawWireTriangle(p1, p3, Taill, Color);

    //Draw top
    DebugDraw::DrawWireTriangle(p1, p2, Head, Color);
    DebugDraw::DrawWireTriangle(p2, p4, Head, Color);
    DebugDraw::DrawWireTriangle(p3, p4, Head, Color);
    DebugDraw::DrawWireTriangle(p1, p3, Head, Color);
#endif
}
