// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Particle Emitter asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class ParticleEmitterPreview : ParticleSystemPreview
    {
        private ParticleEmitter _emitter;
        private ParticleSystem _system;
        private float _playbackDuration = 5.0f;

        /// <summary>
        /// Gets or sets the particle emitter asset to preview.
        /// </summary>
        public ParticleEmitter Emitter
        {
            get => _emitter;
            set
            {
                if (_emitter != value)
                {
                    _emitter = value;
                    _system.Init(value, _playbackDuration);
                    PreviewActor.ResetSimulation();
                }
            }
        }

        /// <summary>
        /// Gets or sets the duration of the emitter playback (in seconds).
        /// </summary>
        public float PlaybackDuration
        {
            get => _playbackDuration;
            set
            {
                value = Mathf.Clamp(value, 0.1f, 100000000000.0f);
                if (Mathf.NearEqual(_playbackDuration, value))
                    return;

                _playbackDuration = value;
                if (_system != null)
                {
                    _system.Init(_emitter, _playbackDuration);
                    PreviewActor.ResetSimulation();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ParticleEmitterPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public ParticleEmitterPreview(bool useWidgets)
        : base(useWidgets)
        {
            _system = FlaxEngine.Content.CreateVirtualAsset<ParticleSystem>();
            System = _system;

            if (useWidgets)
            {
                var playbackDuration = ViewWidgetButtonMenu.AddButton("Duration");
                var playbackDurationValue = new FloatValueBox(_playbackDuration, 90, 2, 70.0f, 0.1f, 1000000.0f, 0.1f);
                playbackDurationValue.Parent = playbackDuration;
                playbackDurationValue.ValueChanged += () => PlaybackDuration = playbackDurationValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => playbackDurationValue.Value = PlaybackDuration;
            }
        }

        /// <inheritdoc />
        public override bool HasLoadedAssets => (_emitter == null || _emitter.IsLoaded) && base.HasLoadedAssets;

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Cleanup objects
            _emitter = null;
            Object.Destroy(ref _system);

            base.OnDestroy();
        }
    }
}
