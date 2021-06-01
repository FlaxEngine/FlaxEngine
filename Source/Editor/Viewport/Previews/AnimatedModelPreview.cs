// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEditor.GUI.Input;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Animated model asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class AnimatedModelPreview : AssetPreview
    {
        private ContextMenuButton _showBoundsButton;
        private AnimatedModel _previewModel;
        private bool _showNodes, _showBounds;

        /// <summary>
        /// Gets or sets the skinned model asset to preview.
        /// </summary>
        public SkinnedModel SkinnedModel
        {
            get => _previewModel.SkinnedModel;
            set => _previewModel.SkinnedModel = value;
        }

        /// <summary>
        /// Gets the skinned model actor used to preview selected asset.
        /// </summary>
        public AnimatedModel PreviewActor => _previewModel;

        /// <summary>
        /// Gets or sets a value indicating whether play the animation in editor.
        /// </summary>
        public bool PlayAnimation { get; set; } = false;

        /// <summary>
        /// Gets or sets a value indicating whether show animated model skeleton nodes debug view.
        /// </summary>
        public bool ShowNodes
        {
            get => _showNodes;
            set
            {
                _showNodes = value;
                if (value)
                    ShowDebugDraw = true;
            }
        }

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
        /// Gets or sets the custom mask for the skeleton nodes. Nodes missing from this list will be skipped during rendering. Works only if <see cref="ShowNodes"/> is set to true and the array matches the attached <see cref="SkinnedModel"/> nodes hierarchy.
        /// </summary>
        public bool[] NodesMask { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="AnimatedModelPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public AnimatedModelPreview(bool useWidgets)
        : base(useWidgets)
        {
            Task.Begin += OnBegin;

            // Setup preview scene
            _previewModel = new AnimatedModel
            {
                UseTimeScale = false,
                UpdateWhenOffscreen = true,
                //_previewModel.BoundsScale = 1000.0f;
                UpdateMode = AnimatedModel.AnimationUpdateMode.Manual
            };

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
            var skinnedModel = SkinnedModel;
            if (skinnedModel && skinnedModel.IsLoaded)
            {
                float targetSize = 50.0f;
                BoundingBox box = skinnedModel.GetBox();
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

            // Draw skeleton nodes
            if (_showNodes)
            {
                _previewModel.GetCurrentPose(out var pose);
                var nodes = _previewModel.SkinnedModel?.Nodes;
                if (pose != null && pose.Length != 0 && nodes != null)
                {
                    // Draw bounding box at the node locations
                    var nodesMask = NodesMask != null && NodesMask.Length == nodes.Length ? NodesMask : null;
                    var localBox = new OrientedBoundingBox(new Vector3(-1.0f), new Vector3(1.0f));
                    for (int nodeIndex = 0; nodeIndex < pose.Length; nodeIndex++)
                    {
                        if (nodesMask != null && !nodesMask[nodeIndex])
                            continue;
                        var transform = pose[nodeIndex];
                        transform.Decompose(out var scale, out Matrix _, out _);
                        transform = Matrix.Invert(Matrix.Scaling(scale)) * transform;
                        var box = localBox * transform;
                        DebugDraw.DrawWireBox(box, Color.Green, 0, false);
                    }

                    // Nodes connections
                    for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                    {
                        int parentIndex = nodes[nodeIndex].ParentIndex;
                        if (parentIndex != -1)
                        {
                            if (nodesMask != null && (!nodesMask[nodeIndex] || !nodesMask[parentIndex]))
                                continue;
                            var parentPos = pose[parentIndex].TranslationVector;
                            var bonePos = pose[nodeIndex].TranslationVector;
                            DebugDraw.DrawLine(parentPos, bonePos, Color.Green, 0, false);
                        }
                    }
                }
            }

            // Draw bounds
            if (_showBounds)
            {
                DebugDraw.DrawWireBox(_previewModel.Box, Color.Violet.RGBMultiplied(0.8f), 0, false);
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Manually update animation
            if (PlayAnimation)
            {
                _previewModel.UpdateAnimation();
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
            Object.Destroy(ref _previewModel);
            NodesMask = null;

            base.OnDestroy();
        }
    }
}
