// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Modules;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Tools
{
    /// <summary>
    /// Vertex painting tab. Allows to modify paint the models vertex colors.
    /// </summary>
    /// <seealso cref="Tab" />
    [HideInEditor]
    public class VertexPaintingTab : Tab
    {
        [CustomEditor(typeof(ProxyEditor))]
        sealed class ProxyObject
        {
            [HideInEditor, NoSerialize]
            public readonly VertexPaintingTab Tab;

            public ProxyObject(VertexPaintingTab tab)
            {
                Tab = tab;
            }

            [EditorOrder(0), EditorDisplay("Brush"), Limit(0.0001f, 100000.0f, 0.1f), Tooltip("The size of the paint brush (sphere radius in world-space).")]
            public float BrushSize
            {
                get => Tab._gizmoMode.BrushSize;
                set => Tab._gizmoMode.BrushSize = value;
            }

            [EditorOrder(10), EditorDisplay("Brush"), Limit(0.0f, 1.0f, 0.01f), Tooltip("The intensity of the brush when painting. Use lower values to make painting smoother and softer.")]
            public float BrushStrength
            {
                get => Tab._gizmoMode.BrushStrength;
                set => Tab._gizmoMode.BrushStrength = value;
            }

            [EditorOrder(20), EditorDisplay("Brush"), Limit(0.0f, 1.0f, 0.01f), Tooltip("The falloff parameter for the brush. Adjusts the paint strength for the vertices that are far from the brush center. Use lower values to make painting smoother and softer.")]
            public float BrushFalloff
            {
                get => Tab._gizmoMode.BrushFalloff;
                set => Tab._gizmoMode.BrushFalloff = value;
            }

            [EditorOrder(30), EditorDisplay("Brush", "Model LOD"), Limit(-1, Model.MaxLODs, 0.001f), Tooltip("The index of the model LOD to paint over it. To paint over all LODs use value -1.")]
            public int ModelLOD
            {
                get => Tab._gizmoMode.ModelLOD;
                set => Tab._gizmoMode.ModelLOD = value;
            }

            [EditorOrder(100), EditorDisplay("Paint"), Tooltip("The paint color (32-bit RGBA).")]
            public Color PaintColor
            {
                get => Tab._gizmoMode.PaintColor;
                set => Tab._gizmoMode.PaintColor = value;
            }

            [EditorOrder(110), EditorDisplay("Paint"), Tooltip("The paint color mask. Can be used to not exclude certain color channels from painting and preserve their contents.")]
            public VertexPaintingGizmoMode.VertexColorsMask PaintMask
            {
                get => Tab._gizmoMode.PaintMask;
                set => Tab._gizmoMode.PaintMask = value;
            }

            [EditorOrder(120), EditorDisplay("Paint"), Tooltip("If checked, the painting will be continuous action while you move the brush over the model. Otherwise it will use single-click paint action form ore precise painting.")]
            public bool ContinuousPaint
            {
                get => Tab._gizmoMode.ContinuousPaint;
                set => Tab._gizmoMode.ContinuousPaint = value;
            }

            [EditorOrder(200), EditorDisplay("Preview"), Tooltip("The preview mode to use for the selected model visualization.")]
            public VertexPaintingGizmoMode.VertexColorsPreviewMode PreviewMode
            {
                get => Tab._gizmoMode.PreviewMode;
                set => Tab._gizmoMode.PreviewMode = value;
            }

            [EditorOrder(210), EditorDisplay("Preview"), Limit(0, 100.0f, 0.1f), Tooltip("The size of the vertices for the painted vertices visualization.")]
            public float PreviewVertexSize
            {
                get => Tab._gizmoMode.PreviewVertexSize;
                set => Tab._gizmoMode.PreviewVertexSize = value;
            }
        }

        sealed class ProxyEditor : GenericEditor
        {
            private Button _removeVertexColorsButton;
            private Button _copyVertexColorsButton;
            private Button _pasteVertexColorsButton;

            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                layout.Space(10.0f);

                _removeVertexColorsButton = layout.Button("Remove vertex colors").Button;
                _removeVertexColorsButton.TooltipText = "Removes the painted vertex colors data from the model instance.";
                _removeVertexColorsButton.Clicked += OnRemoveVertexColorsButtonClicked;

                _copyVertexColorsButton = layout.Button("Copy vertex colors").Button;
                _copyVertexColorsButton.TooltipText = "Copies the painted vertex colors data from the model instance to the system clipboard.";
                _copyVertexColorsButton.Clicked += OnCopyVertexColorsButtonClicked;

                _pasteVertexColorsButton = layout.Button("Paste vertex colors").Button;
                _pasteVertexColorsButton.TooltipText = "Pastes the copied vertex colors data from the system clipboard to the model instance.";
                _pasteVertexColorsButton.Clicked += OnPasteVertexColorsButtonClicked;
            }

            private void OnRemoveVertexColorsButtonClicked()
            {
                var proxy = (ProxyObject)Values[0];
                var undoAction = new EditModelVertexColorsAction(proxy.Tab.SelectedModel);
                proxy.Tab.SelectedModel.RemoveVertexColors();
                undoAction.RecordEnd();
                Editor.Instance.Undo.AddAction(undoAction);
            }

            private void OnCopyVertexColorsButtonClicked()
            {
                var proxy = (ProxyObject)Values[0];
                Clipboard.Text = EditModelVertexColorsAction.GetState(proxy.Tab.SelectedModel);
            }

            private void OnPasteVertexColorsButtonClicked()
            {
                var proxy = (ProxyObject)Values[0];
                var undoAction = new EditModelVertexColorsAction(proxy.Tab.SelectedModel);
                EditModelVertexColorsAction.SetState(proxy.Tab.SelectedModel, Clipboard.Text);
                undoAction.RecordEnd();
                Editor.Instance.Undo.AddAction(undoAction);
            }

            public override void Refresh()
            {
                var proxy = (ProxyObject)Values[0];
                _removeVertexColorsButton.Enabled = _copyVertexColorsButton.Enabled = proxy.Tab.SelectedModel?.HasVertexColors ?? false;
                _pasteVertexColorsButton.Enabled = EditModelVertexColorsAction.IsValidState(Clipboard.Text);

                base.Refresh();
            }
        }

        private readonly ProxyObject _proxy;
        private readonly CustomEditorPresenter _presenter;
        private VertexPaintingGizmoMode _gizmoMode;
        internal MaterialBase[] _cachedMaterials;
        private readonly Editor _editor;

        /// <summary>
        /// The cached selected model. It's synchronized with <see cref="SceneEditingModule.Selection"/>.
        /// </summary>
        public StaticModel SelectedModel;

        /// <summary>
        /// Occurs when selected model gets changed (to a different value).
        /// </summary>
        public event Action SelectedModelChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="VertexPaintingTab"/> class.
        /// </summary>
        /// <param name="icon">The icon.</param>
        /// <param name="editor">The editor instance.</param>
        public VertexPaintingTab(SpriteHandle icon, Editor editor)
        : base(string.Empty, icon)
        {
            _editor = editor;

            _proxy = new ProxyObject(this);
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };
            var presenter = new CustomEditorPresenter(null, "No model selected");
            presenter.Panel.Parent = panel;
            _presenter = presenter;
        }

        private void OnSelectionChanged()
        {
            var node = _editor.SceneEditing.SelectionCount > 0 ? _editor.SceneEditing.Selection[0] as ActorNode : null;
            var model = node?.Actor as StaticModel;
            if (model != SelectedModel)
            {
                if (SelectedModel != null)
                {
                    Level.SceneSaving -= OnSceneSaving;
                    if (_cachedMaterials != null)
                    {
                        for (int i = 0; i < _cachedMaterials.Length; i++)
                            SelectedModel.SetMaterial(i, _cachedMaterials[i]);
                        _cachedMaterials = null;
                    }
                }
                _presenter.Select(model ? _proxy : null);
                SelectedModel = model;
                if (SelectedModel != null)
                {
                    var entries = SelectedModel.Entries;
                    _cachedMaterials = new MaterialBase[entries.Length];
                    for (int i = 0; i < _cachedMaterials.Length; i++)
                        _cachedMaterials[i] = entries[i].Material;
                    Level.SceneSaving += OnSceneSaving;
                }
                SelectedModelChanged?.Invoke();
            }
        }

        private void OnSceneSaving(Scene scene, Guid id)
        {
            // Ensure that selected model has own materials during saving
            if (_cachedMaterials != null && SelectedModel != null)
            {
                for (int i = 0; i < _cachedMaterials.Length; i++)
                    SelectedModel.SetMaterial(i, _cachedMaterials[i]);
            }
        }

        private void UpdateGizmoMode()
        {
            if (_gizmoMode == null)
            {
                _gizmoMode = new VertexPaintingGizmoMode
                {
                    Tab = this,
                };
                _editor.Windows.EditWin.Viewport.Gizmos.AddMode(_gizmoMode);
            }
            _editor.Windows.EditWin.Viewport.Gizmos.ActiveMode = _gizmoMode;
        }

        /// <inheritdoc />
        public override void OnSelected()
        {
            base.OnSelected();

            UpdateGizmoMode();
            OnSelectionChanged();
            _editor.SceneEditing.SelectionChanged += OnSelectionChanged;
        }

        /// <inheritdoc />
        public override void OnDeselected()
        {
            if (SelectedModel)
            {
                Level.SceneSaving -= OnSceneSaving;
                if (_cachedMaterials != null)
                {
                    for (int i = 0; i < _cachedMaterials.Length; i++)
                        SelectedModel.SetMaterial(i, _cachedMaterials[i]);
                    _cachedMaterials = null;
                }
                _presenter.Deselect();
                SelectedModel = null;
                SelectedModelChanged?.Invoke();
            }
            _editor.SceneEditing.SelectionChanged -= OnSelectionChanged;

            base.OnDeselected();
        }
    }

    class VertexPaintingGizmoMode : EditorGizmoMode
    {
        public enum VertexColorsPreviewMode
        {
            None,
            RGB,
            Red,
            Green,
            Blue,
            Alpha,
        }

        [Flags]
        public enum VertexColorsMask
        {
            Red = 1,
            Green = 2,
            Blue = 4,
            Alpha = 8,
            RGB = Red | Green | Blue,
            RGBA = Red | Green | Blue | Alpha,
        }

        public VertexPaintingTab Tab;
        public VertexPaintingGizmo Gizmo;

        public VertexColorsPreviewMode PreviewMode = VertexColorsPreviewMode.RGB;
        public float PreviewVertexSize = 6.0f;
        public float BrushSize = 100.0f;
        public float BrushStrength = 1.0f;
        public float BrushFalloff = 1.0f;
        public bool ContinuousPaint;
        public int ModelLOD = -1;
        public Color PaintColor = Color.White;
        public VertexColorsMask PaintMask = VertexColorsMask.RGB;

        public override void Init(IGizmoOwner viewport)
        {
            base.Init(viewport);

            Gizmo = new VertexPaintingGizmo(viewport, this);
        }

        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = Gizmo;
        }
    }

    sealed class VertexPaintingGizmo : GizmoBase
    {
        private MaterialInstance _vertexColorsPreviewMaterial;
        private Model _brushModel;
        private MaterialInstance _brushMaterial;
        private MaterialBase _verticesPreviewMaterial;
        private VertexPaintingGizmoMode _gizmoMode;
        private bool _isPainting;
        private int _paintUpdateCount;
        private bool _hasHit;
        private Vector3 _hitLocation;
        private Vector3 _hitNormal;
        private StaticModel _selectedModel;
        private MeshDataCache _meshDatas;
        private EditModelVertexColorsAction _undoAction;

        public bool IsPainting => _isPainting;

        public VertexPaintingGizmo(IGizmoOwner owner, VertexPaintingGizmoMode mode)
        : base(owner)
        {
            _gizmoMode = mode;
        }

        private void SetPaintModel(StaticModel model)
        {
            if (_selectedModel == model)
                return;
            PaintEnd();
            _selectedModel = model;
            if (model && model.Model)
            {
                if (_meshDatas == null)
                    _meshDatas = new MeshDataCache();
                _meshDatas.RequestMeshData(_selectedModel.Model);
            }
            else
            {
                _meshDatas?.Dispose();
            }
            _hasHit = false;
        }

        private void PaintStart()
        {
            if (IsPainting)
                return;

            if (Editor.Instance.Undo.Enabled)
                _undoAction = new EditModelVertexColorsAction(_selectedModel);
            _isPainting = true;
            _paintUpdateCount = 0;
        }

        private void PaintUpdate()
        {
            if (!IsPainting || _gizmoMode.PaintMask == 0)
                return;
            if (!_gizmoMode.ContinuousPaint && _paintUpdateCount > 0)
                return;

            Profiler.BeginEvent("Vertex Paint");

            // Ensure to have vertex data ready
            _meshDatas.WaitForMeshDataRequestEnd();

            // Edit the model vertex colors
            var meshDatas = _meshDatas.MeshDatas;
            if (meshDatas == null)
                throw new Exception("Missing mesh data of the model to paint.");
            var instanceTransform = _selectedModel.Transform;
            var brushSphere = new BoundingSphere(_hitLocation, _gizmoMode.BrushSize);
            if (_paintUpdateCount == 0 && !_selectedModel.HasVertexColors)
            {
                // Initialize the instance vertex colors with originals from the asset
                for (int lodIndex = 0; lodIndex < meshDatas.Length; lodIndex++)
                {
                    if (_gizmoMode.ModelLOD != -1 && _gizmoMode.ModelLOD != lodIndex)
                        continue;
                    var lodData = meshDatas[lodIndex];
                    for (int meshIndex = 0; meshIndex < lodData.Length; meshIndex++)
                    {
                        var meshData = lodData[meshIndex];
                        var colors = meshData.VertexAccessor.Colors;
                        if (colors == null)
                            continue;
                        var vertexCount = colors.Length;
                        for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
                        {
                            _selectedModel.SetVertexColor(lodIndex, meshIndex, vertexIndex, colors[vertexIndex]);
                        }
                    }
                }
            }
            for (int lodIndex = 0; lodIndex < meshDatas.Length; lodIndex++)
            {
                if (_gizmoMode.ModelLOD != -1 && _gizmoMode.ModelLOD != lodIndex)
                    continue;
                var lodData = meshDatas[lodIndex];
                for (int meshIndex = 0; meshIndex < lodData.Length; meshIndex++)
                {
                    var meshData = lodData[meshIndex];
                    var positionStream = meshData.VertexAccessor.Position();
                    if (!positionStream.IsValid)
                        continue;
                    var vertexCount = positionStream.Count;
                    for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
                    {
                        var pos = instanceTransform.LocalToWorld(positionStream.GetFloat3(vertexIndex));
                        var dst = Vector3.Distance(ref pos, ref brushSphere.Center);
                        if (dst > brushSphere.Radius)
                            continue;
                        float strength = _gizmoMode.BrushStrength * Mathf.Lerp(1.0f, 1.0f - (float)dst / (float)brushSphere.Radius, _gizmoMode.BrushFalloff);
                        if (strength > Mathf.Epsilon)
                        {
                            // Paint the vertex
                            var color = (Color)_selectedModel.GetVertexColor(lodIndex, meshIndex, vertexIndex);
                            var paintColor = _gizmoMode.PaintColor;
                            var paintMask = _gizmoMode.PaintMask;
                            color = new Color
                            (
                             Mathf.Lerp(color.R, paintColor.R, ((paintMask & VertexPaintingGizmoMode.VertexColorsMask.Red) == VertexPaintingGizmoMode.VertexColorsMask.Red ? strength : 0.0f)),
                             Mathf.Lerp(color.G, paintColor.G, ((paintMask & VertexPaintingGizmoMode.VertexColorsMask.Green) == VertexPaintingGizmoMode.VertexColorsMask.Green ? strength : 0.0f)),
                             Mathf.Lerp(color.B, paintColor.B, ((paintMask & VertexPaintingGizmoMode.VertexColorsMask.Blue) == VertexPaintingGizmoMode.VertexColorsMask.Blue ? strength : 0.0f)),
                             Mathf.Lerp(color.A, paintColor.A, ((paintMask & VertexPaintingGizmoMode.VertexColorsMask.Alpha) == VertexPaintingGizmoMode.VertexColorsMask.Alpha ? strength : 0.0f))
                            );
                            _selectedModel.SetVertexColor(lodIndex, meshIndex, vertexIndex, color);
                        }
                    }
                }
            }
            _paintUpdateCount++;

            Profiler.EndEvent();
        }

        private void PaintEnd()
        {
            if (!IsPainting)
                return;

            if (_undoAction != null)
            {
                _undoAction.RecordEnd();
                Editor.Instance.Undo.AddAction(_undoAction);
                _undoAction = null;
            }
            _isPainting = false;
            _paintUpdateCount = 0;
        }

        /// <inheritdoc />
        public override bool IsControllingMouse => IsPainting;

        /// <inheritdoc />
        public override BoundingSphere FocusBounds => _selectedModel != null ? _selectedModel.Sphere : base.FocusBounds;

        /// <inheritdoc />
        public override void Update(float dt)
        {
            _hasHit = false;

            base.Update(dt);

            if (!IsActive)
            {
                SetPaintModel(null);
                return;
            }

            // Select model
            var model = _gizmoMode.Tab.SelectedModel;
            SetPaintModel(model);
            if (!model)
            {
                return;
            }

            // Perform detailed tracing to find cursor location for the brush
            var ray = Owner.MouseRay;
            var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
            var rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.SkipColliders | SceneGraphNode.RayCastData.FlagTypes.SkipEditorPrimitives;
            var hit = Editor.Instance.Scene.Root.RayCast(ref ray, ref view, out var closest, out var hitNormal, rayCastFlags);
            if (hit != null && hit is ActorNode actorNode && actorNode.Actor as StaticModel != model)
            {
                // Cursor hit other model
                PaintEnd();
                return;
            }
            if (hit != null)
            {
                _hasHit = true;
                _hitLocation = ray.GetPoint(closest);
                _hitNormal = hitNormal;
            }

            // Handle painting
            if (Owner.IsLeftMouseButtonDown)
                PaintStart();
            else
                PaintEnd();
            PaintUpdate();
        }

        /// <inheritdoc />
        public override void Pick()
        {
            var ray = Owner.MouseRay;
            var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
            var rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.SkipColliders | SceneGraphNode.RayCastData.FlagTypes.SkipEditorPrimitives;
            var hit = Owner.SceneGraphRoot.RayCast(ref ray, ref view, out _, rayCastFlags);
            if (hit != null && hit is ActorNode actorNode && actorNode.Actor is StaticModel model)
                Owner.Select(new List<SceneGraphNode> { hit });
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!IsActive || !_selectedModel)
                return;

            base.Draw(ref renderContext);

            if (_hasHit)
            {
                var viewOrigin = renderContext.View.Origin;

                // Draw paint brush
                if (!_brushModel)
                {
                    _brushModel = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Sphere");
                }
                if (!_brushMaterial)
                {
                    var material = FlaxEngine.Content.LoadAsyncInternal<Material>(EditorAssets.FoliageBrushMaterial);
                    _brushMaterial = material.CreateVirtualInstance();
                }
                if (_brushModel && _brushMaterial)
                {
                    _brushMaterial.SetParameterValue("Color", new Color(1.0f, 0.85f, 0.0f)); // TODO: expose to editor options
                    _brushMaterial.SetParameterValue("DepthBuffer", Owner.RenderTask.Buffers.DepthBuffer);
                    Quaternion rotation = RootNode.RaycastNormalRotation(ref _hitNormal);
                    Matrix transform = Matrix.Scaling(_gizmoMode.BrushSize * 0.01f) * Matrix.RotationQuaternion(rotation) * Matrix.Translation(_hitLocation - viewOrigin);
                    _brushModel.Draw(ref renderContext, _brushMaterial, ref transform);
                }

                // Draw intersecting vertices
                var meshDatas = _meshDatas?.MeshDatas;
                if (meshDatas != null && _gizmoMode.PreviewVertexSize > Mathf.Epsilon)
                {
                    if (!_verticesPreviewMaterial)
                    {
                        _verticesPreviewMaterial = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.WiresDebugMaterial);
                    }
                    var instanceTransform = _selectedModel.Transform;
                    var modelScaleMatrix = Matrix.Scaling(_gizmoMode.PreviewVertexSize * 0.01f);
                    var brushSphere = new BoundingSphere(_hitLocation, _gizmoMode.BrushSize);
                    var lodIndex = _gizmoMode.ModelLOD == -1 ? RenderTools.ComputeModelLOD(_selectedModel.Model, ref renderContext.View.Position, (float)_selectedModel.Sphere.Radius, ref renderContext) : _gizmoMode.ModelLOD;
                    lodIndex = Mathf.Clamp(lodIndex, 0, meshDatas.Length - 1);
                    var lodData = meshDatas[lodIndex];
                    if (lodData != null)
                    {
                        for (int meshIndex = 0; meshIndex < lodData.Length; meshIndex++)
                        {
                            var meshData = lodData[meshIndex];
                            var positionStream = meshData.VertexAccessor.Position();
                            if (!positionStream.IsValid)
                                continue;
                            var vertexCount = positionStream.Count;
                            for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
                            {
                                var pos = instanceTransform.LocalToWorld(positionStream.GetFloat3(vertexIndex));
                                if (brushSphere.Contains(ref pos) == ContainmentType.Disjoint)
                                    continue;
                                Matrix transform = modelScaleMatrix * Matrix.Translation(pos - viewOrigin);
                                _brushModel.Draw(ref renderContext, _verticesPreviewMaterial, ref transform);
                            }
                        }
                    }
                }
            }

            // Update vertex colors preview
            var cachedMaterials = _gizmoMode.Tab._cachedMaterials;
            if (cachedMaterials == null)
                return;
            var previewMode = _gizmoMode.PreviewMode;
            if (previewMode == VertexPaintingGizmoMode.VertexColorsPreviewMode.None)
            {
                for (int i = 0; i < cachedMaterials.Length; i++)
                    _selectedModel.SetMaterial(i, cachedMaterials[i]);
                return;
            }
            if (!_vertexColorsPreviewMaterial)
                _vertexColorsPreviewMaterial = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.VertexColorsPreviewMaterial).CreateVirtualInstance();
            if (!_vertexColorsPreviewMaterial)
                return;
            var channelMask = new Float4();
            switch (previewMode)
            {
            case VertexPaintingGizmoMode.VertexColorsPreviewMode.RGB:
                channelMask = new Float4(1, 1, 1, 0);
                break;
            case VertexPaintingGizmoMode.VertexColorsPreviewMode.Red:
                channelMask.X = 1.0f;
                break;
            case VertexPaintingGizmoMode.VertexColorsPreviewMode.Green:
                channelMask.Y = 1.0f;
                break;
            case VertexPaintingGizmoMode.VertexColorsPreviewMode.Blue:
                channelMask.Z = 1.0f;
                break;
            case VertexPaintingGizmoMode.VertexColorsPreviewMode.Alpha:
                channelMask.W = 1.0f;
                break;
            }
            _vertexColorsPreviewMaterial.SetParameterValue("ChannelMask", channelMask);
            for (int i = 0; i < cachedMaterials.Length; i++)
                _selectedModel.SetMaterial(i, _vertexColorsPreviewMaterial);
        }

        public override void OnActivated()
        {
            base.OnActivated();

            _hasHit = false;
        }

        public override void OnDeactivated()
        {
            base.OnDeactivated();

            PaintEnd();
            SetPaintModel(null);
            Object.Destroy(ref _vertexColorsPreviewMaterial);
            Object.Destroy(ref _brushMaterial);
            _brushModel = null;
            _verticesPreviewMaterial = null;
        }
    }

    sealed class EditModelVertexColorsAction : IUndoAction
    {
        private Guid _actorId;
        private string _before, _after;

        public EditModelVertexColorsAction(StaticModel model)
        {
            _actorId = model.ID;
            _before = GetState(model);
        }

        public static bool IsValidState(string state)
        {
            return state != null && state.Contains("\"VertexColors\":");
        }

        public static string GetState(StaticModel model)
        {
            var json = model.ToJson();
            var start = json.IndexOf("\"VertexColors\":");
            if (start == -1)
                return null;
            var end = json.IndexOf(']', start);
            json = "{" + json.Substring(start, end - start) + "]}";
            return json;
        }

        public static void SetState(StaticModel model, string state)
        {
            if (state == null)
                model.RemoveVertexColors();
            else
                Editor.Internal_DeserializeSceneObject(Object.GetUnmanagedPtr(model), state);
        }

        public void RecordEnd()
        {
            var model = Object.Find<StaticModel>(ref _actorId);
            _after = GetState(model);
            Editor.Instance.Scene.MarkSceneEdited(model.Scene);
        }

        private void Set(string state)
        {
            var model = Object.Find<StaticModel>(ref _actorId);
            SetState(model, state);
            Editor.Instance.Scene.MarkSceneEdited(model.Scene);
        }

        public string ActionString => "Edit Vertex Colors";

        public void Do()
        {
            Set(_after);
        }

        public void Undo()
        {
            Set(_before);
        }

        public void Dispose()
        {
            _before = _after = null;
        }
    }
}
