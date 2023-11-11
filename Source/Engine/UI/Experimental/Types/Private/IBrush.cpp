#include "Engine/UI/Experimental/Types/IBrush.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Render2D/Render2D.h"

void IBrush::OnPreCunstruct(bool isInDesigner) {}
void IBrush::OnCunstruct() {}
void IBrush::OnDraw(const Float2& At)
{
    Render2D::FillRectangle(Rectangle(At, GetDesiredSize()), Color::White);
}
void IBrush::OnDestruct() {}
Float2 IBrush::GetDesiredSize() 
{
    return Owner->GetSlot()->GetSize(); 
}
