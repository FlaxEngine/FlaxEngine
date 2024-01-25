
using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEngine.GUI;

namespace FlaxEngine.Experimental.UI
{
    //note this is temporary solution

    /// <summary>
    /// host for Native UI
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class NativeUIHost : Control
    {
        /// <summary>
        /// The panel component
        /// </summary>
        [EditorOrder(0)]
        public UIBlueprintAsset Asset;

        /// <summary>
        /// The panel component
        /// </summary>
        //[HideInEditor]
        public UIPanelComponent PanelComponent;
        /// <summary>
        /// The action
        /// </summary>
        public UIActionEvent Action = new UIActionEvent();
        /// <summary>
        /// The pointer event
        /// </summary>
        public UIPointerEvent pointerEvent = new UIPointerEvent();
        /// <inheritdoc />
        public override void Draw()
        {
            PanelComponent?.Render();
            base.Draw();
        }
        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (PanelComponent != null)
            {
                //fore update layout on c++ side
                PanelComponent.Transform = PanelComponent.Transform;
            }
            base.PerformLayout(force);
        }
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (PanelComponent != null)
            {
            }
            else if (Asset != null)
            {
                if (Asset.IsLoaded)
                {
                    if (Asset.Component != null)
                    {
                        PanelComponent = (UIPanelComponent)Asset.Component;
                    }
                }
            }
        }
        /// <summary>
        /// The focused
        /// </summary>
        public UIComponent focused;
        /// <summary>
        /// The mouse location
        /// </summary>
        public Float2 MouseLocation;
        /// <summary>
        /// The pointer event has capture
        /// </summary>
        public bool PointerEventHasCapture;
        /// <summary>
        /// The locations
        /// </summary>
        public List<Float2> Locations = new List<Float2>();
        void SendPointerEvent(Float2 point, int id, InputActionState state)
        {
            if (PanelComponent.DesignerFlags == Editor.UIComponentDesignFlags.None)
            {
                if (Locations.Count < id)
                {
                    Locations.Add(point);
                }
                else
                {
                    if (state == InputActionState.Release && id != 0)
                    {
                        Locations.RemoveAt(id);
                    }
                }
                pointerEvent.Locations = Locations.ToArray();
                if (focused == null)
                {
                    UIBlueprintAsset.SendEvent(PanelComponent, pointerEvent, out var hit, out var response);
                    if (response.HasFlag(UIEventResponse.Focus))
                    {
                        focused = hit;
                    }
                    PointerEventHasCapture = response.HasFlag(UIEventResponse.Capture);
                }
                else
                {
                    foreach (var loc in pointerEvent.Locations)
                    {
                        if (focused.Contains(loc))
                        {
                            var response = focused.OnPointerInput(pointerEvent);
                            if (!response.HasFlag(UIEventResponse.Focus))
                            {
                                focused = null;
                            }
                            PointerEventHasCapture = response.HasFlag(UIEventResponse.Capture);
                            break;
                        }
                    }
                }
            }
        }

        /// <inheritdoc/>
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            pointerEvent.IsTouch = false;
            pointerEvent.MouseButton = button;
            SendPointerEvent(MouseLocation = location, 0, InputActionState.Press);
            return base.OnMouseUp(location, button);
        }
        /// <inheritdoc/>
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            pointerEvent.IsTouch = false;
            pointerEvent.MouseButton = button;
            SendPointerEvent(MouseLocation = location, 0, InputActionState.Release);
            return base.OnMouseDown(location, button);
        }
        /// <inheritdoc/>
        public override void OnMouseMove(Float2 location)
        {
            pointerEvent.IsTouch = false;
            SendPointerEvent(MouseLocation = location, 0, InputActionState.Waiting);
            base.OnMouseMove(location);
        }
        /// <inheritdoc/>
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            pointerEvent.IsTouch = false;
            pointerEvent.MouseButton = button;
            SendPointerEvent(MouseLocation = location, 0, InputActionState.Press);
            return base.OnMouseDoubleClick(location, button);
        }
        /// <inheritdoc/>
        public override void OnMouseEnter(Float2 location)
        {
            pointerEvent.IsTouch = false;
            SendPointerEvent(location, 0, InputActionState.Waiting);
            base.OnMouseEnter(location);
        }
        /// <inheritdoc/>
        public override void OnMouseLeave()
        {
            pointerEvent.IsTouch = false;
            SendPointerEvent(Float2.Zero, 0, InputActionState.Release);
            base.OnMouseLeave();
        }
        void OnSendActionEvent()
        {
            if (focused != null)
            {
                var response = focused.OnActionInput(Action);
                if (!response.HasFlag(UIEventResponse.Focus))
                {
                    focused = null;
                }
                PointerEventHasCapture = response.HasFlag(UIEventResponse.Capture);
            }
        }

        /// <inheritdoc/>
        public override bool OnKeyDown(KeyboardKeys key)
        {
            Action.Key = key;
            Action.State = InputActionState.Press;
            OnSendActionEvent();
            return base.OnKeyDown(key);
        }
        /// <inheritdoc/>
        public override void OnKeyUp(KeyboardKeys key)
        {
            Action.Key = key;
            Action.State = InputActionState.Release;
            OnSendActionEvent();
            base.OnKeyUp(key);
        }
        /// <inheritdoc/>
        public override void OnTouchEnter(Float2 location, int pointerId)
        {
            pointerEvent.IsTouch = true;
            SendPointerEvent(location,pointerId, InputActionState.Waiting);
            base.OnTouchEnter(location, pointerId);
        }
        /// <inheritdoc/>
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            pointerEvent.IsTouch = true;
            SendPointerEvent(location, pointerId, InputActionState.Press);
            return base.OnTouchDown(location, pointerId);
        }
        /// <inheritdoc/>
        public override void OnTouchLeave(int pointerId)
        {
            pointerEvent.IsTouch = true;
            SendPointerEvent(Float2.Zero, pointerId, InputActionState.Release);
            base.OnTouchLeave(pointerId);
        }
        /// <inheritdoc/>
        public override void OnTouchMove(Float2 location, int pointerId)
        {
            pointerEvent.IsTouch = true;
            SendPointerEvent(location, pointerId, InputActionState.Waiting);
            base.OnTouchMove(location, pointerId);
        }
        /// <inheritdoc/>
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            pointerEvent.IsTouch = true;
            SendPointerEvent(location, pointerId, InputActionState.Release);
            return base.OnTouchUp(location, pointerId);
        }
    }
}
