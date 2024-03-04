using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine.Experimental.UI
{
    public partial class UIInputEventHook
    {
        public Func<UIActionEvent, UIEventResponse> ActionInput
        {
            get => m_ActionInput;
            set
            {
                m_ActionInput = value;
                HandleActionInputCPPBindings();
            }
        }

        public Func<UIPointerEvent, UIEventResponse> PointerInput
        {
            get => m_PointerInput;
            set
            {
                m_PointerInput = value;
                HandlePointerInputCPPBindings();
            }
        }

        #region CSHACK

        private Func<UIPointerEvent, UIEventResponse> m_PointerInput;
        private Action<UIPointerEvent> m_PointerInputAction;
        private Func<UIActionEvent, UIEventResponse> m_ActionInput;
        private Action<UIActionEvent> m_ActionInputAction;
        private void HandlePointerInputCPPBindings()
        {
            if (m_PointerInput == null)
            {
                CSHACK_PointerInput -= m_PointerInputAction;
                CSHACK_UnBindPointerInput();
                m_PointerInputAction = null;
            }

            if (CSHACK_IsBindedPointerInput())
            {
                CSHACK_PointerInput -= m_PointerInputAction;
                m_PointerInputAction = (UIPointerEvent InEvent) =>
                {
                    if (m_PointerInput != null)
                    {
                        CSHACK_SetPointerInputUIEventResponse(m_PointerInput(InEvent));
                    }
                };
                CSHACK_BindPointerInput();
                CSHACK_PointerInput += m_PointerInputAction;
            }
            else
            {
                m_PointerInputAction = (UIPointerEvent InEvent) =>
                {
                    if (m_PointerInput != null)
                    {
                        CSHACK_SetPointerInputUIEventResponse(m_PointerInput(InEvent));
                    }
                };
                CSHACK_BindPointerInput();
                CSHACK_PointerInput += m_PointerInputAction;
            }
        }

        private void HandleActionInputCPPBindings()
        {
            if (m_ActionInput == null)
            {
                CSHACK_ActionInput -= m_ActionInputAction;
                CSHACK_UnBindActionInput();
                m_ActionInputAction = null;
            }

            if (CSHACK_IsBindedActionInput())
            {
                CSHACK_ActionInput -= m_ActionInputAction;
                m_ActionInputAction = (UIActionEvent InEvent) =>
                {
                    if (m_ActionInput != null)
                    {
                        CSHACK_SetActionInputUIEventResponse(m_ActionInput(InEvent));
                    }
                };
                CSHACK_BindActionInput();
                CSHACK_ActionInput += m_ActionInputAction;
            }
            else
            {
                m_ActionInputAction = (UIActionEvent InEvent) =>
                {
                    if (m_ActionInput != null)
                    {
                        CSHACK_SetActionInputUIEventResponse(m_ActionInput(InEvent));
                    }
                };
                CSHACK_BindActionInput();
                CSHACK_ActionInput += m_ActionInputAction;
            }
        }

        #endregion
    }
}
