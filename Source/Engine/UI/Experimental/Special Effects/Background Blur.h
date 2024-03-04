//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIPanelComponent.h"
#include "../Types/Margin.h"
#include "../Types/HorizontalAlignment.h"
#include "../Types/VerticalAlignment.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class UIBackgroundBlurSlot : public UIPanelSlot
{
    DECLARE_SCRIPTING_TYPE(UIBackgroundBlurSlot);
private:
    UIMargin Padding;
    UIHorizontalAlignment HorizontalAlignment;
    UIVerticalAlignment   VerticalAlignment;
public:
    API_PROPERTY() void SetPadding(const UIMargin& InPadding);
    API_PROPERTY() const UIMargin& GetPadding();

    API_PROPERTY() void SetHorizontalAlignment(UIHorizontalAlignment InHorizontalAlignment);
    API_PROPERTY() UIHorizontalAlignment GetHorizontalAlignment();

    API_PROPERTY() void SetVerticalAlignment(UIVerticalAlignment InVerticalAlignment);
    API_PROPERTY() UIVerticalAlignment GetVerticalAlignment();
protected:
    virtual void Layout(const Rectangle& InNewBounds, const Vector2& InNewPoivt, const Rectangle& InNewParentBounds) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};


API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"Background Blur\",CategoryName=\"Special Effects\",EditorComponent=false,HiddenInDesigner=false)")
class FLAXENGINE_API UIBackgroundBlur : public UIPanelComponent
{
    DECLARE_SCRIPTING_TYPE(UIBackgroundBlur);
public:
    API_FIELD() float BlurStrength;
private:
    virtual void OnDraw() override;
    virtual ScriptingTypeInitializer& GetSlotClass() const;
protected:
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
