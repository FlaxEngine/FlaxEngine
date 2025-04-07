// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using FlaxEditor.Viewport.Widgets;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Model asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class ModelPreview : AssetPreview
    {
        private ContextMenuButton _showBoundsButton, _showCurrentLODButton, _showNormalsButton, _showTangentsButton, _showBitangentsButton, _showFloorButton;
        private ContextMenu _previewLODsWidgetButtonMenu;
        private StaticModel _previewModel, _floorModel;
        private bool _showBounds, _showCurrentLOD, _showNormals, _showTangents, _showBitangents, _showFloor;
        private MeshDataCache _meshDatas;

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
                if (_showCurrentLODButton != null)
                    _showCurrentLODButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets the model asset to preview.
        /// </summary>
        public Model Model
        {
            get => _previewModel.Model;
            set
            {
                if (_previewModel.Model == value)
                    return;
                _previewModel.Model = value;
                _meshDatas?.Dispose();
                if (_meshDatas != null)
                {
                    _meshDatas.Dispose();
                    ShowNormals = false;
                    ShowTangents = false;
                }
            }
        }

        /// <summary>
        /// Gets the model actor used to preview selected asset.
        /// </summary>
        public StaticModel PreviewActor => _previewModel;

        /// <summary>
        /// Gets or sets a value indicating whether show model bounding box debug view.
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
        /// Gets or sets a value indicating whether show model geometry normal vectors debug view.
        /// </summary>
        public bool ShowNormals
        {
            get => _showNormals;
            set
            {
                if (_showNormals == value)
                    return;
                _showNormals = value;
                if (value)
                {
                    ShowDebugDraw = true;
                    if (_meshDatas == null)
                        _meshDatas = new MeshDataCache();
                    _meshDatas.RequestMeshData(_previewModel.Model);
                }
                if (_showNormalsButton != null)
                    _showNormalsButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show model geometry tangent vectors debug view.
        /// </summary>
        public bool ShowTangents
        {
            get => _showTangents;
            set
            {
                if (_showTangents == value)
                    return;
                _showTangents = value;
                if (value)
                {
                    ShowDebugDraw = true;
                    if (_meshDatas == null)
                        _meshDatas = new MeshDataCache();
                    _meshDatas.RequestMeshData(_previewModel.Model);
                }
                if (_showTangentsButton != null)
                    _showTangentsButton.Checked = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show model geometry bitangent vectors (aka binormals) debug view.
        /// </summary>
        public bool ShowBitangents
        {
            get => _showBitangents;
            set
            {
                if (_showBitangents == value)
                    return;
                _showBitangents = value;
                if (value)
                {
                    ShowDebugDraw = true;
                    if (_meshDatas == null)
                        _meshDatas = new MeshDataCache();
                    _meshDatas.RequestMeshData(_previewModel.Model);
                }
                if (_showBitangentsButton != null)
                    _showBitangentsButton.Checked = value;
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
        /// Initializes a new instance of the <see cref="ModelPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public ModelPreview(bool useWidgets)
        : base(useWidgets)
        {
            Task.Begin += OnBegin;

            _previewModel = new StaticModel();
            Task.AddCustomActor(_previewModel);

            if (useWidgets)
            {
                _showBoundsButton = ViewWidgetShowMenu.AddButton("Bounds", () => ShowBounds = !ShowBounds);
                _showBoundsButton.CloseMenuOnClick = false;
                _showNormalsButton = ViewWidgetShowMenu.AddButton("Normals", () => ShowNormals = !ShowNormals);
                _showNormalsButton.CloseMenuOnClick = false;
                _showTangentsButton = ViewWidgetShowMenu.AddButton("Tangents", () => ShowTangents = !ShowTangents);
                _showTangentsButton.CloseMenuOnClick = false;
                _showBitangentsButton = ViewWidgetShowMenu.AddButton("Bitangents", () => ShowBitangents = !ShowBitangents);
                _showBitangentsButton.CloseMenuOnClick = false;

                // Show Floor
                _showFloorButton = ViewWidgetShowMenu.AddButton("Floor", button => ShowFloor = !ShowFloor);
                _showFloorButton.IndexInParent = 1;
                _showFloorButton.CloseMenuOnClick = false;

                // Show current LOD widget
                _showCurrentLODButton = ViewWidgetShowMenu.AddButton("Current LOD", button =>
                {
                    _showCurrentLOD = !_showCurrentLOD;
                    _showCurrentLODButton.Icon = _showCurrentLOD ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                });
                _showCurrentLODButton.IndexInParent = 2;
                _showCurrentLODButton.CloseMenuOnClick = false;

                // Preview LODs mode widget
                var PreviewLODsMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                _previewLODsWidgetButtonMenu = new ContextMenu();
                _previewLODsWidgetButtonMenu.VisibleChanged += control =>
                {
                    if (!control.Visible)
                        return;
                    var model = _previewModel.Model;
                    if (model && !model.WaitForLoaded())
                    {
                        _previewLODsWidgetButtonMenu.ItemsContainer.DisposeChildren();
                        var lods = model.LODs.Length;
                        for (int i = -1; i < lods; i++)
                        {
                            var index = i;
                            var button = _previewLODsWidgetButtonMenu.AddButton("LOD " + (index == -1 ? "Auto" : index));
                            button.ButtonClicked += _ => _previewModel.ForcedLOD = index;
                            button.Checked = _previewModel.ForcedLOD == index;
                            button.Tag = index;
                            if (lods <= 1)
                                break;
                        }
                    }
                };
                new ViewportWidgetButton("Preview LOD", SpriteHandle.Invalid, _previewLODsWidgetButtonMenu)
                {
                    TooltipText = "Preview LOD properties",
                    Parent = PreviewLODsMode,
                };
                PreviewLODsMode.Parent = this;
            }
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            if (!ScaleToFit)
            {
                _previewModel.Scale = Float3.One;
                _previewModel.Position = Vector3.Zero;
                return;
            }

            // Update preview model scale to fit the preview
            var model = Model;
            if (model && model.IsLoaded)
            {
                float targetSize = 50.0f;
                BoundingBox box = model.GetBox();
                float maxSize = Mathf.Max(0.001f, (float)box.Size.MaxValue);
                float scale = targetSize / maxSize;
                _previewModel.Scale = new Float3(scale);
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

            // Draw normals
            if (_showNormals && _meshDatas.RequestMeshData(Model))
            {
                var meshDatas = _meshDatas.MeshDatas;
                var lodIndex = ComputeLODIndex(Model, out _);
                var lod = meshDatas[lodIndex];
                for (int meshIndex = 0; meshIndex < lod.Length; meshIndex++)
                {
                    var meshData = lod[meshIndex];
                    var positionStream = meshData.VertexAccessor.Position();
                    var normalStream = meshData.VertexAccessor.Normal();
                    if (positionStream.IsValid && normalStream.IsValid)
                    {
                        var count = positionStream.Count;
                        for (int i = 0; i < count; i++)
                        {
                            var position = positionStream.GetFloat3(i);
                            var normal = normalStream.GetFloat3(i);
                            MeshAccessor.UnpackNormal(ref normal);
                            DebugDraw.DrawLine(position, position + normal * 4.0f, Color.Blue);
                        }
                    }
                }
            }

            // Draw tangents
            if (_showTangents && _meshDatas.RequestMeshData(Model))
            {
                var meshDatas = _meshDatas.MeshDatas;
                var lodIndex = ComputeLODIndex(Model, out _);
                var lod = meshDatas[lodIndex];
                for (int meshIndex = 0; meshIndex < lod.Length; meshIndex++)
                {
                    var meshData = lod[meshIndex];
                    var positionStream = meshData.VertexAccessor.Position();
                    var tangentStream = meshData.VertexAccessor.Tangent();
                    if (positionStream.IsValid && tangentStream.IsValid)
                    {
                        var count = positionStream.Count;
                        for (int i = 0; i < count; i++)
                        {
                            var position = positionStream.GetFloat3(i);
                            var tangent = tangentStream.GetFloat3(i);
                            MeshAccessor.UnpackNormal(ref tangent);
                            DebugDraw.DrawLine(position, position + tangent * 4.0f, Color.Red);
                        }
                    }
                }
            }

            // Draw bitangents
            if (_showBitangents && _meshDatas.RequestMeshData(Model))
            {
                var meshDatas = _meshDatas.MeshDatas;
                var lodIndex = ComputeLODIndex(Model, out _);
                var lod = meshDatas[lodIndex];
                for (int meshIndex = 0; meshIndex < lod.Length; meshIndex++)
                {
                    var meshData = lod[meshIndex];
                    var positionStream = meshData.VertexAccessor.Position();
                    var normalStream = meshData.VertexAccessor.Normal();
                    var tangentStream = meshData.VertexAccessor.Tangent();
                    if (positionStream.IsValid && normalStream.IsValid && tangentStream.IsValid)
                    {
                        var count = positionStream.Count;
                        for (int i = 0; i < count; i++)
                        {
                            var position = positionStream.GetFloat3(i);
                            var normal = normalStream.GetFloat3(i);
                            var tangent = tangentStream.GetFloat4(i);
                            MeshAccessor.UnpackNormal(ref normal);
                            MeshAccessor.UnpackNormal(ref tangent, out var bitangentSign);
                            var bitangent = Float3.Cross(normal, new Float3(tangent)) * bitangentSign;
                            DebugDraw.DrawLine(position, position + bitangent * 4.0f, Color.Green);
                        }
                    }
                }
            }
        }


        private int ComputeLODIndex(Model model, out float screenSize)
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
        public override void Draw()
        {
            base.Draw();

            if (_showCurrentLOD)
            {
                var asset = Model;
                var lodIndex = ComputeLODIndex(asset, out var screenSize);
                var auto = _previewModel.ForcedLOD == -1;
                string text = auto ? "LOD Automatic" : "";
                text += auto ? string.Format("\nScreen Size: {0:F2}", screenSize) : "";
                text += string.Format("\nCurrent LOD: {0}", lodIndex);
                if (lodIndex != -1)
                {
                    var lods = asset.LODs;
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

        /// <summary>
        /// Resets the camera to focus on a object.
        /// </summary>
        public void ResetCamera()
        {
            ViewportCamera.SetArcBallView(_previewModel.Box);
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
            Object.Destroy(ref _floorModel);
            Object.Destroy(ref _previewModel);
            _showBoundsButton = null;
            _showCurrentLODButton = null;
            _previewLODsWidgetButtonMenu = null;
            _showNormalsButton = null;
            _showTangentsButton = null;
            _showBitangentsButton = null;
            _showFloorButton = null;
            _meshDatas?.Dispose();
            _meshDatas = null;

            base.OnDestroy();
        }
    }
}
