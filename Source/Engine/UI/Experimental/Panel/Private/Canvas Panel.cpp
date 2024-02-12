#include "../Canvas Panel.h"
#include "Engine/Serialization/Serialization.h"

UICanvasPanelSlot::UICanvasPanelSlot(const SpawnParams& params) : UIPanelOrderedSlot(params) { ZOrder = 0; }

void UICanvasPanelSlot::Layout(const Rectangle& InNewBounds, const Vector2& InNewPoivt, const Rectangle& InNewParentBounds)
{
    auto ParentBounds = Parent->GetRect();
    Rectangle rect = InNewBounds;

    if (InNewParentBounds != ParentBounds)
    {
        //min(0.5, 0.5) max(0.5, 0.5) is anchor to center
        //min(0, 1)     max(0, 1)     is anchor to top left
        //min(1, 1)     max(1, 1)     is anchor to top right
        //min(1, 0)     max(1, 0)     is anchor to bottom right
        //min(0, 0)     max(0, 0)     is anchor to bottom left
        //min(0, 0)     max(1, 1)     is stretch to top bottom left right
        //min(0, 0.5)   max(1, 0.5)   is stretch to left right and anchor to center
        //min(0.5, 0)   max(0.5, 1)   is stretch to top bottom and anchor to center

        float LastAnhorMinX = Math::Lerp(ParentBounds.GetLeft(), ParentBounds.GetRight(), Anchor.Min.X);
        float LastAnhorMaxX = Math::Lerp(ParentBounds.GetLeft(), ParentBounds.GetRight(), Anchor.Max.X);
        float LastAnhorMinY = Math::Lerp(ParentBounds.GetTop(), ParentBounds.GetBottom(), Anchor.Min.Y);
        float LastAnhorMaxY = Math::Lerp(ParentBounds.GetTop(), ParentBounds.GetBottom(), Anchor.Max.Y);

        float AnhorMinX = Math::Lerp(InNewParentBounds.GetLeft(), InNewParentBounds.GetRight(), Anchor.Min.X);
        float AnhorMaxX = Math::Lerp(InNewParentBounds.GetLeft(), InNewParentBounds.GetRight(), Anchor.Max.X);
        float AnhorMinY = Math::Lerp(InNewParentBounds.GetTop(), InNewParentBounds.GetBottom(), Anchor.Min.Y);
        float AnhorMaxY = Math::Lerp(InNewParentBounds.GetTop(), InNewParentBounds.GetBottom(), Anchor.Max.Y);
        rect = Content->GetRect();
        //auto& CurentRect 
        rect.SetRight(rect.GetRight() + (AnhorMaxX - LastAnhorMaxX));
        rect.SetLeft(rect.GetLeft() + (AnhorMinX - LastAnhorMinX));
        rect.SetBottom(rect.GetBottom() + (AnhorMaxY - LastAnhorMaxY));
        rect.SetTop(rect.GetTop() + (AnhorMinY - LastAnhorMinY));
    }
    if (Content->GetPivot() != InNewPoivt)
    {
        //Recalculate povit
        rect.Location -= Math::Lerp(Float2::Zero,rect.Size, Content->GetPivot());
        rect.Location += Math::Lerp(Float2::Zero,rect.Size, InNewPoivt);
    }

    // Apply the modifications 
    // [Note] 
    // this is not calling a Layout
    // it is a correct way to do it
    Applay(rect, InNewPoivt);
}

void UICanvasPanelSlot::Serialize(SerializeStream& stream, const void* otherObj)
{
    UIPanelOrderedSlot::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(UICanvasPanelSlot);
    SERIALIZE(Anchor);
}

void UICanvasPanelSlot::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    UIPanelOrderedSlot::Deserialize(stream, modifier);
    DESERIALIZE(Anchor);
}


UICanvasPanel::UICanvasPanel(const SpawnParams& params) : UIPanelComponent(params)
{
    CanHaveMultipleChildren = true;
}
ScriptingTypeInitializer& UICanvasPanel::GetSlotClass() const
{
    return UICanvasPanelSlot::TypeInitializer;
}
