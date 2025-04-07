// Copyright (c) Wojciech Figat. All rights reserved.

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
        }

        /// <summary>
        /// Resets the camera to focus on a object.
        /// </summary>
        public void ResetCamera()
        {
            ViewportCamera.SetArcBallView(StaticModel.Model != null ? StaticModel.Box : AnimatedModel.Box);
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            var position = Vector3.Zero;
            var scale = Float3.One;

            // Update preview model scale to fit the preview
            var model = Asset;
            if (ScaleToFit && model && model.IsLoaded)
            {
                float targetSize = 50.0f;
                BoundingBox box = model is Model ? ((Model)model).GetBox() : ((SkinnedModel)model).GetBox();
                float maxSize = Mathf.Max(0.001f, (float)box.Size.MaxValue);
                scale = new Float3(targetSize / maxSize);
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
                ResetCamera();
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
