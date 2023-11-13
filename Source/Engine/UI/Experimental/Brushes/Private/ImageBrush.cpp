// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/UI/Experimental/Brushes/ImageBrush.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Render2D/Render2D.h"

ImageBrush::ImageBrush(const SpawnParams& params) : ScriptingObject(params)
{

}

void ImageBrush::OnPreCunstruct(bool isInDesigner)
{
    if (Image && !Image->WaitForLoaded())
    {
        OnImageAssetChanged();
    }
};

void ImageBrush::OnCunstruct()
{
    Image.Changed.Bind<ImageBrush, &ImageBrush::OnImageAssetChanged>(this);
};
void ImageBrush::OnDraw(const Float2& At)
{
    auto texture = Image.Get();
    if (texture) {
        Render2D::DrawTexture(texture, Rectangle(At,Owner->GetSlot()->GetDesiredSize()), Color::White);
    }
    else
    {
        // if texture is null call default IBrush::OnDraw to render FillRectangle
        IBrush::OnDraw(At);
    }
}
void ImageBrush::OnDestruct()
{
    Image.Changed.Unbind<ImageBrush, &ImageBrush::OnImageAssetChanged>(this);
};

void ImageBrush::OnImageAssetChanged()
{
    ImageSize = Image.Get()->Size();
}
