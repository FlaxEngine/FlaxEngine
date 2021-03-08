// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Options;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Editor viewports base class.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.RenderOutputControl" />
    public class EditorViewport : RenderOutputControl
    {
        /// <summary>
        /// Gathered input data.
        /// </summary>
        public struct Input
        {
            /// <summary>
            /// The is panning state.
            /// </summary>
            public bool IsPanning;

            /// <summary>
            /// The is rotating state.
            /// </summary>
            public bool IsRotating;

            /// <summary>
            /// The is moving state.
            /// </summary>
            public bool IsMoving;

            /// <summary>
            /// The is zooming state.
            /// </summary>
            public bool IsZooming;

            /// <summary>
            /// The is orbiting state.
            /// </summary>
            public bool IsOrbiting;

            /// <summary>
            /// The is control down flag.
            /// </summary>
            public bool IsControlDown;

            /// <summary>
            /// The is shift down flag.
            /// </summary>
            public bool IsShiftDown;

            /// <summary>
            /// The is alt down flag.
            /// </summary>
            public bool IsAltDown;

            /// <summary>
            /// The is mouse right down flag.
            /// </summary>
            public bool IsMouseRightDown;

            /// <summary>
            /// The is mouse middle down flag.
            /// </summary>
            public bool IsMouseMiddleDown;

            /// <summary>
            /// The is mouse left down flag.
            /// </summary>
            public bool IsMouseLeftDown;

            /// <summary>
            /// The mouse wheel delta.
            /// </summary>
            public float MouseWheelDelta;

            /// <summary>
            /// Gets a value indicating whether use is controlling mouse.
            /// </summary>
            public bool IsControllingMouse => IsMouseMiddleDown || IsMouseRightDown || (IsAltDown && IsMouseLeftDown) || Mathf.Abs(MouseWheelDelta) > 0.1f;

            /// <summary>
            /// Gathers input from the specified window.
            /// </summary>
            /// <param name="window">The window.</param>
            /// <param name="useMouse">True if use mouse input, otherwise will skip mouse.</param>
            public void Gather(Window window, bool useMouse)
            {
                IsControlDown = window.GetKey(KeyboardKeys.Control);
                IsShiftDown = window.GetKey(KeyboardKeys.Shift);
                IsAltDown = window.GetKey(KeyboardKeys.Alt);

                IsMouseRightDown = useMouse && window.GetMouseButton(MouseButton.Right);
                IsMouseMiddleDown = useMouse && window.GetMouseButton(MouseButton.Middle);
                IsMouseLeftDown = useMouse && window.GetMouseButton(MouseButton.Left);
            }

            /// <summary>
            /// Clears the data.
            /// </summary>
            public void Clear()
            {
                IsControlDown = false;
                IsShiftDown = false;
                IsAltDown = false;

                IsMouseRightDown = false;
                IsMouseMiddleDown = false;
                IsMouseLeftDown = false;
            }
        }

        /// <summary>
        /// The FPS camera filtering frames count (how much frames we want to keep in the buffer to calculate the avg. delta currently hardcoded).
        /// </summary>
        public const int FpsCameraFilteringFrames = 3;

        /// <summary>
        /// The speed widget button.
        /// </summary>
        protected ViewportWidgetButton _speedWidget;

        private float _mouseSensitivity;
        private float _movementSpeed;
        private float _mouseAccelerationScale;
        private bool _useMouseFiltering;
        private bool _useMouseAcceleration;

        // Input

        private bool _isControllingMouse;
        private int _deltaFilteringStep;
        private Vector2 _startPos;
        private Vector2 _mouseDeltaRightLast;
        private Vector2[] _deltaFilteringBuffer = new Vector2[FpsCameraFilteringFrames];

        /// <summary>
        /// The previous input (from the previous update).
        /// </summary>
        protected Input _prevInput;

        /// <summary>
        /// The input data (from the current frame).
        /// </summary>
        protected Input _input;

        /// <summary>
        /// The view mouse position.
        /// </summary>
        protected Vector2 _viewMousePos;

        /// <summary>
        /// The mouse delta (right button down).
        /// </summary>
        protected Vector2 _mouseDeltaRight;

        /// <summary>
        /// The mouse delta (left button down).
        /// </summary>
        protected Vector2 _mouseDeltaLeft;

        // Camera

        private ViewportCamera _camera;
        private float _yaw;
        private float _pitch;
        private float _fieldOfView;
        private float _nearPlane;
        private float _farPlane;
        private float _orthoSize = 1.0f;
        private bool _isOrtho = false;
        private float _wheelMovementChangeDeltaSum = 0;
        private bool _invertPanning;

        /// <summary>
        /// Speed of the mouse.
        /// </summary>
        public float MouseSpeed = 1;

        /// <summary>
        /// Speed of the mouse wheel zooming.
        /// </summary>
        public float MouseWheelZoomSpeedFactor = 1;

        /// <summary>
        /// Gets or sets the camera movement speed.
        /// </summary>
        public float MovementSpeed
        {
            get => _movementSpeed;
            set
            {
                for (int i = 0; i < EditorViewportCameraSpeedValues.Length; i++)
                {
                    if (Math.Abs(value - EditorViewportCameraSpeedValues[i]) < 0.001f)
                    {
                        _movementSpeed = EditorViewportCameraSpeedValues[i];
                        if (_speedWidget != null)
                            _speedWidget.Text = _movementSpeed.ToString();
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Gets the mouse movement delta for the right button (user press and move).
        /// </summary>
        public Vector2 MouseDeltaRight => _mouseDeltaRight;

        /// <summary>
        /// Gets the mouse movement delta for the left button (user press and move).
        /// </summary>
        public Vector2 MouseDeltaLeft => _mouseDeltaLeft;

        /// <summary>
        /// Camera's pitch angle clamp range (in degrees).
        /// </summary>
        public Vector2 CamPitchAngles = new Vector2(-88, 88);

        /// <summary>
        /// Gets the view transform.
        /// </summary>
        public Transform ViewTransform
        {
            get => new Transform(ViewPosition, ViewOrientation);
            set
            {
                ViewPosition = value.Translation;
                ViewOrientation = value.Orientation;
            }
        }

        /// <summary>
        /// Gets or sets the view position.
        /// </summary>
        public Vector3 ViewPosition { get; set; }

        /// <summary>
        /// Gets or sets the view orientation.
        /// </summary>
        public Quaternion ViewOrientation
        {
            get => Quaternion.RotationYawPitchRoll(_yaw * Mathf.DegreesToRadians, _pitch * Mathf.DegreesToRadians, 0);
            set => EulerAngles = value.EulerAngles;
        }

        /// <summary>
        /// Gets or sets the view direction vector.
        /// </summary>
        public Vector3 ViewDirection
        {
            get => Vector3.Forward * ViewOrientation;
            set
            {
                Vector3 right = Vector3.Cross(value, Vector3.Up);
                Vector3 up = Vector3.Cross(right, value);
                ViewOrientation = Quaternion.LookRotation(value, up);
            }
        }

        /// <summary>
        /// Gets or sets the view ray (position and direction).
        /// </summary>
        public Ray ViewRay
        {
            get => new Ray(ViewPosition, ViewDirection);
            set
            {
                ViewPosition = value.Position;
                ViewDirection = value.Direction;
            }
        }

        /// <summary>
        /// Gets or sets the yaw angle (in degrees).
        /// </summary>
        public float Yaw
        {
            get => _yaw;
            set => _yaw = value;
        }

        /// <summary>
        /// Gets or sets the pitch angle (in degrees).
        /// </summary>
        public float Pitch
        {
            get => _pitch;
            set => _pitch = Mathf.Clamp(value, CamPitchAngles.X, CamPitchAngles.Y);
        }

        /// <summary>
        /// Gets or sets the absolute mouse position (normalized, not in pixels). Yaw is X, Pitch is Y.
        /// </summary>
        public Vector2 YawPitch
        {
            get => new Vector2(_yaw, _pitch);
            set
            {
                Yaw = value.X;
                Pitch = value.Y;
            }
        }

        /// <summary>
        /// Gets or sets the euler angles (pitch, yaw, roll).
        /// </summary>
        public Vector3 EulerAngles
        {
            get => new Vector3(_pitch, _yaw, 0);
            set
            {
                Pitch = value.X;
                Yaw = value.Y;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this viewport has loaded dependant assets.
        /// </summary>
        public virtual bool HasLoadedAssets => true;

        /// <summary>
        /// The 'View' widget button context menu.
        /// </summary>
        public ContextMenu ViewWidgetButtonMenu;

        /// <summary>
        /// The 'View' widget 'Show' category context menu.
        /// </summary>
        public ContextMenu ViewWidgetShowMenu;

        /// <summary>
        /// Gets or sets the viewport camera controller.
        /// </summary>
        public ViewportCamera ViewportCamera
        {
            get => _camera;
            set
            {
                if (_camera != null)
                    _camera.Viewport = null;

                _camera = value;

                if (_camera != null)
                    _camera.Viewport = this;
            }
        }

        /// <summary>
        /// Gets or sets the camera near clipping plane.
        /// </summary>
        public float NearPlane
        {
            get => _nearPlane;
            set => _nearPlane = value;
        }

        /// <summary>
        /// Gets or sets the camera far clipping plane.
        /// </summary>
        public float FarPlane
        {
            get => _farPlane;
            set => _farPlane = value;
        }

        /// <summary>
        /// Gets or sets the camera field of view (in degrees).
        /// </summary>
        public float FieldOfView
        {
            get => _fieldOfView;
            set => _fieldOfView = value;
        }

        /// <summary>
        /// Gets or sets the camera orthographic size scale (if camera uses orthographic mode).
        /// </summary>
        public float OrthographicScale
        {
            get => _orthoSize;
            set => _orthoSize = value;
        }

        /// <summary>
        /// Gets or sets the camera orthographic mode (otherwise uses perspective projection).
        /// </summary>
        public bool UseOrthographicProjection
        {
            get => _isOrtho;
            set => _isOrtho = value;
        }

        /// <summary>
        /// Gets or sets if the panning direction is inverted.
        /// </summary>
        public bool InvertPanning
        {
            get => _invertPanning;
            set => _invertPanning = value;
        }

        /// <summary>
        /// The input actions collection to processed during user input.
        /// </summary>
        public InputActionsContainer InputActions = new InputActionsContainer();

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorViewport"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="camera">The camera controller.</param>
        /// <param name="useWidgets">Enable/disable viewport widgets.</param>
        public EditorViewport(SceneRenderTask task, ViewportCamera camera, bool useWidgets)
        : base(task)
        {
            _mouseAccelerationScale = 0.1f;
            _useMouseFiltering = false;
            _useMouseAcceleration = false;
            _camera = camera;
            if (_camera != null)
                _camera.Viewport = this;

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;

            // Setup options
            {
                var options = Editor.Instance.Options.Options;
                _movementSpeed = options.Viewport.DefaultMovementSpeed;
                _nearPlane = options.Viewport.DefaultNearPlane;
                _farPlane = options.Viewport.DefaultFarPlane;
                _fieldOfView = options.Viewport.DefaultFieldOfView;
                _invertPanning = options.Viewport.DefaultInvertPanning;

                Editor.Instance.Options.OptionsChanged += OnEditorOptionsChanged;
                OnEditorOptionsChanged(options);
            }

            if (useWidgets)
            {
                // Camera speed widget
                var camSpeed = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                var camSpeedCM = new ContextMenu();
                var camSpeedButton = new ViewportWidgetButton(_movementSpeed.ToString(), Editor.Instance.Icons.ArrowRightBorder16, camSpeedCM)
                {
                    Tag = this,
                    TooltipText = "Camera speed scale"
                };
                _speedWidget = camSpeedButton;
                for (int i = 0; i < EditorViewportCameraSpeedValues.Length; i++)
                {
                    var v = EditorViewportCameraSpeedValues[i];
                    var button = camSpeedCM.AddButton(v.ToString());
                    button.Tag = v;
                }
                camSpeedCM.ButtonClicked += button => MovementSpeed = (float)button.Tag;
                camSpeedCM.VisibleChanged += WidgetCamSpeedShowHide;
                camSpeedButton.Parent = camSpeed;
                camSpeed.Parent = this;

                // View mode widget
                var viewMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                ViewWidgetButtonMenu = new ContextMenu();
                var viewModeButton = new ViewportWidgetButton("View", SpriteHandle.Invalid, ViewWidgetButtonMenu)
                {
                    TooltipText = "View properties",
                    Parent = viewMode
                };
                viewMode.Parent = this;

                // Show
                {
                    ViewWidgetShowMenu = ViewWidgetButtonMenu.AddChildMenu("Show").ContextMenu;

                    // Show FPS
                    {
                        InitFpsCounter();
                        _showFpsButon = ViewWidgetShowMenu.AddButton("FPS Counter", () => ShowFpsCounter = !ShowFpsCounter);
                    }
                }

                // View Flags
                {
                    var viewFlags = ViewWidgetButtonMenu.AddChildMenu("View Flags").ContextMenu;
                    viewFlags.AddButton("Reset flags", () => Task.ViewFlags = ViewFlags.DefaultEditor).Icon = Editor.Instance.Icons.Rotate16;
                    viewFlags.AddSeparator();
                    for (int i = 0; i < EditorViewportViewFlagsValues.Length; i++)
                    {
                        var v = EditorViewportViewFlagsValues[i];
                        var button = viewFlags.AddButton(v.Name);
                        button.Tag = v.Mode;
                    }
                    viewFlags.ButtonClicked += button =>
                    {
                        if (button.Tag != null)
                            Task.ViewFlags ^= (ViewFlags)button.Tag;
                    };
                    viewFlags.VisibleChanged += WidgetViewFlagsShowHide;
                }

                // Debug View
                {
                    var debugView = ViewWidgetButtonMenu.AddChildMenu("Debug View").ContextMenu;
                    for (int i = 0; i < EditorViewportViewModeValues.Length; i++)
                    {
                        var v = EditorViewportViewModeValues[i];
                        var button = debugView.AddButton(v.Name);
                        button.Tag = v.Mode;
                    }
                    debugView.ButtonClicked += button => Task.ViewMode = (ViewMode)button.Tag;
                    debugView.VisibleChanged += WidgetViewModeShowHide;
                }

                ViewWidgetButtonMenu.AddSeparator();

                // Orthographic
                {
                    var ortho = ViewWidgetButtonMenu.AddButton("Orthographic");
                    var orthoValue = new CheckBox(90, 2, _isOrtho)
                    {
                        Parent = ortho
                    };
                    orthoValue.StateChanged += checkBox =>
                    {
                        if (checkBox.Checked != _isOrtho)
                        {
                            _isOrtho = checkBox.Checked;
                            ViewWidgetButtonMenu.Hide();
                        }
                    };
                    ViewWidgetButtonMenu.VisibleChanged += control => orthoValue.Checked = _isOrtho;
                }

                // Cara Orientation
                {
                    var cameraView = ViewWidgetButtonMenu.AddChildMenu("Orientation").ContextMenu;
                    for (int i = 0; i < EditorViewportCameraOrientationValues.Length; i++)
                    {
                        var co = EditorViewportCameraOrientationValues[i];
                        var button = cameraView.AddButton(co.Name);
                        button.Tag = co.Orientation;
                    }
                    cameraView.ButtonClicked += button => ViewOrientation = Quaternion.Euler((Vector3)button.Tag);
                }

                // Field of View
                {
                    var fov = ViewWidgetButtonMenu.AddButton("Field Of View");
                    var fovValue = new FloatValueBox(1, 90, 2, 70.0f, 35.0f, 160.0f, 0.1f)
                    {
                        Parent = fov
                    };

                    fovValue.ValueChanged += () => _fieldOfView = fovValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control =>
                    {
                        fov.Visible = !_isOrtho;
                        fovValue.Value = _fieldOfView;
                    };
                }

                // Ortho Scale
                {
                    var orthoSize = ViewWidgetButtonMenu.AddButton("Ortho Scale");
                    var orthoSizeValue = new FloatValueBox(_orthoSize, 90, 2, 70.0f, 0.001f, 100000.0f, 0.01f)
                    {
                        Parent = orthoSize
                    };

                    orthoSizeValue.ValueChanged += () => _orthoSize = orthoSizeValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control =>
                    {
                        orthoSize.Visible = _isOrtho;
                        orthoSizeValue.Value = _orthoSize;
                    };
                }

                // Near Plane
                {
                    var nearPlane = ViewWidgetButtonMenu.AddButton("Near Plane");
                    var nearPlaneValue = new FloatValueBox(2.0f, 90, 2, 70.0f, 0.001f, 1000.0f)
                    {
                        Parent = nearPlane
                    };
                    nearPlaneValue.ValueChanged += () => _nearPlane = nearPlaneValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => nearPlaneValue.Value = _nearPlane;
                }

                // Far Plane
                {
                    var farPlane = ViewWidgetButtonMenu.AddButton("Far Plane");
                    var farPlaneValue = new FloatValueBox(1000, 90, 2, 70.0f, 10.0f)
                    {
                        Parent = farPlane
                    };
                    farPlaneValue.ValueChanged += () => _farPlane = farPlaneValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => farPlaneValue.Value = _farPlane;
                }

                // Brightness
                {
                    var brightness = ViewWidgetButtonMenu.AddButton("Brightness");
                    var brightnessValue = new FloatValueBox(1.0f, 90, 2, 70.0f, 0.001f, 10.0f, 0.001f)
                    {
                        Parent = brightness
                    };
                    brightnessValue.ValueChanged += () => Brightness = brightnessValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => brightnessValue.Value = Brightness;
                }

                // Resolution
                {
                    var resolution = ViewWidgetButtonMenu.AddButton("Resolution");
                    var resolutionValue = new FloatValueBox(1.0f, 90, 2, 70.0f, 0.1f, 4.0f, 0.001f)
                    {
                        Parent = resolution
                    };
                    resolutionValue.ValueChanged += () => ResolutionScale = resolutionValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => resolutionValue.Value = ResolutionScale;
                }

                // Invert Panning
                {
                    var invert = ViewWidgetButtonMenu.AddButton("Invert Panning");
                    var invertValue = new CheckBox(90, 2, _invertPanning)
                    {
                        Parent = invert
                    };

                    invertValue.StateChanged += checkBox =>
                    {
                        if (checkBox.Checked != _invertPanning)
                        {
                            _invertPanning = checkBox.Checked;
                        }
                    };
                    ViewWidgetButtonMenu.VisibleChanged += control => invertValue.Checked = _invertPanning;
                }
            }

            // Link for task event
            task.Begin += OnRenderBegin;
        }

        private void OnEditorOptionsChanged(EditorOptions options)
        {
            _mouseSensitivity = options.Viewport.MouseSensitivity;
        }

        private void OnRenderBegin(RenderTask task, GPUContext context)
        {
            var sceneTask = (SceneRenderTask)task;

            var view = sceneTask.View;
            CopyViewData(ref view);
            sceneTask.View = view;
        }

        #region FPS Counter

        private class FpsCounter : Control
        {
            public FpsCounter(float x, float y)
            : base(x, y, 64, 32)
            {
            }

            public override void Draw()
            {
                base.Draw();

                int fps = Engine.FramesPerSecond;
                Color color = Color.Green;
                if (fps < 13)
                    color = Color.Red;
                else if (fps < 22)
                    color = Color.Yellow;
                var text = string.Format("FPS: {0}", fps);
                var font = Style.Current.FontMedium;
                Render2D.DrawText(font, text, new Rectangle(Vector2.One, Size), Color.Black);
                Render2D.DrawText(font, text, new Rectangle(Vector2.Zero, Size), color);
            }
        }

        private FpsCounter _fpsCounter;
        private ContextMenuButton _showFpsButon;

        /// <summary>
        /// Gets or sets a value indicating whether show or hide FPS counter.
        /// </summary>
        public bool ShowFpsCounter
        {
            get => _fpsCounter.Visible;
            set
            {
                _fpsCounter.Visible = value;
                _fpsCounter.Enabled = value;
                _showFpsButon.Icon = value ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
            }
        }

        private void InitFpsCounter()
        {
            _fpsCounter = new FpsCounter(10, ViewportWidgetsContainer.WidgetsHeight + 14)
            {
                Visible = false,
                Enabled = false,
                Parent = this
            };
        }

        #endregion

        /// <summary>
        /// Takes the screenshot of the current viewport.
        /// </summary>
        /// <param name="path">The output file path. Set null to use default value.</param>
        public void TakeScreenshot(string path = null)
        {
            Screenshot.Capture(Task, path);
        }

        /// <summary>
        /// Copies the render view data to <see cref="RenderView"/> structure.
        /// </summary>
        /// <param name="view">The view.</param>
        public void CopyViewData(ref RenderView view)
        {
            // Create matrices
            CreateProjectionMatrix(out view.Projection);
            CreateViewMatrix(out view.View);

            // Copy data
            view.Position = ViewPosition;
            view.Direction = ViewDirection;
            view.Near = _nearPlane;
            view.Far = _farPlane;

            view.UpdateCachedData();
        }

        /// <summary>
        /// Gets the input state data.
        /// </summary>
        /// <param name="input">The input.</param>
        public void GetInput(out Input input)
        {
            input = _input;
        }

        /// <summary>
        /// Gets the input state data (from the previous update).
        /// </summary>
        /// <param name="input">The input.</param>
        public void GetPrevInput(out Input input)
        {
            input = _prevInput;
        }

        /// <summary>
        /// Creates the projection matrix.
        /// </summary>
        /// <param name="result">The result.</param>
        protected virtual void CreateProjectionMatrix(out Matrix result)
        {
            if (_isOrtho)
            {
                Matrix.Ortho(Width * _orthoSize, Height * _orthoSize, _nearPlane, _farPlane, out result);
            }
            else
            {
                float aspect = Width / Height;
                Matrix.PerspectiveFov(_fieldOfView * Mathf.DegreesToRadians, aspect, _nearPlane, _farPlane, out result);
            }
        }

        /// <summary>
        /// Creates the view matrix.
        /// </summary>
        /// <param name="result">The result.</param>
        protected virtual void CreateViewMatrix(out Matrix result)
        {
            Vector3 position = ViewPosition;
            Vector3 direction = ViewDirection;
            Vector3 target = position + direction;
            Vector3 right = Vector3.Normalize(Vector3.Cross(Vector3.Up, direction));
            Vector3 up = Vector3.Normalize(Vector3.Cross(direction, right));
            Matrix.LookAt(ref position, ref target, ref up, out result);
        }

        /// <summary>
        /// Gets the mouse ray.
        /// </summary>
        public Ray MouseRay
        {
            get
            {
                if (IsMouseOver)
                    return ConvertMouseToRay(ref _viewMousePos);
                return new Ray(Vector3.Maximum, Vector3.Up);
            }
        }

        /// <summary>
        /// Converts the mouse position to the ray (in world space of the viewport).
        /// </summary>
        /// <param name="mousePosition">The mouse position.</param>
        /// <returns>The result ray.</returns>
        public Ray ConvertMouseToRay(ref Vector2 mousePosition)
        {
            // Prepare
            var viewport = new FlaxEngine.Viewport(0, 0, Width, Height);
            CreateProjectionMatrix(out var p);
            CreateViewMatrix(out var v);
            Matrix.Multiply(ref v, ref p, out var ivp);
            ivp.Invert();

            // Create near and far points
            Vector3 nearPoint = new Vector3(mousePosition, 0.0f);
            Vector3 farPoint = new Vector3(mousePosition, 1.0f);
            viewport.Unproject(ref nearPoint, ref ivp, out nearPoint);
            viewport.Unproject(ref farPoint, ref ivp, out farPoint);

            // Create direction vector
            Vector3 direction = farPoint - nearPoint;
            direction.Normalize();

            return new Ray(nearPoint, direction);
        }

        /// <summary>
        /// Called when mouse control begins.
        /// </summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseBegin(Window win)
        {
            _wheelMovementChangeDeltaSum = 0;

            // Hide cursor and start tracking mouse movement
            win.StartTrackingMouse(false);
            win.Cursor = CursorType.Hidden;

            // Center mouse position
            //_viewMousePos = Center;
            //win.MousePosition = PointToWindow(_viewMousePos);
        }

        /// <summary>
        /// Called when mouse control ends.
        /// </summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseEnd(Window win)
        {
            // Restore cursor and stop tracking mouse movement
            win.Cursor = CursorType.Default;
            win.EndTrackingMouse();
        }

        /// <summary>
        /// Called when left mouse button goes down (on press).
        /// </summary>
        protected virtual void OnLeftMouseButtonDown()
        {
            _startPos = _viewMousePos;
        }

        /// <summary>
        /// Called when left mouse button goes up (on release).
        /// </summary>
        protected virtual void OnLeftMouseButtonUp()
        {
        }

        /// <summary>
        /// Called when right mouse button goes down (on press).
        /// </summary>
        protected virtual void OnRightMouseButtonDown()
        {
            _startPos = _viewMousePos;
        }

        /// <summary>
        /// Called when right mouse button goes up (on release).
        /// </summary>
        protected virtual void OnRightMouseButtonUp()
        {
        }

        /// <summary>
        /// Called when middle mouse button goes down (on press).
        /// </summary>
        protected virtual void OnMiddleMouseButtonDown()
        {
            _startPos = _viewMousePos;
        }

        /// <summary>
        /// Called when middle mouse button goes up (on release).
        /// </summary>
        protected virtual void OnMiddleMouseButtonUp()
        {
        }

        /// <summary>
        /// Updates the view.
        /// </summary>
        /// <param name="dt">The delta time (in seconds).</param>
        /// <param name="moveDelta">The move delta (scaled).</param>
        /// <param name="mouseDelta">The mouse delta (scaled).</param>
        /// <param name="centerMouse">True if center mouse after the update.</param>
        protected virtual void UpdateView(float dt, ref Vector3 moveDelta, ref Vector2 mouseDelta, out bool centerMouse)
        {
            centerMouse = true;
            _camera?.UpdateView(dt, ref moveDelta, ref mouseDelta, out centerMouse);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Update camera
            bool useMovementSpeed = false;
            if (_camera != null)
            {
                _camera.Update(deltaTime);
                useMovementSpeed = _camera.UseMovementSpeed;

                if (_speedWidget != null)
                    _speedWidget.Parent.Visible = useMovementSpeed;
            }

            // Get parent window
            var win = (WindowRootControl)Root;

            // Get current mouse position in the view
            _viewMousePos = PointFromWindow(win.MousePosition);

            // Update input
            {
                // Get input buttons and keys (skip if viewport has no focus or mouse is over a child control)
                bool useMouse = Mathf.IsInRange(_viewMousePos.X, 0, Width) && Mathf.IsInRange(_viewMousePos.Y, 0, Height);
                _prevInput = _input;
                if (ContainsFocus && GetChildAt(_viewMousePos, c => c.Visible) == null)
                    _input.Gather(win.Window, useMouse);
                else
                    _input.Clear();

                // Track controlling mouse state change
                bool wasControllingMouse = _prevInput.IsControllingMouse;
                _isControllingMouse = _input.IsControllingMouse;
                if (wasControllingMouse != _isControllingMouse)
                {
                    if (_isControllingMouse)
                        OnControlMouseBegin(win.Window);
                    else
                        OnControlMouseEnd(win.Window);
                }

                // Track mouse buttons state change
                if (!_prevInput.IsMouseLeftDown && _input.IsMouseLeftDown)
                    OnLeftMouseButtonDown();
                else if (_prevInput.IsMouseLeftDown && !_input.IsMouseLeftDown)
                    OnLeftMouseButtonUp();
                //
                if (!_prevInput.IsMouseRightDown && _input.IsMouseRightDown)
                    OnRightMouseButtonDown();
                else if (_prevInput.IsMouseRightDown && !_input.IsMouseRightDown)
                    OnRightMouseButtonUp();
                //
                if (!_prevInput.IsMouseMiddleDown && _input.IsMouseMiddleDown)
                    OnMiddleMouseButtonDown();
                else if (_prevInput.IsMouseMiddleDown && !_input.IsMouseMiddleDown)
                    OnMiddleMouseButtonUp();
            }

            // Get clamped delta time (more stable during lags)
            var dt = Math.Min(Time.UnscaledDeltaTime, 1.0f);

            // Check if update mouse
            Vector2 size = Size;
            var options = Editor.Instance.Options.Options.Input;
            if (_isControllingMouse)
            {
                // Gather input
                {
                    bool isAltDown = _input.IsAltDown;
                    bool lbDown = _input.IsMouseLeftDown;
                    bool mbDown = _input.IsMouseMiddleDown;
                    bool rbDown = _input.IsMouseRightDown;
                    bool wheelInUse = Math.Abs(_input.MouseWheelDelta) > Mathf.Epsilon;

                    _input.IsPanning = !isAltDown && mbDown && !rbDown;
                    _input.IsRotating = !isAltDown && !mbDown && rbDown;
                    _input.IsMoving = !isAltDown && mbDown && rbDown;
                    _input.IsZooming = wheelInUse && !_input.IsShiftDown;
                    _input.IsOrbiting = isAltDown && lbDown && !mbDown && !rbDown;

                    // Control move speed with RMB+Wheel
                    if (useMovementSpeed && _input.IsMouseRightDown && wheelInUse)
                    {
                        float step = 4.0f;
                        _wheelMovementChangeDeltaSum += _input.MouseWheelDelta;
                        int camValueIndex = -1;
                        for (int i = 0; i < EditorViewportCameraSpeedValues.Length; i++)
                        {
                            if (Mathf.NearEqual(EditorViewportCameraSpeedValues[i], _movementSpeed))
                            {
                                camValueIndex = i;
                                break;
                            }
                        }
                        if (camValueIndex != -1)
                        {
                            if (_wheelMovementChangeDeltaSum >= step)
                            {
                                _wheelMovementChangeDeltaSum -= step;
                                MovementSpeed = EditorViewportCameraSpeedValues[Mathf.Min(camValueIndex + 1, EditorViewportCameraSpeedValues.Length - 1)];
                            }
                            else if (_wheelMovementChangeDeltaSum <= -step)
                            {
                                _wheelMovementChangeDeltaSum += step;
                                MovementSpeed = EditorViewportCameraSpeedValues[Mathf.Max(camValueIndex - 1, 0)];
                            }
                        }
                    }
                }

                // Get input movement
                Vector3 moveDelta = Vector3.Zero;
                if (win.GetKey(options.Forward.Key))
                {
                    moveDelta += Vector3.Forward;
                }
                if (win.GetKey(options.Backward.Key))
                {
                    moveDelta += Vector3.Backward;
                }
                if (win.GetKey(options.Right.Key))
                {
                    moveDelta += Vector3.Right;
                }
                if (win.GetKey(options.Left.Key))
                {
                    moveDelta += Vector3.Left;
                }
                if (win.GetKey(options.Up.Key))
                {
                    moveDelta += Vector3.Up;
                }
                if (win.GetKey(options.Down.Key))
                {
                    moveDelta += Vector3.Down;
                }
                moveDelta.Normalize(); // normalize direction
                moveDelta *= _movementSpeed;

                // Speed up or speed down
                if (_input.IsShiftDown)
                    moveDelta *= 4.0f;
                if (_input.IsControlDown)
                    moveDelta *= 0.3f;

                // Calculate smooth mouse delta not dependant on viewport size

                Vector2 offset = _viewMousePos - _startPos;
                if (_input.IsZooming && !_input.IsMouseRightDown && !_input.IsMouseLeftDown && !_input.IsMouseMiddleDown)
                {
                    offset = Vector2.Zero;
                }

                offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                _mouseDeltaRight = offset / size;
                _mouseDeltaRight.Y *= size.Y / size.X;

                Vector2 mouseDelta = Vector2.Zero;
                if (_useMouseFiltering)
                {
                    // Update delta filtering buffer
                    _deltaFilteringBuffer[_deltaFilteringStep] = _mouseDeltaRight;
                    _deltaFilteringStep++;

                    // If the step is too far, zero
                    if (_deltaFilteringStep == FpsCameraFilteringFrames)
                        _deltaFilteringStep = 0;

                    // Calculate filtered delta (avg)
                    for (int i = 0; i < FpsCameraFilteringFrames; i++)
                        mouseDelta += _deltaFilteringBuffer[i];

                    mouseDelta /= FpsCameraFilteringFrames;
                }
                else
                    mouseDelta = _mouseDeltaRight;

                if (_useMouseAcceleration)
                {
                    // Accelerate the delta
                    var currentDelta = mouseDelta;
                    mouseDelta += _mouseDeltaRightLast * _mouseAccelerationScale;
                    _mouseDeltaRightLast = currentDelta;
                }

                // Update
                moveDelta *= dt * (60.0f * 4.0f);
                mouseDelta *= 200.0f * MouseSpeed * _mouseSensitivity;
                UpdateView(dt, ref moveDelta, ref mouseDelta, out var centerMouse);

                // Move mouse back to the root position
                if (centerMouse && (_input.IsMouseRightDown || _input.IsMouseLeftDown || _input.IsMouseMiddleDown))
                {
                    Vector2 center = PointToWindow(_startPos);
                    win.MousePosition = center;
                }
            }
            else
            {
                _mouseDeltaRight = _mouseDeltaRightLast = Vector2.Zero;

                if (ContainsFocus)
                {
                    _input.IsPanning = false;
                    _input.IsRotating = false;
                    _input.IsMoving = true;
                    _input.IsZooming = false;
                    _input.IsOrbiting = false;

                    // Get input movement
                    Vector3 moveDelta = Vector3.Zero;
                    if (win.GetKey(KeyboardKeys.ArrowRight))
                    {
                        moveDelta += Vector3.Right;
                    }
                    if (win.GetKey(KeyboardKeys.ArrowLeft))
                    {
                        moveDelta += Vector3.Left;
                    }
                    if (win.GetKey(KeyboardKeys.ArrowUp))
                    {
                        moveDelta += Vector3.Up;
                    }
                    if (win.GetKey(KeyboardKeys.ArrowDown))
                    {
                        moveDelta += Vector3.Down;
                    }
                    moveDelta.Normalize();
                    moveDelta *= _movementSpeed;

                    // Update
                    moveDelta *= dt * (60.0f * 4.0f);
                    Vector2 mouseDelta = Vector2.Zero;
                    UpdateView(dt, ref moveDelta, ref mouseDelta, out _);
                }
            }
            if (_input.IsMouseLeftDown && false)
            {
                // Calculate smooth mouse delta not dependant on viewport size
                Vector2 offset = _viewMousePos - _startPos;
                offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                _mouseDeltaLeft = offset / size;
                _startPos = _viewMousePos;
            }
            else
            {
                _mouseDeltaLeft = Vector2.Zero;
            }

            _input.MouseWheelDelta = 0;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            Focus();

            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Vector2 location, float delta)
        {
            _input.MouseWheelDelta += delta;

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Base
            if (base.OnKeyDown(key))
                return true;

            // Custom input events
            return InputActions.Process(Editor.Instance, this, key);
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            ViewportWidgetsContainer.ArrangeWidgets(this);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Add overlay during debugger breakpoint hang
            if (Editor.Instance.Simulation.IsDuringBreakpointHang)
            {
                var bounds = new Rectangle(Vector2.Zero, Size);
                Render2D.FillRectangle(bounds, new Color(0.0f, 0.0f, 0.0f, 0.2f));
                Render2D.DrawText(Style.Current.FontLarge, "Debugger breakpoint hit...", bounds, Color.White, TextAlignment.Center, TextAlignment.Center);
            }
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();

            if (_isControllingMouse)
            {
                OnControlMouseEnd(RootWindow.Window);
                _isControllingMouse = false;
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Editor.Instance.Options.OptionsChanged -= OnEditorOptionsChanged;

            base.OnDestroy();
        }

        private struct CameraOrientation
        {
            public readonly string Name;
            public readonly Vector3 Orientation;

            public CameraOrientation(string name, Vector3 orientation)
            {
                Name = name;
                Orientation = orientation;
            }
        }

        private readonly CameraOrientation[] EditorViewportCameraOrientationValues =
        {
            new CameraOrientation("Front", new Vector3(0, 0, 0)),
            new CameraOrientation("Back", new Vector3(0, 180, 0)),
            new CameraOrientation("Left", new Vector3(0, -90, 0)),
            new CameraOrientation("Right", new Vector3(0, 90, 0)),
            new CameraOrientation("Top", new Vector3(90, 0, 0)),
            new CameraOrientation("Bottom", new Vector3(-90, 0, 0))
        };

        private readonly float[] EditorViewportCameraSpeedValues =
        {
            0.1f,
            0.25f,
            0.5f,
            1.0f,
            2.0f,
            4.0f,
            6.0f,
            8.0f,
            16.0f,
            32.0f,
            64.0f,
        };

        private struct ViewModeOptions
        {
            public readonly ViewMode Mode;
            public readonly string Name;

            public ViewModeOptions(ViewMode mode, string name)
            {
                Mode = mode;
                Name = name;
            }
        }

        private static readonly ViewModeOptions[] EditorViewportViewModeValues =
        {
            new ViewModeOptions(ViewMode.Default, "Default"),
            new ViewModeOptions(ViewMode.Unlit, "Unlit"),
            new ViewModeOptions(ViewMode.NoPostFx, "No PostFx"),
            new ViewModeOptions(ViewMode.Wireframe, "Wireframe"),
            new ViewModeOptions(ViewMode.LightBuffer, "Light Buffer"),
            new ViewModeOptions(ViewMode.Reflections, "Reflections Buffer"),
            new ViewModeOptions(ViewMode.Depth, "Depth Buffer"),
            new ViewModeOptions(ViewMode.Diffuse, "Diffuse"),
            new ViewModeOptions(ViewMode.Metalness, "Metalness"),
            new ViewModeOptions(ViewMode.Roughness, "Roughness"),
            new ViewModeOptions(ViewMode.Specular, "Specular"),
            new ViewModeOptions(ViewMode.SpecularColor, "Specular Color"),
            new ViewModeOptions(ViewMode.SubsurfaceColor, "Subsurface Color"),
            new ViewModeOptions(ViewMode.ShadingModel, "Shading Model"),
            new ViewModeOptions(ViewMode.Emissive, "Emissive Light"),
            new ViewModeOptions(ViewMode.Normals, "Normals"),
            new ViewModeOptions(ViewMode.AmbientOcclusion, "Ambient Occlusion"),
            new ViewModeOptions(ViewMode.MotionVectors, "Motion Vectors"),
            new ViewModeOptions(ViewMode.LightmapUVsDensity, "Lightmap UVs Density"),
            new ViewModeOptions(ViewMode.VertexColors, "Vertex Colors"),
        };

        private void WidgetCamSpeedShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;

            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b)
                {
                    var v = (float)b.Tag;
                    b.Icon = Mathf.Abs(MovementSpeed - v) < 0.001f
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
                }
            }
        }

        private void WidgetViewModeShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;

            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b)
                {
                    var v = (ViewMode)b.Tag;
                    b.Icon = Task.View.Mode == v
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
                }
            }
        }

        private struct ViewFlagOptions
        {
            public readonly ViewFlags Mode;
            public readonly string Name;

            public ViewFlagOptions(ViewFlags mode, string name)
            {
                Mode = mode;
                Name = name;
            }
        }

        private static readonly ViewFlagOptions[] EditorViewportViewFlagsValues =
        {
            new ViewFlagOptions(ViewFlags.AntiAliasing, "Anti Aliasing"),
            new ViewFlagOptions(ViewFlags.Shadows, "Shadows"),
            new ViewFlagOptions(ViewFlags.EditorSprites, "Editor Sprites"),
            new ViewFlagOptions(ViewFlags.Reflections, "Reflections"),
            new ViewFlagOptions(ViewFlags.SSR, "Screen Space Reflections"),
            new ViewFlagOptions(ViewFlags.AO, "Ambient Occlusion"),
            new ViewFlagOptions(ViewFlags.GI, "Global Illumination"),
            new ViewFlagOptions(ViewFlags.DirectionalLights, "Directional Lights"),
            new ViewFlagOptions(ViewFlags.PointLights, "Point Lights"),
            new ViewFlagOptions(ViewFlags.SpotLights, "Spot Lights"),
            new ViewFlagOptions(ViewFlags.SkyLights, "Sky Lights"),
            new ViewFlagOptions(ViewFlags.Fog, "Fog"),
            new ViewFlagOptions(ViewFlags.SpecularLight, "Specular Light"),
            new ViewFlagOptions(ViewFlags.Decals, "Decals"),
            new ViewFlagOptions(ViewFlags.CustomPostProcess, "Custom Post Process"),
            new ViewFlagOptions(ViewFlags.Bloom, "Bloom"),
            new ViewFlagOptions(ViewFlags.ToneMapping, "Tone Mapping"),
            new ViewFlagOptions(ViewFlags.EyeAdaptation, "Eye Adaptation"),
            new ViewFlagOptions(ViewFlags.CameraArtifacts, "Camera Artifacts"),
            new ViewFlagOptions(ViewFlags.LensFlares, "Lens Flares"),
            new ViewFlagOptions(ViewFlags.DepthOfField, "Depth of Field"),
            new ViewFlagOptions(ViewFlags.MotionBlur, "Motion Blur"),
            new ViewFlagOptions(ViewFlags.ContactShadows, "Contact Shadows"),
            new ViewFlagOptions(ViewFlags.PhysicsDebug, "Physics Debug"),
            new ViewFlagOptions(ViewFlags.DebugDraw, "Debug Draw"),
        };

        private void WidgetViewFlagsShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;

            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b.Tag != null)
                {
                    var v = (ViewFlags)b.Tag;
                    b.Icon = (Task.View.Flags & v) != 0
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
                }
            }
        }
    }
}
