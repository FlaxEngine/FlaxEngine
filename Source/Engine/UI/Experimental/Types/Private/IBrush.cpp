// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/UI/Experimental/Types/IBrush.h"
#include "Engine/UI/Experimental/Types/UIElement.h"

#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Render2D/Render2D.h"

void IBrush::OnPreCunstruct(bool isInDesigner) {}
void IBrush::OnCunstruct() {}
void IBrush::OnDraw(const Float2& At)
{
    Render2D::FillRectangle(Rectangle(At, Owner->GetSlot()->GetDesiredSize()), Color::White);
}
void IBrush::OnDestruct() {}
