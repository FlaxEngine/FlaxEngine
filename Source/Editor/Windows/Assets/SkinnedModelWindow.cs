// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="SkinnedModel"/> asset.
    /// </summary>
    /// <seealso cref="SkinnedModel" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class SkinnedModelWindow : ModelBaseWindow<SkinnedModel, SkinnedModelWindow>
    {
        private sealed class Preview : SkinnedModelPreview
        {
            private readonly SkinnedModelWindow _window;

            public Preview(SkinnedModelWindow window)
            : base(true)
            {
                _window = window;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var asset = _window.Asset;
                if (asset == null || !asset.IsLoaded)
                {
                    Render2D.DrawText(style.FontLarge, asset != null && asset.LastLoadFailed ? "Failed to load" : "Loading...", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class MeshesPropertiesProxy : MeshesPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class SkeletonPropertiesProxy : PropertiesProxyBase
        {
            internal Tree NodesTree;

            private class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (SkeletonPropertiesProxy)Values[0];
                    if (Utilities.Utils.OnAssetProperties(layout, proxy.Asset))
                        return;
                    var nodes = proxy.Asset.Nodes;
                    var bones = proxy.Asset.Bones;

                    // Skeleton Bones
                    {
                        var group = layout.Group("Skeleton Bones");
                        group.Panel.Close();

                        var tree = group.Tree();
                        tree.TreeControl.RightClick += OnTreeNodeRightClick;
                        for (int i = 0; i < bones.Length; i++)
                        {
                            if (bones[i].ParentIndex == -1)
                            {
                                var node = tree.Node(nodes[bones[i].NodeIndex].Name);
                                BuildSkeletonBonesTree(nodes, bones, node, i);
                                node.TreeNode.ExpandAll(true);
                            }
                        }
                    }

                    // Skeleton Nodes
                    {
                        var group = layout.Group("Skeleton Nodes");

                        var tree = group.Tree();
                        tree.TreeControl.RightClick += OnTreeNodeRightClick;
                        tree.TreeControl.SelectedChanged += OnTreeSelectedChanged;
                        for (int i = 0; i < nodes.Length; i++)
                        {
                            if (nodes[i].ParentIndex == -1)
                            {
                                var node = tree.Node(nodes[i].Name);
                                BuildSkeletonNodesTree(nodes, node, i);
                                node.TreeNode.ExpandAll(true);
                            }
                        }
                        proxy.NodesTree = tree.TreeControl;
                    }

                    // Blend Shapes
                    var blendShapes = proxy.Asset.BlendShapes;
                    if (blendShapes.Length != 0)
                    {
                        var group = layout.Group("Blend Shapes");
                        proxy.Window._preview.PreviewActor.ClearBlendShapeWeights();

                        for (int i = 0; i < blendShapes.Length; i++)
                        {
                            var blendShape = blendShapes[i];
                            var label = new PropertyNameLabel(blendShape);
                            label.SetupContextMenu += (nameLabel, menu, linkedEditor) => { menu.AddButton("Copy name", () => Clipboard.Text = blendShape); };
                            var property = group.AddPropertyItem(label);
                            var editor = property.FloatValue();
                            editor.ValueBox.Value = 0.0f;
                            editor.ValueBox.MinValue = -1;
                            editor.ValueBox.MaxValue = 1;
                            editor.ValueBox.SlideSpeed = 0.01f;
                            editor.ValueBox.ValueChanged += () => { proxy.Window._preview.SetBlendShapeWeight(blendShape, editor.ValueBox.Value); };
                        }
                    }
                }

                private void OnTreeNodeRightClick(TreeNode node, Float2 location)
                {
                    var menu = new ContextMenu();

                    var b = menu.AddButton("Copy name");
                    b.Tag = node.Text;
                    b.ButtonClicked += OnTreeNodeCopyName;

                    menu.Show(node, location);
                }

                private void OnTreeSelectedChanged(List<TreeNode> before, List<TreeNode> after)
                {
                    if (after.Count != 0)
                    {
                        var proxy = (SkeletonPropertiesProxy)Values[0];
                        proxy.Window._preview.ShowDebugDraw = true;
                    }
                }

                private void OnTreeNodeCopyName(ContextMenuButton b)
                {
                    Clipboard.Text = (string)b.Tag;
                }

                private void BuildSkeletonBonesTree(SkeletonNode[] nodes, SkeletonBone[] bones, TreeNodeElement layout, int boneIndex)
                {
                    for (int i = 0; i < bones.Length; i++)
                    {
                        if (bones[i].ParentIndex == boneIndex)
                        {
                            var node = layout.Node(nodes[bones[i].NodeIndex].Name);
                            BuildSkeletonBonesTree(nodes, bones, node, i);
                        }
                    }
                }

                private void BuildSkeletonNodesTree(SkeletonNode[] nodes, TreeNodeElement layout, int nodeIndex)
                {
                    for (int i = 0; i < nodes.Length; i++)
                    {
                        if (nodes[i].ParentIndex == nodeIndex)
                        {
                            var node = layout.Node(nodes[i].Name);
                            BuildSkeletonNodesTree(nodes, node, i);
                        }
                    }
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class MaterialsPropertiesProxy : MaterialsPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class UVsPropertiesProxy : UVsPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class RetargetPropertiesProxy : PropertiesProxyBase
        {
            internal class SetupProxy
            {
                public SkinnedModel Skeleton;
                public Dictionary<string, string> NodesMapping;
            }

            internal Dictionary<Asset, SetupProxy> Setups;

            public override void OnSave()
            {
                base.OnSave();

                if (Setups != null)
                {
                    var retargetSetups = new SkinnedModel.SkeletonRetarget[Setups.Count];
                    int i = 0;
                    foreach (var setup in Setups)
                    {
                        retargetSetups[i++] = new SkinnedModel.SkeletonRetarget
                        {
                            SourceAsset = setup.Key?.ID ?? Guid.Empty,
                            SkeletonAsset = setup.Value.Skeleton?.ID ?? Guid.Empty,
                            NodesMapping = setup.Value.NodesMapping,
                        };
                    }
                    Window.Asset.SkeletonRetargets = retargetSetups;
                }
            }

            private class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    if (Utilities.Utils.OnAssetProperties(layout, proxy.Asset))
                        return;
                    if (proxy.Setups == null)
                    {
                        proxy.Setups = new Dictionary<Asset, SetupProxy>();
                        var retargetSetups = proxy.Asset.SkeletonRetargets;
                        foreach (var retargetSetup in retargetSetups)
                        {
                            var sourceAsset = FlaxEngine.Content.LoadAsync(retargetSetup.SourceAsset);
                            if (sourceAsset)
                            {
                                proxy.Setups.Add(sourceAsset, new SetupProxy
                                {
                                    Skeleton = FlaxEngine.Content.LoadAsync<SkinnedModel>(retargetSetup.SkeletonAsset),
                                    NodesMapping = retargetSetup.NodesMapping,
                                });
                            }
                        }
                    }
                    var targetNodes = proxy.Asset.Nodes;

                    layout.Space(10.0f);
                    var infoLabel = layout.Label("Each retarget setup defines how to convert animated skeleton pose from a source asset to this skinned model skeleton. It allows to play animation from different skeleton on this skeleton. See documentation to learn more.").Label;
                    infoLabel.Wrapping = TextWrapping.WrapWords;
                    infoLabel.AutoHeight = true;
                    layout.Space(10.0f);

                    // New setup
                    {
                        var setupGroup = layout.Group("New setup");
                        setupGroup.Panel.Tag = null;
                        setupGroup.Panel.MouseButtonRightClicked += OnPanelHeaderRightClicked;
                        infoLabel = setupGroup.Label("Select model or animation asset to add new retarget source", TextAlignment.Center).Label;
                        infoLabel.Wrapping = TextWrapping.WrapWords;
                        infoLabel.AutoHeight = true;
                        var sourceAssetPicker = setupGroup.AddPropertyItem("Source Asset").Custom<AssetPicker>().CustomControl;
                        sourceAssetPicker.Height = 48;
                        sourceAssetPicker.CheckValid = CheckSourceAssetValid;
                        sourceAssetPicker.SelectedItemChanged += () =>
                        {
                            proxy.Setups.Add(sourceAssetPicker.Validator.SelectedAsset, new SetupProxy());
                            proxy.Window.MarkAsEdited();
                            RebuildLayout();
                        };
                    }

                    // Setups
                    foreach (var setup in proxy.Setups)
                    {
                        var sourceAsset = setup.Key;
                        if (sourceAsset == null)
                            continue;
                        var setupGroup = layout.Group(Path.GetFileNameWithoutExtension(sourceAsset.Path));
                        setupGroup.Panel.Tag = sourceAsset;
                        setupGroup.Panel.MouseButtonRightClicked += OnPanelHeaderRightClicked;
                        var settingsButton = setupGroup.AddSettingsButton();
                        settingsButton.Tag = sourceAsset;
                        settingsButton.Clicked += OnShowSetupSettings;

                        // Source asset picker
                        var sourceAssetPicker = setupGroup.AddPropertyItem("Source Asset").Custom<AssetPicker>().CustomControl;
                        sourceAssetPicker.Validator.SelectedAsset = sourceAsset;
                        sourceAssetPicker.CanEdit = false;
                        sourceAssetPicker.Height = 48;

                        if (sourceAsset is SkinnedModel sourceModel)
                        {
                            // Initialize nodes mapping structure
                            if (sourceModel.WaitForLoaded())
                                continue;
                            var sourceNodes = sourceModel.Nodes;
                            if (setup.Value.NodesMapping == null)
                                setup.Value.NodesMapping = new Dictionary<string, string>();
                            var nodesMapping = setup.Value.NodesMapping;
                            foreach (var targetNode in targetNodes)
                            {
                                if (!nodesMapping.ContainsKey(targetNode.Name))
                                {
                                    var node = string.Empty;
                                    foreach (var sourceNode in sourceNodes)
                                    {
                                        if (string.Equals(targetNode.Name, sourceNode.Name, StringComparison.OrdinalIgnoreCase))
                                        {
                                            node = sourceNode.Name;
                                            break;
                                        }
                                    }
                                    nodesMapping.Add(targetNode.Name, node);
                                }
                            }

                            // Build source skeleton nodes list (with hierarchy indentation)
                            var items = new string[sourceNodes.Length + 1];
                            items[0] = string.Empty;
                            for (int i = 0; i < sourceNodes.Length; i++)
                                items[i + 1] = sourceNodes[i].Name;

                            // Show combo boxes with this skeleton nodes to retarget from
                            foreach (var targetNode in targetNodes)
                            {
                                var nodeName = targetNode.Name;
                                var propertyName = nodeName;
                                var tmp = targetNode.ParentIndex;
                                while (tmp != -1)
                                {
                                    tmp = targetNodes[tmp].ParentIndex;
                                    propertyName = " " + propertyName;
                                }
                                var comboBox = setupGroup.AddPropertyItem(propertyName).Custom<ComboBox>().CustomControl;
                                comboBox.AddItems(items);
                                comboBox.Tag = new KeyValuePair<string, Asset>(nodeName, sourceAsset);
                                comboBox.SelectedItem = nodesMapping[nodeName];
                                if (comboBox.SelectedIndex == -1)
                                    comboBox.SelectedIndex = 0; // Auto-select empty node
                                comboBox.SelectedIndexChanged += OnSelectedNodeChanged;
                            }
                        }
                        else if (sourceAsset is Animation sourceAnimation)
                        {
                            // Show skeleton asset picker
                            var sourceSkeletonPicker = setupGroup.AddPropertyItem("Skeleton", "Skinned model that contains a skeleton for this animation retargeting.").Custom<AssetPicker>().CustomControl;
                            sourceSkeletonPicker.Validator.AssetType = new ScriptType(typeof(SkinnedModel));
                            sourceSkeletonPicker.Validator.SelectedAsset = setup.Value.Skeleton;
                            sourceSkeletonPicker.Height = 48;
                            sourceSkeletonPicker.SelectedItemChanged += () =>
                            {
                                setup.Value.Skeleton = (SkinnedModel)sourceSkeletonPicker.Validator.SelectedAsset;
                                proxy.Window.MarkAsEdited();
                            };
                        }
                    }
                }

                private void OnPanelHeaderRightClicked(DropPanel panel, Float2 location)
                {
                    OnShowSetupSettings((Asset)panel.Tag, panel, location);
                }

                private void OnSelectedNodeChanged(ComboBox comboBox)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    var sourceAsset = ((KeyValuePair<string, Asset>)comboBox.Tag).Value;
                    var nodeMappingKey = ((KeyValuePair<string, Asset>)comboBox.Tag).Key;
                    var nodeMappingValue = comboBox.SelectedItem;
                    // TODO: check for recursion in setup
                    proxy.Setups[sourceAsset].NodesMapping[nodeMappingKey] = nodeMappingValue;
                    proxy.Window.MarkAsEdited();
                }

                private void OnShowSetupSettings(Image settingsButton, MouseButton button)
                {
                    if (button == MouseButton.Left)
                    {
                        OnShowSetupSettings((Asset)settingsButton.Tag, settingsButton, new Float2(0, settingsButton.Height));
                    }
                }

                private void OnShowSetupSettings(Asset sourceAsset, Control targetControl, Float2 targetLocation)
                {
                    var menu = new ContextMenu { Tag = sourceAsset };
                    if (sourceAsset != null)
                    {
                        menu.AddButton("Clear", OnClearSetup);
                        menu.AddButton("Remove", OnRemoveSetup).Icon = Editor.Instance.Icons.Cross12;
                        menu.AddButton("Copy", OnCopySetup);
                    }
                    menu.AddButton("Paste", OnPasteSetup);
                    menu.Show(targetControl, targetLocation);
                }

                private void OnClearSetup(ContextMenuButton button)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    var sourceAsset = (Asset)button.ParentContextMenu.Tag;
                    var setup = proxy.Setups[sourceAsset];
                    setup.Skeleton = null;
                    foreach (var e in setup.NodesMapping.Keys.ToArray())
                        setup.NodesMapping[e] = string.Empty;
                    proxy.Window.MarkAsEdited();
                    RebuildLayout();
                }

                private void OnRemoveSetup(ContextMenuButton button)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    var sourceAsset = (Asset)button.ParentContextMenu.Tag;
                    proxy.Setups.Remove(sourceAsset);
                    proxy.Window.MarkAsEdited();
                    RebuildLayout();
                }

                private struct RetargetSetupData
                {
                    public Asset SourceAsset;
                    public SetupProxy Proxy;
                }

                private void OnCopySetup(ContextMenuButton button)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    var sourceAsset = (Asset)button.ParentContextMenu.Tag;
                    var setup = proxy.Setups[sourceAsset];
                    var str = FlaxEngine.Json.JsonSerializer.Serialize(new RetargetSetupData
                    {
                        SourceAsset = sourceAsset,
                        Proxy = setup,
                    });
                    Clipboard.Text = str;
                }

                private void OnPasteSetup(ContextMenuButton button)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    var sourceAsset = (Asset)button.ParentContextMenu.Tag;
                    var str = Clipboard.Text;
                    var data = FlaxEngine.Json.JsonSerializer.Deserialize<RetargetSetupData>(str);
                    if (sourceAsset == null)
                        sourceAsset = data.SourceAsset;
                    if (proxy.Setups.TryGetValue(sourceAsset, out var setup))
                    {
                        // Copy mappings for existing nodes in that mapping
                        foreach (var e in setup.NodesMapping.Keys.ToArray())
                        {
                            data.Proxy.NodesMapping.TryGetValue(e, out string name);
                            setup.NodesMapping[e] = name;
                        }
                    }
                    else
                    {
                        // Add a new mapping
                        proxy.Setups.Add(sourceAsset, setup = new SetupProxy());
                        setup.Skeleton = data.Proxy.Skeleton;
                        setup.NodesMapping = data.Proxy.NodesMapping;
                    }
                    proxy.Window.MarkAsEdited();
                    RebuildLayout();
                }

                private bool CheckSourceAssetValid(ContentItem item)
                {
                    var proxy = (RetargetPropertiesProxy)Values[0];
                    return item is BinaryAssetItem binaryItem &&
                           (binaryItem.Type == typeof(SkinnedModel) || binaryItem.Type == typeof(Animation)) &&
                           item != proxy.Window.Item &&
                           !proxy.Setups.ContainsKey(binaryItem.LoadAsync());
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class ImportPropertiesProxy : ImportPropertiesProxyBase
        {
        }

        private class MeshesTab : Tab
        {
            public MeshesTab(SkinnedModelWindow window)
            : base("Meshes", window)
            {
                Proxy = new MeshesPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class SkeletonTab : Tab
        {
            public SkeletonTab(SkinnedModelWindow window)
            : base("Skeleton", window)
            {
                Proxy = new SkeletonPropertiesProxy();
                Presenter.Select(Proxy);
                // Draw highlight on selected node
                window._preview.CustomDebugDraw += OnDebugDraw;
            }

            private void OnDebugDraw(GPUContext context, ref RenderContext renderContext)
            {
                var proxy = (SkeletonPropertiesProxy)Proxy;
                if (proxy.NodesTree != null)
                {
                    // Draw selected skeleton nodes
                    foreach (var node in proxy.NodesTree.Selection)
                    {
                        proxy.Window._preview.PreviewActor.GetNodeTransformation(node.Text, out var t, true);
                        DebugDraw.DrawWireSphere(new BoundingSphere(t.TranslationVector, 4.0f), Color.Red, 0.0f, false);
                        float tangentFrameSize = 0.05f * Utilities.Units.Meters2Units;
                        Vector3 arrowsOrigin = t.TranslationVector + 0.001f * Utilities.Units.Meters2Units;
                        DebugDraw.DrawLine(arrowsOrigin, arrowsOrigin + t.Forward * tangentFrameSize, CustomEditors.Editors.ActorTransformEditor.AxisColorX, 0.0f, false);
                        DebugDraw.DrawLine(arrowsOrigin, arrowsOrigin + t.Up * tangentFrameSize, CustomEditors.Editors.ActorTransformEditor.AxisColorY, 0.0f, false);
                        DebugDraw.DrawLine(arrowsOrigin, arrowsOrigin + t.Right * tangentFrameSize, CustomEditors.Editors.ActorTransformEditor.AxisColorZ, 0.0f, false);
                    }
                }
            }
        }

        private class MaterialsTab : Tab
        {
            public MaterialsTab(SkinnedModelWindow window)
            : base("Materials", window)
            {
                Proxy = new MaterialsPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class UVsTab : Tab
        {
            public UVsTab(SkinnedModelWindow window)
            : base("UVs", window, false)
            {
                Proxy = new UVsPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class RetargetTab : Tab
        {
            public RetargetTab(SkinnedModelWindow window)
            : base("Retarget", window)
            {
                Proxy = new RetargetPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class ImportTab : Tab
        {
            public ImportTab(SkinnedModelWindow window)
            : base("Import", window, false)
            {
                Proxy = new ImportPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private Preview _preview;
        private AnimatedModel _highlightActor;
        private ToolStripButton _showNodesButton;
        private ToolStripButton _showCurrentLODButton;

        /// <inheritdoc />
        public SkinnedModelWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Toolstrip
            _toolstrip.AddSeparator();
            _showCurrentLODButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Info64, () => _preview.ShowCurrentLOD = !_preview.ShowCurrentLOD).LinkTooltip("Show LOD statistics");
            _showNodesButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Bone64, () => _preview.ShowNodes = !_preview.ShowNodes).LinkTooltip("Show animated model nodes debug view");
            _toolstrip.AddButton(editor.Icons.CenterView64, () => _preview.ResetCamera()).LinkTooltip("Show whole model");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/skinned-model/index.html")).LinkTooltip("See documentation to learn more");

            // Model preview
            _preview = new Preview(this)
            {
                ViewportCamera = new FPSCamera(),
                ScaleToFit = false,
                Parent = _split.Panel1
            };

            // Properties tabs
            _tabs.AddTab(new MeshesTab(this));
            _tabs.AddTab(new SkeletonTab(this));
            _tabs.AddTab(new MaterialsTab(this));
            _tabs.AddTab(new UVsTab(this));
            _tabs.AddTab(new RetargetTab(this));
            _tabs.AddTab(new ImportTab(this));

            // Automatically show nodes when switching to skeleton page
            _tabs.SelectedTabChanged += (tabs) =>
            {
                if (tabs.SelectedTab is SkeletonTab)
                {
                    _preview.ShowNodes = true;
                }
            };

            // Highlight actor (used to highlight selected material slot, see UpdateEffectsOnAsset)
            _highlightActor = new AnimatedModel
            {
                IsActive = false
            };
            _preview.Task.AddCustomActor(_highlightActor);
        }

        /// <inheritdoc />
        protected override void UpdateEffectsOnAsset()
        {
            var entries = _preview.PreviewActor.Entries;
            if (entries != null)
            {
                for (int i = 0; i < entries.Length; i++)
                {
                    entries[i].Visible = _isolateIndex == -1 || _isolateIndex == i;
                }
                _preview.PreviewActor.Entries = entries;
            }

            if (_highlightIndex != -1)
            {
                _highlightActor.IsActive = true;

                var highlightMaterial = EditorAssets.Cache.HighlightMaterialInstance;
                entries = _highlightActor.Entries;
                if (entries != null)
                {
                    for (int i = 0; i < entries.Length; i++)
                    {
                        entries[i].Material = highlightMaterial;
                        entries[i].Visible = _highlightIndex == i;
                    }
                }
                _highlightActor.Entries = entries;
            }
            else
            {
                _highlightActor.IsActive = false;
            }
        }


        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Sync highlight actor size with actual preview model (preview scales model for better usage experience)
            if (_highlightActor && _highlightActor.IsActive)
            {
                _highlightActor.Transform = _preview.PreviewActor.Transform;
            }

            // Model is loaded but LODs data may be during streaming so refresh properties on fully loaded
            if (_refreshOnLODsLoaded && _asset && _asset.LoadedLODs == _asset.LODs.Length)
            {
                _refreshOnLODsLoaded = false;
                foreach (var child in _tabs.Children)
                {
                    if (child is Tab tab)
                    {
                        tab.Presenter.BuildLayout();
                    }
                }
            }

            _showCurrentLODButton.Checked = _preview.ShowCurrentLOD;
            _showNodesButton.Checked = _preview.ShowNodes;

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;
            if (_asset.WaitForLoaded())
                return;

            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab && tab.Proxy.Window != null)
                    tab.Proxy.OnSave();
            }

            if (_asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _preview.SkinnedModel = null;
            _highlightActor.SkinnedModel = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.SkinnedModel = _asset;
            _highlightActor.SkinnedModel = _asset;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _preview.ViewportCamera.SetArcBallView(_preview.GetBounds());

            // Reset any root motion
            _preview.PreviewActor.ResetLocalTransform();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            base.OnDestroy();

            Object.Destroy(ref _highlightActor);
            _preview = null;
            _showNodesButton = null;
            _showCurrentLODButton = null;
        }
    }
}
