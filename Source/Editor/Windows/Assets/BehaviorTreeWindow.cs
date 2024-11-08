// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;

namespace FlaxEditor.Windows.Assets
{
    [CustomEditor(typeof(BehaviorKnowledge)), DefaultEditor]
    sealed class BehaviorKnowledgeEditor : CustomEditor
    {
        private HashSet<Type> _goals;
        private LayoutElementsContainer _layout;

        public override void Initialize(LayoutElementsContainer layout)
        {
            var knowledge = (BehaviorKnowledge)Values[0];
            _layout = layout;

            // Blackboard
            var blackboard = knowledge.Blackboard;
            var blackboardValue = new CustomValueContainer(TypeUtils.GetObjectType(blackboard), blackboard, (instance, _) => ((BehaviorKnowledge)instance).Blackboard, (instance, _, value) => ((BehaviorKnowledge)instance).Blackboard = value);
            var blackboardEditor = CustomEditorsUtil.CreateEditor(blackboardValue.Type, false);
            layout.Object(blackboardValue, blackboardEditor);

            // Goals
            _goals = new();
            var goals = knowledge.Goals;
            foreach (var goal in goals)
            {
                if (goal == null)
                    continue;
                var goalType = goal.GetType();
                var goalValue = new CustomValueContainer(new ScriptType(goalType), goal, (instance, _) => ((BehaviorKnowledge)instance).GetGoal(goalType), (instance, _, value) => ((BehaviorKnowledge)instance).AddGoal(value));
                var goalEditor = CustomEditorsUtil.CreateEditor(goalValue.Type, false);
                var goalPanel = _layout.Group("Goal " + goalType.Name);
                goalPanel.Object(goalValue, goalEditor);
                _goals.Add(goalType);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            var knowledge = (BehaviorKnowledge)Values[0];
            if (!knowledge)
                return;

            // Rebuild layout when goals list gets changed
            var goals = knowledge.Goals;
            foreach (var goal in goals)
            {
                if (goal == null)
                    continue;
                var goalType = goal.GetType();
                if (!_goals.Contains(goalType))
                {
                    RebuildLayout();
                    return;
                }
            }
            foreach (var goalType in _goals.ToArray())
            {
                if (!knowledge.HasGoal(goalType))
                {
                    RebuildLayout();
                    return;
                }
            }

            base.Refresh();
        }
    }

    /// <summary>
    /// Behavior Tree window allows to view and edit <see cref="BehaviorTree"/> asset.
    /// </summary>
    /// <seealso cref="BehaviorTree" />
    /// <seealso cref="BehaviorTreeSurface" />
    public sealed class BehaviorTreeWindow : AssetEditorWindowBase<BehaviorTree>, IVisjectSurfaceWindow, IPresenterOwner
    {
        private readonly SplitPanel _split1;
        private readonly SplitPanel _split2;
        private CustomEditorPresenter _nodePropertiesEditor;
        private CustomEditorPresenter _knowledgePropertiesEditor;
        private BehaviorTreeSurface _surface;
        private Undo _undo;
        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private FlaxObjectRefPickerControl _behaviorPicker;
        private Guid _cachedBehaviorId;
        private bool _showWholeGraphOnLoad = true;
        private bool _isWaitingForSurfaceLoad;
        private bool _canEdit = true, _canDebug = false;

        /// <summary>
        /// Gets the Visject Surface.
        /// </summary>
        public BehaviorTreeSurface Surface => _surface;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public Undo Undo => _undo;

        /// <summary>
        /// Current instance of the Behavior Knowledge's blackboard type or null.
        /// </summary>
        public object Blackboard => _knowledgePropertiesEditor.Selection.Count != 0 ? _knowledgePropertiesEditor.Selection[0] : null;

        /// <summary>
        /// Gets instance of the root node of the graph. Returns null if not added (or graph not yet loaded).
        /// </summary>
        public BehaviorTreeRootNode RootNode => (_surface.FindNode(19, 2) as Surface.Archetypes.BehaviorTree.Node)?.Instance as BehaviorTreeRootNode;

        /// <inheritdoc />
        public BehaviorTreeWindow(Editor editor, BinaryAssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoAction;

            // Split Panels
            _split1 = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };
            _split2 = new SplitPanel(Orientation.Vertical, ScrollBars.Vertical, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                SplitterValue = 0.5f,
                Parent = _split1.Panel2
            };

            // Surface
            _surface = new BehaviorTreeSurface(this, Save, _undo)
            {
                Parent = _split1.Panel1,
                Enabled = false
            };
            _surface.SelectionChanged += OnNodeSelectionChanged;

            // Properties editors
            _nodePropertiesEditor = new CustomEditorPresenter(null, null, this); // Surface handles undo for nodes editing
            _nodePropertiesEditor.Features = FeatureFlags.UseDefault;
            _nodePropertiesEditor.Panel.Parent = _split2.Panel1;
            _nodePropertiesEditor.Modified += OnNodePropertyEdited;
            _knowledgePropertiesEditor = new CustomEditorPresenter(null, "No blackboard type assigned", this); // No undo for knowledge editing
            _knowledgePropertiesEditor.Features = FeatureFlags.None;
            _knowledgePropertiesEditor.Panel.Parent = _split2.Panel2;

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/scripting/ai/behavior-trees/index.html")).LinkTooltip("See documentation to learn more");

            // Debug behavior picker
            var behaviorPickerContainer = new ContainerControl();
            var behaviorPickerLabel = new Label
            {
                AnchorPreset = AnchorPresets.VerticalStretchLeft,
                VerticalAlignment = TextAlignment.Center,
                HorizontalAlignment = TextAlignment.Far,
                Parent = behaviorPickerContainer,
                Size = new Float2(60.0f, _toolstrip.Height),
                Text = "Behavior:",
                TooltipText = "The behavior instance to preview. Pick the behavior to debug it's logic and data.",
            };
            _behaviorPicker = new FlaxObjectRefPickerControl
            {
                Location = new Float2(behaviorPickerLabel.Right + 4.0f, 8.0f),
                Width = 140.0f,
                Type = new ScriptType(typeof(Behavior)),
                Parent = behaviorPickerContainer,
            };
            behaviorPickerContainer.Width = _behaviorPicker.Right + 2.0f;
            behaviorPickerContainer.Size = new Float2(_behaviorPicker.Right + 2.0f, _toolstrip.Height);
            behaviorPickerContainer.Parent = _toolstrip;
            _behaviorPicker.CheckValid = OnBehaviorPickerCheckValid;
            _behaviorPicker.ValueChanged += OnBehaviorPickerValueChanged;

            SetCanEdit(!Editor.IsPlayMode);
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
            _nodePropertiesEditor.BuildLayoutOnUpdate();
        }

        private void OnUndoAction(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        private void OnNodeSelectionChanged()
        {
            // Select node instances to view/edit
            var selection = new List<object>();
            var nodes = _surface.Nodes;
            if (nodes != null)
            {
                for (var i = 0; i < nodes.Count; i++)
                {
                    if (nodes[i] is Surface.Archetypes.BehaviorTree.NodeBase node && node.IsSelected && node.Instance)
                        selection.Add(node.Instance);
                }
            }
            _nodePropertiesEditor.Select(selection);
        }

        private void OnNodePropertyEdited()
        {
            _surface.MarkAsEdited();
            var nodes = _surface.Nodes;
            for (var i = 0; i < _nodePropertiesEditor.Selection.Count; i++)
            {
                if (_nodePropertiesEditor.Selection[i] is BehaviorTreeNode instance)
                {
                    // Sync instance data with surface node value storage
                    for (var j = 0; j < nodes.Count; j++)
                    {
                        if (nodes[j] is Surface.Archetypes.BehaviorTree.NodeBase node && node.Instance == instance)
                        {
                            node._isValueEditing = true;
                            node.SetValue(1, FlaxEngine.Json.JsonSerializer.SaveToBytes(instance));
                            node._isValueEditing = false;
                            break;
                        }
                    }
                }
            }
        }

        private bool OnBehaviorPickerCheckValid(FlaxEngine.Object obj, ScriptType type)
        {
            return obj is Behavior behavior && behavior.Tree == Asset;
        }

        private void OnBehaviorPickerValueChanged()
        {
            _cachedBehaviorId = _behaviorPicker.ValueID;
            UpdateKnowledge();
        }

        private void OnScriptsReloadBegin()
        {
            // TODO: impl hot-reload for BT to nicely refresh state (save asset, clear undo/properties, reload surface)
            Close();
        }

        private void UpdateKnowledge()
        {
            // Pick knowledge from the behavior
            var behavior = (Behavior)_behaviorPicker.Value;
            if (_canDebug && behavior)
            {
                _knowledgePropertiesEditor.ReadOnly = false;
                _knowledgePropertiesEditor.Select(behavior.Knowledge);
                return;
            }

            // Use blackboard from the root node
            var rootNode = _surface.FindNode(19, 2) as Surface.Archetypes.BehaviorTree.Node;
            if (rootNode != null)
            {
                rootNode.ValuesChanged -= UpdateKnowledge;
                rootNode.ValuesChanged += UpdateKnowledge;
            }
            
            var rootInstance = rootNode?.Instance as BehaviorTreeRootNode;
            var blackboardType = TypeUtils.GetType(rootInstance?.BlackboardType);
            if (blackboardType)
            {
                var blackboardInstance = blackboardType.CreateInstance();
                Utilities.Utils.InitDefaultValues(blackboardInstance);
                _knowledgePropertiesEditor.ReadOnly = true;
                _knowledgePropertiesEditor.Select(blackboardInstance);
            }
            else
            {
                _knowledgePropertiesEditor.Deselect();
            }
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited || _asset == null || _isWaitingForSurfaceLoad)
                return;

            // Check if surface has been edited
            if (_surface.IsEdited)
            {
                if (SaveSurface())
                    return;
            }

            ClearEditedFlag();
            OnSurfaceEditedChanged();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = _canEdit && IsEdited;
            _undoButton.Enabled = _canEdit && _undo.CanUndo;
            _redoButton.Enabled = _canEdit && _undo.CanRedo;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _isWaitingForSurfaceLoad = true;

            base.OnAssetLinked();
        }

        /// <summary>
        /// Focuses the node.
        /// </summary>
        /// <param name="node">The node.</param>
        public void ShowNode(SurfaceNode node)
        {
            SelectTab();
            RootWindow.Focus();
            Surface.Focus();
            Surface.FocusNode(node);
        }

        /// <inheritdoc />
        public Asset SurfaceAsset => Asset;

        /// <inheritdoc />
        public string SurfaceName => "Behavior Tree";

        /// <inheritdoc />
        public byte[] SurfaceData
        {
            get => _asset.LoadSurface();
            set
            {
                // Save data to the asset
                if (_asset.SaveSurface(value))
                {
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save surface data");
                }
                _asset.Reload();
            }
        }

        /// <inheritdoc />
        public VisjectSurfaceContext ParentContext => null;

        /// <inheritdoc />
        public void OnContextCreated(VisjectSurfaceContext context)
        {
        }

        /// <inheritdoc />
        public void OnSurfaceEditedChanged()
        {
            if (_surface.IsEdited)
                MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnSurfaceGraphEdited()
        {
        }

        /// <inheritdoc />
        public void OnSurfaceClose()
        {
            Close();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _isWaitingForSurfaceLoad = false;

            base.UnlinkItem();
        }

        private bool LoadSurface()
        {
            if (_surface.Load())
            {
                Editor.LogError("Failed to load Behavior Tree surface.");
                return true;
            }
            return false;
        }

        private bool SaveSurface()
        {
            return _surface.Save();
        }

        private void SetCanEdit(bool canEdit)
        {
            if (_canEdit == canEdit)
                return;
            _canEdit = canEdit;
            _undo.Enabled = canEdit;
            _surface.CanEdit = canEdit;
            _nodePropertiesEditor.ReadOnly = !canEdit;
            UpdateToolstrip();
        }

        private void SetCanDebug(bool canDebug)
        {
            if (_canDebug == canDebug)
                return;
            _canDebug = canDebug;
            UpdateKnowledge();
        }

        private void UpdateDebugInfos(bool playMode)
        {
            var behavior = playMode ? (Behavior)_behaviorPicker.Value : null;
            if (!behavior)
                behavior = null;
            foreach (var e in _surface.Nodes)
            {
                if (e is Surface.Archetypes.BehaviorTree.NodeBase node)
                    node.UpdateDebug(behavior);
            }
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            base.OnPlayBegin();

            SetCanEdit(false);
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            SetCanEdit(true);
            UpdateDebugInfos(false);

            base.OnPlayEnd();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Wait for asset loaded
            if (_isWaitingForSurfaceLoad && _asset.IsLoaded)
            {
                _isWaitingForSurfaceLoad = false;
                if (LoadSurface())
                {
                    Close();
                    return;
                }

                // Setup
                _undo.Clear();
                _surface.Enabled = true;
                _nodePropertiesEditor.BuildLayout();
                _knowledgePropertiesEditor.BuildLayout();
                ClearEditedFlag();
                if (_showWholeGraphOnLoad)
                {
                    _showWholeGraphOnLoad = false;
                    _surface.ShowWholeGraph();
                }
                SurfaceLoaded?.Invoke();
                UpdateKnowledge();
            }

            // Check if don't have valid behavior picked
            if (!_behaviorPicker.Value)
            {
                // Try to reassign the debug behavior
                var id = _cachedBehaviorId;
                if (id != Guid.Empty)
                {
                    var obj = FlaxEngine.Object.TryFind<Behavior>(ref id);
                    if (obj && obj.Tree == Asset)
                        _behaviorPicker.Value = obj;
                }
                else
                {
                    // Deselect (eg. if native C++ object has been deleted)
                    _behaviorPicker.Value = null;

                    // Remove any links to the behavior instance (eg. it could be deleted)
                    UpdateKnowledge();
                }

                // Preserve cache value
                _cachedBehaviorId = id;
            }

            // Update behavior debugging
            SetCanDebug(Editor.IsPlayMode && _behaviorPicker.Value);

            // Update debug info texts on all nodes
            if (Editor.IsPlayMode)
            {
                UpdateDebugInfos(true);
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split1", _split1);
            LayoutSerializeSplitter(writer, "Split2", _split1);
            writer.WriteAttributeString("SelectedBehavior", (_behaviorPicker.Value?.ID ?? _cachedBehaviorId).ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split1", _split1);
            LayoutDeserializeSplitter(node, "Split2", _split2);
            if (Guid.TryParse(node.GetAttribute("SelectedBehavior"), out Guid value1))
                _cachedBehaviorId = value1;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split1.SplitterValue = 0.7f;
            _split2.SplitterValue = 0.5f;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            _undo.Enabled = false;
            _nodePropertiesEditor.Deselect();
            _knowledgePropertiesEditor.Deselect();
            _undo.Clear();
            _behaviorPicker = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public IEnumerable<ScriptType> NewParameterTypes => Editor.CodeEditing.VisualScriptPropertyTypes.Get();

        /// <inheritdoc />
        public event Action SurfaceLoaded;

        /// <inheritdoc />
        public void OnParamRenameUndo()
        {
        }

        /// <inheritdoc />
        public void OnParamEditAttributesUndo()
        {
        }

        /// <inheritdoc />
        public void OnParamAddUndo()
        {
        }

        /// <inheritdoc />
        public void OnParamRemoveUndo()
        {
        }

        /// <inheritdoc />
        public object GetParameter(int index)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc />
        public void SetParameter(int index, object value)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc />
        public Asset VisjectAsset => Asset;

        /// <inheritdoc />
        public VisjectSurface VisjectSurface => _surface;

        /// <inheritdoc />
        public EditorViewport PresenterViewport => null;

        /// <inheritdoc />
        public void Select(List<SceneGraphNode> nodes)
        {
        }
    }
}
