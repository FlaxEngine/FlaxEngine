// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Model Base asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class ModelBasePreview : AssetPreview
    {
        /// <summary>
        /// Gets or sets the model asset to preview.
        /// </summary>
        public ModelBase Asset
        {
            get => (ModelBase)StaticModel.Model ?? AnimatedModel.SkinnedModel;
            set
            {
                StaticModel.Model = value as Model;
                AnimatedModel.SkinnedModel = value as SkinnedModel;
            }
        }

        /// <summary>
        /// The static model for display.
        /// </summary>
        public StaticModel StaticModel;

        /// <summary>
        /// The animated model for display.
        /// </summary>
        public AnimatedModel AnimatedModel;

        /// <summary>
        /// Gets or sets a value indicating whether scale the model to the normalized bounds.
        /// </summary>
        public bool ScaleToFit { get; set; } = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ModelBasePreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public ModelBasePreview(bool useWidgets)
        : base(useWidgets)
        {
            Task.Begin += OnBegin;

            // Setup preview scene
            StaticModel = new StaticModel();
            AnimatedModel = new AnimatedModel();

            // Link actors for rendering
            Task.AddCustomActor(StaticModel);
            Task.AddCustomActor(AnimatedModel);

            if (useWidgets)
            {
                // Preview LOD
                {
                    var previewLOD = ViewWidgetButtonMenu.AddButton("Preview LOD");
                    var previewLODValue = new IntValueBox(-1, 90, 2, 70.0f, -1, 10, 0.02f)
                    {
                        Parent = previewLOD
                    };
                    previewLODValue.ValueChanged += () =>
                    {
                        StaticModel.ForcedLOD = previewLODValue.Value;
                        AnimatedModel.ForcedLOD = previewLODValue.Value;
                    };
                    ViewWidgetButtonMenu.VisibleChanged += control => previewLODValue.Value = StaticModel.ForcedLOD;
                }
            }
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            var position = Vector3.Zero;
            var scale = Vector3.One;

            // Update preview model scale to fit the preview
            var model = Asset;
            if (ScaleToFit && model && model.IsLoaded)
            {
                float targetSize = 50.0f;
                BoundingBox box = model is Model ? ((Model)model).GetBox() : ((SkinnedModel)model).GetBox();
                float maxSize = Mathf.Max(0.001f, box.Size.MaxValue);
                scale = new Vector3(targetSize / maxSize);
                position = box.Center * (-0.5f * scale.X) + new Vector3(0, -10, 0);
            }

            StaticModel.Transform = new Transform(position, StaticModel.Orientation, scale);
            AnimatedModel.Transform = new Transform(position, AnimatedModel.Orientation, scale);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.F:
                // Pay respect..
                ViewportCamera.SetArcBallView(StaticModel.Model != null ? StaticModel.Box : AnimatedModel.Box);
                break;
            }
            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Ensure to cleanup created actor objects
            Object.Destroy(ref StaticModel);
            Object.Destroy(ref AnimatedModel);

            base.OnDestroy();
        }
    }
}
