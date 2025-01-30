// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Options;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

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
            /// The is alt down flag cached from the previous input. Used to make <see cref="IsControllingMouse"/> consistent when user releases Alt while orbiting with Alt+LMB.
            /// </summary>
            public bool WasAltDownBefore;

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
            public bool IsControllingMouse => IsMouseMiddleDown || IsMouseRightDown || ((IsAltDown || WasAltDownBefore) && IsMouseLeftDown) || Mathf.Abs(MouseWheelDelta) > 0.1f;

            /// <summary>
            /// Gathers input from the specified window.
            /// </summary>
            /// <param name="window">The window.</param>
            /// <param name="useMouse">True if use mouse input, otherwise will skip mouse.</param>
            /// <param name="prevInput">Previous input state.</param>
            public void Gather(Window window, bool useMouse, ref Input prevInput)
            {
                IsControlDown = window.GetKey(KeyboardKeys.Control);
                IsShiftDown = window.GetKey(KeyboardKeys.Shift);
                IsAltDown = window.GetKey(KeyboardKeys.Alt);
                WasAltDownBefore = prevInput.WasAltDownBefore || prevInput.IsAltDown;

                IsMouseRightDown = useMouse && window.GetMouseButton(MouseButton.Right);
                IsMouseMiddleDown = useMouse && window.GetMouseButton(MouseButton.Middle);
                IsMouseLeftDown = useMouse && window.GetMouseButton(MouseButton.Left);

                if (WasAltDownBefore && !IsMouseLeftDown && !IsAltDown)
                    WasAltDownBefore = false;
            }

            /// <summary>
            /// Clears the data.
            /// </summary>
            public void Clear()
            {
                IsControlDown = false;
                IsShiftDown = false;
                IsAltDown = false;
                WasAltDownBefore = false;

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
        /// The camera settings widget.
        /// </summary>
        protected ViewportWidgetsContainer _cameraWidget;

        /// <summary>
        /// The camera settings widget button.
        /// </summary>
        protected ViewportWidgetButton _cameraButton;

        /// <summary>
        /// The orthographic mode widget button.
        /// </summary>
        protected ViewportWidgetButton _orthographicModeButton;

        private readonly Editor _editor;

        private float _mouseSensitivity;
        private float _movementSpeed;
        private float _minMovementSpeed;
        private float _maxMovementSpeed;
        private float _mouseAccelerationScale;
        private bool _useMouseFiltering;
        private bool _useMouseAcceleration;

        // Input

        internal bool _disableInputUpdate;
        private bool _isControllingMouse, _isViewportControllingMouse, _wasVirtualMouseRightDown, _isVirtualMouseRightDown;
        private int _deltaFilteringStep;
        private Float2 _startPos;
        private Float2 _mouseDeltaLast;
        private Float2[] _deltaFilteringBuffer = new Float2[FpsCameraFilteringFrames];

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
        protected Float2 _viewMousePos;

        /// <summary>
        /// The mouse position delta.
        /// </summary>
        protected Float2 _mouseDelta;

        // Camera

        private ViewportCamera _camera;
        private float _yaw;
        private float _pitch;
        private float _fieldOfView;
        private float _nearPlane;
        private float _farPlane;
        private float _orthoSize;
        private bool _isOrtho;
        private bool _useCameraEasing;
        private float _cameraEasingDegree;
        private float _panningSpeed;
        private bool _relativePanning;
        private bool _invertPanning;

        private int _speedStep;
        private int _maxSpeedSteps;

        /// <summary>
        /// Speed of the mouse.
        /// </summary>
        public float MouseSpeed = 1;

        /// <summary>
        /// Speed of the mouse wheel zooming.
        /// </summary>
        public float MouseWheelZoomSpeedFactor = 1;

        /// <summary>
        /// Format of the text for the camera move speed.
        /// </summary>
        private string MovementSpeedTextFormat
        {
            get
            {
                if (Mathf.Abs(_movementSpeed - _maxMovementSpeed) < Mathf.Epsilon || Mathf.Abs(_movementSpeed - _minMovementSpeed) < Mathf.Epsilon)
                    return "{0:0.##}";
                if (_movementSpeed < 10.0f)
                    return "{0:0.00}";
                if (_movementSpeed < 100.0f)
                    return "{0:0.0}";
                return "{0:#}";
            }
        }

        /// <summary>
        /// Gets or sets the camera movement speed.
        /// </summary>
        public float MovementSpeed
        {
            get => _movementSpeed;
            set
            {
                _movementSpeed = value;

                if (_cameraButton != null)
                    _cameraButton.Text = string.Format(MovementSpeedTextFormat, _movementSpeed);
            }
        }

        /// <summary>
        /// Gets or sets the minimum camera movement speed.
        /// </summary>
        public float MinMovementSpeed
        {
            get => _minMovementSpeed;
            set => _minMovementSpeed = value;
        }

        /// <summary>
        /// Gets or sets the maximum camera movement speed.
        /// </summary>
        public float MaxMovementSpeed
        {
            get => _maxMovementSpeed;
            set => _maxMovementSpeed = value;
        }

        /// <summary>
        /// Gets or sets the camera easing mode.
        /// </summary>
        public bool UseCameraEasing
        {
            get => _useCameraEasing;
            set => _useCameraEasing = value;
        }

        /// <summary>
        /// Gets the mouse movement position delta (user press and move).
        /// </summary>
        public Float2 MousePositionDelta => _mouseDelta;

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
        public Float3 ViewDirection
        {
            get => Float3.Forward * ViewOrientation;
            set
            {
                var right = Mathf.Abs(Float3.Dot(value, Float3.Up)) < 1.0f - Mathf.Epsilon ? Float3.Cross(value, Float3.Up) : Float3.Forward;
                var up = Float3.Cross(right, value);
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
        /// Gets the bounding frustum of the current viewport camera.
        /// </summary>
        public BoundingFrustum ViewFrustum
        {
            get
            {
                Vector3 viewOrigin = Task.View.Origin;
                Float3 position = ViewPosition - viewOrigin;
                CreateViewMatrix(position, out var view);
                CreateProjectionMatrix(out var projection);
                Matrix.Multiply(ref view, ref projection, out var viewProjection);
                return new BoundingFrustum(ref viewProjection);
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
            set
            {
                var pitchLimit = _isOrtho ? new Float2(-90, 90) : new Float2(-88, 88);
                _pitch = Mathf.Clamp(value, pitchLimit.X, pitchLimit.Y);
            }
        }

        /// <summary>
        /// Gets or sets the absolute mouse position (normalized, not in pixels). Yaw is X, Pitch is Y.
        /// </summary>
        public Float2 YawPitch
        {
            get => new Float2(_yaw, _pitch);
            set
            {
                Yaw = value.X;
                Pitch = value.Y;
            }
        }

        /// <summary>
        /// Gets or sets the euler angles (pitch, yaw, roll).
        /// </summary>
        public Float3 EulerAngles
        {
            get => new Float3(_pitch, _yaw, 0);
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
        /// Gets or sets if the panning speed should be relative to the camera target.
        /// </summary>
        public bool RelativePanning
        {
            get => _relativePanning;
            set => _relativePanning = value;
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
        /// Gets or sets the camera panning speed.
        /// </summary>
        public float PanningSpeed
        {
            get => _panningSpeed;
            set => _panningSpeed = value;
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
            _editor = Editor.Instance;

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
                Editor.Instance.Options.OptionsChanged += OnEditorOptionsChanged;
                SetupViewportOptions();
            }

            // Initialize camera values from cache
            if (_editor.ProjectCache.TryGetCustomData("CameraMovementSpeedValue", out float cachedFloat))
                MovementSpeed = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("CameraMinMovementSpeedValue", out cachedFloat))
                _minMovementSpeed = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("CameraMaxMovementSpeedValue", out cachedFloat))
                _maxMovementSpeed = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("UseCameraEasingState", out bool cachedBool))
                _useCameraEasing = cachedBool;
            if (_editor.ProjectCache.TryGetCustomData("CameraPanningSpeedValue", out cachedFloat))
                _panningSpeed = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("CameraInvertPanningState", out cachedBool))
                _invertPanning = cachedBool;
            if (_editor.ProjectCache.TryGetCustomData("CameraRelativePanningState", out cachedBool))
                _relativePanning = cachedBool;
            if (_editor.ProjectCache.TryGetCustomData("CameraOrthographicState", out cachedBool))
                _isOrtho = cachedBool;
            if (_editor.ProjectCache.TryGetCustomData("CameraOrthographicSizeValue", out cachedFloat))
                _orthoSize = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("CameraFieldOfViewValue", out cachedFloat))
                _fieldOfView = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("CameraNearPlaneValue", out cachedFloat))
                _nearPlane = cachedFloat;
            if (_editor.ProjectCache.TryGetCustomData("CameraFarPlaneValue", out cachedFloat))
                _farPlane = cachedFloat;

            OnCameraMovementProgressChanged();

            if (useWidgets)
            {
                #region Camera settings widget

                var largestText = "Relative Panning";
                var textSize = Style.Current.FontMedium.MeasureText(largestText);
                var xLocationForExtras = textSize.X + 5;
                var cameraSpeedTextWidth = Style.Current.FontMedium.MeasureText("0.00").X;

                // Camera Settings Widget
                _cameraWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);

                // Camera Settings Menu
                var cameraCM = new ContextMenu();
                _cameraButton = new ViewportWidgetButton(string.Format(MovementSpeedTextFormat, _movementSpeed), Editor.Instance.Icons.Camera64, cameraCM, false, cameraSpeedTextWidth)
                {
                    Tag = this,
                    TooltipText = "Camera Settings",
                    Parent = _cameraWidget
                };
                _cameraWidget.Parent = this;

                // Orthographic/Perspective Mode Widget
                _orthographicModeButton = new ViewportWidgetButton(string.Empty, Editor.Instance.Icons.CamSpeed32, null, true)
                {
                    Checked = !_isOrtho,
                    TooltipText = "Toggle Orthographic/Perspective Mode",
                    Parent = _cameraWidget
                };
                _orthographicModeButton.Toggled += OnOrthographicModeToggled;

                // Camera Speed
                var camSpeedButton = cameraCM.AddButton("Camera Speed");
                camSpeedButton.CloseMenuOnClick = false;
                var camSpeedValue = new FloatValueBox(_movementSpeed, xLocationForExtras, 2, 70.0f, _minMovementSpeed, _maxMovementSpeed, 0.5f)
                {
                    Parent = camSpeedButton
                };

                camSpeedValue.ValueChanged += () => OnMovementSpeedChanged(camSpeedValue);
                cameraCM.VisibleChanged += control => camSpeedValue.Value = _movementSpeed;

                // Minimum & Maximum Camera Speed
                var minCamSpeedButton = cameraCM.AddButton("Min Cam Speed");
                minCamSpeedButton.CloseMenuOnClick = false;
                var minCamSpeedValue = new FloatValueBox(_minMovementSpeed, xLocationForExtras, 2, 70.0f, 0.05f, _maxMovementSpeed, 0.5f)
                {
                    Parent = minCamSpeedButton
                };
                var maxCamSpeedButton = cameraCM.AddButton("Max Cam Speed");
                maxCamSpeedButton.CloseMenuOnClick = false;
                var maxCamSpeedValue = new FloatValueBox(_maxMovementSpeed, xLocationForExtras, 2, 70.0f, _minMovementSpeed, 1000.0f, 0.5f)
                {
                    Parent = maxCamSpeedButton
                };

                minCamSpeedValue.ValueChanged += () =>
                {
                    OnMinMovementSpeedChanged(minCamSpeedValue);

                    maxCamSpeedValue.MinValue = minCamSpeedValue.Value;

                    if (Math.Abs(camSpeedValue.MinValue - minCamSpeedValue.Value) > Mathf.Epsilon)
                        camSpeedValue.MinValue = minCamSpeedValue.Value;
                };
                cameraCM.VisibleChanged += control => minCamSpeedValue.Value = _minMovementSpeed;
                maxCamSpeedValue.ValueChanged += () =>
                {
                    OnMaxMovementSpeedChanged(maxCamSpeedValue);

                    minCamSpeedValue.MaxValue = maxCamSpeedValue.Value;

                    if (Math.Abs(camSpeedValue.MaxValue - maxCamSpeedValue.Value) > Mathf.Epsilon)
                        camSpeedValue.MaxValue = maxCamSpeedValue.Value;
                };
                cameraCM.VisibleChanged += control => maxCamSpeedValue.Value = _maxMovementSpeed;

                // Camera Easing
                {
                    var useCameraEasing = cameraCM.AddButton("Camera Easing");
                    useCameraEasing.CloseMenuOnClick = false;
                    var useCameraEasingValue = new CheckBox(xLocationForExtras, 2, _useCameraEasing)
                    {
                        Parent = useCameraEasing
                    };

                    useCameraEasingValue.StateChanged += OnCameraEasingToggled;
                    cameraCM.VisibleChanged += control => useCameraEasingValue.Checked = _useCameraEasing;
                }

                // Panning Speed
                {
                    var panningSpeed = cameraCM.AddButton("Panning Speed");
                    panningSpeed.CloseMenuOnClick = false;
                    var panningSpeedValue = new FloatValueBox(_panningSpeed, xLocationForExtras, 2, 70.0f, 0.01f, 128.0f, 0.1f)
                    {
                        Parent = panningSpeed
                    };

                    panningSpeedValue.ValueChanged += () => OnPanningSpeedChanged(panningSpeedValue);
                    cameraCM.VisibleChanged += control =>
                    {
                        panningSpeed.Visible = !_relativePanning;
                        panningSpeedValue.Value = _panningSpeed;
                    };
                }

                // Relative Panning
                {
                    var relativePanning = cameraCM.AddButton("Relative Panning");
                    relativePanning.CloseMenuOnClick = false;
                    var relativePanningValue = new CheckBox(xLocationForExtras, 2, _relativePanning)
                    {
                        Parent = relativePanning
                    };

                    relativePanningValue.StateChanged += checkBox =>
                    {
                        if (checkBox.Checked != _relativePanning)
                        {
                            OnRelativePanningToggled(checkBox);
                            cameraCM.Hide();
                        }
                    };
                    cameraCM.VisibleChanged += control => relativePanningValue.Checked = _relativePanning;
                }

                // Invert Panning
                {
                    var invertPanning = cameraCM.AddButton("Invert Panning");
                    invertPanning.CloseMenuOnClick = false;
                    var invertPanningValue = new CheckBox(xLocationForExtras, 2, _invertPanning)
                    {
                        Parent = invertPanning
                    };

                    invertPanningValue.StateChanged += OnInvertPanningToggled;
                    cameraCM.VisibleChanged += control => invertPanningValue.Checked = _invertPanning;
                }

                cameraCM.AddSeparator();

                // Camera Viewpoints
                {
                    var cameraView = cameraCM.AddChildMenu("Viewpoints").ContextMenu;
                    for (int i = 0; i < CameraViewpointValues.Length; i++)
                    {
                        var co = CameraViewpointValues[i];
                        var button = cameraView.AddButton(co.Name);
                        button.Tag = co.Orientation;
                    }

                    cameraView.ButtonClicked += OnViewpointChanged;
                }

                // Orthographic Mode
                {
                    var ortho = cameraCM.AddButton("Orthographic");
                    ortho.CloseMenuOnClick = false;
                    var orthoValue = new CheckBox(xLocationForExtras, 2, _isOrtho)
                    {
                        Parent = ortho
                    };

                    orthoValue.StateChanged += checkBox =>
                    {
                        if (checkBox.Checked != _isOrtho)
                        {
                            OnOrthographicModeToggled(checkBox);
                            cameraCM.Hide();
                        }
                    };
                    cameraCM.VisibleChanged += control => orthoValue.Checked = _isOrtho;
                }

                // Field of View
                {
                    var fov = cameraCM.AddButton("Field Of View");
                    fov.CloseMenuOnClick = false;
                    var fovValue = new FloatValueBox(_fieldOfView, xLocationForExtras, 2, 70.0f, 35.0f, 160.0f, 0.1f)
                    {
                        Parent = fov
                    };

                    fovValue.ValueChanged += () => OnFieldOfViewChanged(fovValue);
                    cameraCM.VisibleChanged += control =>
                    {
                        fov.Visible = !_isOrtho;
                        fovValue.Value = _fieldOfView;
                    };
                }

                // Orthographic Scale
                {
                    var orthoSize = cameraCM.AddButton("Ortho Scale");
                    orthoSize.CloseMenuOnClick = false;
                    var orthoSizeValue = new FloatValueBox(_orthoSize, xLocationForExtras, 2, 70.0f, 0.001f, 100000.0f, 0.01f)
                    {
                        Parent = orthoSize
                    };

                    orthoSizeValue.ValueChanged += () => OnOrthographicSizeChanged(orthoSizeValue);
                    cameraCM.VisibleChanged += control =>
                    {
                        orthoSize.Visible = _isOrtho;
                        orthoSizeValue.Value = _orthoSize;
                    };
                }

                // Near Plane
                {
                    var nearPlane = cameraCM.AddButton("Near Plane");
                    nearPlane.CloseMenuOnClick = false;
                    var nearPlaneValue = new FloatValueBox(_nearPlane, xLocationForExtras, 2, 70.0f, 0.001f, 1000.0f)
                    {
                        Parent = nearPlane
                    };

                    nearPlaneValue.ValueChanged += () => OnNearPlaneChanged(nearPlaneValue);
                    cameraCM.VisibleChanged += control => nearPlaneValue.Value = _nearPlane;
                }

                // Far Plane
                {
                    var farPlane = cameraCM.AddButton("Far Plane");
                    farPlane.CloseMenuOnClick = false;
                    var farPlaneValue = new FloatValueBox(_farPlane, xLocationForExtras, 2, 70.0f, 10.0f)
                    {
                        Parent = farPlane
                    };

                    farPlaneValue.ValueChanged += () => OnFarPlaneChanged(farPlaneValue);
                    cameraCM.VisibleChanged += control => farPlaneValue.Value = _farPlane;
                }

                cameraCM.AddSeparator();

                // Reset Button
                {
                    var reset = cameraCM.AddButton("Reset to default");
                    reset.ButtonClicked += button =>
                    {
                        SetupViewportOptions();

                        // if the context menu is opened without triggering the value changes beforehand,
                        // the movement speed will not be correctly reset to its default value in certain cases
                        // therefore, a UI update needs to be triggered here
                        minCamSpeedValue.Value = _minMovementSpeed;
                        camSpeedValue.Value = _movementSpeed;
                        maxCamSpeedValue.Value = _maxMovementSpeed;
                    };
                }

                #endregion Camera settings widget

                #region View mode widget

                largestText = "Brightness";
                textSize = Style.Current.FontMedium.MeasureText(largestText);
                xLocationForExtras = textSize.X + 5;

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
                        _showFpsButton = ViewWidgetShowMenu.AddButton("FPS Counter", () => ShowFpsCounter = !ShowFpsCounter);
                        _showFpsButton.CloseMenuOnClick = false;
                    }
                }

                // View Layers
                {
                    var viewLayers = ViewWidgetButtonMenu.AddChildMenu("View Layers").ContextMenu;
                    viewLayers.AddButton("Copy layers", () => Clipboard.Text = JsonSerializer.Serialize(Task.View.RenderLayersMask));
                    viewLayers.AddButton("Paste layers", () =>
                    {
                        try
                        {
                            Task.ViewLayersMask = JsonSerializer.Deserialize<LayersMask>(Clipboard.Text);
                        }
                        catch
                        {
                        }
                    });
                    viewLayers.AddButton("Reset layers", () => Task.ViewLayersMask = LayersMask.Default).Icon = Editor.Instance.Icons.Rotate32;
                    viewLayers.AddButton("Disable layers", () => Task.ViewLayersMask = new LayersMask(0)).Icon = Editor.Instance.Icons.Rotate32;
                    viewLayers.AddSeparator();
                    var layers = LayersAndTagsSettings.GetCurrentLayers();
                    if (layers != null && layers.Length > 0)
                    {
                        for (int i = 0; i < layers.Length; i++)
                        {
                            var layer = layers[i];
                            var button = viewLayers.AddButton(layer);
                            button.CloseMenuOnClick = false;
                            button.Tag = 1 << i;
                        }
                    }
                    viewLayers.ButtonClicked += button =>
                    {
                        if (button.Tag != null)
                        {
                            int layerIndex = (int)button.Tag;
                            LayersMask mask = new LayersMask(layerIndex);
                            Task.ViewLayersMask ^= mask;
                            button.Icon = (Task.ViewLayersMask & mask) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                        }
                    };
                    viewLayers.VisibleChanged += WidgetViewLayersShowHide;
                }

                // View Flags
                {
                    var viewFlags = ViewWidgetButtonMenu.AddChildMenu("View Flags").ContextMenu;
                    viewFlags.AddButton("Copy flags", () => Clipboard.Text = JsonSerializer.Serialize(Task.ViewFlags));
                    viewFlags.AddButton("Paste flags", () =>
                    {
                        try
                        {
                            Task.ViewFlags = JsonSerializer.Deserialize<ViewFlags>(Clipboard.Text);
                        }
                        catch
                        {
                        }
                    });
                    viewFlags.AddButton("Reset flags", () => Task.ViewFlags = ViewFlags.DefaultEditor).Icon = Editor.Instance.Icons.Rotate32;
                    viewFlags.AddButton("Disable flags", () => Task.ViewFlags = ViewFlags.None).Icon = Editor.Instance.Icons.Rotate32;
                    viewFlags.AddSeparator();
                    for (int i = 0; i < ViewFlagsValues.Length; i++)
                    {
                        var v = ViewFlagsValues[i];
                        var button = viewFlags.AddButton(v.Name);
                        button.CloseMenuOnClick = false;
                        button.Tag = v.Mode;
                    }
                    viewFlags.ButtonClicked += button =>
                    {
                        if (button.Tag != null)
                        {
                            var v = (ViewFlags)button.Tag;
                            Task.ViewFlags ^= v;
                            button.Icon = (Task.ViewFlags & v) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                        }
                    };
                    viewFlags.VisibleChanged += WidgetViewFlagsShowHide;
                }

                // Debug View
                {
                    var debugView = ViewWidgetButtonMenu.AddChildMenu("Debug View").ContextMenu;
                    debugView.AddButton("Copy view", () => Clipboard.Text = JsonSerializer.Serialize(Task.ViewMode));
                    debugView.AddButton("Paste view", () =>
                    {
                        try
                        {
                            Task.ViewMode = JsonSerializer.Deserialize<ViewMode>(Clipboard.Text);
                        }
                        catch
                        {
                        }
                    });
                    debugView.AddSeparator();
                    for (int i = 0; i < ViewModeValues.Length; i++)
                    {
                        ref var v = ref ViewModeValues[i];
                        if (v.Options != null)
                        {
                            var childMenu = debugView.AddChildMenu(v.Name).ContextMenu;
                            childMenu.ButtonClicked += WidgetViewModeShowHideClicked;
                            childMenu.VisibleChanged += WidgetViewModeShowHide;
                            for (int j = 0; j < v.Options.Length; j++)
                            {
                                ref var vv = ref v.Options[j];
                                var button = childMenu.AddButton(vv.Name);
                                button.CloseMenuOnClick = false;
                                button.Tag = vv.Mode;
                            }
                        }
                        else
                        {
                            var button = debugView.AddButton(v.Name);
                            button.CloseMenuOnClick = false;
                            button.Tag = v.Mode;
                        }
                    }
                    debugView.ButtonClicked += WidgetViewModeShowHideClicked;
                    debugView.VisibleChanged += WidgetViewModeShowHide;
                }

                ViewWidgetButtonMenu.AddSeparator();

                // Brightness
                {
                    var brightness = ViewWidgetButtonMenu.AddButton("Brightness");
                    brightness.CloseMenuOnClick = false;
                    var brightnessValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.001f, 10.0f, 0.001f)
                    {
                        Parent = brightness
                    };
                    brightnessValue.ValueChanged += () => Brightness = brightnessValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => brightnessValue.Value = Brightness;
                }

                // Resolution
                {
                    var resolution = ViewWidgetButtonMenu.AddButton("Resolution");
                    resolution.CloseMenuOnClick = false;
                    var resolutionValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.1f, 4.0f, 0.001f)
                    {
                        Parent = resolution
                    };
                    resolutionValue.ValueChanged += () => ResolutionScale = resolutionValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => resolutionValue.Value = ResolutionScale;
                }

                #endregion View mode widget
            }

            InputActions.Add(options => options.ViewpointTop, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Top").Orientation)));
            InputActions.Add(options => options.ViewpointBottom, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Bottom").Orientation)));
            InputActions.Add(options => options.ViewpointFront, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Front").Orientation)));
            InputActions.Add(options => options.ViewpointBack, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Back").Orientation)));
            InputActions.Add(options => options.ViewpointRight, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Right").Orientation)));
            InputActions.Add(options => options.ViewpointLeft, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Left").Orientation)));
            InputActions.Add(options => options.CameraToggleRotation, () => _isVirtualMouseRightDown = !_isVirtualMouseRightDown);
            InputActions.Add(options => options.CameraIncreaseMoveSpeed, () => AdjustCameraMoveSpeed(1));
            InputActions.Add(options => options.CameraDecreaseMoveSpeed, () => AdjustCameraMoveSpeed(-1));
            InputActions.Add(options => options.ToggleOrthographic, () => OnOrthographicModeToggled(null));

            // Link for task event
            task.Begin += OnRenderBegin;
        }

        /// <summary>
        /// Sets the viewport options to the default values.
        /// </summary>
        private void SetupViewportOptions()
        {
            var options = Editor.Instance.Options.Options;
            _minMovementSpeed = options.Viewport.MinMovementSpeed;
            MovementSpeed = options.Viewport.MovementSpeed;
            _maxMovementSpeed = options.Viewport.MaxMovementSpeed;
            _useCameraEasing = options.Viewport.UseCameraEasing;
            _panningSpeed = options.Viewport.PanningSpeed;
            _invertPanning = options.Viewport.InvertPanning;
            _relativePanning = options.Viewport.UseRelativePanning;

            _isOrtho = options.Viewport.UseOrthographicProjection;
            _orthoSize = options.Viewport.OrthographicScale;
            _fieldOfView = options.Viewport.FieldOfView;
            _nearPlane = options.Viewport.NearPlane;
            _farPlane = options.Viewport.FarPlane;

            OnEditorOptionsChanged(options);
        }

        private void OnMovementSpeedChanged(FloatValueBox control)
        {
            var value = Mathf.Clamp(control.Value, _minMovementSpeed, _maxMovementSpeed);
            MovementSpeed = value;

            OnCameraMovementProgressChanged();
            _editor.ProjectCache.SetCustomData("CameraMovementSpeedValue", _movementSpeed);
        }

        private void OnMinMovementSpeedChanged(FloatValueBox control)
        {
            var value = Mathf.Clamp(control.Value, 0.05f, _maxMovementSpeed);
            _minMovementSpeed = value;

            if (_movementSpeed < value)
                MovementSpeed = value;

            OnCameraMovementProgressChanged();
            _editor.ProjectCache.SetCustomData("CameraMinMovementSpeedValue", _minMovementSpeed);
        }

        private void OnMaxMovementSpeedChanged(FloatValueBox control)
        {
            var value = Mathf.Clamp(control.Value, _minMovementSpeed, 1000.0f);
            _maxMovementSpeed = value;

            if (_movementSpeed > value)
                MovementSpeed = value;

            OnCameraMovementProgressChanged();
            _editor.ProjectCache.SetCustomData("CameraMaxMovementSpeedValue", _maxMovementSpeed);
        }

        private void OnCameraEasingToggled(Control control)
        {
            _useCameraEasing = !_useCameraEasing;

            OnCameraMovementProgressChanged();
            _editor.ProjectCache.SetCustomData("UseCameraEasingState", _useCameraEasing);
        }

        private void OnPanningSpeedChanged(FloatValueBox control)
        {
            _panningSpeed = control.Value;
            _editor.ProjectCache.SetCustomData("CameraPanningSpeedValue", _panningSpeed);
        }

        private void OnRelativePanningToggled(Control control)
        {
            _relativePanning = !_relativePanning;
            _editor.ProjectCache.SetCustomData("CameraRelativePanningState", _relativePanning);
        }

        private void OnInvertPanningToggled(Control control)
        {
            _invertPanning = !_invertPanning;
            _editor.ProjectCache.SetCustomData("CameraInvertPanningState", _invertPanning);
        }


        private void OnViewpointChanged(ContextMenuButton button)
        {
            var orient = Quaternion.Euler((Float3)button.Tag);
            OrientViewport(ref orient);
        }

        private void OnFieldOfViewChanged(FloatValueBox control)
        {
            _fieldOfView = control.Value;
            _editor.ProjectCache.SetCustomData("CameraFieldOfViewValue", _fieldOfView);
        }

        private void OnOrthographicModeToggled(Control control)
        {
            _isOrtho = !_isOrtho;

            if (_orthographicModeButton != null)
                _orthographicModeButton.Checked = !_isOrtho;

            if (_isOrtho)
            {
                var orient = ViewOrientation;
                OrientViewport(ref orient);
            }

            _editor.ProjectCache.SetCustomData("CameraOrthographicState", _isOrtho);
        }

        private void OnOrthographicSizeChanged(FloatValueBox control)
        {
            _orthoSize = control.Value;
            _editor.ProjectCache.SetCustomData("CameraOrthographicSizeValue", _orthoSize);
        }

        private void OnNearPlaneChanged(FloatValueBox control)
        {
            _nearPlane = control.Value;
            _editor.ProjectCache.SetCustomData("CameraNearPlaneValue", _nearPlane);
        }

        private void OnFarPlaneChanged(FloatValueBox control)
        {
            _farPlane = control.Value;
            _editor.ProjectCache.SetCustomData("CameraFarPlaneValue", _farPlane);
        }

        /// <summary>
        /// Gets a value indicating whether this viewport is using mouse currently (eg. user moving objects).
        /// </summary>
        protected virtual bool IsControllingMouse => false;

        /// <summary>
        /// Orients the viewport.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        protected void OrientViewport(Quaternion orientation)
        {
            OrientViewport(ref orientation);
        }

        /// <summary>
        /// Orients the viewport.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        protected virtual void OrientViewport(ref Quaternion orientation)
        {
            if (ViewportCamera is FPSCamera fpsCamera)
            {
                var pos = ViewPosition + Float3.Backward * orientation * 2000.0f;
                fpsCamera.MoveViewport(pos, orientation);
            }
            else
            {
                ViewportCamera.SetArcBallView(orientation, ViewPosition, 2000.0f);
            }
        }

        private void OnCameraMovementProgressChanged()
        {
            // prevent NaN
            if (Math.Abs(_minMovementSpeed - _maxMovementSpeed) < Mathf.Epsilon)
            {
                _speedStep = 0;
                return;
            }

            if (Math.Abs(_movementSpeed - _maxMovementSpeed) < Mathf.Epsilon)
            {
                _speedStep = _maxSpeedSteps;
                return;
            }
            else if (Math.Abs(_movementSpeed - _minMovementSpeed) < Mathf.Epsilon)
            {
                _speedStep = 0;
                return;
            }

            // calculate current linear/eased progress
            var progress = Mathf.Remap(_movementSpeed, _minMovementSpeed, _maxMovementSpeed, 0.0f, 1.0f);

            if (_useCameraEasing)
                progress = Mathf.Pow(progress, 1.0f / _cameraEasingDegree);

            _speedStep = Mathf.RoundToInt(progress * _maxSpeedSteps);
        }

        /// <summary>
        /// Increases or decreases the camera movement speed.
        /// </summary>
        /// <param name="step">The stepping direction for speed adjustment.</param>
        protected void AdjustCameraMoveSpeed(int step)
        {
            _speedStep = Mathf.Clamp(_speedStep + step, 0, _maxSpeedSteps);

            // calculate new linear/eased progress
            var progress = _useCameraEasing
                           ? Mathf.Pow((float)_speedStep / _maxSpeedSteps, _cameraEasingDegree)
                           : (float)_speedStep / _maxSpeedSteps;

            var speed = Mathf.Lerp(_minMovementSpeed, _maxMovementSpeed, progress);
            MovementSpeed = (float)Math.Round(speed, 3);
            _editor.ProjectCache.SetCustomData("CameraMovementSpeedValue", _movementSpeed);
        }

        private void OnEditorOptionsChanged(EditorOptions options)
        {
            _mouseSensitivity = options.Viewport.MouseSensitivity;
            _maxSpeedSteps = options.Viewport.TotalCameraSpeedSteps;
            _cameraEasingDegree = options.Viewport.CameraEasingDegree;
            OnCameraMovementProgressChanged();
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
                Render2D.DrawText(font, text, new Rectangle(Float2.One, Size), Color.Black);
                Render2D.DrawText(font, text, new Rectangle(Float2.Zero, Size), color);
            }
        }

        private FpsCounter _fpsCounter;
        private ContextMenuButton _showFpsButton;

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
                _showFpsButton.Icon = value ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
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
            Vector3 position = ViewPosition;
            LargeWorlds.UpdateOrigin(ref view.Origin, position);
            view.Position = position - view.Origin;
            view.Direction = ViewDirection;
            view.Near = _nearPlane;
            view.Far = _farPlane;

            CreateProjectionMatrix(out view.Projection);
            CreateViewMatrix(view.Position, out view.View);

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
        /// <param name="position">The view position.</param>
        /// <param name="result">The result.</param>
        protected virtual void CreateViewMatrix(Float3 position, out Matrix result)
        {
            var direction = ViewDirection;
            var target = position + direction;
            var right = Mathf.Abs(Float3.Dot(direction, Float3.Up)) < 1.0f - Mathf.Epsilon ? Float3.Normalize(Float3.Cross(Float3.Up, direction)) : Float3.Forward;
            var up = Float3.Normalize(Float3.Cross(direction, right));
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
        /// <param name="mousePosition">The mouse position (in UI space of the viewport [0; Size]).</param>
        /// <returns>The result ray.</returns>
        public Ray ConvertMouseToRay(ref Float2 mousePosition)
        {
            var viewport = new FlaxEngine.Viewport(0, 0, Width, Height);
            if (viewport.Width < Mathf.Epsilon || viewport.Height < Mathf.Epsilon)
                return ViewRay;

            Vector3 viewOrigin = Task.View.Origin;
            Float3 position = ViewPosition - viewOrigin;

            // Use different logic in orthographic projection
            if (_isOrtho)
            {
                var screenPosition = new Float2(mousePosition.X / viewport.Width - 0.5f, -mousePosition.Y / viewport.Height + 0.5f);
                var orientation = ViewOrientation;
                var direction = Float3.Forward * orientation;
                var rayOrigin = new Vector3(screenPosition.X * viewport.Width * _orthoSize, screenPosition.Y * viewport.Height * _orthoSize, 0);
                rayOrigin = position + Vector3.Transform(rayOrigin, orientation);
                rayOrigin += direction * _nearPlane;
                rayOrigin += viewOrigin;
                return new Ray(rayOrigin, direction);
            }

            // Create view frustum
            CreateProjectionMatrix(out var p);
            CreateViewMatrix(position, out var v);
            Matrix.Multiply(ref v, ref p, out var ivp);
            ivp.Invert();

            // Create near and far points
            var nearPoint = new Vector3(mousePosition, _nearPlane);
            var farPoint = new Vector3(mousePosition, _farPlane);
            viewport.Unproject(ref nearPoint, ref ivp, out nearPoint);
            viewport.Unproject(ref farPoint, ref ivp, out farPoint);

            return new Ray(nearPoint + viewOrigin, Vector3.Normalize(farPoint - nearPoint));
        }

        /// <summary>
        /// Projects the point from 3D world-space to viewport coordinates.
        /// </summary>
        /// <param name="worldSpaceLocation">The input world-space location (XYZ in world).</param>
        /// <param name="viewportSpaceLocation">The output viewport window coordinates (XY in screen pixels).</param>
        public void ProjectPoint(Vector3 worldSpaceLocation, out Float2 viewportSpaceLocation)
        {
            viewportSpaceLocation = Float2.Minimum;
            var viewport = new FlaxEngine.Viewport(0, 0, Width, Height);
            if (viewport.Width < Mathf.Epsilon || viewport.Height < Mathf.Epsilon)
                return;
            Vector3 viewOrigin = Task.View.Origin;
            Float3 position = ViewPosition - viewOrigin;
            CreateProjectionMatrix(out var p);
            CreateViewMatrix(position, out var v);
            Matrix.Multiply(ref v, ref p, out var vp);
            viewport.Project(ref worldSpaceLocation, ref vp, out var projected);
            viewportSpaceLocation = new Float2((float)projected.X, (float)projected.Y);
        }

        /// <summary>
        /// Called when mouse control begins.
        /// </summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseBegin(Window win)
        {
            // Hide cursor and start tracking mouse movement
            win.StartTrackingMouse(false);
            win.Cursor = CursorType.Hidden;

            // Center mouse position if it's too close to the edge
            var size = Size;
            var center = Float2.Round(size * 0.5f);
            if (Mathf.Abs(_viewMousePos.X - center.X) > center.X * 0.8f || Mathf.Abs(_viewMousePos.Y - center.Y) > center.Y * 0.8f)
            {
                _viewMousePos = center;
                win.MousePosition = PointToWindow(_viewMousePos);
            }
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
        protected virtual void UpdateView(float dt, ref Vector3 moveDelta, ref Float2 mouseDelta, out bool centerMouse)
        {
            centerMouse = true;
            _camera?.UpdateView(dt, ref moveDelta, ref mouseDelta, out centerMouse);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_disableInputUpdate)
                return;

            // Update camera
            bool useMovementSpeed = false;
            if (_camera != null)
            {
                _camera.Update(deltaTime);
                useMovementSpeed = _camera.UseMovementSpeed;

                if (_cameraButton != null)
                    _cameraButton.Parent.Visible = useMovementSpeed;
            }

            // Get parent window
            var win = (WindowRootControl)Root;

            // Get current mouse position in the view
            {
                // When the window is not focused, the position in window does not return sane values
                Float2 pos = PointFromWindow(win.MousePosition);
                if (!float.IsInfinity(pos.LengthSquared))
                    _viewMousePos = pos;
            }

            // Update input
            var window = win.Window;
            var canUseInput = window != null && window.IsFocused && window.IsForegroundWindow;
            {
                // Get input buttons and keys (skip if viewport has no focus or mouse is over a child control)
                var isViewportControllingMouse = canUseInput && IsControllingMouse;
                if (isViewportControllingMouse != _isViewportControllingMouse)
                {
                    _isViewportControllingMouse = isViewportControllingMouse;
                    if (isViewportControllingMouse)
                        StartMouseCapture();
                    else
                        EndMouseCapture();
                }
                bool useMouse = IsControllingMouse || (Mathf.IsInRange(_viewMousePos.X, 0, Width) && Mathf.IsInRange(_viewMousePos.Y, 0, Height));
                _prevInput = _input;
                var hit = GetChildAt(_viewMousePos, c => c.Visible && !(c is CanvasRootControl) && !(c is UIEditorRoot));
                if (canUseInput && ContainsFocus && hit == null)
                    _input.Gather(win.Window, useMouse, ref _prevInput);
                else
                    _input.Clear();

                // Track controlling mouse state change
                bool wasControllingMouse = _prevInput.IsControllingMouse;
                _isControllingMouse = _input.IsControllingMouse;

                // Simulate holding mouse right down for trackpad users
                if ((_prevInput.IsMouseRightDown && !_input.IsMouseRightDown) || win.GetKeyDown(KeyboardKeys.Escape))
                    _isVirtualMouseRightDown = false; // Cancel when mouse right or escape is pressed
                if (_wasVirtualMouseRightDown)
                    wasControllingMouse = true;
                if (_isVirtualMouseRightDown)
                    _isControllingMouse = _isVirtualMouseRightDown;

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

                if ((!_prevInput.IsMouseRightDown && _input.IsMouseRightDown) || (!_wasVirtualMouseRightDown && _isVirtualMouseRightDown))
                    OnRightMouseButtonDown();
                else if ((_prevInput.IsMouseRightDown && !_input.IsMouseRightDown) || (_wasVirtualMouseRightDown && !_isVirtualMouseRightDown))
                    OnRightMouseButtonUp();

                if (!_prevInput.IsMouseMiddleDown && _input.IsMouseMiddleDown)
                    OnMiddleMouseButtonDown();
                else if (_prevInput.IsMouseMiddleDown && !_input.IsMouseMiddleDown)
                    OnMiddleMouseButtonUp();

                _wasVirtualMouseRightDown = _isVirtualMouseRightDown;
            }

            // Get clamped delta time (more stable during lags)
            var dt = Math.Min(Time.UnscaledDeltaTime, 1.0f);

            // Check if update mouse
            var size = Size;
            var options = Editor.Instance.Options.Options;
            if (_isControllingMouse)
            {
                var rmbWheel = false;

                // Gather input
                {
                    bool isAltDown = _input.IsAltDown;
                    bool lbDown = _input.IsMouseLeftDown;
                    bool mbDown = _input.IsMouseMiddleDown;
                    bool rbDown = _input.IsMouseRightDown || _isVirtualMouseRightDown;
                    bool wheelInUse = Math.Abs(_input.MouseWheelDelta) > Mathf.Epsilon;

                    _input.IsPanning = !isAltDown && mbDown && !rbDown;
                    _input.IsRotating = !isAltDown && !mbDown && rbDown;
                    _input.IsMoving = !isAltDown && mbDown && rbDown;
                    _input.IsZooming = wheelInUse && !(_input.IsShiftDown || (!ContainsFocus && FlaxEngine.Input.GetKey(KeyboardKeys.Shift)));
                    _input.IsOrbiting = isAltDown && lbDown && !mbDown && !rbDown;

                    // Control move speed with RMB+Wheel
                    rmbWheel = useMovementSpeed && (_input.IsMouseRightDown || _isVirtualMouseRightDown) && wheelInUse;
                    if (rmbWheel)
                    {
                        var step = _input.MouseWheelDelta * options.Viewport.MouseWheelSensitivity;
                        AdjustCameraMoveSpeed(step > 0.0f ? 1 : -1);
                    }
                }

                // Get input movement
                var moveDelta = Vector3.Zero;
                if (win.GetKey(options.Input.Forward.Key))
                {
                    moveDelta += Vector3.Forward;
                }
                if (win.GetKey(options.Input.Backward.Key))
                {
                    moveDelta += Vector3.Backward;
                }
                if (win.GetKey(options.Input.Right.Key))
                {
                    moveDelta += Vector3.Right;
                }
                if (win.GetKey(options.Input.Left.Key))
                {
                    moveDelta += Vector3.Left;
                }
                if (win.GetKey(options.Input.Up.Key))
                {
                    moveDelta += Vector3.Up;
                }
                if (win.GetKey(options.Input.Down.Key))
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
                var offset = _viewMousePos - _startPos;
                if (_input.IsZooming && !_input.IsMouseRightDown && !_input.IsMouseLeftDown && !_input.IsMouseMiddleDown && !_isOrtho && !rmbWheel && !_isVirtualMouseRightDown)
                {
                    offset = Float2.Zero;
                }

                var mouseDelta = Float2.Zero;
                if (_useMouseFiltering)
                {
                    offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                    offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                    _mouseDelta = offset;

                    // Update delta filtering buffer
                    _deltaFilteringBuffer[_deltaFilteringStep] = _mouseDelta;
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
                {
                    _mouseDelta = offset;
                    mouseDelta = _mouseDelta;
                }

                if (_useMouseAcceleration)
                {
                    // Accelerate the delta
                    var currentDelta = mouseDelta;
                    mouseDelta += _mouseDeltaLast * _mouseAccelerationScale;
                    _mouseDeltaLast = currentDelta;
                }

                // Update
                moveDelta *= dt * (60.0f * 4.0f);
                mouseDelta *= 0.1833f * MouseSpeed * _mouseSensitivity;
                if (options.Viewport.InvertMouseYAxisRotation)
                    mouseDelta *= new Float2(1, -1);
                UpdateView(dt, ref moveDelta, ref mouseDelta, out var centerMouse);

                // Move mouse back to the root position
                if (centerMouse && (_input.IsMouseRightDown || _input.IsMouseLeftDown || _input.IsMouseMiddleDown || _isVirtualMouseRightDown))
                {
                    var center = PointToWindow(_startPos);
                    win.MousePosition = center;
                }

                // Change Ortho size on mouse scroll
                if (_isOrtho && !rmbWheel)
                {
                    var scroll = _input.MouseWheelDelta;
                    if (scroll > Mathf.Epsilon || scroll < -Mathf.Epsilon)
                        _orthoSize -= scroll * options.Viewport.MouseWheelSensitivity * 0.2f * _orthoSize;
                }
            }
            else
            {
                if (_input.IsMouseLeftDown || _input.IsMouseRightDown || _isVirtualMouseRightDown)
                {
                    // Calculate smooth mouse delta not dependant on viewport size
                    var offset = _viewMousePos - _startPos;
                    offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                    offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                    _mouseDelta = offset;
                    _startPos = _viewMousePos;
                }
                else
                {
                    _mouseDelta = Float2.Zero;
                }
                _mouseDeltaLast = Float2.Zero;

                if (ContainsFocus)
                {
                    _input.IsPanning = false;
                    _input.IsRotating = false;
                    _input.IsMoving = true;
                    _input.IsZooming = false;
                    _input.IsOrbiting = false;

                    // Get input movement
                    var moveDelta = Vector3.Zero;
                    var mouseDelta = Float2.Zero;
                    if (FlaxEngine.Input.GamepadsCount > 0)
                    {
                        // Gamepads handling
                        moveDelta += new Vector3(GetGamepadAxis(GamepadAxis.LeftStickX), 0, GetGamepadAxis(GamepadAxis.LeftStickY));
                        mouseDelta += new Float2(GetGamepadAxis(GamepadAxis.RightStickX), -GetGamepadAxis(GamepadAxis.RightStickY));
                        _input.IsRotating |= !mouseDelta.IsZero;
                    }
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
                    if (FlaxEngine.Input.GamepadsCount > 0)
                        moveDelta *= Mathf.Remap(GetGamepadAxis(GamepadAxis.RightTrigger), 0, 1, 1, 4.0f);

                    // Update
                    moveDelta *= dt * (60.0f * 4.0f);
                    UpdateView(dt, ref moveDelta, ref mouseDelta, out _);
                }
            }

            _input.MouseWheelDelta = 0;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();

            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
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
                var bounds = new Rectangle(Float2.Zero, Size);
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
                _isVirtualMouseRightDown = false;
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Editor.Instance.Options.OptionsChanged -= OnEditorOptionsChanged;

            base.OnDestroy();
        }

        private struct CameraViewpoint
        {
            public readonly string Name;
            public readonly Float3 Orientation;

            public CameraViewpoint(string name, Vector3 orientation)
            {
                Name = name;
                Orientation = orientation;
            }
        }

        private readonly CameraViewpoint[] CameraViewpointValues =
        {
            new CameraViewpoint("Front", new Float3(0, 180, 0)),
            new CameraViewpoint("Back", new Float3(0, 0, 0)),
            new CameraViewpoint("Left", new Float3(0, 90, 0)),
            new CameraViewpoint("Right", new Float3(0, -90, 0)),
            new CameraViewpoint("Top", new Float3(90, 0, 0)),
            new CameraViewpoint("Bottom", new Float3(-90, 0, 0))
        };

        private struct ViewModeOptions
        {
            public readonly string Name;
            public readonly ViewMode Mode;
            public readonly ViewModeOptions[] Options;

            public ViewModeOptions(ViewMode mode, string name)
            {
                Mode = mode;
                Name = name;
                Options = null;
            }

            public ViewModeOptions(string name, ViewModeOptions[] options)
            {
                Name = name;
                Mode = ViewMode.Default;
                Options = options;
            }
        }

        private static readonly ViewModeOptions[] ViewModeValues =
        {
            new ViewModeOptions(ViewMode.Default, "Default"),
            new ViewModeOptions(ViewMode.Unlit, "Unlit"),
            new ViewModeOptions(ViewMode.NoPostFx, "No PostFx"),
            new ViewModeOptions(ViewMode.Wireframe, "Wireframe"),
            new ViewModeOptions(ViewMode.LightBuffer, "Light Buffer"),
            new ViewModeOptions(ViewMode.Reflections, "Reflections Buffer"),
            new ViewModeOptions(ViewMode.Depth, "Depth Buffer"),
            new ViewModeOptions("GBuffer", new[]
            {
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
            }),
            new ViewModeOptions(ViewMode.MotionVectors, "Motion Vectors"),
            new ViewModeOptions(ViewMode.LightmapUVsDensity, "Lightmap UVs Density"),
            new ViewModeOptions(ViewMode.VertexColors, "Vertex Colors"),
            new ViewModeOptions(ViewMode.PhysicsColliders, "Physics Colliders"),
            new ViewModeOptions(ViewMode.LODPreview, "LOD Preview"),
            new ViewModeOptions(ViewMode.MaterialComplexity, "Material Complexity"),
            new ViewModeOptions(ViewMode.QuadOverdraw, "Quad Overdraw"),
            new ViewModeOptions(ViewMode.GlobalSDF, "Global SDF"),
            new ViewModeOptions(ViewMode.GlobalSurfaceAtlas, "Global Surface Atlas"),
            new ViewModeOptions(ViewMode.GlobalIllumination, "Global Illumination"),
        };

        private void WidgetViewModeShowHideClicked(ContextMenuButton button)
        {
            if (button.Tag is ViewMode v)
            {
                Task.ViewMode = v;
                var cm = button.ParentContextMenu;
                WidgetViewModeShowHide(cm);
                var mainCM = ViewWidgetButtonMenu.GetChildMenu("Debug View").ContextMenu;
                if (mainCM != null && cm != mainCM)
                    WidgetViewModeShowHide(mainCM);
            }
        }

        private void WidgetViewModeShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;

            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b.Tag is ViewMode v)
                    b.Icon = Task.ViewMode == v ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
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

        private static readonly ViewFlagOptions[] ViewFlagsValues =
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
            new ViewFlagOptions(ViewFlags.Sky, "Sky"),
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
            new ViewFlagOptions(ViewFlags.LightsDebug, "Lights Debug"),
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
                    b.Icon = (Task.View.Flags & v) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }

        private void WidgetViewLayersShowHide(Control cm)
        {
            if (cm.Visible == false)
                return;
            var ccm = (ContextMenu)cm;
            var layersMask = Task.ViewLayersMask;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b != null && b.Tag != null)
                {
                    int layerIndex = (int)b.Tag;
                    LayersMask mask = new LayersMask(layerIndex);
                    b.Icon = (layersMask & mask) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }

        private float GetGamepadAxis(GamepadAxis axis)
        {
            var value = FlaxEngine.Input.GetGamepadAxis(InputGamepadIndex.All, axis);
            var deadZone = 0.2f;
            return value >= deadZone || value <= -deadZone ? value : 0.0f;
        }
    }
}
