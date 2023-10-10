using System;
using System.Linq;
using FlaxEditor.Gizmo;
using FlaxEditor.Options;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Editor viewports base class.
    /// </summary>
    /// <seealso cref="T:FlaxEngine.GUI.RenderOutputControl" />
    public partial class EditorViewport : RenderOutputControl, IGizmoOwner
    {
        /// <summary>
        /// caled wen camera moves
        /// </summary>
        public Action OnCameraMoved;

        /// <inheritdoc />
        public EditorViewport Viewport
        {
            get
            {
                return this;
            }
        }

        /// <inheritdoc />
        public GizmosCollection Gizmos { get; } = new GizmosCollection();

        /// <inheritdoc />
        public Ray MouseRay { get; private set; }

        /// <inheritdoc />
        public Float2 MouseDelta
        {
            get
            {
                return _input.MouseDelta;
            }
        }

        /// <inheritdoc />
        public Undo Undo { get; internal set; }

        /// <inheritdoc />
        public virtual RootNode SceneGraphRoot { get; }

        /// <summary>
        /// Gets or sets the camera movement speed.
        /// </summary>
        public float MovementSpeed
        {
            get
            {
                return Camera.MovementSpeed;
            }
            set
            {
                Camera.MovementSpeed = value;
                if (_speedWidget != null)
                {
                    _speedWidget.Text = Camera.MovementSpeed.ToString();
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether this viewport has loaded dependant assets.
        /// </summary>
        public virtual bool HasLoadedAssets
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// Gets or sets the viewport camera controller.
        /// </summary>
        public ViewportCamera Camera
        {
            get
            {
                return _camera;
            }
        }
        /// <summary>
        /// Gets a value indicating whether this viewport is using mouse currently (eg. user moving objects).
        /// </summary>
        protected virtual bool IsGizmoControllingMouse
        {
            get
            {
                if (Gizmos != null)
                {
                    if (Gizmos.Active != null)
                    {
                        return Gizmos.Active.IsControllingMouse;
                    }
                }
                return false;
            }
        }

        /// <summary>
        /// The camera
        /// </summary>
        protected ViewportCamera _camera;

        /// <summary>
        /// Initializes a new instance of the <see cref="T:FlaxEditor.Viewport.EditorViewport" /> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="camera">The camera controller.</param>
        /// <param name="useWidgets">Enable/disable viewport widgets.</param>
        /// <param name="undo">The undo.</param>
        /// <param name="sceneGraphRoot">The scene graph root.</param>
        public EditorViewport(SceneRenderTask task, ViewportCamera camera, bool useWidgets = true, Undo undo = null, RootNode sceneGraphRoot = null) : base(task)
        {
            Undo = undo;
            SceneGraphRoot = sceneGraphRoot;
            _camera = camera;
            if (_camera != null)
                _camera.Viewport = this;

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;

            // Setup options
            {
                var options = Editor.Instance.Options.Options;
                camera.MovementSpeed = options.Viewport.DefaultMovementSpeed;
                camera.NearPlane = options.Viewport.DefaultNearPlane;
                camera.FarPlane = options.Viewport.DefaultFarPlane;
                camera.FieldOfView = options.Viewport.DefaultFieldOfView;
                camera.InvertPanning = options.Viewport.DefaultInvertPanning;

                Editor.Instance.Options.OptionsChanged += OnEditorOptionsChanged;
                OnEditorOptionsChanged(options);
            }

            if (useWidgets)
            {
                ConstructWidgets();
            }
            InputActions.Add(options => options.ViewpointTop, () =>  Camera.AddOrSetAnimatedAction(Quaternion.Euler(EditorViewportCameraViewpointValues.First(vp => vp.Name == "Top").Orientation)));
            InputActions.Add(options => options.ViewpointBottom, () => Camera.AddOrSetAnimatedAction(Quaternion.Euler(EditorViewportCameraViewpointValues.First(vp => vp.Name == "Bottom").Orientation)));
            InputActions.Add(options => options.ViewpointFront, () =>  Camera.AddOrSetAnimatedAction(Quaternion.Euler(EditorViewportCameraViewpointValues.First(vp => vp.Name == "Front").Orientation)));
            InputActions.Add(options => options.ViewpointBack, () =>  Camera.AddOrSetAnimatedAction(Quaternion.Euler(EditorViewportCameraViewpointValues.First(vp => vp.Name == "Back").Orientation)));
            InputActions.Add(options => options.ViewpointRight, () => Camera.AddOrSetAnimatedAction(Quaternion.Euler(EditorViewportCameraViewpointValues.First(vp => vp.Name == "Right").Orientation)));
            InputActions.Add(options => options.ViewpointLeft, () =>  Camera.AddOrSetAnimatedAction(Quaternion.Euler(EditorViewportCameraViewpointValues.First(vp => vp.Name == "Left").Orientation)));
            InputActions.Add(options => options.ToggleMouseBottonRight, () => _input.ToggleMouseBottonRight());
            InputActions.Add(options => options.CameraIncreaseMoveSpeed, () => MovementSpeed += 1);
            InputActions.Add(options => options.CameraDecreaseMoveSpeed, () => MovementSpeed -= 1);
            task.Begin += OnRenderBegin;
            Editor.Instance.OnUpdate += OnEditorViewportUpdate;
        }
        /// <summary>
        /// Called in Editor update loop use it for any logic not linked with Widgets
        /// </summary>
        /// <param name="deltaTime"></param>
        protected virtual void OnEditorViewportUpdate(float deltaTime)
        {
            Profiler.BeginEvent("EditorViewportUpdate");
            Profiler.BeginEvent("Input.Update.ProccesInput");
            _input.ProccesInput(this);
            Profiler.EndEvent();
            Profiler.BeginEvent("Gizmos.Update");
            GizmosCollection gizmos = Gizmos;
            if (gizmos != null)
            {
                GizmoBase active = gizmos.Active;
                active?.Update(deltaTime);
            }
            Profiler.EndEvent();

            if (_camera != null)
            {
                Profiler.BeginEvent("ViewportCamera.Update");
                _camera.Update(deltaTime);
                Profiler.EndEvent();
            }
            Profiler.BeginEvent("Input.Update.MouseCapture");
            if (IsGizmoControllingMouse)
            {
                CaptureMouse();
            }
            else if (_input.IsControllingMouse)
            {
                CaptureMouse();
            }
            else
            {
                ReliseMouse();
            }
            Profiler.EndEvent();
            Profiler.BeginEvent("Input.Update.ConsumeInput");
            _input.ConsumeInput();
            Profiler.EndEvent();
            Profiler.EndEvent();
        }


        /// <summary>
        /// Orients the viewport.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        protected void OrientViewport(Quaternion orientation)
        {
            Quaternion quat = orientation;
            OrientViewport(ref quat);
        }

        /// <summary>
        /// Orients the viewport.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        protected virtual void OrientViewport(ref Quaternion orientation)
        {
            Camera.SetArcBallView(orientation, Camera.OrbitCenter, Camera.OrbitRadius);
        }

        private void OnEditorOptionsChanged(EditorOptions options)
        {
            Camera.MouseSensitivity = options.Viewport.MouseSensitivity;
        }

        private void OnRenderBegin(RenderTask task, GPUContext context)
        {
            SceneRenderTask sceneTask = (SceneRenderTask)task;
            RenderView view = sceneTask.View;
            CopyViewData(ref view);
            sceneTask.View = view;
        }

        /// <summary>
        /// Takes the screenshot of the current viewport.
        /// </summary>
        /// <param name="path">The output file path. Set null to use default value.</param>
        public void TakeScreenshot(string path = null)
        {
            Screenshot.Capture(base.Task, path);
        }

        /// <summary>
        /// Copies the render view data to <see cref="T:FlaxEngine.RenderView" /> structure.
        /// </summary>
        /// <param name="view">The view.</param>
        public void CopyViewData(ref RenderView view)
        {
            Vector3 position = Camera.Translation;
            LargeWorlds.UpdateOrigin(ref view.Origin, position);
            view.Position = position - view.Origin;
            view.Direction = Camera.Forward;
            view.Near = Camera.NearPlane;
            view.Far = Camera.FarPlane;
            view.Projection = Camera.ProjectionMatrix;
            view.View = Camera.ViewMatrix;
            view.UpdateCachedData();
        }

        /// <summary>
        /// Converts the mouse position to the ray (in world space of the viewport).
        /// </summary>
        /// <param name="mousePosition">The mouse position.</param>
        /// <returns>The result ray.</returns>
        public Ray ConvertMouseToRay(in Float2 mousePosition)
        {
            FlaxEngine.Viewport viewport = Task.Viewport;
            Vector3 viewOrigin = Task.View.Origin;
            Float3 position = Camera.Translation - viewOrigin;
            Matrix ivp = Task.View.IVP;
            Vector3 nearPoint = new Vector3(mousePosition, 0f);
            Vector3 farPoint = new Vector3(mousePosition, 1f);
            viewport.Unproject(ref nearPoint, ref ivp, out nearPoint);
            viewport.Unproject(ref farPoint, ref ivp, out farPoint);
            Vector3 direction = farPoint - nearPoint;
            direction.Normalize();
            return new Ray(position, direction);
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);
            PerformLayout(false);
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
            bool isDuringBreakpointHang = Editor.Instance.Simulation.IsDuringBreakpointHang;
            if (isDuringBreakpointHang)
            {
                Rectangle bounds = new Rectangle(Float2.Zero, base.Size);
                Render2D.FillRectangle(bounds, new Color(0f, 0f, 0f, 0.2f));
                Render2D.DrawText(Style.Current.FontLarge, "Debugger breakpoint hit...", bounds, Color.White, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap, 1f, 1f);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Editor.Instance.Options.OptionsChanged -= OnEditorOptionsChanged;
            Editor.Instance.OnUpdate -= OnEditorViewportUpdate;
            base.OnDestroy();
        }
    }
}
