// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
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
        private AnimatedModel _previewModel;
        private StaticModel _previewNodesActor;
        private Model _previewNodesModel;
        private int _previewNodesCounter;
        private List<Vector3> _previewNodesVB;
        private List<int> _previewNodesIB;

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
        public bool ShowNodes { get; set; } = false;

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

            _previewNodesModel = FlaxEngine.Content.CreateVirtualAsset<Model>();
            _previewNodesModel.SetupLODs(new[] { 1 });
            _previewNodesActor = new StaticModel
            {
                Model = _previewNodesModel
            };
            _previewNodesActor.SetMaterial(0, FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.WiresDebugMaterial));

            // Link actors for rendering
            Task.AddCustomActor(_previewModel);
            Task.AddCustomActor(_previewNodesActor);

            if (useWidgets)
            {
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
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Manually update animation
            if (PlayAnimation)
            {
                _previewModel.UpdateAnimation();
            }

            // Update the nodes debug (once every few frames)
            _previewNodesActor.Transform = _previewModel.Transform;
            var updateNodesCount = PlayAnimation || _previewNodesVB?.Count == 0 ? 1 : 10;
            _previewNodesActor.IsActive = ShowNodes;
            if (_previewNodesCounter++ % updateNodesCount == 0 && ShowNodes)
            {
                _previewModel.GetCurrentPose(out var pose);
                var nodes = _previewModel.SkinnedModel?.Nodes;
                if (pose == null || pose.Length == 0 || nodes == null)
                {
                    _previewNodesActor.IsActive = false;
                }
                else
                {
                    if (_previewNodesVB == null)
                        _previewNodesVB = new List<Vector3>(1024 * 2);
                    else
                        _previewNodesVB.Clear();
                    if (_previewNodesIB == null)
                        _previewNodesIB = new List<int>(1024 * 3);
                    else
                        _previewNodesIB.Clear();

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

                        // Some inlined code to improve performance
                        var box = localBox * transform;
                        //
                        var iStart = _previewNodesVB.Count;
                        box.GetCorners(_previewNodesVB);
                        //
                        _previewNodesIB.Add(iStart + 0);
                        _previewNodesIB.Add(iStart + 1);
                        _previewNodesIB.Add(iStart + 0);
                        //
                        _previewNodesIB.Add(iStart + 0);
                        _previewNodesIB.Add(iStart + 4);
                        _previewNodesIB.Add(iStart + 0);
                        //
                        _previewNodesIB.Add(iStart + 1);
                        _previewNodesIB.Add(iStart + 2);
                        _previewNodesIB.Add(iStart + 1);
                        //
                        _previewNodesIB.Add(iStart + 1);
                        _previewNodesIB.Add(iStart + 5);
                        _previewNodesIB.Add(iStart + 1);
                        //
                        _previewNodesIB.Add(iStart + 2);
                        _previewNodesIB.Add(iStart + 3);
                        _previewNodesIB.Add(iStart + 2);
                        //
                        _previewNodesIB.Add(iStart + 2);
                        _previewNodesIB.Add(iStart + 6);
                        _previewNodesIB.Add(iStart + 2);
                        //
                        _previewNodesIB.Add(iStart + 3);
                        _previewNodesIB.Add(iStart + 7);
                        _previewNodesIB.Add(iStart + 3);
                        //
                        _previewNodesIB.Add(iStart + 4);
                        _previewNodesIB.Add(iStart + 5);
                        _previewNodesIB.Add(iStart + 4);
                        //
                        _previewNodesIB.Add(iStart + 4);
                        _previewNodesIB.Add(iStart + 7);
                        _previewNodesIB.Add(iStart + 4);
                        //
                        _previewNodesIB.Add(iStart + 5);
                        _previewNodesIB.Add(iStart + 6);
                        _previewNodesIB.Add(iStart + 5);
                        //
                        _previewNodesIB.Add(iStart + 6);
                        _previewNodesIB.Add(iStart + 7);
                        _previewNodesIB.Add(iStart + 6);
                        //
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

                            var iStart = _previewNodesVB.Count;
                            _previewNodesVB.Add(parentPos);
                            _previewNodesVB.Add(bonePos);
                            _previewNodesIB.Add(iStart + 0);
                            _previewNodesIB.Add(iStart + 1);
                            _previewNodesIB.Add(iStart + 0);
                        }
                    }

                    if (_previewNodesIB.Count > 0)
                        _previewNodesModel.LODs[0].Meshes[0].UpdateMesh(_previewNodesVB, _previewNodesIB);
                    else
                        _previewNodesActor.IsActive = false;
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Ensure to cleanup created actor objects
            _previewNodesActor.Model = null;
            Object.Destroy(ref _previewModel);
            Object.Destroy(ref _previewNodesActor);
            Object.Destroy(ref _previewNodesModel);
            NodesMask = null;

            base.OnDestroy();
        }
    }
}
