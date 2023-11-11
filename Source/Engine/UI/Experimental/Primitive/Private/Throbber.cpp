#include "Engine/UI/Experimental/Primitive/Throbber.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Engine/Time.h"

Throbber::Throbber(const SpawnParams& params) : UIElement(params) 
{

}

void Throbber::OnDraw()
{
    auto deltaTime = Time::Draw.DeltaTime;
    if ((States.Count()-1) != NumberOfPieces) 
    {
        States.Resize(NumberOfPieces, true);
    }
    auto size = GetSlot()->GetSize();
    if (EnumHasAllFlags(Animate, AnimateFlags::Horizontal))
        States[CurentElement].X = Math::PingPong(States[CurentElement].X, size.X);
    if (EnumHasAllFlags(Animate, AnimateFlags::Vertical))
        States[CurentElement].Y = Math::PingPong(States[CurentElement].Y, size.Y / NumberOfPieces);
    if (EnumHasAllFlags(Animate, AnimateFlags::Alpha))
        States[CurentElement].Z = Math::PingPong(States[CurentElement].Z, 1.0f);

    Float3 sce = States[CurentElement];
    for (auto i = 0; i < States.Count(); i++)
    {
        Render2D::FillRectangle(Rectangle(Float2(0, i * GetSlot()->GetSize().X), Float2(sce.X, sce.Y)), Color(Color::White).AlphaMultiplied(sce.Z));
    }
    CurentElement++;
}
