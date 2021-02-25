// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    public abstract class AssetPreview : EditorViewport
    {
        private ContextMenuButton _showDefaultSceneButton;

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

            var orbitRadius = 200.0f;
            if (camera is ArcBallCamera arcBallCamera)
                orbitRadius = arcBallCamera.OrbitRadius;
            camera.SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), Vector3.Zero, orbitRadius);

            if (useWidgets)
            {
                // Show Default Scene
                _showDefaultSceneButton = ViewWidgetShowMenu.AddButton("Default Scene", () => ShowDefaultSceneActors = !ShowDefaultSceneActors);
                _showDefaultSceneButton.Checked = true;
            }

            // Setup preview scene
            PreviewLight = new DirectionalLight();
            PreviewLight.Brightness = 8.0f;
            PreviewLight.ShadowsMode = ShadowsCastingMode.None;
            PreviewLight.Orientation = Quaternion.Euler(new Vector3(52.1477f, -109.109f, -111.739f));
            //
            EnvProbe = new EnvironmentProbe();
            EnvProbe.AutoUpdate = false;
            EnvProbe.CustomProbe = FlaxEngine.Content.LoadAsyncInternal<CubeTexture>(EditorAssets.DefaultSkyCubeTexture);
            //
            Sky = new Sky();
            Sky.SunLight = PreviewLight;
            Sky.SunPower = 9.0f;
            //
            SkyLight = new SkyLight();
            SkyLight.Mode = SkyLight.Modes.CustomTexture;
            SkyLight.Brightness = 2.1f;
            SkyLight.CustomTexture = EnvProbe.CustomProbe;
            //
            PostFxVolume = new PostFxVolume();
            PostFxVolume.IsBounded = false;

            // Link actors for rendering
            Task.ActorsSource = ActorsSources.CustomActors;
            Task.AddCustomActor(PreviewLight);
            Task.AddCustomActor(EnvProbe);
            Task.AddCustomActor(Sky);
            Task.AddCustomActor(SkyLight);
            Task.AddCustomActor(PostFxVolume);
        }

        /// <inheritdoc />
        public override bool HasLoadedAssets => base.HasLoadedAssets && Sky.HasContentLoaded && EnvProbe.Probe.IsLoaded && PostFxVolume.HasContentLoaded;

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Ensure to cleanup created actor objects
            Object.Destroy(ref PreviewLight);
            Object.Destroy(ref EnvProbe);
            Object.Destroy(ref Sky);
            Object.Destroy(ref SkyLight);
            Object.Destroy(ref PostFxVolume);

            base.OnDestroy();
        }
    }
}
