using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    public partial class EditorViewport
    {
        private bool _mouseCaptured;

        /// <summary>
        /// The input actions collection to processed during user input.
        /// </summary>
        public InputActionsContainer InputActions = new InputActionsContainer();

        /// <summary>
        /// The input data (from the current frame).
        /// </summary>
        protected ViewportInput _input;

        /// <summary>
        /// Gets the input state data.
        /// </summary>
        public ViewportInput Input
        {
            get
            {
                return _input;
            }
        }

        /// <summary>
        /// Gathered input data.
        /// </summary>
        public struct ViewportInput
        {
            private bool _toggleMouseBottonRight;

            /// <summary>
            /// The is control down flag.
            /// </summary>
            public bool IsControlDown { readonly get; private set; }

            /// <summary>
            /// The is shift down flag.
            /// </summary>
            public bool IsShiftDown { readonly get; private set; }

            /// <summary>
            /// The is alt down flag.
            /// </summary>
            public bool IsAltDown { readonly get; private set; }

            /// <summary>
            /// The is mouse right down flag.
            /// </summary>
            public bool IsMouseRightDown { readonly get; private set; }

            /// <summary>
            /// The is mouse middle down flag.
            /// </summary>
            public bool IsMouseMiddleDown { readonly get; private set; }

            /// <summary>
            /// The is mouse left down flag.
            /// </summary>
            public bool IsMouseLeftDown { readonly get; private set; }

            /// <summary>
            /// The is mouse wheel moving flag.
            /// </summary>
            public bool IsWheelInUse { readonly get; internal set; }

            /// <summary>
            /// Gets a value indicating whether use is controlling mouse.
            /// </summary>
            public bool IsControllingMouse { readonly get; private set; }

            /// <summary>
            /// The is panning state.
            /// </summary>
            public bool IsPanning { readonly get; private set; }

            /// <summary>
            /// The is rotating state.
            /// </summary>
            public bool IsRotating { readonly get; private set; }

            /// <summary>
            /// The is moving state.
            /// </summary>
            public bool IsMoving { readonly get; private set; }

            /// <summary>
            /// The is zooming state.
            /// </summary>
            public bool IsZooming { readonly get; private set; }

            /// <summary>
            /// The is orbiting state.
            /// </summary>
            public bool IsOrbiting { readonly get; private set; }

            /// <summary>
            /// The mouse position (in control space)
            /// </summary>
            public Float2 MousePosition { readonly get; internal set; }

            /// <summary>
            /// The mouse delta (in control space)
            /// </summary>s
            public Float2 MouseDelta { readonly get; internal set; }

            /// <summary>
            /// The Movment Axis. (world)
            /// can be ither controler or keyboard
            /// </summary>
            public Vector3 Axis { readonly get; private set; }

            /// <summary>
            /// The Movment Axis. (world)
            /// can be ither controler or keyboard
            /// </summary>
            public float Roll { readonly get; private set; }

            /// <summary>
            /// The mouse wheel delta.
            /// </summary>
            public float MouseWheelDelta { readonly get; internal set; }

            /// <summary>
            /// flag for if currently the editor is controlled by gamepad
            /// </summary>
            public bool ControledByGamepad { readonly get; internal set; }
            internal void UpdateKeyboardStates(EditorViewport viewport, KeyboardKeys keys, bool state)
            {
                switch (keys)
                {
                    case KeyboardKeys.Shift:
                        IsShiftDown = state;
                        break;
                    case KeyboardKeys.Control:
                        IsControlDown = state;
                        break;
                    case KeyboardKeys.Alt:
                        IsAltDown = state;
                        break;
                }
                WindowRootControl win = viewport.RootWindow;
                EditorOptions options = Editor.Instance.Options.Options;
                bool rollR = win.GetKey(options.Input.RollRight.Key);
                bool rollL = win.GetKey(options.Input.RollLeft.Key);
                bool rollactive = rollL || rollR;
                Axis = Vector3.Zero;

                if (win.GetKey(options.Input.Forward.Key))
                {
                    Axis += Vector3.Forward;
                }

                if (win.GetKey(options.Input.Backward.Key))
                {
                    Axis += Vector3.Backward;
                }

                if (win.GetKey(options.Input.Right.Key))
                {
                    Axis += Vector3.Right;
                }

                if (win.GetKey(options.Input.Left.Key))
                {
                    Axis += Vector3.Left;
                }
                if (rollactive)
                {
                    if (rollR)
                    {
                        Roll = 1f;
                    }
                    if (rollL)
                    {
                        Roll = -1f;
                    }
                }
                else
                {
                    Roll = 0f;
                    if (win.GetKey(options.Input.Up.Key))
                    {
                        Axis += Vector3.Up;
                    }
                    if (win.GetKey(options.Input.Down.Key))
                    {
                        Axis += Vector3.Down;
                    }
                }
                Axis.Normalize();
                UpdateSheredStates();
            }

            internal void UpdateMouseStates(EditorViewport viewport, MouseButton button, bool state)
            {
                switch (button)
                {
                    case MouseButton.Left:
                        IsMouseLeftDown = state;
                        if (state)
                        {
                            viewport.OnLeftMouseButtonDown();
                        }
                        else
                        {
                            viewport.OnLeftMouseButtonUp();
                        }
                        break;
                    case MouseButton.Middle:
                        IsMouseMiddleDown = state;
                        if (state)
                        {
                            viewport.OnMiddleMouseButtonDown();
                        }
                        else
                        {
                            viewport.OnMiddleMouseButtonUp();
                        }
                        break;
                    case MouseButton.Right:
                        if (_toggleMouseBottonRight)
                        {
                            if (state)
                            {
                                IsMouseRightDown = !IsMouseRightDown;
                            }
                        }
                        else
                        {
                            IsMouseRightDown = state;
                        }
                        if (state)
                        {
                            viewport.OnRightMouseButtonDown();
                        }
                        else
                        {
                            viewport.OnRightMouseButtonUp();
                        }
                        break;
                }
                UpdateSheredStates();
            }

            internal void ToggleMouseBottonRight()
            {
                _toggleMouseBottonRight = !_toggleMouseBottonRight;
                IsMouseRightDown = false;
            }

            internal void UpdateSheredStates()
            {
                IsPanning = (!IsAltDown && IsMouseMiddleDown && !IsMouseRightDown);
                IsRotating = (!IsAltDown && !IsMouseMiddleDown && IsMouseRightDown);
                IsMoving = (!Mathf.IsZero(Axis.X) || !Mathf.IsZero(Axis.Y) || !Mathf.IsZero(Axis.Z));
                IsZooming = (IsWheelInUse && !IsShiftDown);
                IsOrbiting = (IsAltDown && IsMouseLeftDown && !IsMouseMiddleDown && !IsMouseRightDown);
                IsControllingMouse = (IsMouseMiddleDown || IsMouseRightDown || (IsAltDown && IsMouseLeftDown) || IsWheelInUse);
            }
            /// <summary>
            /// processes the gamepad inputs and events
            /// </summary>
            public void ProccesInput()
            {
                if (FlaxEngine.Input.GamepadsCount > 0)
                {
                    if (FlaxEngine.Input.GetGamepadButtonDown(InputGamepadIndex.All, GamepadButton.Start))
                    {
                        ControledByGamepad = !ControledByGamepad;
                        Axis = Vector3.Zero;
                        MouseDelta = Float2.Zero;
                        IsShiftDown = false;
                        IsControlDown = false;
                        IsAltDown = false;
                        UpdateSheredStates();
                    }
                    if (ControledByGamepad)
                    {
                        // Gamepads handling
                        Axis = new Vector3(GetGamepadAxis(GamepadAxis.LeftStickX), 0, GetGamepadAxis(GamepadAxis.LeftStickY));
                        MouseDelta = new Float2(GetGamepadAxis(GamepadAxis.RightStickX), -GetGamepadAxis(GamepadAxis.RightStickY)) * 45;
                        IsMouseRightDown = FlaxEngine.Input.GetGamepadButton(InputGamepadIndex.All, GamepadButton.RightShoulder);
                        IsMouseLeftDown = FlaxEngine.Input.GetGamepadButton(InputGamepadIndex.All, GamepadButton.LeftShoulder);
                        //IsShiftDown = FlaxEngine.Input.GetGamepadButton(InputGamepadIndex.All, GamepadButton.LeftTrigger);
                        UpdateSheredStates();
                    }
                }
            }
            /// <summary>
            /// </summary>
            public void ConsumeInput()
            {
                MouseDelta = Float2.Zero;
                MouseWheelDelta = 0f;
                var isWheelInUse = IsWheelInUse;
                IsWheelInUse = false;
                if (isWheelInUse != IsWheelInUse)
                {
                    UpdateSheredStates();
                }
            }

            /// <summary>
            /// </summary>
            /// <returns>state table</returns>
            public override string ToString()
            {
                return string.Concat(new string[]
                {
                    "IsControlDown       ",
                    IsControlDown.ToString(),
                    "\nIsShiftDown         ",
                    IsShiftDown.ToString(),
                    "\nIsAltDown           ",
                    IsAltDown.ToString(),
                    "\nIsMouseRightDown    ",
                    IsMouseRightDown.ToString(),
                    "\nIsMouseMiddleDown   ",
                    IsMouseMiddleDown.ToString(),
                    "\nIsMouseLeftDown     ",
                    IsMouseLeftDown.ToString(),
                    "\nMouseWheelDelta     ",
                    MouseWheelDelta.ToString(),
                    "\nIsWheelInUse        ",
                    IsWheelInUse.ToString(),
                    "\nIsControllingMouse  ",
                    IsControllingMouse.ToString(),
                    "\nIsPanning           ",
                    IsPanning.ToString(),
                    "\nIsRotating          ",
                    IsRotating.ToString(),
                    "\nIsMoving            ",
                    IsMoving.ToString(),
                    "\nIsZooming           ",
                    IsZooming.ToString(),
                    "\nIsOrbiting          ",
                    IsOrbiting.ToString(),
                    "\nMousePosition       ",
                    MousePosition.ToString(),
                    "\nMouseDelta          ",
                    MouseDelta.ToString(),
                    "\nAxis                ",
                    Axis.ToString(),
                    "\nRoll                ",
                    Roll.ToString()
                });
            }


            private float GetGamepadAxis(GamepadAxis axis)
            {
                float value = FlaxEngine.Input.GetGamepadAxis(InputGamepadIndex.All, axis);
                float deadZone = 0.2f;
                return (value >= deadZone || value <= -deadZone) ? value : 0f;
            }
        }

        /// <summary>
        /// captures the mouse, do not allow the mouse leave the viewport area, hides the cursor
        /// <br>remember to call <see cref="ReliseMouse" /></br>
        /// </summary>
        /// <param name="HidenMouse"></param>
        public void CaptureMouse(bool HidenMouse = false)
        {
            if (!_mouseCaptured)
            {
                //lock cursor to the bounds of the viewport
                RootWindow.Window.StartClippingCursor(new Rectangle(PointToWindow(Bounds.Location) + RootWindow.Window.ClientPosition, Bounds.Size));
                if (HidenMouse)
                    RootWindow.Window.Cursor = CursorType.Hidden;
                _mouseCaptured = true;
            }
        }

        /// <summary>
        /// return mouse to normal state
        /// </summary>
        public void ReliseMouse()
        {
            if (_mouseCaptured)
            {
                RootWindow.Window.Cursor = CursorType.Default;
                _mouseCaptured = false;
                RootWindow.Window.EndClippingCursor();
            }
        }
        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            bool isFocused = IsFocused;
            if (isFocused)
            {
                if (_mouseCaptured)
                {
                    var mdelta = RootWindow.Window.MousePositionDelta;
                    //var location = PointFromWindow(RootWindow.Window.MousePosition + RootWindow.Window.MousePositionDelta);
                    const int pading = 1;

                    bool hitLeft = location.X == 0;
                    bool hitTop = location.Y == 0;

                    bool hitRight = location.X == Size.X-1;
                    bool hitBottom = location.Y == Size.Y-1;

                    Float2 mouseWarpOffset = location;

                    if (hitLeft ||
                        hitTop ||
                        hitRight ||
                        hitBottom)
                    {
                        //lock mouse to bounds of the Viewport
                        if (hitLeft)
                            mouseWarpOffset.X = Size.X - pading;
                        if (hitTop)
                            mouseWarpOffset.Y = Size.Y - pading;

                        if (hitRight)
                            mouseWarpOffset.X = pading;
                        if (hitBottom)
                            mouseWarpOffset.Y = pading;


                        mouseWarpOffset += mdelta;

                        RootWindow.Window.MousePosition = PointToWindow(mouseWarpOffset);
                        _input.MousePosition = mouseWarpOffset;// just in case when mouse gets released in the same frame
                    }
                    else
                    {
                        _input.MouseDelta = mdelta;
                        _input.MousePosition = location;
                    }
                }
                else
                {
                    _input.MouseDelta = location - _input.MousePosition;
                    _input.MousePosition = location;
                }

                Float2 mousePosition = _input.MousePosition;
                MouseRay = ConvertMouseToRay(mousePosition);
            }
            base.OnMouseMove(location);
        }
        /// <inheritdoc />
        public override void OnGotFocus()
        {
            _input.MousePosition = PointFromWindow(RootWindow.Window.MousePosition);
            base.OnGotFocus();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();
            ReliseMouse();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            _input.MousePosition = location;
            base.OnMouseEnter(location);
        }
        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            bool isFocused = IsFocused;
            if (isFocused)
            {
                _input.UpdateMouseStates(this, button, false);
            }
            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();
            _input.UpdateMouseStates(this, button, true);
            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            bool isFocused = IsFocused;
            if (isFocused)
            {
                _input.MouseWheelDelta += delta;
                _input.IsWheelInUse = true;
                _input.UpdateSheredStates();
                Debug.Log(delta.ToString());
            }
            else
            {
                if (EditorBounds.Contains(location))
                {
                    Focus();
                }
            }
            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            bool isFocused = IsFocused;
            if (isFocused)
            {
                _input.UpdateKeyboardStates(this, key, true);
            }
            else
            {
                bool isMouseOver = IsMouseOver;
                if (isMouseOver)
                {
                    Focus();
                }
            }
            bool flag = base.OnKeyDown(key);
            return flag || InputActions.Process(Editor.Instance, this, key);
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            bool isFocused = IsFocused;
            if (isFocused)
            {
                _input.UpdateKeyboardStates(this, key, false);
            }
            base.OnKeyUp(key);
        }

        /// <summary>
        /// Called when left mouse button goes down (on press).
        /// </summary>
        protected virtual void OnLeftMouseButtonDown(){}

        /// <summary>
        /// Called when left mouse button goes up (on release).
        /// </summary>
        protected virtual void OnLeftMouseButtonUp(){}

        /// <summary>
        /// Called when right mouse button goes down (on press).
        /// </summary>
        protected virtual void OnRightMouseButtonDown(){}

        /// <summary>
        /// Called when right mouse button goes up (on release).
        /// </summary>
        protected virtual void OnRightMouseButtonUp(){}

        /// <summary>
        /// Called when middle mouse button goes down (on press).
        /// </summary>
        protected virtual void OnMiddleMouseButtonDown(){}

        /// <summary>
        /// Called when middle mouse button goes up (on release).
        /// </summary>
        protected virtual void OnMiddleMouseButtonUp(){}
    }
}
