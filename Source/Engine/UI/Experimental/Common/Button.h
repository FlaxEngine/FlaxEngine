//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIComponent.h"
#include "../Brushes/Brush.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"Button\",CategoryName=\"Common\",EditorComponent=false,HiddenInDesigner=false)")
class UIButton : public UIComponent
{
    DECLARE_SCRIPTING_TYPE(UIButton);
public:

    API_ENUM() enum State
    {        
        /// <summary>
        /// The the default if is triggered the button has lot the focus none
        /// </summary>
        None = 0,

        /// <summary>
        /// The hover state
        /// </summary>
        Hover = 1,

        /// <summary>
        /// User is pressing the button.
        /// </summary>
        Pressing = 2,

        /// <summary>
        /// The press
        /// </summary>
        Press = 3,

        /// <summary>
        /// The release
        /// </summary>
        Release = 4,
    };
    /// <summary>
    /// Called when Button changes state
    /// </summary>
    API_EVENT() Delegate<UIButton*,State> StateChanged;
    
    /// <summary>
    /// The Event Response for raycaster allows to ignore raycast but still receive the event
    /// </summary>
    API_FIELD()
    UIEventResponse Response = UIEventResponse::Focus;

    API_FIELD(Attributes = "CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.UIBrushEditor)), DefaultEditor")
        UIBrush* BrushNormal;
    API_FIELD(Attributes = "CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.UIBrushEditor)), DefaultEditor")
        UIBrush* BrushHover;
    API_FIELD(Attributes = "CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.UIBrushEditor)), DefaultEditor")
        UIBrush* BrushPressed;

private:

    State ButtonState;
    virtual void OnDraw() override;

    virtual UIEventResponse OnPointerInput(const UIPointerEvent& InEvent) override;
private:
    void SetState(State InNewState);
    
protected:
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
private:
    void OnDeleteObject() override;
};
