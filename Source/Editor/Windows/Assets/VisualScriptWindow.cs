// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Threading;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;
using FlaxEngine.Windows.Search;

#pragma warning disable 649

// ReSharper disable UnusedMember.Local
// ReSharper disable UnusedMember.Global
// ReSharper disable MemberCanBePrivate.Local

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// VisualScript window allows to view and edit <see cref="VisualScript"/> asset.
    /// </summary>
    /// <seealso cref="VisualScript" />
    /// <seealso cref="VisualScriptSurface" />
    public sealed class VisualScriptWindow : AssetEditorWindowBase<VisualScript>, IVisjectSurfaceWindow, ISearchWindow
    {
        private struct BreakpointData
        {
            public uint NodeId;
            public bool Enabled;
        }

        private sealed class EditParamAccessAction : IUndoAction
        {
            public IVisjectSurfaceWindow Window;
            public int Index;
            public bool Before;
            public bool After;

            public string ActionString => "Edit parameter access";

            public void Do()
            {
                Set(After);
            }

            public void Undo()
            {
                Set(Before);
            }

            private void Set(bool value)
            {
                var param = Window.VisjectSurface.Parameters[Index];
                param.IsPublic = value;
                Window.VisjectSurface.OnParamEdited(param);
            }

            public void Dispose()
            {
                Window = null;
            }
        }

        private sealed class EditParamTypeAction : IUndoAction
        {
            private struct BrokenConnection
            {
                public uint ANode;
                public int ABox;
                public uint BNode;
                public int BBox;
            }

            private List<BrokenConnection> _connectionsToRestore = new List<BrokenConnection>();
            private List<BrokenConnection> _connectionsToRestorePrev = new List<BrokenConnection>();

            public IVisjectSurfaceWindow Window;
            public int Index;
            public ScriptType BeforeType;
            public object BeforeValue;
            public ScriptType AfterType;
            public object AfterValue;

            public string ActionString => "Edit parameter type";

            public void Do()
            {
                Set(AfterType, AfterValue);
            }

            public void Undo()
            {
                Set(BeforeType, BeforeValue);
            }

            private void Set(ScriptType type, object value)
            {
                _connectionsToRestore.Clear();
                Window.VisjectSurface.NodesDisconnected += OnNodesDisconnected;
                var param = Window.VisjectSurface.Parameters[Index];
                param.Type = type;
                param.Value = value;
                Window.VisjectSurface.OnParamEdited(param);
                Window.VisjectSurface.NodesDisconnected -= OnNodesDisconnected;

                // Restore connections broken previously
                foreach (var connection in _connectionsToRestorePrev)
                {
                    var aNode = Window.VisjectSurface.FindNode(connection.ANode);
                    var bNode = Window.VisjectSurface.FindNode(connection.BNode);
                    var aBox = aNode?.GetBox(connection.ABox) ?? null;
                    var bBox = bNode?.GetBox(connection.BBox) ?? null;
                    if (aBox != null && bBox != null)
                        aBox.CreateConnection(bBox);
                }
                _connectionsToRestorePrev.Clear();
                _connectionsToRestorePrev.AddRange(_connectionsToRestore);
            }

            private void OnNodesDisconnected(IConnectionInstigator a, IConnectionInstigator b)
            {
                // Capture any auto-disconnected boxes after parameter type change to restore them back on undo
                if (a is Surface.Elements.Box aBox && b is Surface.Elements.Box bBox)
                {
                    _connectionsToRestore.Add(new BrokenConnection
                    {
                        ANode = aBox.ParentNode.ID, ABox = aBox.ID,
                        BNode = bBox.ParentNode.ID, BBox = bBox.ID,
                    });
                }
            }

            public void Dispose()
            {
                _connectionsToRestore = null;
                _connectionsToRestorePrev = null;
                Window = null;
            }
        }

        private sealed class VisualParametersEditor : ParametersEditor
        {
            public VisualParametersEditor()
            {
                ShowOnlyPublic = false;
            }

            protected override void OnParamContextMenu(int index, ContextMenu menu)
            {
                var window = (VisualScriptWindow)Values[0];
                var param = window.Surface.Parameters[index];

                // Parameter access level editing
                var cmAccess = menu.AddChildMenu("Access");
                {
                    var b = cmAccess.ContextMenu.AddButton("Public", () => window.SetParamAccess(index, true));
                    b.Checked = param.IsPublic;
                    b.Enabled = window._canEdit;
                    b.TooltipText = "Sets the parameter access level to Public. It will be accessible and visible everywhere.";
                    b = cmAccess.ContextMenu.AddButton("Private", () => window.SetParamAccess(index, false));
                    b.Checked = !param.IsPublic;
                    b.Enabled = window._canEdit;
                    b.TooltipText = "Sets the parameter access level to Private. It will be accessible only within this script and won't be visible outside (eg. in script properties panel).";
                }

                // Parameter type editing
                var cmType = menu.AddChildMenu("Type");
                {
                    var isArray = param.Type.IsArray;
                    var isDictionary = !isArray && param.Type.IsDictionary;
                    ScriptType singleValueType, arrayType, dictionaryType;
                    ContextMenuButton b;
                    if (isDictionary)
                    {
                        var args = param.Type.GetGenericArguments();
                        singleValueType = new ScriptType(args[0]);
                        arrayType = singleValueType.MakeArrayType();
                        dictionaryType = param.Type;
                        var keyName = window.Surface.GetTypeName(new ScriptType(args[0]));
                        var valueName = window.Surface.GetTypeName(new ScriptType(args[1]));

                        b = cmType.ContextMenu.AddButton($"Dictionary<{keyName}, {valueName}>");
                        b.Enabled = false;

                        b = cmType.ContextMenu.AddButton($"Edit key type: {keyName}...", () => OnChangeType(item => window.SetParamType(index, ScriptType.MakeDictionaryType((ScriptType)item.Tag, new ScriptType(args[1])))));
                        b.Enabled = window._canEdit;
                        b.TooltipText = "Opens the type picker window to change the parameter type.";

                        b = cmType.ContextMenu.AddButton($"Edit value type: {valueName}...", () => OnChangeType(item => window.SetParamType(index, ScriptType.MakeDictionaryType(new ScriptType(args[0]), (ScriptType)item.Tag))));
                        b.Enabled = window._canEdit;
                        b.TooltipText = "Opens the type picker window to change the parameter type.";
                    }
                    else
                    {
                        if (param.Type == ScriptType.Null)
                        {
                            b = cmType.ContextMenu.AddButton(window.Surface.GetTypeName(param.Type) + "...", () => OnChangeType(item => window.SetParamType(index, (ScriptType)item.Tag)));
                            return;
                        }
                        else if (isArray)
                        {
                            singleValueType = param.Type.GetElementType();
                            arrayType = param.Type;
                            dictionaryType = ScriptType.MakeDictionaryType(new ScriptType(typeof(int)), singleValueType);
                            b = cmType.ContextMenu.AddButton(window.Surface.GetTypeName(singleValueType) + "[]...", () => OnChangeType(item => window.SetParamType(index, ((ScriptType)item.Tag).MakeArrayType())));
                        }
                        else
                        {
                            singleValueType = param.Type;
                            arrayType = param.Type.MakeArrayType();
                            dictionaryType = ScriptType.MakeDictionaryType(new ScriptType(typeof(int)), singleValueType);
                            b = cmType.ContextMenu.AddButton(window.Surface.GetTypeName(param.Type) + "...", () => OnChangeType(item => window.SetParamType(index, (ScriptType)item.Tag)));
                        }
                        b.Enabled = window._canEdit;
                        b.TooltipText = "Opens the type picker window to change the parameter type.";
                    }
                    cmType.ContextMenu.AddSeparator();

                    b = cmType.ContextMenu.AddButton("Value", () => window.SetParamType(index, singleValueType));
                    b.Checked = param.Type == singleValueType;
                    b.Enabled = window._canEdit;
                    b.TooltipText = "Changes parameter type to a single value.";
                    b = cmType.ContextMenu.AddButton("Array", () => window.SetParamType(index, arrayType));
                    b.Checked = param.Type == arrayType;
                    b.Enabled = window._canEdit;
                    b.TooltipText = "Changes parameter type to an array.";
                    b = cmType.ContextMenu.AddButton("Dictionary", () => window.SetParamType(index, dictionaryType));
                    b.Checked = param.Type == dictionaryType;
                    b.Enabled = window._canEdit;
                    b.TooltipText = "Changes parameter type to a dictionary.";
                }

                menu.AddSeparator();
                menu.AddButton("Find references...", () => OnFindReferences(index));
            }

            private void OnChangeType(Action<ItemsListContextMenu.Item> itemClicked)
            {
                // Show context menu with list of parameter types to use
                var cm = new ItemsListContextMenu(180);
                var window = (VisualScriptWindow)Values[0];
                var newParameterTypes = window.NewParameterTypes;
                foreach (var newParameterType in newParameterTypes)
                {
                    var item = new TypeSearchPopup.TypeItemView(newParameterType);
                    if (newParameterType.Type != null)
                        item.Name = window.VisjectSurface.GetTypeName(newParameterType);
                    cm.AddItem(item);
                }
                cm.ItemClicked += itemClicked;
                cm.SortItems();
                cm.Show(window, window.PointFromScreen(Input.MouseScreenPosition));
            }

            private void OnFindReferences(int index)
            {
                var window = (VisualScriptWindow)Values[0];
                var param = window.Surface.Parameters[index];
                Editor.Instance.ContentFinding.ShowSearch(window, '\"' + window.Asset.ScriptTypeName + '.' + param.Name + '\"');
            }
        }

        private sealed class PropertiesProxy
        {
            [EditorOrder(0), EditorDisplay("Options"), CustomEditor(typeof(OptionsEditor)), NoSerialize]
            public VisualScriptWindow Window2;

            [EditorOrder(10), EditorDisplay("Options"), TypeReference(typeof(FlaxEngine.Object), nameof(IsValid)), NoSerialize]
            [Tooltip("The base class of the new Visual Script to inherit from.")]
            public Type BaseClass;

            [EditorOrder(20), EditorDisplay("Options")]
            [Tooltip("If checked, the class is abstract and cannot be instantiated directly.")]
            public bool IsAbstract;

            [EditorOrder(30), EditorDisplay("Options")]
            [Tooltip("If checked, the class is sealed and cannot be inherited by other scripts.")]
            public bool IsSealed;

            [EditorOrder(900), CustomEditor(typeof(FunctionsEditor)), NoSerialize]
            public VisualScriptWindow Window1;

            [EditorOrder(1000), EditorDisplay("Parameters"), CustomEditor(typeof(VisualParametersEditor)), NoSerialize]
            public VisualScriptWindow Window;

            [HideInEditor]
            public object[] Parameters
            {
                get
                {
                    var parameters = Window.Surface.Parameters;
                    var result = new object[parameters.Count];
                    for (int i = 0; i < result.Length; i++)
                        result[i] = parameters[i].Value;
                    return result;
                }
                set
                {
                    var parameters = Window.Surface.Parameters;
                    if (value != null && parameters.Count == value.Length)
                    {
                        for (int i = 0; i < parameters.Count; i++)
                            parameters[i].Value = value[i];
                    }
                }
            }

            private static bool IsValid(Type type)
            {
                return (type.IsPublic || type.IsNestedPublic) && !type.IsSealed && !type.IsGenericType;
            }

            public void OnLoad(VisualScriptWindow window)
            {
                var asset = window.Asset;
                var meta = asset.Meta;
                BaseClass = TypeUtils.GetType(meta.BaseTypename).Type;
                IsAbstract = (meta.Flags & VisualScript.Flags.Abstract) != 0;
                IsSealed = (meta.Flags & VisualScript.Flags.Sealed) != 0;
                Window = Window1 = Window2 = window;
            }

            public void OnClean()
            {
                Window = Window1 = Window2 = null;
            }
        }

        class OptionsEditor : CustomEditor
        {
            /// <inheritdoc />
            public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                var group = (CustomEditors.Elements.GroupElement)layout;

                // Options button
                const float buttonSize = 14;
                var optionsButton = new Image
                {
                    TooltipText = "Script options",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = group.Panel,
                    Bounds = new Rectangle(group.Panel.Width - buttonSize, 0, buttonSize, buttonSize),
                    IsScrollable = false,
                    Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(Editor.Instance.Icons.Settings12),
                };
                optionsButton.Clicked += OnOptionsButtonClicked;
            }

            private void OnOptionsButtonClicked(Image image, MouseButton button)
            {
                if (button != MouseButton.Left)
                    return;

                var cm = new ContextMenu();
                cm.AddButton("Copy typename", () => Clipboard.Text = ((VisualScriptWindow)Values[0]).Asset.ScriptTypeName);
                cm.AddButton("Edit attributes...", OnEditAttributes);
                cm.Show(image, new Float2(0, image.Height));
            }

            private void OnEditAttributes(ContextMenuButton button)
            {
                var window = (VisualScriptWindow)Values[0];
                var attributes = window.Surface.Context.Meta.GetAttributes();
                var editor = new AttributesEditor(attributes, NodeFactory.TypeAttributeTypes);
                editor.Edited += newValue =>
                {
                    var action = new EditSurfaceAttributesAction
                    {
                        Window = (IVisjectSurfaceWindow)window.Surface.Owner,
                        Before = window.Surface.Context.Meta.GetAttributes(),
                        After = newValue,
                    };
                    action.Window.Undo?.AddAction(action);
                    action.Do();
                };
                editor.Show(window, window.Surface.Parent.UpperRight);
            }
        }

        class FunctionsEditor : CustomEditor
        {
            /// <inheritdoc />
            public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

            private Control _overrideButton;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                var window = Values[0] as IVisjectSurfaceWindow;
                var asset = window?.VisjectAsset;
                if (asset == null || !asset.IsLoaded)
                    return;

                var group = layout.Group("Functions");
                var nodes = window.VisjectSurface.Nodes;

                var grid = group.CustomContainer<UniformGridPanel>();
                var gridControl = grid.CustomControl;
                gridControl.ClipChildren = false;
                gridControl.Height = Button.DefaultHeight;
                gridControl.SlotsHorizontally = 2;
                gridControl.SlotsVertically = 1;

                var addOverride = grid.Button("Add Override");
                addOverride.Button.Clicked += OnOverrideMethodClicked;
                // TODO: Add sender arg to button clicked action?
                _overrideButton = addOverride.Control;

                var addFuncction = grid.Button("Add Function");
                addFuncction.Button.Clicked += OnAddNewFunctionClicked;

                // List of functions in the graph
                for (int i = 0; i < nodes.Count; i++)
                {
                    var node = nodes[i];
                    if (node is Surface.Archetypes.Function.VisualScriptFunctionNode functionNode && !string.IsNullOrEmpty(functionNode._signature.Name))
                    {
                        var label = group.ClickableLabel(functionNode._signature.Name ?? string.Empty).CustomControl;
                        label.TextColorHighlighted = Color.FromBgra(0xFFA0A0A0);
                        label.TooltipText = functionNode.TooltipText;
                        label.DoubleClick += () => ((VisualScriptWindow)Values[0]).Surface.FocusNode(functionNode);
                        label.RightClick += () => ShowContextMenu(functionNode, label);
                    }
                    else if (node is Surface.Archetypes.Function.MethodOverrideNode overrideNode)
                    {
                        var label = group.ClickableLabel($"{overrideNode.Title} (override)").CustomControl;
                        label.TextColorHighlighted = Color.FromBgra(0xFFA0A0A0);
                        label.TooltipText = overrideNode.TooltipText;
                        label.DoubleClick += () => ((VisualScriptWindow)Values[0]).Surface.FocusNode(overrideNode);
                        label.RightClick += () => ShowContextMenu(overrideNode, label);
                    }
                }
            }

            private void ShowContextMenu(SurfaceNode node, ClickableLabel label)
            {
                var cm = new ContextMenu();
                cm.AddButton("Show", () => ((VisualScriptWindow)Values[0]).Surface.FocusNode(node)).Icon = Editor.Instance.Icons.Search12;
                cm.AddButton("Delete", () => ((VisualScriptWindow)Values[0]).Surface.Delete(node)).Icon = Editor.Instance.Icons.Cross12;
                node.OnShowSecondaryContextMenu(cm, Float2.Zero);
                cm.Show(label, new Float2(0, label.Height));
            }

            private void OnAddNewFunctionClicked()
            {
                var surface = ((VisualScriptWindow)Values[0]).Surface;
                var surfaceBounds = surface.AllNodesBounds;
                surface.ShowArea(new Rectangle(surfaceBounds.BottomLeft, new Float2(200, 150)).MakeExpanded(400.0f));
                var node = surface.Context.SpawnNode(16, 6, surfaceBounds.BottomLeft + new Float2(0, 50));
                surface.Select(node);
            }

            private void OnOverrideMethodClicked()
            {
                var cm = new ContextMenu();
                var window = (VisualScriptWindow)Values[0];
                var scriptMeta = window.Asset.Meta;
                var baseType = TypeUtils.GetType(scriptMeta.BaseTypename);
                var members = baseType.GetMembers(BindingFlags.Public | BindingFlags.Instance);
                for (var i = 0; i < members.Length; i++)
                {
                    ref var member = ref members[i];
                    if (SurfaceUtils.IsValidVisualScriptOverrideMethod(member, out var parameters))
                    {
                        var name = member.Name;

                        // Skip already added override method nodes
                        bool isAlreadyAdded = false;
                        foreach (var n in window.Surface.Nodes)
                        {
                            if (n.GroupArchetype.GroupID == 16 &&
                                n.Archetype.TypeID == 3 &&
                                (string)n.Values[0] == name &&
                                (int)n.Values[1] == parameters.Length)
                            {
                                isAlreadyAdded = true;
                                break;
                            }
                        }
                        if (isAlreadyAdded)
                            continue;

                        var cmButton = cm.AddButton($"{name} (in {member.DeclaringType.Name})");
                        cmButton.TooltipText = Editor.Instance.CodeDocs.GetTooltip(member);
                        cmButton.Clicked += () =>
                        {
                            var surface = ((VisualScriptWindow)Values[0]).Surface;
                            var surfaceBounds = surface.AllNodesBounds;
                            surface.ShowArea(new Rectangle(surfaceBounds.BottomLeft, new Float2(200, 150)).MakeExpanded(400.0f));
                            var node = surface.Context.SpawnNode(16, 3, surfaceBounds.BottomLeft + new Float2(0, 50), new object[]
                            {
                                name,
                                parameters.Length,
                                Utils.GetEmptyArray<byte>()
                            });
                            surface.Select(node);
                        };
                    }
                }
                if (!cm.Items.Any())
                {
                    cm.AddButton("Nothing to override");
                }
                cm.Show(_overrideButton, new Float2(0, _overrideButton.Height));
            }
        }

        private readonly PropertiesProxy _properties;
        private readonly SplitPanel _split;
        private CustomEditorPresenter _propertiesEditor;
        private VisualScriptSurface _surface;
        private FlaxObjectRefPickerControl _debugObjectPicker;
        private Undo _undo;
        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private Control[] _debugToolstripControls;
        private bool _showWholeGraphOnLoad = true;
        private bool _tmpAssetIsDirty;
        private bool _isWaitingForSurfaceLoad;
        private bool _refreshPropertiesOnLoad;
        private bool _paramValueChange;
        private bool _canEdit = true;
        private List<uint> _debugStepOverNodesIds;
        private List<KeyValuePair<VisualScript, uint>> _debugStepOutNodesIds;
        private bool _debugBreakNext;
        private ulong _debugBreakNextThreadId;
        private readonly List<Editor.VisualScriptingDebugFlowInfo> _debugFlows = new List<Editor.VisualScriptingDebugFlowInfo>();

        /// <summary>
        /// Gets the Visject Surface.
        /// </summary>
        public VisualScriptSurface Surface => _surface;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public Undo Undo => _undo;

        /// <inheritdoc />
        public VisualScriptWindow(Editor editor, VisualScriptItem item)
        : base(editor, item)
        {
            var isPlayMode = Editor.IsPlayMode;
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };

            // Surface
            _surface = new VisualScriptSurface(this, Save, _undo)
            {
                Parent = _split.Panel1,
                Enabled = false
            };
            _surface.NodeSpawned += OnNodeValuesChange;
            _surface.NodeValuesEdited += OnNodeValuesChange;
            _surface.NodeDeleted += OnNodeValuesChange;

            // Properties editor
            _propertiesEditor = new CustomEditorPresenter(_undo);
            _propertiesEditor.Features = FeatureFlags.None;
            _propertiesEditor.Panel.Parent = _split.Panel2;
            _propertiesEditor.Modified += OnPropertyEdited;
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);

            // Toolstrip
            SurfaceUtils.PerformCommonSetup(this, _toolstrip, _surface, out _saveButton, out _undoButton, out _redoButton);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/scripting/visual/index.html")).LinkTooltip("See documentation to learn more");
            _debugToolstripControls = new[]
            {
                _toolstrip.AddSeparator(),
                _toolstrip.AddButton(editor.Icons.Play64, OnDebuggerContinue).LinkTooltip("Continue", ref inputOptions.DebuggerContinue),
                _toolstrip.AddButton(editor.Icons.Search64, OnDebuggerNavigateToCurrentNode).LinkTooltip("Navigate to the current stack trace node"),
                _toolstrip.AddButton(editor.Icons.Right64, OnDebuggerStepOver).LinkTooltip("Step Over", ref inputOptions.DebuggerStepOver),
                _toolstrip.AddButton(editor.Icons.Down64, OnDebuggerStepInto).LinkTooltip("Step Into", ref inputOptions.DebuggerStepInto),
                _toolstrip.AddButton(editor.Icons.Up64, OnDebuggerStepOut).LinkTooltip("Step Out", ref inputOptions.DebuggerStepOut),
                _toolstrip.AddButton(editor.Icons.Stop64, OnDebuggerStop).LinkTooltip("Stop debugging"),
            };
            foreach (var control in _debugToolstripControls)
                control.Visible = false;

            // Debug object picker
            var debugObjectPickerContainer = new ContainerControl();
            var debugObjectPickerLabel = new Label
            {
                AnchorPreset = AnchorPresets.VerticalStretchLeft,
                VerticalAlignment = TextAlignment.Center,
                HorizontalAlignment = TextAlignment.Far,
                Parent = debugObjectPickerContainer,
                Size = new Float2(60.0f, _toolstrip.Height),
                Text = "Debug:",
                TooltipText = "The Visual Script object instance to debug. Can be used to filter the logic to debug a specific instance.",
            };
            _debugObjectPicker = new FlaxObjectRefPickerControl
            {
                Location = new Float2(debugObjectPickerLabel.Right + 4.0f, 8.0f),
                Width = 140.0f,
                Type = item.ScriptType,
                Parent = debugObjectPickerContainer,
            };
            debugObjectPickerContainer.Enabled = debugObjectPickerContainer.Visible = isPlayMode;
            debugObjectPickerContainer.Size = new Float2(_debugObjectPicker.Right + 2.0f, _toolstrip.Height);
            debugObjectPickerContainer.Parent = _toolstrip;

            // Setup input actions
            InputActions.Add(options => options.DebuggerContinue, OnDebuggerContinue);
            InputActions.Add(options => options.DebuggerStepOver, OnDebuggerStepOver);
            InputActions.Add(options => options.DebuggerStepOut, OnDebuggerStepOut);
            InputActions.Add(options => options.DebuggerStepInto, OnDebuggerStepInto);

            SetCanEdit(!isPlayMode);
        }

        private void OnNodeValuesChange(SurfaceNode node)
        {
            if (node is Surface.Archetypes.Function.VisualScriptFunctionNode || node is Surface.Archetypes.Function.MethodOverrideNode)
            {
                _propertiesEditor.BuildLayoutOnUpdate();
            }
        }

        private void OnUndoRedo(IUndoAction action)
        {
            _paramValueChange = false;
            MarkAsEdited();
            UpdateToolstrip();
            _propertiesEditor.BuildLayoutOnUpdate();
        }

        /// <summary>
        /// Called when the asset properties proxy object gets edited.
        /// </summary>
        private void OnPropertyEdited()
        {
            _surface.MarkAsEdited(!_paramValueChange);
            _paramValueChange = false;
        }

        /// <summary>
        /// Shows the whole surface graph.
        /// </summary>
        public void ShowWholeGraph()
        {
            _surface.ShowWholeGraph();
        }

        /// <summary>
        /// Refreshes temporary asset to see changes live when editing the surface.
        /// </summary>
        /// <returns>True if cannot refresh it, otherwise false.</returns>
        public bool RefreshTempAsset()
        {
            return false;
        }

        /// <inheritdoc />
        public override void Save()
        {
            // Early check
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
            _surface.Script = Asset;
            _isWaitingForSurfaceLoad = true;
            _refreshPropertiesOnLoad = false;

            base.OnAssetLinked();
        }

        private void OnDebugFlow(Editor.VisualScriptingDebugFlowInfo flowInfo)
        {
            // Skip any debug flows during hang
            if (Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Check if moving into next flow
            if (_debugBreakNext && _debugBreakNextThreadId == Platform.CurrentThreadID)
            {
                _debugBreakNext = false;
                var item = Editor.ContentDatabase.Find(flowInfo.Script.ID);
                if (item != null)
                {
                    var editor = Editor.ContentEditing.Open(item);
                    if (editor is VisualScriptWindow vsWindow)
                    {
                        lock (vsWindow._debugFlows)
                            vsWindow._debugFlows.Add(flowInfo);
                        vsWindow.OnDebugBreakpointHit(ref flowInfo, vsWindow.Surface.FindNode(flowInfo.NodeId));
                    }
                }
                return;
            }

            // Check if moving outside the current scope
            if (_debugStepOutNodesIds != null && _debugStepOutNodesIds.Contains(new KeyValuePair<VisualScript, uint>(flowInfo.Script, flowInfo.NodeId)))
            {
                _debugStepOutNodesIds.Clear();
                var item = Editor.ContentDatabase.Find(flowInfo.Script.ID);
                if (item != null)
                {
                    var editor = Editor.ContentEditing.Open(item);
                    if (editor is VisualScriptWindow vsWindow)
                    {
                        lock (vsWindow._debugFlows)
                            vsWindow._debugFlows.Add(flowInfo);
                        vsWindow.OnDebugBreakpointHit(ref flowInfo, vsWindow.Surface.FindNode(flowInfo.NodeId));
                    }
                }
                return;
            }

            // Filter the flow
            if (_debugObjectPicker.Value != null && flowInfo.ScriptInstance != null && _debugObjectPicker.Value != flowInfo.ScriptInstance)
                return;
            if (flowInfo.Script != Asset)
                return;

            // Register flow to show it in UI on a surface
            lock (_debugFlows)
            {
                _debugFlows.Add(flowInfo);
            }

            // Check if moving over the flow with debugger
            if (_debugStepOverNodesIds != null && _debugStepOverNodesIds.Contains(flowInfo.NodeId))
            {
                OnDebugBreakpointHit(ref flowInfo, Surface.FindNode(flowInfo.NodeId));
                return;
            }

            // Check if any breakpoint was hit
            var breakpoints = Surface.Breakpoints;
            for (int i = 0; i < breakpoints.Count; i++)
            {
                if (breakpoints[i].ID == flowInfo.NodeId)
                {
                    OnDebugBreakpointHit(ref flowInfo, breakpoints[i]);
                    break;
                }
            }
        }

        internal struct BreakpointHangState
        {
            public SurfaceNode Node;
            public VisualScriptWindow Window;
            public Editor.VisualScriptLocal[] Locals;
            public Editor.VisualScriptStackFrame[] StackFrames;
        }

        internal static BreakpointHangState GetLocals()
        {
            var state = (BreakpointHangState)Editor.Instance.Simulation.BreakpointHangTag;
            if (state.Locals == null)
            {
                state.Locals = Editor.GetVisualScriptLocals();
                Editor.Instance.Simulation.BreakpointHangTag = state;
            }
            return state;
        }

        internal static BreakpointHangState GetStackFrames()
        {
            var state = (BreakpointHangState)Editor.Instance.Simulation.BreakpointHangTag;
            if (state.StackFrames == null)
            {
                state.StackFrames = Editor.GetVisualScriptStackFrames();
                Editor.Instance.Simulation.BreakpointHangTag = state;
            }
            return state;
        }

        private void OnDebugBreakpointHit(ref Editor.VisualScriptingDebugFlowInfo flowInfo, SurfaceNode node)
        {
            if (!Platform.IsInMainThread || GPUDevice.Instance.IsRendering)
                return;
            _debugStepOverNodesIds?.Clear();
            _debugStepOutNodesIds?.Clear();
            _debugBreakNext = false;

            Editor.Log(string.Format("Visual Script breakpoint hit at node '{0}' in script '{1}'{2} (thread: 0x{3:x})", node.Title, Item.ShortName, flowInfo.ScriptInstance ? $" (instance {flowInfo.ScriptInstance})" : "", Platform.CurrentThreadID));

            try
            {
                Editor.Simulation.BreakpointHangTag = new BreakpointHangState
                {
                    Node = node,
                    Window = this,
                };
                Editor.Simulation.OnBreakpointHangBegin();

                // Disable all UI that is not supported during breakpoint hang
                _debugObjectPicker.Enabled = false;
                foreach (var window in Editor.Windows.Windows)
                {
                    if (!(window is VisualScriptWindow) && !(window is VisualScriptDebuggerWindow))
                        window.Enabled = false;
                }
                foreach (var control in _debugToolstripControls)
                    control.Visible = true;
                Editor.UI.UpdateToolstrip();
                Editor.UI.UpdateMainMenu();
                UpdateToolstrip();

                // Focus stack node
                node.Breakpoint.Hit = true;
                OnDebuggerNavigateToCurrentNode();

                // Handle breakpoint by hanging the main thread and manually processing input events with rendering of the current Visual Script window
                var lastTick = DateTime.Now;
                var targetTickTime = TimeSpan.FromSeconds(1.0 / 60.0);
                var tickMargin = TimeSpan.FromMilliseconds(1);
                var deltaTime = 1.0f / 60.0f;
                while (Editor.Simulation.ContinueBreakpointHang())
                {
                    // Handle input and drawing (UI only)
                    Editor.Internal_RunVisualScriptBreakpointLoopTick(deltaTime);

                    // Keep stable update rate
                    var now = DateTime.Now;
                    var tickTime = now - lastTick;
                    deltaTime = (float)tickTime.TotalMilliseconds;
                    var tickTimeLeft = targetTickTime - tickTime - tickMargin;
                    if (tickTimeLeft > TimeSpan.Zero)
                        Thread.Sleep(tickTimeLeft);
                    lastTick = now;
                }
            }
            catch (Exception ex)
            {
                Editor.LogError("Error while Visual Script breakpoint handling.");
                Editor.LogWarning(ex);
            }
            finally
            {
                node.Breakpoint.Hit = false;
                Editor.Simulation.OnBreakpointHangEnd();

                // Restore all UI
                _debugObjectPicker.Enabled = true;
                foreach (var window in Editor.Windows.Windows)
                    window.Enabled = true;
                foreach (var control in _debugToolstripControls)
                    control.Visible = false;
                Editor.UI.UpdateToolstrip();
                Editor.UI.UpdateMainMenu();
                UpdateToolstrip();
            }
        }

        private void OnDebuggerContinue()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Stop hang
            Editor.Simulation.StopBreakpointHang();
        }

        private void OnDebuggerNavigateToCurrentNode()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Focus node
            var state = (BreakpointHangState)Editor.Simulation.BreakpointHangTag;
            ShowNode(state.Node);
        }

        private void OnDebuggerStepOver()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Break on any of the output connections from the current node
            var state = (BreakpointHangState)Editor.Simulation.BreakpointHangTag;
            if (_debugStepOverNodesIds == null)
                _debugStepOverNodesIds = new List<uint>();
            else
                _debugStepOverNodesIds.Clear();
            foreach (var element in state.Node.Elements)
            {
                if (element is Surface.Elements.OutputBox outputBox)
                {
                    foreach (var box in outputBox.Connections)
                        _debugStepOverNodesIds.Add(box.ParentNode.ID);
                }
            }
            if (_debugStepOverNodesIds.Count == 0)
                OnDebuggerStepOut();
            else
                Editor.Simulation.StopBreakpointHang();
        }

        private void OnDebuggerStepInto()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Break on any next flow on this thread
            _debugBreakNext = true;
            _debugBreakNextThreadId = Platform.CurrentThreadID;
            Editor.Simulation.StopBreakpointHang();
        }

        private void OnDebuggerStepOut()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Break on any of the output connects from the previous scope node
            var frame = Editor.GetVisualScriptPreviousScopeFrame();
            if (frame.Script != null)
            {
                if (_debugStepOutNodesIds == null)
                    _debugStepOutNodesIds = new List<KeyValuePair<VisualScript, uint>>();
                else
                    _debugStepOutNodesIds.Clear();

                VisualScriptWindow vsWindow = null;
                foreach (var window in Editor.Instance.Windows.Windows)
                {
                    if (window is VisualScriptWindow w && w.Asset == frame.Script)
                    {
                        vsWindow = (VisualScriptWindow)window;
                        break;
                    }
                }
                if (vsWindow == null)
                    vsWindow = Editor.Instance.ContentEditing.Open(frame.Script) as VisualScriptWindow;
                var node = vsWindow?.Surface.FindNode(frame.NodeId);
                _debugStepOutNodesIds.Add(new KeyValuePair<VisualScript, uint>(frame.Script, frame.NodeId));
                if (node != null)
                {
                    foreach (var element in node.Elements)
                    {
                        if (element is Surface.Elements.OutputBox outputBox)
                        {
                            foreach (var box in outputBox.Connections)
                                _debugStepOutNodesIds.Add(new KeyValuePair<VisualScript, uint>(frame.Script, box.ParentNode.ID));
                        }
                    }
                }
            }
            Editor.Simulation.StopBreakpointHang();
        }

        private void OnDebuggerStop()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Stop play
            Editor.Simulation.RequestStopPlay();
        }

        private void LoadBreakpoints()
        {
            try
            {
                // Try to restore the cached breakpoints from the last session
                if (Editor.ProjectCache.TryGetCustomData(_asset.ScriptTypeName + ".Breakpoints", out string breakpointsData))
                {
                    var data = JsonSerializer.Deserialize<BreakpointData[]>(breakpointsData);
                    if (data != null)
                    {
                        for (int i = 0; i < data.Length; i++)
                        {
                            ref var e = ref data[i];
                            var node = Surface.FindNode(e.NodeId);
                            if (node != null)
                            {
                                node.Breakpoint.Set = true;
                                node.Breakpoint.Enabled = e.Enabled;
                                Surface.OnNodeBreakpointEdited(node);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to load breakpoints data.");
                Editor.LogWarning(ex);
            }
        }

        private void SaveBreakpoints()
        {
            try
            {
                var breakpointsCount = 0;
                var nodes = Surface.Nodes;
                for (int i = 0; i < nodes.Count; i++)
                {
                    if (nodes[i].Breakpoint.Set)
                        breakpointsCount++;
                }
                if (breakpointsCount == 0)
                {
                    // No breakpoints in use
                    Editor.ProjectCache.RemoveCustomData(_asset.ScriptTypeName + ".Breakpoints");
                    return;
                }

                var data = new BreakpointData[breakpointsCount];
                for (int i = 0, j = 0; i < nodes.Count; i++)
                {
                    var node = nodes[i];
                    if (node.Breakpoint.Set)
                    {
                        data[j++] = new BreakpointData
                        {
                            NodeId = node.ID,
                            Enabled = node.Breakpoint.Enabled,
                        };
                    }
                }

                // Set the cached breakpoints
                Editor.ProjectCache.SetCustomData(_asset.ScriptTypeName + ".Breakpoints", JsonSerializer.Serialize(data));
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to save breakpoints data.");
                Editor.LogWarning(ex);
            }
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
        public string SurfaceName => "Visual Script";

        /// <inheritdoc />
        public byte[] SurfaceData
        {
            get => _asset.LoadSurface();
            set
            {
                // Create metadata
                var meta = new VisualScript.Metadata
                {
                    BaseTypename = _properties.BaseClass?.FullName,
                    Flags = VisualScript.Flags.None,
                };
                if (_properties.IsAbstract)
                    meta.Flags |= VisualScript.Flags.Abstract;
                if (_properties.IsSealed)
                    meta.Flags |= VisualScript.Flags.Sealed;

                // Save data to the asset
                if (_asset.SaveSurface(value, ref meta))
                {
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save surface data");
                }
                _asset.Reload();
                SaveBreakpoints();

                // Asset got edited so the methods/properties types info could be changed
                Editor.CodeEditing.ClearTypes();
                Editor.CodeEditing.OnTypesChanged();
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
            // Mark as dirty
            _tmpAssetIsDirty = true;
        }

        /// <inheritdoc />
        public void OnSurfaceClose()
        {
            Close();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            Editor.VisualScriptingDebugFlow -= OnDebugFlow;
            SaveBreakpoints();
            _surface.Script = null;
            _isWaitingForSurfaceLoad = false;
            _properties.OnClean();

            base.UnlinkItem();
        }

        private bool LoadSurface()
        {
            // Init properties and parameters proxy
            _properties.OnLoad(this);

            // Load surface graph
            if (_surface.Load())
            {
                Editor.LogError("Failed to load Visual Script surface.");
                return true;
            }

            return false;
        }

        private bool SaveSurface()
        {
            if (_surface.Save())
                return true;

            // Reselect actors to prevent issues after Visual Script properties were modified
            Editor.Windows.PropertiesWin.Presenter.BuildLayoutOnUpdate();

            return false;
        }

        private void SetCanEdit(bool canEdit)
        {
            if (_canEdit == canEdit)
                return;
            _canEdit = canEdit;
            _undo.Enabled = canEdit;
            _surface.CanEdit = canEdit;
            _propertiesEditor.ReadOnly = !canEdit;
            UpdateToolstrip();
        }

        private void SetParamAccess(int index, bool isPublic)
        {
            var param = Surface.Parameters[index];
            if (param.IsPublic == isPublic)
                return;
            var action = new EditParamAccessAction
            {
                Window = this,
                Index = index,
                Before = param.IsPublic,
                After = isPublic,
            };
            _undo.AddAction(action);
            action.Do();
        }

        private void SetParamType(int index, ScriptType type)
        {
            var param = Surface.Parameters[index];
            if (param.Type == type)
                return;
            var action = new EditParamTypeAction
            {
                Window = this,
                Index = index,
                BeforeType = param.Type,
                BeforeValue = param.Value,
                AfterType = type,
                AfterValue = TypeUtils.GetDefaultValue(type),
            };
            if (action.BeforeValue != null)
            {
                // Try to maintain existing value
                var beforeType = TypeUtils.GetObjectType(action.BeforeValue);
                if (type.IsArray && beforeType.IsArray && type.GetElementType().IsAssignableFrom(beforeType.GetElementType()))
                {
                    var beforeArray = (Array)action.BeforeValue;
                    var afterArray = TypeUtils.CreateArrayInstance(type.GetElementType(), beforeArray.Length);
                    Array.Copy(beforeArray, afterArray, beforeArray.Length);
                    action.AfterValue = afterArray;
                }
                else if (type.IsAssignableFrom(beforeType))
                {
                    action.AfterValue = action.BeforeValue;
                }
            }
            _undo.AddAction(action);
            action.Do();
        }

        /// <inheritdoc />
        public override void OnPlayBeginning()
        {
            base.OnPlayBeginning();

            if (IsEdited && Editor.Options.Options.General.AutoSaveVisualScriptOnPlayStart)
            {
                Save();
            }
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            base.OnPlayBegin();

            SetCanEdit(false);
            _debugObjectPicker.Parent.Visible = _debugObjectPicker.Parent.Enabled = true;
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            _debugObjectPicker.Parent.Visible = _debugObjectPicker.Parent.Enabled = false;
            _debugObjectPicker.Value = null;
            SetCanEdit(true);

            base.OnPlayEnd();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_tmpAssetIsDirty)
            {
                _tmpAssetIsDirty = false;

                RefreshTempAsset();
            }

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
                _propertiesEditor.BuildLayout();
                ClearEditedFlag();
                if (_showWholeGraphOnLoad)
                {
                    _showWholeGraphOnLoad = false;
                    _surface.ShowWholeGraph();
                }
                LoadBreakpoints();
                SurfaceLoaded?.Invoke();
                Editor.VisualScriptingDebugFlow += OnDebugFlow;
            }
            else if (_refreshPropertiesOnLoad && _asset.IsLoaded)
            {
                _refreshPropertiesOnLoad = false;

                _propertiesEditor.BuildLayout();
            }

            // Update script execution flow debugging visualization
            lock (_debugFlows)
            {
                foreach (var debugFlow in _debugFlows)
                {
                    var node = Surface.Context.FindNode(debugFlow.NodeId);
                    var box = node?.GetBox(debugFlow.BoxId);
                    box?.HighlightConnections();
                }
                _debugFlows.Clear();
            }
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

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _undo.Enabled = false;
            _propertiesEditor.Deselect();
            _undo.Clear();
            _debugObjectPicker = null;
            _debugToolstripControls = null;

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
            _propertiesEditor.BuildLayout();
        }

        /// <inheritdoc />
        public void OnParamAddUndo()
        {
            _refreshPropertiesOnLoad = true;
        }

        /// <inheritdoc />
        public void OnParamRemoveUndo()
        {
            _refreshPropertiesOnLoad = true;
            _propertiesEditor.BuildLayout();
        }

        /// <inheritdoc />
        public object GetParameter(int index)
        {
            var param = Surface.Parameters[index];
            return param.Value;
        }

        /// <inheritdoc />
        public void SetParameter(int index, object value)
        {
            var param = Surface.Parameters[index];
            param.Value = value;
            _paramValueChange = true;
        }

        /// <inheritdoc />
        public Asset VisjectAsset => Asset;

        /// <inheritdoc />
        public VisjectSurface VisjectSurface => _surface;

        /// <inheritdoc />
        public SearchAssetTypes AssetType => SearchAssetTypes.VisualScript;
    }
}
