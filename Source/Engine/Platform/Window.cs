// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    partial class Window
    {
        /// <summary>
        /// Window closing delegate.
        /// </summary>
        /// <param name="reason">The closing reason.</param>
        /// <param name="cancel">If set to <c>true</c> operation will be cancelled, otherwise window will be closed.</param>
        public delegate void ClosingDelegate(ClosingReason reason, ref bool cancel);

        /// <summary>
        /// Perform window hit test delegate.
        /// </summary>
        /// <param name="mouse">The mouse position. The coordinate is relative to the upper-left corner of the screen. Use <see cref="ScreenToClient"/> to convert position into client space coordinates.</param>
        /// <returns>Hit result.</returns>
        public delegate WindowHitCodes HitTestDelegate(ref Vector2 mouse);

        /// <summary>
        /// Perform mouse buttons action.
        /// </summary>
        /// <param name="mouse">The mouse position.</param>
        /// <param name="button">The mouse buttons state.</param>
        /// <param name="handled">The flag that indicated that event has been handled by the custom code and should not be passed further. By default it is set to false.</param>
        public delegate void MouseButtonDelegate(ref Vector2 mouse, MouseButton button, ref bool handled);

        /// <summary>
        /// Perform mouse move action.
        /// </summary>
        /// <param name="mouse">The mouse position.</param>
        public delegate void MouseMoveDelegate(ref Vector2 mouse);

        /// <summary>
        /// Perform mouse wheel action.
        /// </summary>
        /// <param name="mouse">The mouse position.</param>
        /// <param name="delta">The mouse wheel move delta (can be positive or negative; normalized to [-1;1] range).</param>
        /// <param name="handled">The flag that indicated that event has been handled by the custom code and should not be passed further. By default it is set to false.</param>
        public delegate void MouseWheelDelegate(ref Vector2 mouse, float delta, ref bool handled);

        /// <summary>
        /// Perform touch action.
        /// </summary>
        /// <param name="pointerPosition">The touch pointer position.</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <param name="handled">The flag that indicated that event has been handled by the custom code and should not be passed further. By default it is set to false.</param>
        public delegate void TouchDelegate(ref Vector2 pointerPosition, int pointerId, ref bool handled);

        /// <summary>
        /// Perform input character action.
        /// </summary>
        /// <param name="c">The input character.</param>
        public delegate void CharDelegate(char c);

        /// <summary>
        /// Perform keyboard action.
        /// </summary>
        /// <param name="key">The key.</param>
        public delegate void KeyboardDelegate(KeyboardKeys key);

        /// <summary>
        /// Event fired on character input.
        /// </summary>
        public event CharDelegate OnCharInput;

        /// <summary>
        /// Event fired on key pressed.
        /// </summary>
        public event KeyboardDelegate KeyDown;

        /// <summary>
        /// Event fired on key released.
        /// </summary>
        public event KeyboardDelegate KeyUp;

        /// <summary>
        /// Event fired when mouse goes down.
        /// </summary>
        public event MouseButtonDelegate MouseDown;

        /// <summary>
        /// Event fired when mouse goes up.
        /// </summary>
        public event MouseButtonDelegate MouseUp;

        /// <summary>
        /// Event fired when mouse double clicks.
        /// </summary>
        public event MouseButtonDelegate MouseDoubleClick;

        /// <summary>
        /// Event fired when mouse wheel is scrolling.
        /// </summary>
        public event MouseWheelDelegate MouseWheel;

        /// <summary>
        /// Event fired when mouse moves
        /// </summary>
        public event MouseMoveDelegate MouseMove;

        /// <summary>
        /// Event fired when mouse leaves window.
        /// </summary>
        public event Action MouseLeave;

        /// <summary>
        /// Event fired when touch begins.
        /// </summary>
        public event TouchDelegate TouchDown;

        /// <summary>
        /// Event fired when touch moves.
        /// </summary>
        public event TouchDelegate TouchMove;

        /// <summary>
        /// Event fired when touch ends.
        /// </summary>
        public event TouchDelegate TouchUp;

        /// <summary>
        /// Event fired when window gets focus.
        /// </summary>
        public event Action GotFocus;

        /// <summary>
        /// Event fired when window lost focus.
        /// </summary>
        public event Action LostFocus;

        /// <summary>
        /// Event fired when window performs hit test, parameter is a mouse position
        /// </summary>
        public HitTestDelegate HitTest;

        /// <summary>
        /// Event fired when left mouse button goes down (hit test performed etc.).
        /// Returns true if event has been processed and further actions should be canceled, otherwise false.
        /// </summary>
        public Func<WindowHitCodes, bool> LeftButtonHit;

        /// <summary>
        /// Event fired when windows wants to be closed. Should return true if suspend window closing, otherwise returns false
        /// </summary>
        public event ClosingDelegate Closing;

        /// <summary>
        /// Event fired when gets closed and deleted, all references to the window object should be removed at this point.
        /// </summary>
        public event Action Closed;

        /// <summary>
        /// The window GUI root object.
        /// </summary>
        public readonly WindowRootControl GUI;

        /// <summary>
        /// Starts the drag and drop operation.
        /// </summary>
        /// <param name="data">The data.</param>
        public void DoDragDrop(DragData data)
        {
            if (data is DragDataText text)
                DoDragDrop(text.Text);
            else
                throw new NotImplementedException("Only DragDataText drag and drop is supported.");
        }

        private Window()
        {
            GUI = new WindowRootControl(this);
        }

        internal void Internal_OnShow()
        {
            GUI.UnlockChildrenRecursive();
            GUI.PerformLayout();
        }

        internal void Internal_OnUpdate(float dt)
        {
            GUI.Update(dt);
        }

        internal void Internal_OnDraw()
        {
            Matrix3x3.Scaling(DpiScale, out var scale);
            Render2D.PushTransform(ref scale);
            GUI.Draw();
            Render2D.PopTransform();
        }

        internal void Internal_OnResize(int width, int height)
        {
            GUI.Size = new Vector2(width / DpiScale, height / DpiScale);
        }

        internal void Internal_OnCharInput(char c)
        {
            OnCharInput?.Invoke(c);
            GUI.OnCharInput(c);
        }

        internal void Internal_OnKeyDown(KeyboardKeys key)
        {
            KeyDown?.Invoke(key);
            GUI.OnKeyDown(key);
        }

        internal void Internal_OnKeyUp(KeyboardKeys key)
        {
            KeyUp?.Invoke(key);
            GUI.OnKeyUp(key);
        }

        internal void Internal_OnMouseDown(ref Vector2 mousePos, MouseButton button)
        {
            Vector2 pos = mousePos / DpiScale;

            bool handled = false;
            MouseDown?.Invoke(ref pos, button, ref handled);
            if (handled)
                return;

            GUI.OnMouseDown(pos, button);
        }

        internal void Internal_OnMouseUp(ref Vector2 mousePos, MouseButton button)
        {
            Vector2 pos = mousePos / DpiScale;

            bool handled = false;
            MouseUp?.Invoke(ref pos, button, ref handled);
            if (handled)
                return;

            GUI.OnMouseUp(pos, button);
        }

        internal void Internal_OnMouseDoubleClick(ref Vector2 mousePos, MouseButton button)
        {
            Vector2 pos = mousePos / DpiScale;

            bool handled = false;
            MouseDoubleClick?.Invoke(ref pos, button, ref handled);
            if (handled)
                return;

            GUI.OnMouseDoubleClick(pos, button);
        }

        internal void Internal_OnMouseWheel(ref Vector2 mousePos, float delta)
        {
            Vector2 pos = mousePos / DpiScale;

            bool handled = false;
            MouseWheel?.Invoke(ref pos, delta, ref handled);
            if (handled)
                return;

            GUI.OnMouseWheel(pos, delta);
        }

        internal void Internal_OnMouseMove(ref Vector2 mousePos)
        {
            Vector2 pos = mousePos / DpiScale;

            MouseMove?.Invoke(ref pos);
            GUI.OnMouseMove(pos);
        }

        internal void Internal_OnMouseLeave()
        {
            MouseLeave?.Invoke();
            GUI.OnMouseLeave();
        }

        internal void Internal_OnTouchDown(ref Vector2 pointerPosition, int pointerId)
        {
            Vector2 pos = pointerPosition / DpiScale;

            bool handled = false;
            TouchDown?.Invoke(ref pos, pointerId, ref handled);
            if (handled)
                return;

            GUI.OnTouchDown(pos, pointerId);
        }

        internal void Internal_OnTouchMove(ref Vector2 pointerPosition, int pointerId)
        {
            Vector2 pos = pointerPosition / DpiScale;

            bool handled = false;
            TouchMove?.Invoke(ref pos, pointerId, ref handled);
            if (handled)
                return;

            GUI.OnTouchMove(pos, pointerId);
        }

        internal void Internal_OnTouchUp(ref Vector2 pointerPosition, int pointerId)
        {
            Vector2 pos = pointerPosition / DpiScale;

            bool handled = false;
            TouchUp?.Invoke(ref pos, pointerId, ref handled);
            if (handled)
                return;

            GUI.OnTouchUp(pos, pointerId);
        }

        internal void Internal_OnGotFocus()
        {
            GotFocus?.Invoke();
            GUI.OnGotFocus();
        }

        internal void Internal_OnLostFocus()
        {
            LostFocus?.Invoke();
            GUI.OnLostFocus();
        }

        internal void Internal_OnHitTest(ref Vector2 mousePos, ref WindowHitCodes result, ref bool handled)
        {
            if (HitTest != null)
            {
                Vector2 pos = mousePos / DpiScale;
                result = HitTest(ref pos);
                handled = true;
            }
        }

        internal void Internal_OnLeftButtonHit(WindowHitCodes hit, ref bool result)
        {
            if (LeftButtonHit != null)
            {
                result = LeftButtonHit(hit);
            }
        }

        internal DragDropEffect Internal_OnDragEnter(ref Vector2 mousePos, bool isText, string[] data)
        {
            DragData dragData;
            if (isText)
                dragData = new DragDataText(data[0]);
            else
                dragData = new DragDataFiles(data);
            Vector2 pos = mousePos / DpiScale;
            return GUI.OnDragEnter(ref pos, dragData);
        }

        internal DragDropEffect Internal_OnDragOver(ref Vector2 mousePos, bool isText, string[] data)
        {
            DragData dragData;
            if (isText)
                dragData = new DragDataText(data[0]);
            else
                dragData = new DragDataFiles(data);
            Vector2 pos = mousePos / DpiScale;
            return GUI.OnDragMove(ref pos, dragData);
        }

        internal DragDropEffect Internal_OnDragDrop(ref Vector2 mousePos, bool isText, string[] data)
        {
            DragData dragData;
            if (isText)
                dragData = new DragDataText(data[0]);
            else
                dragData = new DragDataFiles(data);
            Vector2 pos = mousePos / DpiScale;
            return GUI.OnDragDrop(ref pos, dragData);
        }

        internal void Internal_OnDragLeave()
        {
            GUI.OnDragLeave();
        }

        internal void Internal_OnClosing(ClosingReason reason, ref bool cancel)
        {
            Closing?.Invoke(reason, ref cancel);
        }

        internal void Internal_OnClosed()
        {
            Closed?.Invoke();

            GUI.Dispose();

            // Force clear all events (we cannot use window after close)
            KeyDown = null;
            KeyUp = null;
            MouseLeave = null;
            MouseDown = null;
            MouseUp = null;
            MouseDoubleClick = null;
            MouseWheel = null;
            MouseMove = null;
            TouchDown = null;
            TouchMove = null;
            TouchUp = null;
            GotFocus = null;
            LostFocus = null;
            LeftButtonHit = null;
            HitTest = null;
            Closing = null;
            Closed = null;
        }
    }
}
