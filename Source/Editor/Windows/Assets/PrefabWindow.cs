// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
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
    public sealed partial class PrefabWindow : AssetEditorWindowBase<Prefab>, IPresenterOwner, ISceneContextWindow
    {
        private readonly SplitPanel _split1;
        private readonly SplitPanel _split2;
        private readonly TextBox _searchBox;
        private readonly Panel _treePanel;
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
        private bool _isDropping = false;

        /// <summary>
        /// Gets the prefab hierarchy tree control.
        /// </summary>
        public PrefabTree Tree => _tree;

        /// <summary>
        /// Gets the viewport.
        /// </summary>
        public PrefabWindowViewport Viewport => _viewport;

        /// <summary>
        /// Gets the prefab objects properties editor.
        /// </summary>
        public CustomEditorPresenter Presenter => _propertiesEditor;

        /// <summary>
        /// Gets the undo system used by this window for changes tracking.
        /// </summary>
        public Undo Undo => _undo;

        /// <summary>
        /// The local scene nodes graph used by the prefab editor.
        /// </summary>
        public readonly LocalSceneGraph Graph;

        /// <summary>
        /// Indication of if the prefab window selection is locked on specific objects.
        /// </summary>
        public bool LockSelectedObjects = false;

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
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoEvent;
            _undo.RedoDone += OnUndoEvent;
            _undo.ActionDone += OnUndoEvent;

            // Split Panel 1
            _split1 = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.2f,
                Parent = this
            };
            var sceneTreePanel = new SceneTreePanel(this)
            {
                Parent = _split1.Panel1,
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
                BackgroundColor = Style.Current.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 18 + 6),
            };
            _searchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            // Prefab structure tree
            _treePanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0.0f, 0.0f, headerPanel.Bottom, 0.0f),
                ScrollBars = ScrollBars.Both,
                IsScrollable = true,
                Parent = sceneTreePanel,
            };
            Graph = new LocalSceneGraph(new CustomRootNode(this));
            Graph.Root.TreeNode.Expand(true);
            _tree = new PrefabTree
            {
                Margin = new Margin(0.0f, 0.0f, -16.0f, _treePanel.ScrollBarsSize), // Hide root node
                IsScrollable = true,
            };
            _tree.AddChild(Graph.Root.TreeNode);
            _tree.SelectedChanged += OnTreeSelectedChanged;
            _tree.RightClick += OnTreeRightClick;
            _tree.Parent = _treePanel;
            headerPanel.Parent = sceneTreePanel;

            // Prefab viewport
            _viewport = new PrefabWindowViewport(this)
            {
                Parent = _split2.Panel1
            };
            _viewport.TransformGizmo.ModeChanged += UpdateToolstrip;

            // Prefab properties editor
            _propertiesEditor = new CustomEditorPresenter(_undo, null, this);
            _propertiesEditor.Panel.Parent = _split2.Panel2;
            _propertiesEditor.Modified += MarkAsEdited;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _toolStripUndo = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _toolStripRedo = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _toolStripTranslate = _toolstrip.AddButton(Editor.Icons.Translate32, () => _viewport.TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate).LinkTooltip("Change Gizmo tool mode to Translate", ref inputOptions.TranslateMode);
            _toolStripRotate = _toolstrip.AddButton(Editor.Icons.Rotate32, () => _viewport.TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate).LinkTooltip("Change Gizmo tool mode to Rotate", ref inputOptions.RotateMode);
            _toolStripScale = _toolstrip.AddButton(Editor.Icons.Scale32, () => _viewport.TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale).LinkTooltip("Change Gizmo tool mode to Scale", ref inputOptions.ScaleMode);
            _toolstrip.AddSeparator();
            _toolStripLiveReload = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Refresh64, () => LiveReload = !LiveReload).SetChecked(true).SetAutoCheck(true).LinkTooltip("Live changes preview (applies prefab changes on modification by auto)");

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
            InputActions.Add(options => options.Rename, RenameSelection);
            InputActions.Add(options => options.FocusSelection, FocusSelection);
        }

        /// <summary>
        /// Enables or disables vertical and horizontal scrolling on the tree panel.
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
        public void ScrollingOnTreeView(bool enabled)
        {
            if (_treePanel.VScrollBar != null)
                _treePanel.VScrollBar.ThumbEnabled = enabled;
            if (_treePanel.HScrollBar != null)
                _treePanel.HScrollBar.ThumbEnabled = enabled;
        }

        /// <summary>
        /// Scrolls to the selected node in the tree.
        /// </summary>
        public void ScrollToSelectedNode()
        {
            // Scroll to node
            var nodeSelection = _tree.Selection;
            if (nodeSelection.Count != 0)
            {
                var scrollControl = nodeSelection[nodeSelection.Count - 1];
                _treePanel.ScrollViewTo(scrollControl);
            }
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            _tree.LockChildrenRecursive();

            // Update tree
            var query = _searchBox.Text;
            var root = Graph.Root;
            root.TreeNode.UpdateFilter(query);

            _tree.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (button == MouseButton.Right && _treePanel.ContainsPoint(ref location))
            {
                _tree.Deselect();
                var locationCM = location + _searchBox.BottomLeft;
                ShowContextMenu(Parent, ref locationCM);
                return true;
            }

            if (button == MouseButton.Left && _treePanel.ContainsPoint(ref location) && !_isDropping)
            {
                _tree.Deselect();
                return true;
            }
            if (_isDropping)
            {
                _isDropping = false;
                return true;
            }
            return false;
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
            OnPrefabOpened();
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

        private void OnPrefabOpened()
        {
            _viewport.Prefab = _asset;
            if (Editor.ProjectCache.TryGetCustomData($"UIMode:{_asset.ID}", out bool value))
                _viewport.SetInitialUIMode(value);
            else
                _viewport.SetInitialUIMode(_viewport._hasUILinked);
            _viewport.UIModeToggled += OnUIModeToggled;
            Graph.MainActor = _viewport.Instance;
            Selection.Clear();
            Select(Graph.Main);
            Graph.Root.TreeNode.Expand(true);
        }

        private void OnUIModeToggled(bool value)
        {
            Editor.ProjectCache.SetCustomData($"UIMode:{_asset.ID}", value);
        }

        /// <inheritdoc />
        public override void Save()
        {
            // Check if don't need to push any new changes to the original asset
            if (!IsEdited)
                return;

            try
            {
                Editor.Scene.OnSaveStart(_viewport._uiParentLink);

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
            finally
            {
                Editor.Scene.OnSaveEnd(_viewport._uiParentLink);
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
            _toolStripTranslate.Checked = gizmoMode == TransformGizmoBase.Mode.Translate;
            _toolStripRotate.Checked = gizmoMode == TransformGizmoBase.Mode.Rotate;
            _toolStripScale.Checked = gizmoMode == TransformGizmoBase.Mode.Scale;
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

            OnPrefabOpened();
            _focusCamera = true;
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
                FlaxEngine.Profiler.BeginEvent("PrefabWindow.Update");
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
                    OnPrefabOpened();
                }
            }
            finally
            {
                FlaxEngine.Profiler.EndEvent();
            }

            // Auto fit
            if (_focusCamera && _viewport.Task.FrameCount > 1)
            {
                _focusCamera = false;
                Editor.GetActorEditorSphere(_viewport.Instance, out BoundingSphere bounds);
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
            LayoutSerializeSplitter(writer, "Split1", _split1);
            LayoutSerializeSplitter(writer, "Split2", _split2);
            writer.WriteAttributeString("LiveReload", LiveReload.ToString());
            writer.WriteAttributeString("GizmoMode", Viewport.TransformGizmo.ActiveMode.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split1", _split1);
            LayoutDeserializeSplitter(node, "Split2", _split2);
            if (bool.TryParse(node.GetAttribute("LiveReload"), out bool value2))
                LiveReload = value2;
            if (Enum.TryParse(node.GetAttribute("GizmoMode"), out TransformGizmoBase.Mode value3))
                Viewport.TransformGizmo.ActiveMode = value3;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split1.SplitterValue = 0.2f;
            _split2.SplitterValue = 0.6f;
            LiveReload = false;
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

        /// <inheritdoc />
        public EditorViewport PresenterViewport => _viewport;
    }
}
