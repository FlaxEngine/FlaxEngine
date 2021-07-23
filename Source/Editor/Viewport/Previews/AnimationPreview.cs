// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        /// Initializes a new instance of the <see cref="AnimationPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public AnimationPreview(bool useWidgets)
        : base(useWidgets)
        {
            PlayAnimation = true;
            PlayAnimationChanged += OnPlayAnimationChanged;

            // Playback Speed
            {
                var playbackSpeed = ViewWidgetButtonMenu.AddButton("Playback Speed");
                var playbackSpeedValue = new FloatValueBox(-1, 90, 2, 70.0f, 0.0f, 10000.0f, 0.001f)
                {
                    Parent = playbackSpeed
                };
                playbackSpeedValue.ValueChanged += () => PreviewActor.UpdateSpeed = playbackSpeedValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => playbackSpeedValue.Value = PreviewActor.UpdateSpeed;
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
                Render2D.DrawText(style.FontLarge, "Missing Base Model", new Rectangle(Vector2.Zero, Size), Color.Red, TextAlignment.Center, TextAlignment.Center, TextWrapping.WrapWords);
            }
            else if (!skinnedModel.IsLoaded)
            {
                Render2D.DrawText(style.FontLarge, "Loading...", new Rectangle(Vector2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
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
