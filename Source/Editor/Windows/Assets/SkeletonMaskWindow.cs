// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="SkeletonMask"/> asset.
    /// </summary>
    /// <seealso cref="SkeletonMask" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class SkeletonMaskWindow : AssetEditorWindowBase<SkeletonMask>
    {
        /// <summary>
        /// The asset properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private SkeletonMaskWindow Window;
            private SkeletonMask Asset;

            [EditorDisplay("Skeleton"), Tooltip("The skinned model asset used for the skeleton mask reference.")]
            public SkinnedModel Skeleton
            {
                get => Window._preview.SkinnedModel;
                set
                {
                    if (value != Window._preview.SkinnedModel)
                    {
                        // Change skeleton, invalidate mask and request UI update
                        Window._preview.SkinnedModel = value;
                        Window._preview.NodesMask = null;
                        Window._propertiesPresenter.BuildLayoutOnUpdate();
                    }
                }
            }

            [HideInEditor]
            public bool[] NodesMask
            {
                get => Window._preview.NodesMask;
                set => Window._preview.NodesMask = value;
            }

            public void OnLoad(SkeletonMaskWindow window)
            {
                // Link
                Window = window;
                Asset = window.Asset;

                // Get data from the asset
                Skeleton = Asset.Skeleton;
                NodesMask = Asset.NodesMask;
            }

            public void OnClean()
            {
                // Unlink
                Window = null;
                Asset = null;
            }

            private class ProxyEditor : GenericEditor
            {
                private bool _waitForSkeletonLoaded;

                /// <inheritdoc />
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (PropertiesProxy)Values[0];
                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    base.Initialize(layout);

                    // Check reference skeleton
                    var skeleton = proxy.Skeleton;
                    if (skeleton == null)
                        return;
                    if (!skeleton.IsLoaded)
                    {
                        // We need to have skeleton loaded for a nodes references
                        _waitForSkeletonLoaded = true;
                        return;
                    }

                    // Init mask if missing or validate it
                    var nodes = skeleton.Nodes;
                    if (nodes == null || nodes.Length == 0)
                        return;
                    var mask = proxy.NodesMask;
                    if (mask == null || mask.Length != nodes.Length)
                    {
                        if (mask != null)
                            Debug.Write(LogType.Error, $"Invalid size nodes mask (got {mask.Length} but there are {nodes.Length} nodes)");
                        mask = proxy.NodesMask = new bool[nodes.Length];
                        for (int i = 0; i < nodes.Length; i++)
                            mask[i] = true;
                    }

                    // Skeleton Mask
                    var group = layout.Group("Mask");
                    var tree = group.Tree();
                    for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                    {
                        if (nodes[nodeIndex].ParentIndex == -1)
                        {
                            BuildSkeletonNodeTree(mask, nodes, nodeIndex, tree);
                        }
                    }
                }

                /// <inheritdoc />
                public override void Refresh()
                {
                    if (_waitForSkeletonLoaded)
                    {
                        _waitForSkeletonLoaded = false;
                        RebuildLayout();
                        return;
                    }

                    base.Refresh();
                }

                private void BuildSkeletonNodeTree(bool[] mask, SkeletonNode[] nodes, int nodeIndex, ITreeElement layout)
                {
                    var node = layout.Node(nodes[nodeIndex].Name);
                    node.TreeNode.ClipChildren = false;
                    node.TreeNode.TextMargin = new Margin(20.0f, 2.0f, 2.0f, 2.0f);
                    node.TreeNode.Expand(true);
                    var checkbox = new CheckBox(0, 0, mask[nodeIndex])
                    {
                        Height = 16.0f,
                        IsScrollable = false,
                        Tag = nodeIndex,
                        Parent = node.TreeNode
                    };
                    checkbox.StateChanged += OnCheckChanged;

                    for (int i = 0; i < nodes.Length; i++)
                    {
                        if (nodes[i].ParentIndex == nodeIndex)
                        {
                            BuildSkeletonNodeTree(mask, nodes, i, node);
                        }
                    }
                }

                private void OnCheckChanged(CheckBox checkBox)
                {
                    var proxy = (PropertiesProxy)Values[0];
                    int nodeIndex = (int)checkBox.Tag;
                    proxy.NodesMask[nodeIndex] = checkBox.Checked;
                    if (Input.GetKey(KeyboardKeys.Shift))
                        SetTreeChecked(checkBox.Parent as TreeNode, checkBox.Checked);
                    proxy.Window.MarkAsEdited();
                }

                private void SetTreeChecked(TreeNode tree, bool state)
                {
                    foreach (var node in tree.Children)
                    {
                        if (node is TreeNode treeNode)
                            SetTreeChecked(treeNode, state);
                        else if (node is CheckBox checkBox)
                            checkBox.Checked = state;
                    }
                }
            }
        }

        private readonly SplitPanel _split;
        private readonly AnimatedModelPreview _preview;
        private readonly CustomEditorPresenter _propertiesPresenter;
        private readonly PropertiesProxy _properties;
        private readonly ToolStripButton _saveButton;

        /// <inheritdoc />
        public SkeletonMaskWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/skeleton-mask.html")).LinkTooltip("See documentation to learn more");

            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };

            // Model preview
            _preview = new AnimatedModelPreview(true)
            {
                ViewportCamera = new FPSCamera(),
                ShowNodes = true,
                Parent = _split.Panel1
            };

            // Model properties
            _propertiesPresenter = new CustomEditorPresenter(null);
            _propertiesPresenter.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _propertiesPresenter.Select(_properties);
            _propertiesPresenter.Modified += MarkAsEdited;
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            _asset.Skeleton = _properties.Skeleton;
            int count = 0;
            var nodesMask = _preview.NodesMask;
            if (nodesMask != null)
            {
                for (int nodeIndex = 0; nodeIndex < nodesMask.Length; nodeIndex++)
                {
                    if (nodesMask[nodeIndex])
                        count++;
                }
            }
            var nodes = new string[count];
            if (nodesMask != null)
            {
                var i = 0;
                for (int nodeIndex = 0; nodeIndex < nodesMask.Length; nodeIndex++)
                {
                    if (nodesMask[nodeIndex])
                        nodes[i++] = _properties.Skeleton.Nodes[nodeIndex].Name;
                }
            }
            _asset.MaskedNodes = nodes;
            if (_asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.SkinnedModel = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.SkinnedModel = null;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _properties.OnLoad(this);
            _propertiesPresenter.BuildLayout();
            ClearEditedFlag();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.7f;
        }
    }
}
