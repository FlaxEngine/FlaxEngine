// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.InputConfig;
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
        private bool _pan;
        public bool Pan { get => _pan; set => _pan = value; }

        private bool _orbit;
        public bool Orbit { get => _orbit; set => _orbit = value; }
        private bool _rotate;
        public bool Rotate { get => _rotate; set => _rotate = value; }
        private bool _move;
        public bool Move { get => _move; set => _move = value; }
        private bool _zoom;
        public bool Zoom { get => _zoom; set => _zoom = value; }


        private float _mouseSensitivity;
        public float MouseSensitivity { get => _mouseSensitivity; set => _mouseSensitivity = value; }

        private float _mouseAccelerationScale;
        public float MouseAccelerationScale { get => _mouseAccelerationScale; set => _mouseAccelerationScale = value; }

        private bool _mouseFiltering;
        public bool UseMouseFiltering { get => _mouseFiltering; set => _mouseFiltering = value; }

        /// <summary>
        /// The FPS camera filtering frames count (how much frames we want to keep in the buffer to calculate the avg. delta currently hardcoded).
        /// </summary>
        private int _mouseFilteringFrames;
        public int MouseFilteringFrames { get => _mouseFilteringFrames; set => _mouseFilteringFrames = value; }


        private bool _mouseAcceleration;
        public bool UseMouseAcceleration { get => _mouseAcceleration; set => _mouseAcceleration = value; }

        private float _mouseWheelDelta;
        public float MouseWheelDelta { get => _mouseWheelDelta; set => _mouseWheelDelta = value; }
        private bool _isControllingMouse;
        public bool IsControllingMouse
        {
            get =>
            _isControllingMouse;
            set => _isControllingMouse = value;
        }

        //used to maintain mouse centering when panning, rotating, and other camera movements that require long strokes
        private bool _centerMouse;
        public bool CenterMouse { get => _centerMouse; set => _centerMouse = value; }

        private float _mouseSpeed = 1;
        /// <summary>
        /// Speed of the mouse.
        /// </summary>
        public float MouseSpeed { get => _mouseSpeed; set => _mouseSpeed = value; }

        private float _panningSpeed;
        /// <summary>
        /// Gets or sets the camera panning speed.
        /// </summary>
        public float PanningSpeed { get => _panningSpeed; set => _panningSpeed = value; }

        private bool _relativePanning;
        /// <summary>
        /// Gets or sets if the panning speed should be relative to the camera target.
        /// </summary>
        public bool RelativePanning { get => _relativePanning; set => _relativePanning = value; }

        private bool _invertPanning;
        /// <summary>
        /// Gets or sets if the panning direction is inverted.
        /// </summary>
        public bool InvertPanning { get => _invertPanning; set => _invertPanning = value; }

        private bool _invertMouseYAxisRotation;
        public bool InvertMouseYAxisRotation { get => _invertMouseYAxisRotation; set => _invertMouseYAxisRotation = value; }


        //todo im just making these public consts until i figure out what to do with them
        public const float MaxAllowedSpeed = 1000f;
        public const float MinAllowedSpeed = 0.05f;
        public const float MovementSpeedEpsilon = 0.01f;

        private float _minMovementSpeed;
        /// <summary>
        /// Gets or sets the minimum camera movement speed.
        /// </summary>
        public float MinMovementSpeed
        {
            get => _minMovementSpeed;
            set
            {
                _minMovementSpeed = Mathf.Clamp(value, MinAllowedSpeed, MaxAllowedSpeed - MovementSpeedEpsilon);

                //adjust max speed if min raised above it
                if (MaxMovementSpeed - _minMovementSpeed < MovementSpeedEpsilon)
                    MaxMovementSpeed = _minMovementSpeed + MovementSpeedEpsilon;

                if (MovementSpeed < _minMovementSpeed)
                    MovementSpeed = _minMovementSpeed;
            }
        }

        private float _maxMovementSpeed;
        /// <summary>
        /// Gets or sets the maximum camera movement speed.
        /// </summary>
        public float MaxMovementSpeed
        {
            get => _maxMovementSpeed;
            set
            {
                _maxMovementSpeed = Mathf.Clamp(value, MinAllowedSpeed + MovementSpeedEpsilon, MaxAllowedSpeed);

                //adjust min speed if max lowered below it it
                if (_maxMovementSpeed - MinMovementSpeed < MovementSpeedEpsilon)
                    MinMovementSpeed = _maxMovementSpeed - MovementSpeedEpsilon;

                if (MovementSpeed > _maxMovementSpeed)
                    MovementSpeed = _maxMovementSpeed;
            }
        }

        private float _movementPercentage;
        public float MovementPercentage { get => _movementPercentage; private set => _movementPercentage = value; }

        private float _movementSpeed;
        /// <summary>
        /// Gets or sets the camera movement speed.
        /// </summary>
        public float MovementSpeed
        {
            get => _movementSpeed;
            set
            {
                _movementSpeed = Mathf.Clamp(value, MinMovementSpeed, MaxMovementSpeed);
                MovementPercentage = Mathf.Remap(_movementSpeed, MinMovementSpeed, MaxMovementSpeed, 0f, 100f);
            }
        }

        private bool _orthographicProjection;
        /// <summary>
        /// Gets or sets the camera orthographic mode (otherwise uses perspective projection).
        /// </summary>
        public bool OrthographicProjection
        {
            get => _orthographicProjection;
            set
            {
                _orthographicProjection = value;
                if (_orthographicProjection)
                {
                    OrientViewport(ViewOrientation);
                }
            }
        }

        private float _orthographicScale;
        /// <summary>
        /// Gets or sets the camera orthographic size scale (if camera uses orthographic mode).
        /// </summary>
        public float OrthographicScale { get => _orthographicScale; set => _orthographicScale = value; }

        private float _fieldOfView;
        /// <summary>
        /// Gets or sets the camera field of view (in degrees).
        /// </summary>
        public float FieldOfView { get => _fieldOfView; set => _fieldOfView = value; }

        private float _nearPlane;
        /// <summary>
        /// Gets or sets the camera near clipping plane.
        /// </summary>
        public float NearPlane { get => _nearPlane; set => _nearPlane = value; }

        private float _farPlane;
        /// <summary>
        /// Gets or sets the camera far clipping plane.
        /// </summary>
        public float FarPlane { get => _farPlane; set => _farPlane = value; }

        private float _mouseWheelSensitivity = 1;
        /// <summary>
        /// Speed of the mouse wheel zooming.
        /// </summary>
        public float MouseWheelSensitivity { get => _mouseWheelSensitivity; set => _mouseWheelSensitivity = value; }

        public InputOptions InputOptions = Editor.Instance.Options.Options.Input;

        private readonly Editor _editor;

        private float _yaw;
        /// <summary>
        /// Gets or sets the yaw angle (in degrees).
        /// </summary>
        public float Yaw { get => _yaw; set => _yaw = value; }

        private float _pitch;
        /// <summary>
        /// Gets or sets the pitch angle (in degrees).
        /// </summary>
        public float Pitch
        {
            get => _pitch;
            set
            {
                var pitchLimit = OrthographicProjection ? new Float2(-90, 90) : new Float2(-88, 88);
                _pitch = Mathf.Clamp(value, pitchLimit.X, pitchLimit.Y);
            }
        }

        /// <summary>
        /// Gets or sets the absolute mouse position (normalized, not in pixels). Yaw is X, Pitch is Y.
        /// </summary>
        public Float2 YawPitch
        {
            get => new Float2(Yaw, Pitch);
            set
            {
                Yaw = value.X;
                Pitch = value.Y;
            }
        }

        public InputBindingList InputBindingList = new InputBindingList();


        public void GetViewportOptions()
        {
            ViewportOptions ViewportOptions = Editor.Instance.Options.Options.Viewport;
            _mouseSpeed = ViewportOptions.MouseSpeed;
            _mouseSensitivity = ViewportOptions.MouseSensitivity;
            _mouseAccelerationScale = ViewportOptions.MouseAccelerationScale;
            _mouseFiltering = ViewportOptions.MouseFiltering;
            _mouseFilteringFrames = ViewportOptions.MouseFilteringFrames;
            _mouseAcceleration = ViewportOptions.MouseAcceleration;
            _mouseWheelSensitivity = ViewportOptions.MouseWheelSensitivity;
            _panningSpeed = ViewportOptions.PanningSpeed;
            _relativePanning = ViewportOptions.RelativePanning;
            _invertPanning = ViewportOptions.InvertPanning;
            _minMovementSpeed = ViewportOptions.MinMovementSpeed;
            _maxMovementSpeed = ViewportOptions.MaxMovementSpeed;
            _movementSpeed = ViewportOptions.MovementSpeed;
            _orthographicProjection = ViewportOptions.OrthographicProjection;
            _orthographicScale = ViewportOptions.OrthographicScale;
            _fieldOfView = ViewportOptions.FieldOfView;
            _nearPlane = ViewportOptions.NearPlane;
            _farPlane = ViewportOptions.FarPlane;
        }
        private void SetBindings()
        {
            InputOptions InputOptions = Editor.Instance.Options.Options.Input;
            InputBindingList.Add(
                [
                (InputOptions.Forward, () => {Move = true; _moveDelta = Vector3.Forward * MovementSpeed; }, () => {Move = false; _moveDelta = Vector3.Zero; }),
                (InputOptions.Backward, () => {Move = true; _moveDelta = Vector3.Backward * MovementSpeed; }, () => {Move = false; _moveDelta = Vector3.Zero; }),
                (InputOptions.Left, () => {Move = true; _moveDelta = Vector3.Left * MovementSpeed; }, () => {Move = false; _moveDelta = Vector3.Zero; }),
                (InputOptions.Right, () => {Move = true; _moveDelta = Vector3.Right * MovementSpeed; }, () => {Move = false; _moveDelta = Vector3.Zero; }),
                (InputOptions.Up, () => {Move = true; _moveDelta = Vector3.Up * MovementSpeed; }, () => {Move = false; _moveDelta = Vector3.Zero; }),
                (InputOptions.Down, () => {Move = true; _moveDelta = Vector3.Down * MovementSpeed; }, () => {Move = false; _moveDelta = Vector3.Zero; }),
                (InputOptions.Orbit, () => {Orbit = true; CenterMouse = true; }, () => {Orbit = false; CenterMouse = false; }),
                (InputOptions.Pan, () => {Pan = true; CenterMouse = true; }, () => {Pan = false; CenterMouse = false; }),
                (InputOptions.Rotate, () => {Rotate = true; CenterMouse = true; }, () => {Rotate = false; CenterMouse = false; }),
                (InputOptions.ZoomIn, () =>
                {
                    Zoom = true;
                    if (OrthographicProjection)
                    {
                        OrthographicScale -= MouseWheelDelta * MouseWheelSensitivity * 0.2f * OrthographicScale;
                    }
                }, () => Zoom = false),
                (InputOptions.ZoomOut, () =>
                {
                    Zoom = true;
                    if (OrthographicProjection)
                    {
                        OrthographicScale -= MouseWheelDelta * MouseWheelSensitivity * 0.2f * OrthographicScale;
                    }
                }, () => Zoom = false),
                (InputOptions.CameraIncreaseMoveSpeed, () => MovementSpeed += MouseWheelDelta * MouseWheelSensitivity, null),
                (InputOptions.CameraDecreaseMoveSpeed, () => MovementSpeed += MouseWheelDelta * MouseWheelSensitivity, null),
                (InputOptions.ViewpointTop, () => OrientViewport(CameraViewpoints[Viewpoint.Front]), null),
                (InputOptions.ViewpointBottom, () => OrientViewport(CameraViewpoints[Viewpoint.Back]), null),
                (InputOptions.ViewpointFront, () => OrientViewport(CameraViewpoints[Viewpoint.Left]), null),
                (InputOptions.ViewpointBack, () => OrientViewport(CameraViewpoints[Viewpoint.Right]), null),
                (InputOptions.ViewpointRight, () => OrientViewport(CameraViewpoints[Viewpoint.Top]), null),
                (InputOptions.ViewpointLeft, () => OrientViewport(CameraViewpoints[Viewpoint.Bottom]), null),
                //todo no idea, we need to hold down mouse buttons for trackpads so they dont have to deal with holding it and moving 
                // (InputOptions.CameraToggleRotation, () => _isVirtualMouseRightDown = !_isVirtualMouseRightDown),
                (InputOptions.ToggleOrthographic, () => OrthographicProjection = !OrthographicProjection, null)
                ]
            );
        }
        private Vector3 _moveDelta = Vector3.Zero;

        public enum Viewpoint { Front, Back, Left, Right, Top, Bottom }
        public static readonly Dictionary<Viewpoint, Quaternion> CameraViewpoints = new(){
            { Viewpoint.Front, Quaternion.Euler(0, 180, 0) },
            { Viewpoint.Back, Quaternion.Euler(0, 0, 0) },
            { Viewpoint.Left, Quaternion.Euler(0, 90, 0) },
            { Viewpoint.Right, Quaternion.Euler(0, -90, 0) },
            { Viewpoint.Top, Quaternion.Euler(90, 0, 0) },
            { Viewpoint.Bottom, Quaternion.Euler(-90, 0, 0) }
        };


        // Input

        internal bool _disableInputUpdate;
        private int _deltaFilteringStep;
        private Float2 _startPos;
        private Float2 _mouseDeltaLast;
        private Float2[] _deltaFilteringBuffer;

        /// <summary>
        /// The view mouse position.
        /// </summary>
        protected Float2 _viewMousePos;

        /// <summary>
        /// The mouse position delta.
        /// </summary>
        protected Float2 _mouseDelta;

        // Camera
        private ViewportCamera _viewportCamera;
        /// <summary>
        /// Gets or sets the viewport camera controller.
        /// </summary>
        public ViewportCamera ViewportCamera
        {
            get => _viewportCamera;
            set
            {
                if (_viewportCamera != null)
                    _viewportCamera.Viewport = null;

                _viewportCamera = value;

                if (_viewportCamera != null)
                    _viewportCamera.Viewport = this;
            }
        }

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

        private bool _useCameraEasing;
        /// <summary>
        /// Gets or sets the camera easing mode.
        /// </summary>
        public bool UseCameraEasing { get => _useCameraEasing; set => _useCameraEasing = value; }

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
        /// Gets or sets the view orientation.
        /// </summary>
        public Quaternion ViewOrientation
        {
            get => Quaternion.RotationYawPitchRoll(Yaw * Mathf.DegreesToRadians, Pitch * Mathf.DegreesToRadians, 0);
            set => EulerAngles = value.EulerAngles;
        }

        /// <summary>
        /// Gets or sets the euler angles (pitch, yaw, roll).
        /// </summary>
        public Float3 EulerAngles
        {
            get => new Float3(Pitch, Yaw, 0);
            set
            {
                Pitch = value.X;
                Yaw = value.Y;
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
        /// Gets a value indicating whether this viewport has loaded dependant assets.
        /// </summary>
        public virtual bool HasLoadedAssets => true;

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
            SetBindings();
            GetViewportOptions();
            _deltaFilteringBuffer = new Float2[MouseFilteringFrames];
            ViewportCamera = camera;

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;



            // Link for task event
            task.Begin += OnRenderBegin;
        }

        /// <summary>
        /// Orients the viewport.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        public virtual void OrientViewport(Quaternion orientation)
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

        private void OnRenderBegin(RenderTask task, GPUContext context)
        {
            var sceneTask = (SceneRenderTask)task;

            var view = sceneTask.View;
            CopyViewData(ref view);
            sceneTask.View = view;
            _rootControl = ((WindowRootControl)Root);
            _rootWindow = _rootControl.Window;
        }

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
        /// Creates the projection matrix.
        /// </summary>
        /// <param name="result">The result.</param>
        protected virtual void CreateProjectionMatrix(out Matrix result)
        {
            if (OrthographicProjection)
            {
                Matrix.Ortho(Width * OrthographicScale, Height * OrthographicScale, _nearPlane, _farPlane, out result);
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
            if (OrthographicProjection)
            {
                var orientation = ViewOrientation;
                var direction = Float3.Forward * orientation;
                return new Ray(viewOrigin, direction);
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
        /// Gets a value indicating whether this viewport is using mouse currently (eg. user moving objects).
        /// </summary>
        protected virtual bool WantsMouseCapture => false;

        private void UpdateView()
        {
            var offset = _viewMousePos - _startPos;
            if (UseMouseFiltering)
            {
                // Calculate smooth mouse delta not dependant on viewport size

                offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                _mouseDelta = offset;

                // Update delta filtering buffer
                _deltaFilteringBuffer[_deltaFilteringStep] = _mouseDelta;
                _deltaFilteringStep++;

                // If the step is too far, zero
                if (_deltaFilteringStep == MouseFilteringFrames)
                    _deltaFilteringStep = 0;

                // Calculate filtered delta (avg)
                for (int i = 0; i < MouseFilteringFrames; i++)
                    _mouseDelta += _deltaFilteringBuffer[i];

                _mouseDelta /= MouseFilteringFrames;
            }
            else
            {
                _mouseDelta = offset;
            }
            if (UseMouseAcceleration)
            {
                // Accelerate the delta
                var currentDelta = _mouseDelta;
                _mouseDelta += _mouseDeltaLast * MouseAccelerationScale;
                _mouseDeltaLast = currentDelta;
            }

            // Update
            _moveDelta *= _deltaTime * (60.0f * 4.0f);
            _mouseDelta *= 0.1833f * MouseSpeed * MouseSensitivity;
            if (InvertMouseYAxisRotation)
                _mouseDelta *= new Float2(1, -1);

            ViewportCamera?.UpdateView(_deltaTime, ref _moveDelta, ref _mouseDelta, out bool centerMouse);
            // Move mouse back to the root position
            //todo this should be in the cameras
            if (IsControllingMouse && CenterMouse)
            {
                _rootWindow.Cursor = CursorType.Hidden;
                var center = PointToWindow(_startPos);
                _rootWindow.MousePosition = center;
            }
            else
            {
                _rootWindow.Cursor = CursorType.Default;
            }
        }

        /// <inheritdoc />
        float _deltaTime = 0;
        //bindings are always bools, they can just be set to false
        InputBinding? _lastBinding;
        Window _rootWindow;
        WindowRootControl _rootControl;

        bool _canUseInput
        {
            get
            {

                return _rootWindow != null && _rootWindow.IsFocused && _rootWindow.IsForegroundWindow;
            }
        }


        public override void Update(float deltaTime)
        {
            if (_rootControl == null)
            {
                return;
            }
            // continue to render in background, and set initial deltas
            _deltaTime = Math.Min(Time.UnscaledDeltaTime, 1.0f);
            _moveDelta = Vector3.Zero;
            _mouseDelta = Float2.Zero;
            base.Update(deltaTime);
            ViewportCamera?.Update(deltaTime);
            InitViewMousePos();

            if (_disableInputUpdate)
                return;

            //check for inputs and fire events if necessary
            InputBindingListProcess();

            if (_canUseInput && ContainsFocus)
            {
                UpdateView();
            }
            MouseWheelDelta = 0;
        }

        private void InputBindingListProcess()
        {
            InputBinding? newBinding = InputBindingList.Process(this);
            if (newBinding != null && _lastBinding == null)
            {
                _lastBinding = newBinding;
            }
            if (_lastBinding != null && newBinding != _lastBinding)
            {
                _lastBinding?.ClearState();
                _lastBinding = newBinding;
            }
            //todo check for trackpad users here...
            //was rotating etc...
            _lastBinding = newBinding;
        }

        private void InitViewMousePos()
        {
            Float2 pos = PointFromWindow(_rootControl.MousePosition);
            if (!float.IsInfinity(pos.LengthSquared))
                _viewMousePos = pos;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();

            base.OnMouseDown(location, button);
            _startPos = _viewMousePos;
            IsControllingMouse = true;
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            base.OnMouseUp(location, button);
            IsControllingMouse = false;
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            MouseWheelDelta += delta;
            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
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

        /// <summary>
        /// Called when left mouse button goes down (on press).
        /// </summary>
        protected virtual void OnLeftMouseButtonDown()
        {
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
        }

        /// <summary>
        /// Called when middle mouse button goes up (on release).
        /// </summary>
        protected virtual void OnMiddleMouseButtonUp()
        {
        }
    }
}
