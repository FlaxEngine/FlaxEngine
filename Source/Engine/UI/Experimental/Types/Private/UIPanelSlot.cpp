//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/UI/Experimental/Types/UIPanelSlot.h"
#include "Engine/UI/Experimental/Types/UIComponent.h"
#include "Engine/UI/Experimental/Types/UIPanelComponent.h"

UIPanelSlot::UIPanelSlot(const SpawnParams& params) : ScriptingObject(params) , Content(nullptr) , Parent(nullptr) {}

FORCE_INLINE bool UIPanelSlot::IsDesignTime() const {   return Parent->IsDesignTime();  }

void UIPanelSlot::Layout(const Rectangle& InNewBounds, const Vector2& InNewPoivt, const Rectangle& InNewParentBounds)
{
    Applay(InNewBounds, InNewPoivt);
}
inline void UIPanelSlot::Applay(const Rectangle& InNewBounds, const Vector2& InNewPoivt)
{
    Content->SetRect_Internal(InNewBounds);
    Content->SetPivot_Internal(InNewPoivt);
    //Content->Layout(InNewBounds, InNewPoivt);
}
void UIPanelSlot::Serialize(SerializeStream& stream, const void* otherObj){}
void UIPanelSlot::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier){}
