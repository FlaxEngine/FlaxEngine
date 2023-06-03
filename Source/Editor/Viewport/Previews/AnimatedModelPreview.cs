// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEditor.GUI.Input;
using FlaxEngine.GUI;
using FlaxEditor.Viewport.Widgets;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Animated model asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class AnimatedModelPreview : AssetPreview
    {
        private ContextMenuButton _showNodesButton, _showBoundsButton, _showFloorButton, _showNodesNamesButton;
        private bool _showNodes, _showBounds, _showFloor, _showCurrentLOD, _showNodesNames;
        private AnimatedModel _previewModel;
        private StaticModel _floorModel;
        private ContextMenuButton _showCurrentLODButton;
        private bool _playAnimation, _playAnimationOnce;
        private float _playSpeed = 1.0f;

        /// <summary>
        /// The "PreviewLODS" widget button context menu.
        /// </summary>
        private ContextMenu previewLODSWidgetButtonMenu;

        /// <summary>
        /// Snaps the preview actor to the world origin.
        /// </summary>
        protected bool _snapToOrigin = true;

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
        public bool PlayAnimation
        {
            get => _playAnimation;
            set
            {
                if (_playAnimation == value)
                    return;
                if (!value)
                    _playSpeed = _previewModel.UpdateSpeed;
                _playAnimation = value;
                _previewModel.UpdateSpeed = value ? _playSpeed : 0.0f;
                PlayAnimationChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when animation playback state gets changed.
        /// </summary>
        public event Action PlayAnimationChanged;

        /// <summary>
        /// Gets or sets the animation playback speed.
        /// </summary>
        public float PlaySpeed
        {
            get => _playAnimation ? _previewModel.UpdateSpeed : _playSpeed;
            set
            {
                if (_playAnimation)
                    _previewModel.UpdateSpeed = value;
                else
                    _playSpeed = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show animated model skeleton nodes debug view.
        /// </summary>
        public bool ShowNodes
        {
            get => _showNodes;
            set
            {
                if (_showNodes == value)
                    return;
                _showNodes = value;
                if (value)
                    ShowDebugDraw = true;
                if (_showNodesButton != null)
                    _showNodesButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show animated model skeleton nodes names debug view.
        /// </summary>
        public bool ShowNodesNames
        {
            get => _showNodesNames;
            set
            {
                if (_showNodesNames == value)
                    return;
                _showNodesNames = value;
                if (value)
                    ShowDebugDraw = true;
                if (_showNodesNamesButton != null)
                    _showNodesNamesButton.Checked = value;
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
                if (_showBounds == value)
                    return;
                _showBounds = value;
                if (value)
                    ShowDebugDraw = true;
                if (_showBoundsButton != null)
                    _showBoundsButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value that shows LOD statistics
        /// </summary>
        public bool ShowCurrentLOD
        {
            get => _showCurrentLOD;
            set
            {
                if (_showCurrentLOD == value)
                    return;
                _showCurrentLOD = value;
                if (value)
                    ShowDebugDraw = true;
                if (_showCurrentLODButton != null)
                    _showCurrentLODButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show floor model.
        /// </summary>
        public bool ShowFloor
        {
            get => _showFloor;
            set
            {
                if (_showFloor == value)
                    return;
                _showFloor = value;
                if (value && !_floorModel)
                {
                    _floorModel = new StaticModel
                    {
                        Position = new Vector3(0, -25, 0),
                        Scale = new Float3(5, 0.5f, 5),
                        Model = FlaxEngine.Content.LoadAsync<Model>(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Primitives/Cube.flax")),
                    };
                }
                if (value)
                    Task.AddCustomActor(_floorModel);
                else
                    Task.RemoveCustomActor(_floorModel);
                if (_showFloorButton != null)
                    _showFloorButton.Checked = value;
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
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;

            // Setup preview scene
            _previewModel = new AnimatedModel
            {
                UseTimeScale = false,
                UpdateWhenOffscreen = true,
                //_previewModel.BoundsScale = 1000.0f;
                UpdateMode = AnimatedModel.AnimationUpdateMode.Manual
            };
            Task.AddCustomActor(_previewModel);

            if (useWidgets)
            {
                // Show Bounds
                _showBoundsButton = ViewWidgetShowMenu.AddButton("Bounds", () => ShowBounds = !ShowBounds);

                // Show Skeleton
                _showNodesButton = ViewWidgetShowMenu.AddButton("Skeleton", () => ShowNodes = !ShowNodes);

                // Show Skeleton Names
                _showNodesNamesButton = ViewWidgetShowMenu.AddButton("Skeleton Names", () => ShowNodesNames = !ShowNodesNames);

                // Show Floor
                _showFloorButton = ViewWidgetShowMenu.AddButton("Floor", button => ShowFloor = !ShowFloor);
                _showFloorButton.IndexInParent = 1;

                // Show Current LOD
                _showCurrentLODButton = ViewWidgetShowMenu.AddButton("Current LOD", button =>
                {
                    _showCurrentLOD = !_showCurrentLOD;
                    _showCurrentLODButton.Icon = _showCurrentLOD ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                });
                _showCurrentLODButton.IndexInParent = 2;

                // PreviewLODS mode widget
                var PreviewLODSMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                previewLODSWidgetButtonMenu = new ContextMenu();
                previewLODSWidgetButtonMenu.VisibleChanged += PreviewLODSWidgetMenuOnVisibleChanged;
                var previewLODSModeButton = new ViewportWidgetButton("Preview LOD", SpriteHandle.Invalid, previewLODSWidgetButtonMenu)
                {
                    TooltipText = "Preview LOD properties",
                    Parent = PreviewLODSMode,
                };
                PreviewLODSMode.Parent = this;
            }

            // Enable shadows
            PreviewLight.ShadowsMode = ShadowsCastingMode.All;
            PreviewLight.CascadeCount = 3;
            PreviewLight.ShadowsDistance = 2000.0f;
            Task.ViewFlags |= ViewFlags.Shadows;
        }

        /// <summary>
        /// Fill out all SkinnedModel LODS
        /// </summary>
        /// <param name="control"></param>
        private void PreviewLODSWidgetMenuOnVisibleChanged(Control control)
        {
            if (!control.Visible)
                return;

            var skinned = _previewModel.SkinnedModel;
            if (skinned && !skinned.WaitForLoaded() && skinned.IsLoaded)
            {
                previewLODSWidgetButtonMenu.ItemsContainer.DisposeChildren();
                var lods = skinned.LODs.Length;
                for (int i = -1; i < lods; i++)
                {
                    var index = i;
                    var button = previewLODSWidgetButtonMenu.AddButton("LOD " + (index == -1 ? "Auto" : index));
                    button.ButtonClicked += (button) => _previewModel.ForcedLOD = index;
                    button.Checked = _previewModel.ForcedLOD == index;
                    button.Tag = index;
                    if (lods <= 1) return;
                }
            }
        }

        /// <summary>
        /// Starts the animation playback.
        /// </summary>
        public void Play()
        {
            PlayAnimation = true;
        }

        /// <summary>
        /// Pauses the animation playback.
        /// </summary>
        public void Pause()
        {
            PlayAnimation = false;
        }

        /// <summary>
        /// Stops the animation playback.
        /// </summary>
        public void Stop()
        {
            PlayAnimation = false;
            _previewModel.ResetAnimation();
            _previewModel.UpdateAnimation();
        }

        /// <summary>
        /// Sets the weight of the blend shape and updates the preview model (if not animated right now).
        /// </summary>
        /// <param name="name">The blend shape name.</param>
        /// <param name="value">The normalized weight of the blend shape (in range -1:1).</param>
        public void SetBlendShapeWeight(string name, float value)
        {
            _previewModel.SetBlendShapeWeight(name, value);
            _playAnimationOnce = true;
        }

        /// <summary>
        /// Gets the skinned model bounds. Handles skeleton-only assets.
        /// </summary>
        /// <returns>The local bounds.</returns>
        public BoundingBox GetBounds()
        {
            var box = BoundingBox.Zero;
            var skinnedModel = SkinnedModel;
            if (skinnedModel && skinnedModel.IsLoaded)
            {
                if (skinnedModel.LODs.Length != 0)
                {
                    // Use model geometry bounds
                    box = skinnedModel.GetBox();
                }
                else
                {
                    // Use skeleton bounds
                    _previewModel.GetCurrentPose(out var pose);
                    if (pose != null && pose.Length != 0)
                    {
                        var point = pose[0].TranslationVector;
                        box = new BoundingBox(point, point);
                        for (int i = 1; i < pose.Length; i++)
                        {
                            point = pose[i].TranslationVector;
                            box.Minimum = Vector3.Min(box.Minimum, point);
                            box.Maximum = Vector3.Max(box.Maximum, point);
                        }
                    }
                }
            }
            return box;
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            if (!ScaleToFit)
            {
                if (_snapToOrigin)
                {
                    _previewModel.Scale = Vector3.One;
                    _previewModel.Position = Vector3.Zero;
                }
                return;
            }

            // Update preview model scale to fit the preview
            var skinnedModel = SkinnedModel;
            if (skinnedModel && skinnedModel.IsLoaded)
            {
                float targetSize = 50.0f;
                BoundingBox box = GetBounds();
                float maxSize = Mathf.Max(0.001f, (float)box.Size.MaxValue);
                float scale = targetSize / maxSize;
                _previewModel.Scale = new Vector3(scale);
                _previewModel.Position = box.Center * (-0.5f * scale) + new Vector3(0, -10, 0);
            }
        }

        private void OnScriptsReloadBegin()
        {
            // Prevent any crashes due to dangling references to anim events
            _previewModel.ResetAnimation();
        }

        private int ComputeLODIndex(SkinnedModel model, out float screenSize)
        {
            screenSize = 1.0f;
            if (PreviewActor.ForcedLOD != -1)
                return PreviewActor.ForcedLOD;

            // Based on RenderTools::ComputeModelLOD
            CreateProjectionMatrix(out var projectionMatrix);
            float screenMultiple = 0.5f * Mathf.Max(projectionMatrix.M11, projectionMatrix.M22);
            var sphere = PreviewActor.Sphere;
            var viewOrigin = ViewPosition;
            var distSqr = Vector3.DistanceSquared(ref sphere.Center, ref viewOrigin);
            var screenRadiusSquared = Mathf.Square(screenMultiple * sphere.Radius) / Mathf.Max(1.0f, distSqr);
            screenSize = Mathf.Sqrt((float)screenRadiusSquared) * 2.0f;

            // Check if model is being culled
            if (Mathf.Square(model.MinScreenSize * 0.5f) > screenRadiusSquared)
                return -1;

            // Skip if no need to calculate LOD
            if (model.LoadedLODs == 0)
                return -1;
            var lods = model.LODs;
            if (lods.Length == 0)
                return -1;
            if (lods.Length == 1)
                return 0;

            // Iterate backwards and return the first matching LOD
            for (int lodIndex = lods.Length - 1; lodIndex >= 0; lodIndex--)
            {
                if (Mathf.Square(lods[lodIndex].ScreenSize * 0.5f) >= screenRadiusSquared)
                {
                    return lodIndex + PreviewActor.LODBias;
                }
            }

            return 0;
        }

        /// <inheritdoc />
        protected override void OnDebugDraw(GPUContext context, ref RenderContext renderContext)
        {
            base.OnDebugDraw(context, ref renderContext);

            // Draw skeleton nodes
            if (_showNodes || _showNodesNames)
            {
                _previewModel.GetCurrentPose(out var pose, true);
                var nodes = _previewModel.SkinnedModel?.Nodes;
                if (pose != null && pose.Length != 0 && nodes != null)
                {
                    var nodesMask = NodesMask != null && NodesMask.Length == nodes.Length ? NodesMask : null;
                    if (_showNodes)
                    {
                        // Draw bounding box at the node locations
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
                    if (_showNodesNames)
                    {
                        // Nodes names
                        for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                        {
                            if (nodesMask != null && !nodesMask[nodeIndex])
                                continue;
                            //var t = new Transform(pose[nodeIndex].TranslationVector, Quaternion.Identity, new Float3(0.1f));
                            DebugDraw.DrawText(nodes[nodeIndex].Name, pose[nodeIndex].TranslationVector, Color.White, 20, 0.0f, 0.1f);
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
        public override void Draw()
        {
            base.Draw();

            var skinnedModel = _previewModel.SkinnedModel;
            if (skinnedModel == null || !skinnedModel.IsLoaded)
                return;
            var lods = skinnedModel.LODs;
            if (lods.Length == 0)
            {
                // Force show skeleton for models without geometry
                ShowNodes = true;
                return;
            }
            if (_showCurrentLOD)
            {
                var lodIndex = ComputeLODIndex(skinnedModel, out var screenSize);
                var auto = _previewModel.ForcedLOD == -1;
                string text = auto ? "LOD Automatic" : "";
                text += auto ? string.Format("\nScreen Size: {0:F2}", screenSize) : "";
                text += string.Format("\nCurrent LOD: {0}", lodIndex);
                if (lodIndex != -1)
                {
                    lodIndex = Mathf.Clamp(lodIndex + PreviewActor.LODBias, 0, lods.Length - 1);
                    var lod = lods[lodIndex];
                    int triangleCount = 0, vertexCount = 0;
                    for (int meshIndex = 0; meshIndex < lod.Meshes.Length; meshIndex++)
                    {
                        var mesh = lod.Meshes[meshIndex];
                        triangleCount += mesh.TriangleCount;
                        vertexCount += mesh.VertexCount;
                    }
                    text += string.Format("\nTriangles: {0:N0}\nVertices: {1:N0}", triangleCount, vertexCount);
                }
                var font = Style.Current.FontMedium;
                var pos = new Float2(10, 50);
                Render2D.DrawText(font, text, new Rectangle(pos + Float2.One, Size), Color.Black);
                Render2D.DrawText(font, text, new Rectangle(pos, Size), Color.White);
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Manually update animation
            if (PlayAnimation || _playAnimationOnce)
            {
                _playAnimationOnce = false;
                _previewModel.UpdateAnimation();
            }
            else
            {
                // Invalidate playback timer (preserves playback state, clears ticking info in LastUpdateTime)
                _previewModel.IsActive = !_previewModel.IsActive;
                _previewModel.IsActive = !_previewModel.IsActive;
            }
        }

        /// <summary>
        /// Calls SetArcBallView from ViewportCamera
        /// </summary>
        public void CallSetArcBallView()
        {
            ViewportCamera.SetArcBallView(_previewModel.Box);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.F:
                    // Pay respect..
                    CallSetArcBallView();
                return true;
            case KeyboardKeys.Spacebar:
                PlayAnimation = !PlayAnimation;
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            Object.Destroy(ref _floorModel);
            Object.Destroy(ref _previewModel);
            NodesMask = null;
            _showNodesButton = null;
            _showBoundsButton = null;
            _showFloorButton = null;
            _showCurrentLODButton = null;
            _showNodesNamesButton = null;

            base.OnDestroy();
        }
    }
}
