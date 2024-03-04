//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIPanelComponent.h"
#include "../Types/UIPanelOrderedSlot.h"
#include "../Types/Anchor.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class UICanvasPanelSlot : public UIPanelOrderedSlot
{
    DECLARE_SCRIPTING_TYPE(UICanvasPanelSlot);

    API_FIELD() Anchor Anchor;
protected:
    virtual void Layout(const Rectangle& InNewBounds, const Vector2& InNewPoivt, const Rectangle& InNewParentBounds) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};

API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"Canvas\",CategoryName=\"Panels\",EditorComponent=false,HiddenInDesigner=false)")
class UICanvasPanel : public UIPanelComponent
{
    DECLARE_SCRIPTING_TYPE(UICanvasPanel);

protected:
    virtual ScriptingTypeInitializer& GetSlotClass() const override;
};
