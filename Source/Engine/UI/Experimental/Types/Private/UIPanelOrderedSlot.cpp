//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "Engine/UI/Experimental/Types/UIPanelOrderedSlot.h"
#include "Engine/Serialization/Serialization.h"

UIPanelOrderedSlot::UIPanelOrderedSlot(const SpawnParams& params) : UIPanelSlot(params) { ZOrder = 0; }

void UIPanelOrderedSlot::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(UIPanelOrderedSlot);
    if (ZOrder != 0)
        SERIALIZE(ZOrder);
}

void UIPanelOrderedSlot::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(ZOrder);
}
