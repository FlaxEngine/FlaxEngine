//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIPanelComponent.h"
#include "../Types/UIPanelOrderedSlot.h"
#include "../Types/Anchor.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class UICanvasPanelSlot : UIPanelOrderedSlot
{
    DECLARE_SCRIPTING_TYPE(UICanvasPanelSlot);

    Anchor Anchor;
protected:

    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};

API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"Canvas\",CategoryName=\"Panels\",EditorComponent=false,HiddenInDesigner=false)")
class CanvasPanel : public UIPanelComponent
{
    DECLARE_SCRIPTING_TYPE(CanvasPanel);

    API_FUNCTION() virtual bool ReplaceChild(UIComponent* CurrentChild, UIComponent* NewChild);
    API_FUNCTION() virtual void ClearChildren();

#if USE_EDITOR
    /// <summary>
    /// Locks to panel on drag.
    /// </summary>
    /// <returns></returns>
    virtual bool LockToPanelOnDrag() const
    {
        return false;
    }
#endif

protected:
    virtual ScriptingTypeInitializer& GetSlotClass() const override;
protected:
    virtual void Layout(const Rectangle& InNewBounds) override;
    virtual void Layout(const Rectangle& InSlotNewBounds, UIPanelSlot* InFor) override;

};
