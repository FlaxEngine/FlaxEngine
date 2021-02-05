// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.IO;
using System.Text;
using FlaxEngine.GUI;
using Newtonsoft.Json;

namespace FlaxEngine
{
    /// <summary>
    /// The canvas rendering modes.
    /// </summary>
    public enum CanvasRenderMode
    {
        /// <summary>
        /// The screen space rendering mode that places UI elements on the screen rendered on top of the scene. If the screen is resized or changes resolution, the Canvas will automatically change size to match this.
        /// </summary>
        [Tooltip("The screen space rendering mode that places UI elements on the screen rendered on top of the scene. If the screen is resized or changes resolution, the Canvas will automatically change size to match this.")]
        ScreenSpace = 0,

        /// <summary>
        /// The camera space rendering mode that places Canvas in a given distance in front of a specified Camera. The UI elements are rendered by this camera, which means that the Camera settings affect the appearance of the UI. If the Camera is set to Perspective, the UI elements will be rendered with perspective, and the amount of perspective distortion can be controlled by the Camera Field of View. If the screen is resized, changes resolution, or the camera frustum changes, the Canvas will automatically change size to match as well.
        /// </summary>
        [Tooltip("The camera space rendering mode that places Canvas in a given distance in front of a specified Camera. The UI elements are rendered by this camera, which means that the Camera settings affect the appearance of the UI. If the Camera is set to Perspective, the UI elements will be rendered with perspective, and the amount of perspective distortion can be controlled by the Camera Field of View. If the screen is resized, changes resolution, or the camera frustum changes, the Canvas will automatically change size to match as well.")]
        CameraSpace = 1,

        /// <summary>
        /// The world space rendering mode that places Canvas as any other object in the scene. The size of the Canvas can be set manually using its Transform, and UI elements will render in front of or behind other objects in the scene based on 3D placement. This is useful for UIs that are meant to be a part of the world. This is also known as a 'diegetic interface'.
        /// </summary>
        [Tooltip("The world space rendering mode that places Canvas as any other object in the scene. The size of the Canvas can be set manually using its Transform, and UI elements will render in front of or behind other objects in the scene based on 3D placement. This is useful for UIs that are meant to be a part of the world. This is also known as a 'diegetic interface'.")]
        WorldSpace = 2,
    }

    /// <summary>
    /// PostFx used to render the <see cref="UICanvas"/>. Used when render mode is <see cref="CanvasRenderMode.CameraSpace"/> or <see cref="CanvasRenderMode.WorldSpace"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.PostProcessEffect" />
    [HideInEditor]
    public sealed class CanvasRenderer : PostProcessEffect
    {
        /// <summary>
        /// The canvas to render.
        /// </summary>
        public UICanvas Canvas;

        /// <inheritdoc />
        public override bool UseSingleTarget => true;

        /// <inheritdoc />
        public override PostProcessEffectLocation Location => Canvas.RenderLocation;

        /// <inheritdoc />
        public override int Order => Canvas.Order;

        /// <inheritdoc />
        public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
        {
            // TODO: apply frustum culling to skip rendering if canvas is not in a viewport

            Profiler.BeginEventGPU("UI Canvas");

            // Calculate rendering matrix (world*view*projection)
            Matrix viewProjectionMatrix;
            Matrix worldMatrix;
            Canvas.GetWorldMatrix(out worldMatrix);
            Matrix viewMatrix;
            Matrix.Multiply(ref worldMatrix, ref renderContext.View.View, out viewMatrix);
            Matrix.Multiply(ref viewMatrix, ref renderContext.View.Projection, out viewProjectionMatrix);

            // Pick a depth buffer
            GPUTexture depthBuffer = Canvas.IgnoreDepth ? null : renderContext.Buffers.DepthBuffer;

            // Render GUI in 3D
            Render2D.CallDrawing(Canvas.GUI, context, input, depthBuffer, ref viewProjectionMatrix);

            Profiler.EndEventGPU();
        }
    }

    partial class UICanvas
    {
        private CanvasRenderMode _renderMode;
        private readonly CanvasRootControl _guiRoot;
        private CanvasRenderer _renderer;
        private bool _isLoading;

        /// <summary>
        /// Gets or sets the canvas rendering mode.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Canvas"), Tooltip("Canvas rendering mode.")]
        public CanvasRenderMode RenderMode
        {
            get => _renderMode;
            set
            {
                if (_renderMode != value)
                {
                    var previous = _renderMode;

                    _renderMode = value;

                    Setup();

                    // Reset size
                    if (previous == CanvasRenderMode.ScreenSpace && _renderMode == CanvasRenderMode.WorldSpace)
                        Size = new Vector2(500, 500);
                }
            }
        }

        /// <summary>
        /// Gets or sets the canvas rendering location within rendering pipeline. Used only in <see cref="CanvasRenderMode.CameraSpace"/> or <see cref="CanvasRenderMode.WorldSpace"/>.
        /// </summary>
        [EditorOrder(13), EditorDisplay("Canvas"), VisibleIf(nameof(Editor_Is3D)), Tooltip("Canvas rendering location within the rendering pipeline. Change this if you want GUI to affect the lighting or post processing effects like bloom.")]
        public PostProcessEffectLocation RenderLocation { get; set; } = PostProcessEffectLocation.Default;

        private int _order;

        /// <summary>
        /// Gets or sets the canvas rendering and input events gather order. Created GUI canvas objects are sorted before rendering (from the lowest order to the highest order). Canvas with the highest order can handle input event first.
        /// </summary>
        [EditorOrder(14), EditorDisplay("Canvas"), Tooltip("The canvas rendering and input events gather order. Created GUI canvas objects are sorted before rendering (from the lowest order to the highest order). Canvas with the highest order can handle input event first.")]
        public int Order
        {
            get => _order;
            set
            {
                if (_order != value)
                {
                    _order = value;
                    RootControl.CanvasRoot.SortCanvases();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether canvas can receive the input events.
        /// </summary>
        [EditorOrder(15), EditorDisplay("Canvas"), Tooltip("If checked, canvas can receive the input events.")]
        public bool ReceivesEvents { get; set; } = true;

        private bool Editor_Is3D => _renderMode != CanvasRenderMode.ScreenSpace;

        private bool Editor_IsWorldSpace => _renderMode == CanvasRenderMode.WorldSpace;

        private bool Editor_IsCameraSpace => _renderMode == CanvasRenderMode.CameraSpace;

        /// <summary>
        /// Gets or sets the size of the canvas. Used only in <see cref="CanvasRenderMode.CameraSpace"/> or <see cref="CanvasRenderMode.WorldSpace"/>.
        /// </summary>
        [EditorOrder(20), EditorDisplay("Canvas"), VisibleIf(nameof(Editor_IsWorldSpace)), Tooltip("Canvas size.")]
        public Vector2 Size
        {
            get => _guiRoot.Size;
            set
            {
                if (_renderMode == CanvasRenderMode.WorldSpace || _isLoading)
                {
                    _guiRoot.Size = value;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ignore scene depth when rendering the GUI (scene objects won't cover the interface).
        /// </summary>
        [EditorOrder(30), EditorDisplay("Canvas"), VisibleIf(nameof(Editor_Is3D)), Tooltip("If checked, scene depth will be ignored when rendering the GUI (scene objects won't cover the interface).")]
        public bool IgnoreDepth { get; set; } = false;

        /// <summary>
        /// Gets or sets the camera used to place the GUI when render mode is set to <see cref="CanvasRenderMode.CameraSpace"/>.
        /// </summary>
        [EditorOrder(50), EditorDisplay("Canvas"), VisibleIf(nameof(Editor_IsCameraSpace)), Tooltip("Camera used to place the GUI when RenderMode is set to CameraSpace")]
        public Camera RenderCamera { get; set; }

        /// <summary>
        /// Gets or sets the distance from the <see cref="RenderCamera"/> to place the plane with GUI. If the screen is resized, changes resolution, or the camera frustum changes, the Canvas will automatically change size to match as well.
        /// </summary>
        [EditorOrder(60), Limit(0.01f), EditorDisplay("Canvas"), VisibleIf(nameof(Editor_IsCameraSpace)), Tooltip("Distance from the RenderCamera to place the plane with GUI. If the screen is resized, changes resolution, or the camera frustum changes, the Canvas will automatically change size to match as well.")]
        public float Distance { get; set; } = 500;

        /// <summary>
        /// Gets the canvas GUI root control.
        /// </summary>
        public CanvasRootControl GUI => _guiRoot;

        /// <summary>
        /// Delegate schema for the callback used to perform custom canvas intersection test. Can be used to implement a canvas that has a holes or non-rectangular shape.
        /// </summary>
        /// <param name="location">The location of the point to test in coordinates of the canvas root control (see <see cref="GUI"/>).</param>
        /// <returns>True if canvas was hit, otherwise false.</returns>
        public delegate bool TestCanvasIntersectionDelegate(ref Vector2 location);

        /// <summary>
        /// The callback used to perform custom canvas intersection test. Can be used to implement a canvas that has a holes or non-rectangular shape.
        /// </summary>
        [HideInEditor]
        public TestCanvasIntersectionDelegate TestCanvasIntersection;

        /// <summary>
        /// Delegate schema for callback used to evaluate the world-space ray from the screen-space position (eg. project mouse position).
        /// </summary>
        /// <param name="location">The location in screen-space.</param>
        /// <param name="ray">The output ray in world-space.</param>
        public delegate void CalculateRayDelegate(ref Vector2 location, out Ray ray);

        /// <summary>
        /// The current implementation of the <see cref="CalculateRayDelegate"/> used to calculate the mouse ray in 3D from the 2D location. Cannot be null.
        /// </summary>
        public static CalculateRayDelegate CalculateRay = DefaultCalculateRay;

        /// <summary>
        /// The default implementation of the <see cref="CalculateRayDelegate"/> that uses the <see cref="Camera.MainCamera"/> to evaluate the 3D ray.
        /// </summary>
        /// <param name="location">The location in screen-space.</param>
        /// <param name="ray">The output ray in world-space.</param>
        public static void DefaultCalculateRay(ref Vector2 location, out Ray ray)
        {
            var camera = Camera.MainCamera;
            if (camera)
            {
                ray = camera.ConvertMouseToRay(location);
            }
            else
            {
                ray = new Ray(Vector3.Zero, Vector3.Forward);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UICanvas"/> class.
        /// </summary>
        public UICanvas()
        {
            _guiRoot = new CanvasRootControl(this);
            _guiRoot.IsLayoutLocked = false;
        }

        /// <summary>
        /// Gets the world-space oriented bounding box that contains a 3D canvas.
        /// </summary>
        public OrientedBoundingBox Bounds
        {
            get
            {
                OrientedBoundingBox bounds = new OrientedBoundingBox();
                bounds.Extents = new Vector3(_guiRoot.Size * 0.5f, Mathf.Epsilon);
                Matrix world;
                GetWorldMatrix(out world);
                Matrix offset;
                Matrix.Translation(bounds.Extents.X, bounds.Extents.Y, 0, out offset);
                Matrix.Multiply(ref offset, ref world, out bounds.Transformation);
                return bounds;
            }
        }

        /// <summary>
        /// Gets a value indicating whether canvas is 2D (screen-space).
        /// </summary>
        public bool Is2D => _renderMode == CanvasRenderMode.ScreenSpace;

        /// <summary>
        /// Gets a value indicating whether canvas is 3D (world-space or camera-space).
        /// </summary>
        public bool Is3D => _renderMode != CanvasRenderMode.ScreenSpace;

        /// <summary>
        /// Gets the world matrix used to transform the GUI from the local space to the world space. Handles canvas rendering mode
        /// </summary>
        /// <param name="world">The world.</param>
        public void GetWorldMatrix(out Matrix world)
        {
            if (_renderMode == CanvasRenderMode.WorldSpace)
            {
                // In 3D world
                GetLocalToWorldMatrix(out world);
            }
            else if (_renderMode == CanvasRenderMode.CameraSpace)
            {
                Matrix tmp1, tmp2;
                Vector3 viewPos, viewUp, viewForward, pos;
                Quaternion viewRot;

                // Use default camera is not specified
                var camera = RenderCamera ?? Camera.MainCamera;

#if FLAX_EDITOR
                if (_editorTask)
                {
                    // Use editor viewport task to override Camera Space placement
                    var view = _editorTask.View;
                    var frustum = view.Frustum;
                    if (!frustum.IsOrthographic)
                        _guiRoot.Size = new Vector2(frustum.GetWidthAtDepth(Distance), frustum.GetHeightAtDepth(Distance));
                    else
                        _guiRoot.Size = _editorTask.Viewport.Size;
                    Matrix.Translation(_guiRoot.Width / -2.0f, _guiRoot.Height / -2.0f, 0, out world);
                    Matrix.RotationYawPitchRoll(Mathf.Pi, Mathf.Pi, 0, out tmp2);
                    Matrix.Multiply(ref world, ref tmp2, out tmp1);
                    viewPos = view.Position;
                    viewRot = view.Direction != Vector3.Up ? Quaternion.LookRotation(view.Direction, Vector3.Up) : Quaternion.LookRotation(view.Direction, Vector3.Right);
                    viewUp = Vector3.Up * viewRot;
                    viewForward = view.Direction;
                    pos = view.Position + view.Direction * Distance;
                    Matrix.Billboard(ref pos, ref viewPos, ref viewUp, ref viewForward, out tmp2);
                    Matrix.Multiply(ref tmp1, ref tmp2, out world);
                    return;
                }
#endif

                // Adjust GUI size to the viewport size at the given distance form the camera
                var viewport = camera.Viewport;
                if (camera.UsePerspective)
                {
                    camera.GetMatrices(out tmp1, out var tmp3, ref viewport);
                    Matrix.Multiply(ref tmp1, ref tmp3, out tmp2);
                    var frustum = new BoundingFrustum(tmp2);
                    _guiRoot.Size = new Vector2(frustum.GetWidthAtDepth(Distance), frustum.GetHeightAtDepth(Distance));
                }
                else
                {
                    _guiRoot.Size = viewport.Size * camera.OrthographicScale;
                }

                // Center viewport (and flip)
                Matrix.Translation(_guiRoot.Width / -2.0f, _guiRoot.Height / -2.0f, 0, out world);
                Matrix.RotationYawPitchRoll(Mathf.Pi, Mathf.Pi, 0, out tmp2);
                Matrix.Multiply(ref world, ref tmp2, out tmp1);

                // In front of the camera
                viewPos = camera.Position;
                viewRot = camera.Orientation;
                viewUp = Vector3.Up * viewRot;
                viewForward = Vector3.Forward * viewRot;
                pos = viewPos + viewForward * Distance;
                Matrix.Billboard(ref pos, ref viewPos, ref viewUp, ref viewForward, out tmp2);

                Matrix.Multiply(ref tmp1, ref tmp2, out world);
            }
            else
            {
                // Direct projection
                world = Matrix.Identity;
            }
        }

        private void Setup()
        {
            if (_isLoading)
                return;

            switch (_renderMode)
            {
            case CanvasRenderMode.ScreenSpace:
            {
                // Fill the screen area
                _guiRoot.AnchorPreset = AnchorPresets.StretchAll;
                _guiRoot.Offsets = Margin.Zero;
                if (_renderer)
                {
#if FLAX_EDITOR
                    _editorTask?.CustomPostFx.Remove(_renderer);
#endif
                    SceneRenderTask.GlobalCustomPostFx.Remove(_renderer);
                    _renderer.Canvas = null;
                    Destroy(_renderer);
                    _renderer = null;
                }
#if FLAX_EDITOR
                if (_editorRoot != null)
                    _guiRoot.Parent = _editorRoot;
#endif
                break;
            }
            case CanvasRenderMode.CameraSpace:
            case CanvasRenderMode.WorldSpace:
            {
                // Render canvas manually
                _guiRoot.AnchorPreset = AnchorPresets.TopLeft;
#if FLAX_EDITOR
                if (_editorRoot != null && _guiRoot != null)
                    _guiRoot.Parent = null;
#endif
                if (_renderer == null)
                {
                    _renderer = New<CanvasRenderer>();
                    _renderer.Canvas = this;
                    if (IsActiveInHierarchy && Scene)
                    {
#if FLAX_EDITOR
                        if (_editorTask != null)
                        {
                            _editorTask.CustomPostFx.Add(_renderer);
                            break;
                        }
#endif
                        SceneRenderTask.GlobalCustomPostFx.Add(_renderer);
                    }
#if FLAX_EDITOR
                    else if (_editorTask != null && IsActiveInHierarchy)
                    {
                        _editorTask.CustomPostFx.Add(_renderer);
                    }
#endif
                }
                break;
            }
            }
        }

        internal string Serialize()
        {
            StringBuilder sb = new StringBuilder(256);
            StringWriter sw = new StringWriter(sb, CultureInfo.InvariantCulture);
            using (JsonTextWriter jsonWriter = new JsonTextWriter(sw))
            {
                jsonWriter.IndentChar = '\t';
                jsonWriter.Indentation = 1;
                jsonWriter.Formatting = Formatting.Indented;

                jsonWriter.WriteStartObject();

                jsonWriter.WritePropertyName("RenderMode");
                jsonWriter.WriteValue(_renderMode);

                jsonWriter.WritePropertyName("RenderLocation");
                jsonWriter.WriteValue(RenderLocation);

                jsonWriter.WritePropertyName("Order");
                jsonWriter.WriteValue(Order);

                jsonWriter.WritePropertyName("ReceivesEvents");
                jsonWriter.WriteValue(ReceivesEvents);

                jsonWriter.WritePropertyName("IgnoreDepth");
                jsonWriter.WriteValue(IgnoreDepth);

                jsonWriter.WritePropertyName("RenderCamera");
                jsonWriter.WriteValue(Json.JsonSerializer.GetStringID(RenderCamera));

                jsonWriter.WritePropertyName("Distance");
                jsonWriter.WriteValue(Distance);

                jsonWriter.WritePropertyName("Size");
                jsonWriter.WriteStartObject();
                jsonWriter.WritePropertyName("X");
                jsonWriter.WriteValue(Size.X);
                jsonWriter.WritePropertyName("Y");
                jsonWriter.WriteValue(Size.Y);
                jsonWriter.WriteEndObject();

                jsonWriter.WriteEndObject();
            }

            return sw.ToString();
        }

        internal string SerializeDiff(UICanvas other)
        {
            StringBuilder sb = new StringBuilder(256);
            StringWriter sw = new StringWriter(sb, CultureInfo.InvariantCulture);
            using (JsonTextWriter jsonWriter = new JsonTextWriter(sw))
            {
                jsonWriter.IndentChar = '\t';
                jsonWriter.Indentation = 1;
                jsonWriter.Formatting = Formatting.Indented;

                jsonWriter.WriteStartObject();

                if (_renderMode != other._renderMode)
                {
                    jsonWriter.WritePropertyName("RenderMode");
                    jsonWriter.WriteValue(_renderMode);
                }

                if (RenderLocation != other.RenderLocation)
                {
                    jsonWriter.WritePropertyName("RenderLocation");
                    jsonWriter.WriteValue(RenderLocation);
                }

                if (Order != other.Order)
                {
                    jsonWriter.WritePropertyName("Order");
                    jsonWriter.WriteValue(Order);
                }

                if (ReceivesEvents != other.ReceivesEvents)
                {
                    jsonWriter.WritePropertyName("ReceivesEvents");
                    jsonWriter.WriteValue(ReceivesEvents);
                }

                if (IgnoreDepth != other.IgnoreDepth)
                {
                    jsonWriter.WritePropertyName("IgnoreDepth");
                    jsonWriter.WriteValue(IgnoreDepth);
                }

                if (RenderCamera != other.RenderCamera)
                {
                    jsonWriter.WritePropertyName("RenderCamera");
                    jsonWriter.WriteValue(Json.JsonSerializer.GetStringID(RenderCamera));
                }

                if (Mathf.Abs(Distance - other.Distance) > Mathf.Epsilon)
                {
                    jsonWriter.WritePropertyName("Distance");
                    jsonWriter.WriteValue(Distance);
                }

                if (Size != other.Size)
                {
                    jsonWriter.WritePropertyName("Size");
                    jsonWriter.WriteStartObject();
                    jsonWriter.WritePropertyName("X");
                    jsonWriter.WriteValue(Size.X);
                    jsonWriter.WritePropertyName("Y");
                    jsonWriter.WriteValue(Size.Y);
                    jsonWriter.WriteEndObject();
                }

                jsonWriter.WriteEndObject();
            }

            return sw.ToString();
        }

        internal void Deserialize(string json)
        {
            _isLoading = true;
            Json.JsonSerializer.Deserialize(this, json);
            _isLoading = false;
        }

        internal void PostDeserialize()
        {
            Setup();
        }

        internal void OnEnable()
        {
#if FLAX_EDITOR
            _guiRoot.Parent = _editorRoot ?? RootControl.CanvasRoot;
#else
            _guiRoot.Parent = RootControl.CanvasRoot;
#endif

            if (_renderer)
            {
#if FLAX_EDITOR
                if (_editorTask != null)
                {
                    _editorTask.CustomPostFx.Add(_renderer);
                    return;
                }
#endif
                SceneRenderTask.GlobalCustomPostFx.Add(_renderer);
            }
        }

        internal void OnDisable()
        {
            _guiRoot.Parent = null;

            if (_renderer)
            {
                SceneRenderTask.GlobalCustomPostFx.Remove(_renderer);
            }
        }

        internal void EndPlay()
        {
            if (_renderer)
            {
                SceneRenderTask.GlobalCustomPostFx.Remove(_renderer);
                _renderer.Canvas = null;
                Destroy(_renderer);
                _renderer = null;
            }
        }

#if FLAX_EDITOR
        private SceneRenderTask _editorTask;
        private ContainerControl _editorRoot;

        internal void EditorOverride(SceneRenderTask task, ContainerControl root)
        {
            if (_editorTask != null && _renderer != null)
                _editorTask.CustomPostFx.Remove(_renderer);
            if (_editorRoot != null && _guiRoot != null)
                _guiRoot.Parent = null;

            _editorTask = task;
            _editorRoot = root;
            Setup();

            if (RenderMode == CanvasRenderMode.ScreenSpace && _editorRoot != null && _guiRoot != null)
                _guiRoot.Parent = _editorRoot;
        }
#endif
    }
}
