// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.Assertions;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Root control implementation used by the <see cref="FlaxEngine.Window"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.RootControl" />
    [HideInEditor]
    public sealed class WindowRootControl : RootControl
    {
        private Window _window;
        private Control _focusedControl;
        private Control _trackingControl;

        /// <summary>
        /// Gets the native window object.
        /// </summary>
        public Window Window => _window;

        /// <summary>
        /// Sets the window title.
        /// </summary>
        public string Title
        {
            get => _window.Title;
            set => _window.Title = value;
        }

        /// <summary>
        /// Gets a value indicating whether this window is in fullscreen mode.
        /// </summary>
        public bool IsFullscreen => _window.IsFullscreen;

        /// <summary>
        /// Gets a value indicating whether this window is in windowed mode.
        /// </summary>
        public bool IsWindowed => _window.IsWindowed;

        /// <summary>
        /// Gets a value indicating whether this instance is visible.
        /// </summary>
        public bool IsShown => _window.IsVisible;

        /// <summary>
        /// Gets a value indicating whether this window is minimized.
        /// </summary>
        public bool IsMinimized => _window.IsMinimized;

        /// <summary>
        /// Gets a value indicating whether this window is maximized.
        /// </summary>
        public bool IsMaximized => _window.IsMaximized;

        internal WindowRootControl(Window window)
        {
            _window = window;
            ClipChildren = false;

            if (Style.Current != null)
                BackgroundColor = Style.Current.Background;
        }

        /// <summary>
        /// Shows the window.
        /// </summary>
        public void Show()
        {
            _window.Show();
        }

        /// <summary>
        /// Hides the window.
        /// </summary>
        public void Hide()
        {
            _window.Hide();
        }

        /// <summary>
        /// Minimizes the window.
        /// </summary>
        public void Minimize()
        {
            _window.Minimize();
        }

        /// <summary>
        /// Maximizes the window.
        /// </summary>
        public void Maximize()
        {
            _window.Maximize();
        }

        /// <summary>
        /// Restores the window state before minimizing or maximizing.
        /// </summary>
        public void Restore()
        {
            _window.Restore();
        }

        /// <summary>
        /// Closes the window.
        /// </summary>
        /// <param name="reason">The closing reason.</param>
        public void Close(ClosingReason reason = ClosingReason.CloseEvent)
        {
            _window.Close(reason);
        }

        /// <summary>
        /// Brings window to the front of the Z order.
        /// </summary>
        /// <param name="force">True if move to the front by force, otherwise false.</param>
        public void BringToFront(bool force = false)
        {
            _window.BringToFront(force);
        }

        /// <summary>
        /// Flashes the window to bring use attention.
        /// </summary>
        public void FlashWindow()
        {
            _window.FlashWindow();
        }

        /// <summary>
        /// Gets or sets the current focused control
        /// </summary>
        public override Control FocusedControl
        {
            get => _focusedControl;
            set
            {
                Assert.IsTrue(_focusedControl == null || _focusedControl.Root == this, "Invalid control to focus");
                Focus(value);
            }
        }

        /// <inheritdoc />
        public override CursorType Cursor
        {
            get => _window.Cursor;
            set => _window.Cursor = value;
        }

        /// <inheritdoc />
        public override Float2 TrackingMouseOffset => _window.TrackingMouseOffset / _window.DpiScale;

        /// <inheritdoc />
        public override WindowRootControl RootWindow => this;

        /// <inheritdoc />
        public override Float2 MousePosition
        {
            get => _window.MousePosition / _window.DpiScale;
            set => _window.MousePosition = value * _window.DpiScale;
        }

        /// <inheritdoc />
        public override void StartTrackingMouse(Control control, bool useMouseScreenOffset)
        {
            if (control == null)
                throw new ArgumentNullException();
            if (_trackingControl == control)
                return;

            if (_trackingControl != null)
                EndTrackingMouse();

            if (control.AutoFocus)
                Focus(control);

            _trackingControl = control;

            _window.StartTrackingMouse(useMouseScreenOffset);
        }

        /// <inheritdoc />
        public override void EndTrackingMouse()
        {
            if (_trackingControl != null)
            {
                var control = _trackingControl;
                _trackingControl = null;
                control.OnEndMouseCapture();

                _window.EndTrackingMouse();
            }
        }

        /// <inheritdoc />
        public override bool GetKey(KeyboardKeys key)
        {
            return _window.GetKey(key);
        }

        /// <inheritdoc />
        public override bool GetKeyDown(KeyboardKeys key)
        {
            return _window.GetKeyDown(key);
        }

        /// <inheritdoc />
        public override bool GetKeyUp(KeyboardKeys key)
        {
            return _window.GetKeyUp(key);
        }

        /// <inheritdoc />
        public override bool GetMouseButton(MouseButton button)
        {
            return _window.GetMouseButton(button);
        }

        /// <inheritdoc />
        public override bool GetMouseButtonDown(MouseButton button)
        {
            return _window.GetMouseButtonDown(button);
        }

        /// <inheritdoc />
        public override bool GetMouseButtonUp(MouseButton button)
        {
            return _window.GetMouseButtonUp(button);
        }

        /// <inheritdoc />
        public override Float2 PointFromScreen(Float2 location)
        {
            return _window.ScreenToClient(location) / _window.DpiScale;
        }

        /// <inheritdoc />
        public override Float2 PointToScreen(Float2 location)
        {
            return _window.ClientToScreen(location * _window.DpiScale);
        }

        /// <inheritdoc />
        public override void Focus()
        {
            _window.Focus();
        }

        /// <inheritdoc />
        public override void DoDragDrop(DragData data)
        {
            _window.DoDragDrop(data);
        }

        /// <inheritdoc />
        protected override bool Focus(Control c)
        {
            if (IsDisposing || _focusedControl == c)
                return false;

            // Change focused control
            Control previous = _focusedControl;
            _focusedControl = c;

            // Fire events
            if (previous != null)
            {
                previous.OnLostFocus();
                Assert.IsFalse(previous.IsFocused);
            }
            if (_focusedControl != null)
            {
                _focusedControl.OnGotFocus();
                Assert.IsTrue(_focusedControl.IsFocused);
            }

            // Update flags
            UpdateContainsFocus();

            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (_trackingControl != null)
            {
                return _trackingControl.OnMouseDown(_trackingControl.PointFromWindow(location), button);
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_trackingControl != null)
            {
                return _trackingControl.OnMouseUp(_trackingControl.PointFromWindow(location), button);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (_trackingControl != null)
            {
                return _trackingControl.OnMouseDoubleClick(_trackingControl.PointFromWindow(location), button);
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (_trackingControl != null)
            {
                return _trackingControl.OnMouseWheel(_trackingControl.PointFromWindow(location), delta);
            }

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_trackingControl != null)
            {
                _trackingControl.OnMouseMove(_trackingControl.PointFromWindow(location));
                return;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseMoveRelative(Float2 mouseMotion)
        {
            if (_trackingControl != null)
            {
                _trackingControl.OnMouseMoveRelative(mouseMotion);
                return;
            }

            base.OnMouseMoveRelative(mouseMotion);
        }
    }
}
