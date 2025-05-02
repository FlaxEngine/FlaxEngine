// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Viewport.Cameras;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Generic asset preview editor viewport base class.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.EditorViewport" />
    public abstract class AssetPreview : EditorViewport, IEditorPrimitivesOwner
    {
        internal ContextMenuButton _showDefaultSceneButton;
        private IntPtr _debugDrawContext;
        private bool _debugDrawEnable;
        private bool _editorPrimitivesEnable;
        private EditorPrimitives _editorPrimitives;

        /// <summary>
        /// The preview light. Allows to modify rendering settings.
        /// </summary>
        public DirectionalLight PreviewLight;

        /// <summary>
        /// The env probe. Allows to modify rendering settings.
        /// </summary>
        public EnvironmentProbe EnvProbe;

        /// <summary>
        /// The sky. Allows to modify rendering settings.
        /// </summary>
        public Sky Sky;

        /// <summary>
        /// The sky light. Allows to modify rendering settings.
        /// </summary>
        public SkyLight SkyLight;

        /// <summary>
        /// Gets the post fx volume. Allows to modify rendering settings.
        /// </summary>
        public PostFxVolume PostFxVolume;

        /// <summary>
        /// Gets or sets a value indicating whether show default scene actors (sky, env probe, skylight, directional light, etc.).
        /// </summary>
        public bool ShowDefaultSceneActors
        {
            get => PreviewLight.IsActive;
            set
            {
                if (ShowDefaultSceneActors != value)
                {
                    PreviewLight.IsActive = value;
                    EnvProbe.IsActive = value;
                    Sky.IsActive = value;
                    SkyLight.IsActive = value;

                    if (_showDefaultSceneButton != null)
                        _showDefaultSceneButton.Checked = value;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether draw <see cref="DebugDraw"/> shapes.
        /// </summary>
        public bool ShowDebugDraw
        {
            get => _debugDrawEnable;
            set
            {
                if (_debugDrawEnable == value)
                    return;
                _debugDrawEnable = value;
                if (_debugDrawContext == IntPtr.Zero)
                {
                    _debugDrawContext = DebugDraw.AllocateContext();
                    var view = Task.View;
                    view.Flags |= ViewFlags.DebugDraw;
                    Task.View = view;
                }
                if (value)
                {
                    // Need to show editor primitives to show debug shapes
                    ShowEditorPrimitives = true;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether draw <see cref="DebugDraw"/> shapes and other editor primitives such as gizmos.
        /// </summary>
        public bool ShowEditorPrimitives
        {
            get => _editorPrimitivesEnable;
            set
            {
                if (_editorPrimitivesEnable == value)
                    return;
                _editorPrimitivesEnable = value;
                if (_editorPrimitives == null)
                {
                    _editorPrimitives = Object.New<EditorPrimitives>();
                    _editorPrimitives.Viewport = this;
                    Task.AddCustomPostFx(_editorPrimitives);
                    Task.PostRender += OnPostRender;
                    var view = Task.View;
                    view.Flags |= ViewFlags.CustomPostProcess;
                    Task.View = view;
                }
            }
        }

        /// <summary>
        /// Gets the editor primitives renderer. Valid only if <see cref="ShowEditorPrimitives"/> is true.
        /// </summary>
        public EditorPrimitives EditorPrimitives => _editorPrimitives;

        /// <summary>
        /// Custom debug drawing event (via <see cref="FlaxEngine.DebugDraw"/>).
        /// </summary>
        public event CustomDebugDrawDelegate CustomDebugDraw;

        /// <summary>
        /// Debug shapes drawing delegate.
        /// </summary>
        /// <param name="context">The GPU context.</param>
        /// <param name="renderContext">The render context.</param>
        public delegate void CustomDebugDrawDelegate(GPUContext context, ref RenderContext renderContext);

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">If set to <c>true</c> use widgets for viewport, otherwise hide them.</param>
        /// <param name="orbitRadius">The initial orbit radius.</param>
        protected AssetPreview(bool useWidgets, float orbitRadius = 50.0f)
        : this(useWidgets, new ArcBallCamera(Vector3.Zero, orbitRadius))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">If set to <c>true</c> use widgets for viewport, otherwise hide them.</param>
        /// <param name="camera">The camera controller.</param>
        protected AssetPreview(bool useWidgets, ViewportCamera camera)
        : base(Object.New<SceneRenderTask>(), camera, useWidgets)
        {
            Task.ViewFlags = ViewFlags.DefaultAssetPreview;
            Task.AllowGlobalCustomPostFx = false;

            Real orbitRadius = 200.0f;
            if (camera is ArcBallCamera arcBallCamera)
                orbitRadius = arcBallCamera.OrbitRadius;
            camera.SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), Vector3.Zero, orbitRadius);

            if (useWidgets)
            {
                // Show Default Scene
                _showDefaultSceneButton = ViewWidgetShowMenu.AddButton("Default Scene", () => ShowDefaultSceneActors = !ShowDefaultSceneActors);
                _showDefaultSceneButton.Checked = true;
                _showDefaultSceneButton.CloseMenuOnClick = false;
            }

            // Setup preview scene
            PreviewLight = new DirectionalLight
            {
                Brightness = 8.0f,
                ShadowsMode = ShadowsCastingMode.None,
                Orientation = Quaternion.Euler(new Vector3(52.1477f, -109.109f, -111.739f))
            };
            //
            EnvProbe = new EnvironmentProbe
            {
                CustomProbe = FlaxEngine.Content.LoadAsyncInternal<CubeTexture>(EditorAssets.DefaultSkyCubeTexture)
            };
            //
            Sky = new Sky
            {
                SunLight = PreviewLight,
                SunPower = 9.0f
            };
            //
            SkyLight = new SkyLight
            {
                Mode = SkyLight.Modes.CustomTexture,
                Brightness = 2.1f,
                CustomTexture = EnvProbe.CustomProbe
            };
            //
            PostFxVolume = new PostFxVolume
            {
                IsBounded = false
            };

            // Link actors for rendering
            Task.ActorsSource = ActorsSources.CustomActors;
            Task.AddCustomActor(PreviewLight);
            Task.AddCustomActor(EnvProbe);
            Task.AddCustomActor(Sky);
            Task.AddCustomActor(SkyLight);
            Task.AddCustomActor(PostFxVolume);
        }

        private void OnPostRender(GPUContext context, ref RenderContext renderContext)
        {
            if (renderContext.View.Mode != ViewMode.Default && _editorPrimitives && _editorPrimitives.CanRender())
            {
                // Render editor primitives, gizmo and debug shapes in debug view modes
                // Note: can use Output buffer as both input and output because EditorPrimitives is using a intermediate buffers
                _editorPrimitives.Render(context, ref renderContext, renderContext.Task.Output, renderContext.Task.Output);
            }
        }

        /// <summary>
        /// Called when drawing debug shapes with <see cref="DebugDraw"/> for this viewport.
        /// </summary>
        /// <param name="context">The GPU context.</param>
        /// <param name="renderContext">The render context.</param>
        protected virtual void OnDebugDraw(GPUContext context, ref RenderContext renderContext)
        {
        }

        /// <inheritdoc />
        public override bool HasLoadedAssets => base.HasLoadedAssets && Sky.HasContentLoaded && EnvProbe.HasContentLoaded && PostFxVolume.HasContentLoaded;

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            Object.Destroy(ref PreviewLight);
            Object.Destroy(ref EnvProbe);
            Object.Destroy(ref Sky);
            Object.Destroy(ref SkyLight);
            Object.Destroy(ref PostFxVolume);
            Object.Destroy(ref _editorPrimitives);
            if (_debugDrawContext != IntPtr.Zero)
            {
                DebugDraw.FreeContext(_debugDrawContext);
                _debugDrawContext = IntPtr.Zero;
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        public virtual void DrawEditorPrimitives(GPUContext context, ref RenderContext renderContext, GPUTexture target, GPUTexture targetDepth)
        {
            // Draw selected objects debug shapes and visuals
            if (ShowDebugDraw && (renderContext.View.Flags & ViewFlags.DebugDraw) == ViewFlags.DebugDraw)
            {
                DebugDraw.SetContext(_debugDrawContext);
                DebugDraw.UpdateContext(_debugDrawContext, 1.0f / Mathf.Max(Engine.FramesPerSecond, 1));
                CustomDebugDraw?.Invoke(context, ref renderContext);
                OnDebugDraw(context, ref renderContext);
                DebugDraw.Draw(ref renderContext, target.View(), targetDepth.View(), true);
                DebugDraw.SetContext(IntPtr.Zero);
            }
        }
    }
}
