#include "IBrush.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Render2D/Render2D.h"

ImageBrush::ImageBrush(const SpawnParams& params) : ScriptingObject(params)
{

}

void ImageBrush::OnDraw(const Float2& At)
{
    auto texture = Image.Get();
    if (texture) {
        Render2D::DrawTexture(texture, Rectangle(At, GetDesiredSize()), Color::White);
    }
    else 
    {
        // if texture is null call default IBrush::OnDraw to render FillRectangle
        IBrush::OnDraw(At);
    }
}
void IBrush::OnDraw(const Float2& At)
{
    Render2D::FillRectangle(Rectangle(At, GetDesiredSize()),Color::White);
}
