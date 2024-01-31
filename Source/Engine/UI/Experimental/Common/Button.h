//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIComponent.h"

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
private:

    State ButtonState;
    virtual void OnDraw() override;

    virtual UIEventResponse OnPointerInput(const UIPointerEvent& InEvent) override;
private:
    void SetState(State InNewState);
};
