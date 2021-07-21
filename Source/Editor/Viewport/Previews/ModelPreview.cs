// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Model asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class ModelPreview : AssetPreview
    {
        private ContextMenuButton _showBoundsButton;
        private StaticModel _previewModel;
        private bool _showBounds;

        /// <summary>
        /// Gets or sets the model asset to preview.
        /// </summary>
        public Model Model
        {
            get => _previewModel.Model;
            set => _previewModel.Model = value;
        }

        /// <summary>
        /// Gets the model actor used to preview selected asset.
        /// </summary>
        public StaticModel PreviewActor => _previewModel;

        /// <summary>
        /// Gets or sets a value indicating whether show animated model bounding box debug view.
        /// </summary>
        public bool ShowBounds
        {
            get => _showBounds;
            set
            {
                _showBounds = value;
                if (value)
                    ShowDebugDraw = true;
                if (_showBoundsButton != null)
                    _showBoundsButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether scale the model to the normalized bounds.
        /// </summary>
        public bool ScaleToFit { get; set; } = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ModelPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public ModelPreview(bool useWidgets)
        : base(useWidgets)
        {
            Task.Begin += OnBegin;

            // Setup preview scene
            _previewModel = new StaticModel();

            // Link actors for rendering
            Task.AddCustomActor(_previewModel);

            if (useWidgets)
            {
                // Show Bounds
                _showBoundsButton = ViewWidgetShowMenu.AddButton("Bounds", () => ShowBounds = !ShowBounds);

                // Preview LOD
                {
                    var previewLOD = ViewWidgetButtonMenu.AddButton("Preview LOD");
                    var previewLODValue = new IntValueBox(-1, 90, 2, 70.0f, -1, 10, 0.02f)
                    {
                        Parent = previewLOD
                    };
                    previewLODValue.ValueChanged += () => _previewModel.ForcedLOD = previewLODValue.Value;
                    ViewWidgetButtonMenu.VisibleChanged += control => previewLODValue.Value = _previewModel.ForcedLOD;
                }
            }
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            if (!ScaleToFit)
            {
                _previewModel.Scale = Vector3.One;
                _previewModel.Position = Vector3.Zero;
                return;
            }

            // Update preview model scale to fit the preview
            var model = Model;
            if (model && model.IsLoaded)
            {
                float targetSize = 50.0f;
                BoundingBox box = model.GetBox();
                float maxSize = Mathf.Max(0.001f, box.Size.MaxValue);
                float scale = targetSize / maxSize;
                _previewModel.Scale = new Vector3(scale);
                _previewModel.Position = box.Center * (-0.5f * scale) + new Vector3(0, -10, 0);
            }
        }

        /// <inheritdoc />
        protected override void OnDebugDraw(GPUContext context, ref RenderContext renderContext)
        {
            base.OnDebugDraw(context, ref renderContext);

            // Draw bounds
            if (_showBounds)
            {
                DebugDraw.DrawWireBox(_previewModel.Box, Color.Violet.RGBMultiplied(0.8f), 0, false);
            }
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.F:
                // Pay respect..
                ViewportCamera.SetArcBallView(_previewModel.Box);
                break;
            }
            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Ensure to cleanup created actor objects
            Object.Destroy(ref _previewModel);

            base.OnDestroy();
        }
    }
}
