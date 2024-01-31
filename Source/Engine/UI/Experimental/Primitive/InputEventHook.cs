using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.Experimental.UI
{
    public partial class InputEventHook
    {
        //Func<UIActionEvent, UIEventResponse> eActionInput
        //{
        //    set { };
        //    get { };
        //}
        ///// <summary>
        ///// Called when input has event's: mouse, touch, stylus, gamepad emulated mouse, etc. and value has changed
        ///// </summary>
        //public Func<UIActionEvent, UIEventResponse> ActionInput;
        ///// <summary>
        ///// Called when input has event's: keyboard, gamepay buttons, etc. and value has changed
        ///// Note: keyboard can have action keys where value is from 0 to 1
        ///// </summary>
        //public Func<UIPointerEvent, UIEventResponse> PointerInput;


        //internal override UIEventResponse OnActionInput(UIActionEvent InEvent)
        //{
        //    if (ActionInput == null)
        //    {
        //        return base.OnActionInput(InEvent);
        //    }
        //    return ActionInput.Invoke(InEvent);
        //}
        //internal override UIEventResponse OnPointerInput(UIPointerEvent InEvent)
        //{
        //    if (PointerInput == null)
        //    {
        //        return base.OnPointerInput(InEvent);
        //    }
        //    return PointerInput.Invoke(InEvent);
        //}
    }
}
