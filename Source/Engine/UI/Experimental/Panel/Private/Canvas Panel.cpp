#include "../Canvas Panel.h"
#include "Engine/Serialization/Serialization.h"

UICanvasPanelSlot::UICanvasPanelSlot(const SpawnParams& params) : UIPanelOrderedSlot(params) { ZOrder = 0; }

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


CanvasPanel::CanvasPanel(const SpawnParams& params) : UIPanelComponent(params)
{
    CanHaveMultipleChildren = true;
}

bool CanvasPanel::ReplaceChild(UIComponent* CurrentChild, UIComponent* NewChild)
{
    return false;
}
void CanvasPanel::ClearChildren()
{
}

ScriptingTypeInitializer& CanvasPanel::GetSlotClass() const
{
    return UICanvasPanelSlot::TypeInitializer;
}

void CanvasPanel::Layout(const Rectangle& InNewBounds)
{
}

void CanvasPanel::Layout(const Rectangle& InSlotNewBounds, UIPanelSlot* InFor)
{
}
