#include "Engine/UI/Experimental/Special Effects/Background Blur.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Render2D/Render2D.h"

BackgroundBlur::BackgroundBlur(const SpawnParams& params) : UIElement(params) 
{
    BlurStrength = 1.0f;
    BlurScaleWithSize = false;
}

void BackgroundBlur::OnDraw()
{
    auto size = GetSlot()->GetSize();
    auto strength = BlurStrength;
    if (BlurScaleWithSize)
        strength *= size.MinValue() / 1000.0f;
    if (strength > 0.00001f)
    {
        Render2D::DrawBlur(Rectangle(Float2::Zero, size), strength);
    }
}
