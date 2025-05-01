// Copyright (c) Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEditor.CustomEditors;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Window used to show and edit current graphics rendering settings via <see cref="Graphics"/> and <see cref="Streaming"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public class GraphicsQualityWindow : EditorWindow
    {
        private static string _optionsName = "GraphicsQualityWindow";
        private ViewModel _viewModel;
        private string _cachedOptions;

        private class ViewModel
        {
            [DefaultValue(true)]
            [EditorOrder(0), EditorDisplay("Rendering"), Tooltip("If checked, the initial values will be from project Graphics Settings, otherwise will be adjusted locally (and stored in project cache).")]
            public bool UseProjectDefaults { get; set; } = true;

            [DefaultValue(false)]
            [EditorOrder(0), EditorDisplay("Rendering", "Use V-Sync"), Tooltip("Enables rendering synchronization with the refresh rate of the display device to avoid \"tearing\" artifacts.")]
            public bool UseVSync
            {
                get => Graphics.UseVSync;
                set => Graphics.UseVSync = value;
            }

            [DefaultValue(Quality.Medium)]
            [EditorOrder(1000), EditorDisplay("Quality", "AA Quality"), Tooltip("Anti Aliasing quality.")]
            public Quality AAQuality
            {
                get => Graphics.AAQuality;
                set => Graphics.AAQuality = value;
            }

            [DefaultValue(Quality.Medium)]
            [EditorOrder(1100), EditorDisplay("Quality", "SSR Quality"), Tooltip("Screen Space Reflections quality.")]
            public Quality SSRQuality
            {
                get => Graphics.SSRQuality;
                set => Graphics.SSRQuality = value;
            }

            [EditorOrder(1200), EditorDisplay("Quality", "SSAO Quality"), Tooltip("Screen Space Ambient Occlusion quality setting.")]
            public Quality SSAOQuality
            {
                get => Graphics.SSAOQuality;
                set => Graphics.SSAOQuality = value;
            }

            [DefaultValue(Quality.High)]
            [EditorOrder(1250), EditorDisplay("Quality", "Volumetric Fog Quality"), Tooltip("Volumetric Fog quality setting.")]
            public Quality VolumetricFogQuality
            {
                get => Graphics.VolumetricFogQuality;
                set => Graphics.VolumetricFogQuality = value;
            }

            [DefaultValue(Quality.High)]
            [EditorOrder(1280), EditorDisplay("Quality"), Tooltip("The Global SDF quality. Controls the volume texture resolution and amount of cascades to use.")]
            public Quality GlobalSDFQuality
            {
                get => Graphics.GlobalSDFQuality;
                set => Graphics.GlobalSDFQuality = value;
            }

            [DefaultValue(Quality.High)]
            [EditorOrder(1290), EditorDisplay("Quality"), Tooltip("The Global Illumination quality. Controls the quality of the GI effect.")]
            public Quality GIQuality
            {
                get => Graphics.GIQuality;
                set => Graphics.GIQuality = value;
            }

            [DefaultValue(Quality.Medium)]
            [EditorOrder(1300), EditorDisplay("Quality", "Shadows Quality"), Tooltip("The shadows quality.")]
            public Quality ShadowsQuality
            {
                get => Graphics.ShadowsQuality;
                set => Graphics.ShadowsQuality = value;
            }

            [DefaultValue(Quality.Medium)]
            [EditorOrder(1310), EditorDisplay("Quality", "Shadow Maps Quality"), Tooltip("The shadow maps quality (textures resolution).")]
            public Quality ShadowMapsQuality
            {
                get => Graphics.ShadowMapsQuality;
                set => Graphics.ShadowMapsQuality = value;
            }

            [NoSerialize, DefaultValue(1.0f), Limit(0.05f, 5, 0)]
            [EditorOrder(1400), EditorDisplay("Quality")]
            [Tooltip("The scale of the rendering resolution relative to the output dimensions. If lower than 1 the scene and postprocessing will be rendered at a lower resolution and upscaled to the output backbuffer.")]
            public float RenderingPercentage
            {
                get => MainRenderTask.Instance.RenderingPercentage;
                set => MainRenderTask.Instance.RenderingPercentage = value;
            }

            [NoSerialize, DefaultValue(RenderingUpscaleLocation.AfterAntiAliasingPass), VisibleIf(nameof(UpscaleLocation_Visible))]
            [EditorOrder(1401), EditorDisplay("Quality")]
            [Tooltip("The image resolution upscale location within rendering pipeline.")]
            public RenderingUpscaleLocation UpscaleLocation
            {
                get => MainRenderTask.Instance.UpscaleLocation;
                set => MainRenderTask.Instance.UpscaleLocation = value;
            }

            private bool UpscaleLocation_Visible => MainRenderTask.Instance.RenderingPercentage < 1.0f;

            [NoSerialize, DefaultValue(1.0f), Limit(0, 1)]
            [EditorOrder(1500), EditorDisplay("Quality"), Tooltip("The global density scale for all foliage instances. The default value is 1. Use values from range 0-1. Lower values decrease amount of foliage instances in-game. Use it to tweak game performance for slower devices.")]
            public float FoliageDensityScale
            {
                get => Foliage.GlobalDensityScale;
                set => Foliage.GlobalDensityScale = value;
            }

            [NoSerialize]
            [EditorOrder(2000), EditorDisplay("Textures", EditorDisplayAttribute.InlineStyle), Tooltip("Textures streaming configuration.")]
            public TextureGroup[] TextureGroups
            {
                get => Streaming.TextureGroups;
                set
                {
                    Streaming.TextureGroups = value;
                    Streaming.RequestStreamingUpdate();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertiesWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public GraphicsQualityWindow(Editor editor)
        : base(editor, true, ScrollBars.Vertical)
        {
            Title = "Graphics Quality";

            var presenter = new CustomEditorPresenter(null);
            presenter.Panel.Offsets = Margin.Zero;
            presenter.Panel.Parent = this;
            presenter.Select(_viewModel = new ViewModel());
        }

        private void OnPlayingStateGameSettingsApplying()
        {
            _cachedOptions = _viewModel.UseProjectDefaults ? null : JsonSerializer.Serialize(_viewModel);
        }

        private void OnPlayingStateGameSettingsApplied()
        {
            if (_cachedOptions != null)
            {
                JsonSerializer.Deserialize(_viewModel, _cachedOptions);
                _cachedOptions = null;
            }
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            if (Editor.ProjectCache.TryGetCustomData(_optionsName, out string options) && !string.IsNullOrEmpty(options))
            {
                // Load cached settings
                JsonSerializer.Deserialize(_viewModel, options);
            }

            var playingState = Editor.StateMachine.PlayingState;
            playingState.GameSettingsApplying += OnPlayingStateGameSettingsApplying;
            playingState.GameSettingsApplied += OnPlayingStateGameSettingsApplied;
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            var playingState = Editor.StateMachine.PlayingState;
            playingState.GameSettingsApplying -= OnPlayingStateGameSettingsApplying;
            playingState.GameSettingsApplied -= OnPlayingStateGameSettingsApplied;

            string data;
            if (_viewModel.UseProjectDefaults)
            {
                // Clear cached settings
                data = string.Empty;
            }
            else
            {
                // Store cached settings
                data = JsonSerializer.Serialize(_viewModel);
            }
            Editor.ProjectCache.SetCustomData(_optionsName, data);
        }
    }
}
