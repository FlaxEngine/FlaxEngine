// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Prefab window allows to view and edit <see cref="Prefab"/> asset.
    /// </summary>
    /// <seealso cref="Prefab" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed partial class PrefabWindow : AssetEditorWindowBase<Prefab>
    {
        private readonly SplitPanel _split1;
        private readonly SplitPanel _split2;
        private TextBox _searchBox;
        private readonly PrefabTree _tree;
        private readonly PrefabWindowViewport _viewport;
        private readonly CustomEditorPresenter _propertiesEditor;

        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _toolStripUndo;
        private readonly ToolStripButton _toolStripRedo;
        private readonly ToolStripButton _toolStripTranslate;
        private readonly ToolStripButton _toolStripRotate;
        private readonly ToolStripButton _toolStripScale;
        private readonly ToolStripButton _toolStripLiveReload;

        private Undo _undo;
        private bool _focusCamera;
        private bool _liveReload = false;
        private bool _isUpdatingSelection, _isScriptsReloading;
        private DateTime _modifiedTime = DateTime.MinValue;

        /// <summary>
        /// Gets the prefab hierarchy tree control.
        /// </summary>
        public PrefabTree Tree => _tree;

        /// <summary>
        /// Gets the viewport.
        /// </summary>
        public PrefabWindowViewport Viewport => _viewport;

        /// <summary>
        /// Gets the undo system used by this window for changes tracking.
        /// </summary>
        public Undo Undo => _undo;

        /// <summary>
        /// The local scene nodes graph used by the prefab editor.
        /// </summary>
        public readonly LocalSceneGraph Graph;

        /// <summary>
        /// Gets or sets a value indicating whether use live reloading for the prefab changes (applies prefab changes on modification by auto).
        /// </summary>
        public bool LiveReload
        {
            get => _liveReload;
            set
            {
                if (_liveReload != value)
                {
                    _liveReload = value;

                    UpdateToolstrip();
                }
            }
        }

        /// <summary>
        /// Gets or sets the live reload timeout. It defines the time to apply prefab changes after modification.
        /// </summary>
        public TimeSpan LiveReloadTimeout { get; set; } = TimeSpan.FromMilliseconds(100);

        /// <inheritdoc />
        public PrefabWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoEvent;
            _undo.RedoDone += OnUndoEvent;
            _undo.ActionDone += OnUndoEvent;

            // Split Panel 1
            _split1 = new SplitPanel(Orientation.Horizontal, ScrollBars.Both, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.2f,
                Parent = this
            };

            // Split Panel 2
            _split2 = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                SplitterValue = 0.6f,
                Parent = _split1.Panel2
            };

            // Prefab structure tree searching query input box
            var headerPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 6),
                Parent = _split1.Panel1,
            };
            _searchBox = new TextBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                WatermarkText = "Search...",
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            // Prefab structure tree
            Graph = new LocalSceneGraph(new CustomRootNode(this));
            _tree = new PrefabTree
            {
                Y = headerPanel.Bottom,
                Margin = new Margin(0.0f, 0.0f, -16.0f, 0.0f), // Hide root node
            };
            _tree.AddChild(Graph.Root.TreeNode);
            _tree.SelectedChanged += OnTreeSelectedChanged;
            _tree.RightClick += OnTreeRightClick;
            _tree.Parent = _split1.Panel1;

            // Prefab viewport
            _viewport = new PrefabWindowViewport(this)
            {
                Parent = _split2.Panel1
            };
            _viewport.TransformGizmo.ModeChanged += UpdateToolstrip;

            // Prefab properties editor
            _propertiesEditor = new CustomEditorPresenter(_undo);
            _propertiesEditor.Panel.Parent = _split2.Panel2;
            _propertiesEditor.Modified += MarkAsEdited;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Save32, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _toolStripUndo = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo32, _undo.PerformUndo).LinkTooltip("Undo (Ctrl+Z)");
            _toolStripRedo = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo32, _undo.PerformRedo).LinkTooltip("Redo (Ctrl+Y)");
            _toolstrip.AddSeparator();
            _toolStripTranslate = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Translate32, () => _viewport.TransformGizmo.ActiveMode = TransformGizmo.Mode.Translate).LinkTooltip("Change Gizmo tool mode to Translate (1)");
            _toolStripRotate = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Rotate32, () => _viewport.TransformGizmo.ActiveMode = TransformGizmo.Mode.Rotate).LinkTooltip("Change Gizmo tool mode to Rotate (2)");
            _toolStripScale = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Scale32, () => _viewport.TransformGizmo.ActiveMode = TransformGizmo.Mode.Scale).LinkTooltip("Change Gizmo tool mode to Scale (3)");
            _toolstrip.AddSeparator();
            _toolStripLiveReload = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Reload32, () => LiveReload = !LiveReload).SetChecked(true).SetAutoCheck(true).LinkTooltip("Live changes preview (applies prefab changes on modification by auto)");

            Editor.Prefabs.PrefabApplied += OnPrefabApplied;
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;

            // Setup input actions
            InputActions.Add(options => options.Undo, () =>
            {
                _undo.PerformUndo();
                Focus();
            });
            InputActions.Add(options => options.Redo, () =>
            {
                _undo.PerformRedo();
                Focus();
            });
            InputActions.Add(options => options.Cut, Cut);
            InputActions.Add(options => options.Copy, Copy);
            InputActions.Add(options => options.Paste, Paste);
            InputActions.Add(options => options.Duplicate, Duplicate);
            InputActions.Add(options => options.Delete, Delete);
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            var root = Graph.Root;
            root.TreeNode.LockChildrenRecursive();

            // Update tree
            var query = _searchBox.Text;
            root.TreeNode.UpdateFilter(query);

            root.TreeNode.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        private void OnScriptsReloadBegin()
        {
            _isScriptsReloading = true;

            if (_asset == null || !_asset.IsLoaded)
                return;

            Editor.Log("Reloading prefab editor data on scripts reload. Prefab: " + _asset.Path);

            // Check if asset has been edited and not saved (and still has linked item)
            if (IsEdited)
            {
                // Ask user for further action
                var result = MessageBox.Show(
                                             string.Format("Asset \'{0}\' has been edited. Save before scripts reload?", _item.Path),
                                             "Save before reloading?",
                                             MessageBoxButtons.YesNo
                                            );
                if (result == DialogResult.OK || result == DialogResult.Yes)
                {
                    Save();
                }
            }

            // Cleanup
            Deselect();
            Graph.MainActor = null;
            _viewport.Prefab = null;
            _undo?.Clear(); // TODO: maybe don't clear undo?
        }

        private void OnScriptsReloadEnd()
        {
            _isScriptsReloading = false;

            if (_asset == null || !_asset.IsLoaded)
                return;

            // Restore
            _viewport.Prefab = _asset;
            Graph.MainActor = _viewport.Instance;
            Selection.Clear();
            Select(Graph.Main);
            Graph.Root.TreeNode.ExpandAll(true);
            _undo.Clear();
            ClearEditedFlag();
        }

        private void OnUndoEvent(IUndoAction action)
        {
            // All undo actions modify the asset except selection change
            if (!(action is SelectionChangeAction))
            {
                MarkAsEdited();
            }

            UpdateToolstrip();
        }

        private void OnPrefabApplied(Prefab prefab, Actor instance)
        {
            if (prefab == Asset)
            {
                ClearEditedFlag();

                _item.RefreshThumbnail();
            }
        }

        /// <inheritdoc />
        public override void Save()
        {
            // Check if don't need to push any new changes to the original asset
            if (!IsEdited)
                return;

            try
            {
                // Simply update changes
                Editor.Prefabs.ApplyAll(_viewport.Instance);

                // Refresh properties panel to sync new prefab default values
                _propertiesEditor.BuildLayout();
            }
            catch (Exception)
            {
                // Disable live reload on error
                if (LiveReload)
                {
                    LiveReload = false;
                }

                throw;
            }
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            var undoRedo = _undo;
            var gizmo = _viewport.TransformGizmo;

            _saveButton.Enabled = IsEdited;
            _toolStripUndo.Enabled = undoRedo.CanUndo;
            _toolStripRedo.Enabled = undoRedo.CanRedo;
            //
            var gizmoMode = gizmo.ActiveMode;
            _toolStripTranslate.Checked = gizmoMode == TransformGizmo.Mode.Translate;
            _toolStripRotate.Checked = gizmoMode == TransformGizmo.Mode.Rotate;
            _toolStripScale.Checked = gizmoMode == TransformGizmo.Mode.Scale;
            //
            _toolStripLiveReload.Checked = _liveReload;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void OnEditedState()
        {
            base.OnEditedState();

            _modifiedTime = DateTime.Now;
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            // Skip during scripts reload to prevent issues
            if (_isScriptsReloading)
            {
                base.OnAssetLoaded();
                return;
            }

            _viewport.Prefab = _asset;
            Graph.MainActor = _viewport.Instance;
            _focusCamera = true;
            Selection.Clear();
            Select(Graph.Main);
            Graph.Root.TreeNode.ExpandAll(true);

            _undo.Clear();
            ClearEditedFlag();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            Deselect();
            Graph.MainActor = null;
            _viewport.Prefab = null;
            _undo?.Clear();

            base.UnlinkItem();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            try
            {
                if (Graph.Main != null)
                {
                    // Due to fact that actors in prefab editor are only created but not added to gameplay 
                    // we have to manually update some data (Level events work only for actors in a gameplay)
                    Update(Graph.Main);
                }

                base.Update(deltaTime);
            }
            catch (Exception ex)
            {
                // Info
                Editor.LogWarning("Error when updating prefab window for " + _asset);
                Editor.LogWarning(ex);

                // Refresh
                Deselect();
                Graph.MainActor = null;
                _viewport.Prefab = null;
                if (_asset.IsLoaded)
                {
                    _viewport.Prefab = _asset;
                    Graph.MainActor = _viewport.Instance;
                    Selection.Clear();
                    Select(Graph.Main);
                    Graph.Root.TreeNode.ExpandAll(true);
                }
            }

            // Auto fit
            if (_focusCamera && _viewport.Task.FrameCount > 1)
            {
                _focusCamera = false;

                BoundingSphere bounds;
                Editor.GetActorEditorSphere(_viewport.Instance, out bounds);
                _viewport.ViewPosition = bounds.Center - _viewport.ViewDirection * (bounds.Radius * 1.2f);
            }

            // Auto save
            if (IsEdited && LiveReload && DateTime.Now - _modifiedTime > LiveReloadTimeout)
            {
                Save();
            }
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("Split1", _split1.SplitterValue.ToString());
            writer.WriteAttributeString("Split2", _split2.SplitterValue.ToString());
            writer.WriteAttributeString("LiveReload", LiveReload.ToString());
            writer.WriteAttributeString("GizmoMode", Viewport.TransformGizmo.ActiveMode.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            float value1;

            if (float.TryParse(node.GetAttribute("Split1"), out value1))
                _split1.SplitterValue = value1;
            if (float.TryParse(node.GetAttribute("Split2"), out value1))
                _split2.SplitterValue = value1;

            bool value2;

            if (bool.TryParse(node.GetAttribute("LiveReload"), out value2))
                LiveReload = value2;

            TransformGizmo.Mode value3;
            if (Enum.TryParse(node.GetAttribute("GizmoMode"), out value3))
                Viewport.TransformGizmo.ActiveMode = value3;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split1.SplitterValue = 0.2f;
            _split2.SplitterValue = 0.6f;
            LiveReload = true;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Editor.Prefabs.PrefabApplied -= OnPrefabApplied;
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;

            _undo.Dispose();
            Graph.Dispose();

            base.OnDestroy();
        }
    }
}
