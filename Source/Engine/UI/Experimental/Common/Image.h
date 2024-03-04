//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIComponent.h"
#include "../Brushes/Brush.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"Image\",CategoryName=\"Common\",EditorComponent=false,HiddenInDesigner=false)")
class UIImage : public UIComponent
{
    DECLARE_SCRIPTING_TYPE(UIImage);
public:
    API_FIELD()
    UIBrush* Brush;
private:
    virtual void OnDraw() override;
protected:
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
