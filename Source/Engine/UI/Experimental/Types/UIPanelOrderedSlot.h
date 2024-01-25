//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "UIPanelSlot.h"

/// <summary>
/// base class for a slot with can be Z Ordered
/// <para><b>Note:</b></para>
/// the Z ordered slots, have a performance cost (N*N) because there are needing to be sorted at draw time don't use it everywhere
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API UIPanelOrderedSlot : public UIPanelSlot
{
    DECLARE_SCRIPTING_TYPE(UIPanelOrderedSlot);
public:
    /// <summary>
    /// The z order
    /// </summary>
    int32 ZOrder;
protected:

    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
