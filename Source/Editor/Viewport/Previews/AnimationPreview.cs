// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Animation asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AnimatedModelPreview" />
    public class AnimationPreview : AnimatedModelPreview
    {
        private ViewportWidgetButton _playPauseButton;

        /// <summary>
        /// Gets or sets a value indicating whether enable root motion.
        /// </summary>
        public bool RootMotion
        {
            get => !_snapToOrigin;
            set
            {
                if (!_snapToOrigin == value)
                    return;
                _snapToOrigin = !value;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AnimationPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public AnimationPreview(bool useWidgets)
        : base(useWidgets)
        {
            PlayAnimation = true;
            PlayAnimationChanged += OnPlayAnimationChanged;
            _snapToOrigin = false;

            // Playback Speed
            {
                var playbackSpeed = ViewWidgetButtonMenu.AddButton("Playback Speed");
                var playbackSpeedValue = new FloatValueBox(-1, 90, 2, 70.0f, -10000.0f, 10000.0f, 0.001f)
                {
                    Parent = playbackSpeed
                };
                playbackSpeedValue.ValueChanged += () => PlaySpeed = playbackSpeedValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => playbackSpeedValue.Value = PlaySpeed;
            }

            // Play/Pause widget
            {
                var playPauseWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                _playPauseButton = new ViewportWidgetButton(null, Editor.Instance.Icons.Pause64)
                {
                    TooltipText = "Animation playback play (F5) or pause (F6)",
                    Parent = playPauseWidget,
                };
                _playPauseButton.Clicked += button => PlayAnimation = !PlayAnimation;
                playPauseWidget.Parent = this;
            }

            // Play Root Motion
            {
                var button = ViewWidgetButtonMenu.AddButton("Root Motion");
                var buttonValue = new CheckBox(90, 2, !_snapToOrigin)
                {
                    Parent = button
                };
                buttonValue.StateChanged += checkbox => RootMotion = !RootMotion;
                ViewWidgetButtonMenu.VisibleChanged += control => buttonValue.Checked = RootMotion;
            }

            // Enable shadows
            PreviewLight.ShadowsMode = ShadowsCastingMode.All;
            PreviewLight.CascadeCount = 2;
            PreviewLight.ShadowsDistance = 1000.0f;
            Task.ViewFlags |= ViewFlags.Shadows;
        }

        private void OnPlayAnimationChanged()
        {
            _playPauseButton.Icon = PlayAnimation ? Editor.Instance.Icons.Pause64 : Editor.Instance.Icons.Play64;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;
            var skinnedModel = SkinnedModel;
            if (skinnedModel == null)
            {
                Render2D.DrawText(style.FontLarge, "Missing Base Model", new Rectangle(Float2.Zero, Size), Color.Red, TextAlignment.Center, TextAlignment.Center, TextWrapping.WrapWords);
            }
            else if (!skinnedModel.IsLoaded)
            {
                Render2D.DrawText(style.FontLarge, skinnedModel.LastLoadFailed ? "Failed to load" : "Loading...", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
            }
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            var inputOptions = Editor.Instance.Options.Options.Input;
            if (inputOptions.Play.Process(this, key))
            {
                PlayAnimation = true;
                return true;
            }
            if (inputOptions.Pause.Process(this, key))
            {
                PlayAnimation = false;
                return true;
            }
            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _playPauseButton = null;

            base.OnDestroy();
        }
    }
}
