// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;
using System;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Particle System asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class ParticleSystemPreview : AssetPreview
    {
        private bool _playSimulation = false;
        private ParticleEffect _previewEffect;
        private ContextMenuButton _showBoundsButton;
        private ContextMenuButton _showOriginButton;
        private ContextMenuButton _showParticleCounterButton;
        private ViewportWidgetButton _playPauseButton;
        private StaticModel _boundsModel;
        private StaticModel _originModel;
        private bool _showParticlesCounter;

        /// <summary>
        /// Gets or sets the particle system asset to preview.
        /// </summary>
        public ParticleSystem System
        {
            get => _previewEffect.ParticleSystem;
            set => _previewEffect.ParticleSystem = value;
        }

        /// <summary>
        /// Gets the particle effect actor used to preview selected asset.
        /// </summary>
        public ParticleEffect PreviewActor => _previewEffect;

        /// <summary>
        /// Gets or sets a value indicating whether to play the particles simulation in editor.
        /// </summary>
        public bool PlaySimulation
        {
            get => _playSimulation;
            set
            {
                if (_playSimulation == value)
                    return;
                _playSimulation = value;
                PlaySimulationChanged?.Invoke();

                if (_playPauseButton != null)
                    _playPauseButton.Icon = value ? Editor.Instance.Icons.Pause64 : Editor.Instance.Icons.Play64;
            }
        }

        /// <summary>
        /// Occurs when particles simulation playback state gets changed.
        /// </summary>
        public event Action PlaySimulationChanged;

        /// <summary>
        /// Gets or sets a value indicating whether to show particle effect bounding box.
        /// </summary>
        public bool ShowBounds
        {
            get => _boundsModel != null ? _boundsModel.IsActive : false;
            set
            {
                if (value == ShowBounds)
                    return;

                if (value)
                {
                    if (!_boundsModel)
                    {
                        _boundsModel = new StaticModel
                        {
                            Model = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/WireBox")
                        };
                        _boundsModel.Model.WaitForLoaded();
                        _boundsModel.SetMaterial(0, FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWireFocus"));
                        Task.AddCustomActor(_boundsModel);
                    }
                    else if (!_boundsModel.IsActive)
                    {
                        _boundsModel.IsActive = true;
                        Task.AddCustomActor(_boundsModel);
                    }

                    UpdateBoundsModel();
                }
                else
                {
                    _boundsModel.IsActive = false;
                    Task.RemoveCustomActor(_boundsModel);
                }

                if (_showBoundsButton != null)
                    _showBoundsButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to show particle effect origin point.
        /// </summary>
        public bool ShowOrigin
        {
            get => _originModel != null ? _originModel.IsActive : false;
            set
            {
                if (value == ShowOrigin)
                    return;

                if (value)
                {
                    if (!_originModel)
                    {
                        _originModel = new StaticModel
                        {
                            Model = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Sphere"),
                            Position = _previewEffect.Position,
                            Scale = new Vector3(0.1f)
                        };
                        _originModel.Model.WaitForLoaded();
                        _originModel.SetMaterial(0, FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialAxisFocus"));
                        Task.AddCustomActor(_originModel);
                    }
                    else if (!_originModel.IsActive)
                    {
                        _originModel.IsActive = true;
                        Task.AddCustomActor(_originModel);
                    }
                }
                else
                {
                    _originModel.IsActive = false;
                    Task.RemoveCustomActor(_originModel);
                }

                if (_showOriginButton != null)
                    _showOriginButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to show spawned particles counter (for CPU particles only).
        /// </summary>
        public bool ShowParticlesCounter
        {
            get => _showParticlesCounter;
            set
            {
                if (value == _showParticlesCounter)
                    return;

                _showParticlesCounter = value;

                if (_showParticleCounterButton != null)
                    _showParticleCounterButton.Checked = value;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ParticleSystemPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public ParticleSystemPreview(bool useWidgets)
        : base(useWidgets, new FPSCamera())
        {
            // Setup preview scene
            _previewEffect = new ParticleEffect
            {
                UseTimeScale = false,
                IsLooping = true,
                CustomViewRenderTask = Task
            };

            // Link actors for rendering
            Task.AddCustomActor(_previewEffect);

            if (!useWidgets)
                return;
            _showBoundsButton = ViewWidgetShowMenu.AddButton("Bounds", () => ShowBounds = !ShowBounds);
            _showBoundsButton.CloseMenuOnClick = false;
            _showOriginButton = ViewWidgetShowMenu.AddButton("Origin", () => ShowOrigin = !ShowOrigin);
            _showOriginButton.CloseMenuOnClick = false;
            _showParticleCounterButton = ViewWidgetShowMenu.AddButton("Particles Counter", () => ShowParticlesCounter = !ShowParticlesCounter);
            _showParticleCounterButton.CloseMenuOnClick = false;

            // Play/Pause widget
            {
                var playPauseWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                _playPauseButton = new ViewportWidgetButton(null, Editor.Instance.Icons.Pause64)
                {
                    TooltipText = "Simulation playback play (F5) or pause (F6)",
                    Parent = playPauseWidget,
                };
                _playPauseButton.Clicked += button => PlaySimulation = !PlaySimulation;
                playPauseWidget.Parent = this;
            }
        }

        private void UpdateBoundsModel()
        {
            var bounds = _previewEffect.Box;
            Transform t = Transform.Identity;
            t.Translation = bounds.Center;
            t.Scale = bounds.Size;
            _boundsModel.Transform = t;
        }

        /// <summary>
        /// Fits the particle system into view (scales the emitter based on the current bounds of the system).
        /// </summary>
        /// <param name="targetSize">The target size of the effect.</param>
        public void FitIntoView(float targetSize = 300.0f)
        {
            _previewEffect.Scale = Float3.One;
            float maxSize = Mathf.Max(0.001f, (float)_previewEffect.Box.Size.MaxValue);
            _previewEffect.Scale = new Float3(targetSize / maxSize);
        }

        /// <inheritdoc />
        public override bool HasLoadedAssets => _previewEffect.HasContentLoaded && base.HasLoadedAssets;

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Manually update simulation
            if (PlaySimulation)
            {
                _previewEffect.UpdateSimulation(true);
            }

            // Keep bounds matching the model
            if (_boundsModel && _boundsModel.IsActive)
            {
                UpdateBoundsModel();
            }
        }

        /// <inheritdoc />
        protected override void OnDebugDraw(GPUContext context, ref RenderContext renderContext)
        {
            base.OnDebugDraw(context, ref renderContext);

            _previewEffect.OnDebugDraw();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (_showParticlesCounter)
            {
                var count = _previewEffect.ParticlesCount;
                Render2D.DrawText(
                                  Style.Current.FontSmall,
                                  "Particles: " + count,
                                  new Rectangle(Float2.Zero, Size),
                                  Color.Wheat,
                                  TextAlignment.Near,
                                  TextAlignment.Far);
            }
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.F:
                ViewportCamera.SetArcBallView(_previewEffect.Box);
                return true;
            case KeyboardKeys.Spacebar:
                PlaySimulation = !PlaySimulation;
                return true;
            }

            var inputOptions = Editor.Instance.Options.Options.Input;
            if (inputOptions.Play.Process(this, key))
            {
                PlaySimulation = true;
                return true;
            }
            if (inputOptions.Pause.Process(this, key))
            {
                PlaySimulation = false;
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            _previewEffect.ParticleSystem = null;
            Object.Destroy(ref _previewEffect);
            Object.Destroy(ref _boundsModel);
            Object.Destroy(ref _originModel);
            _showBoundsButton = null;
            _showOriginButton = null;
            _showParticleCounterButton = null;
            _playPauseButton = null;

            base.OnDestroy();
        }
    }
}
