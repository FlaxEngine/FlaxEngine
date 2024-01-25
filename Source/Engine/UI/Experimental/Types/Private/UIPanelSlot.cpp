//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/UI/Experimental/Types/UIPanelSlot.h"
#include "Engine/UI/Experimental/Types/UIComponent.h"
#include "Engine/UI/Experimental/Types/UIPanelComponent.h"

UIPanelSlot::UIPanelSlot(const SpawnParams& params) : ScriptingObject(params) {}

FORCE_INLINE bool UIPanelSlot::IsDesignTime() const {   return Parent->IsDesignTime();  }

void UIPanelSlot::Layout(const Rectangle& InNewBounds)
{
    Content->Transform.Rect = InNewBounds;
    Content->Transform.UpdateTransform();
}

void UIPanelSlot::Serialize(SerializeStream& stream, const void* otherObj)
{
}

void UIPanelSlot::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
}
