// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    public sealed class VisualScriptWindow : AssetEditorWindowBase<VisualScript>, IVisjectSurfaceWindow
    {
        private struct BreakpointData
        {
            public uint NodeId;
            public bool Enabled;
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

            [EditorOrder(1000), EditorDisplay("Parameters"), CustomEditor(typeof(ParametersEditor)), NoSerialize]
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
                return type.IsPublic && !type.IsSealed && !type.IsGenericType;
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
                cm.Show(image, new Vector2(0, image.Height));
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

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                var window = Values[0] as IVisjectSurfaceWindow;
                var asset = window?.VisjectAsset;
                if (asset == null || !asset.IsLoaded)
                    return;

                var group = layout.Group("Functions");
                var nodes = window.VisjectSurface.Nodes;

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
                        var label = group.ClickableLabel(overrideNode.Title + " (override)").CustomControl;
                        label.TextColorHighlighted = Color.FromBgra(0xFFA0A0A0);
                        label.TooltipText = overrideNode.TooltipText;
                        label.DoubleClick += () => ((VisualScriptWindow)Values[0]).Surface.FocusNode(overrideNode);
                        label.RightClick += () => ShowContextMenu(overrideNode, label);
                    }
                }

                // New function button
                const float groupPanelButtonSize = 14;
                var addNewFunction = new Image
                {
                    TooltipText = "Add new function",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = group.Panel,
                    Bounds = new Rectangle(group.Panel.Width - groupPanelButtonSize, 0, groupPanelButtonSize, groupPanelButtonSize),
                    IsScrollable = false,
                    Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(Editor.Instance.Icons.Add48),
                };
                addNewFunction.Clicked += OnAddNewFunctionClicked;

                // Override method button
                var overrideMethod = new Image
                {
                    TooltipText = "Override method",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = group.Panel,
                    Bounds = new Rectangle(group.Panel.Width - groupPanelButtonSize * 2, 0, groupPanelButtonSize, groupPanelButtonSize),
                    IsScrollable = false,
                    Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(Editor.Instance.Icons.Import32),
                };
                overrideMethod.Clicked += OnOverrideMethodClicked;
            }

            private void OnAddNewFunctionClicked(Image image, MouseButton button)
            {
                if (button != MouseButton.Left)
                    return;
                var surface = ((VisualScriptWindow)Values[0]).Surface;
                var surfaceBounds = surface.AllNodesBounds;
                surface.ShowArea(new Rectangle(surfaceBounds.BottomLeft, new Vector2(200, 150)).MakeExpanded(400.0f));
                var node = surface.Context.SpawnNode(16, 6, surfaceBounds.BottomLeft + new Vector2(0, 50));
                surface.Select(node);
            }

            private void ShowContextMenu(SurfaceNode node, ClickableLabel label)
            {
                ((VisualScriptWindow)Values[0]).Surface.FocusNode(node);
                var cm = new ContextMenu();
                cm.AddButton("Show", () => ((VisualScriptWindow)Values[0]).Surface.FocusNode(node)).Icon = Editor.Instance.Icons.Search12;
                cm.AddButton("Delete", () => ((VisualScriptWindow)Values[0]).Surface.Delete(node)).Icon = Editor.Instance.Icons.Cross12;
                node.OnShowSecondaryContextMenu(cm, Vector2.Zero);
                cm.Show(label, new Vector2(0, label.Height));
            }

            private void OnOverrideMethodClicked(Image image, MouseButton button)
            {
                if (button != MouseButton.Left)
                    return;

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
                        var attributes = member.GetAttributes(true);
                        var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                        if (tooltipAttribute != null)
                            cmButton.TooltipText = tooltipAttribute.Text;
                        cmButton.Clicked += () =>
                        {
                            var surface = ((VisualScriptWindow)Values[0]).Surface;
                            var surfaceBounds = surface.AllNodesBounds;
                            surface.ShowArea(new Rectangle(surfaceBounds.BottomLeft, new Vector2(200, 150)).MakeExpanded(400.0f));
                            var node = surface.Context.SpawnNode(16, 3, surfaceBounds.BottomLeft + new Vector2(0, 50), new object[]
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
                cm.Show(image, new Vector2(0, image.Height));
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
            _propertiesEditor.Panel.Parent = _split.Panel2;
            _propertiesEditor.Modified += OnPropertyEdited;
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Save32, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _undoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo32, _undo.PerformUndo).LinkTooltip("Undo (Ctrl+Z)");
            _redoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo32, _undo.PerformRedo).LinkTooltip("Redo (Ctrl+Y)");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.PageScale32, ShowWholeGraph).LinkTooltip("Show whole graph");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs32, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/scripting/visual/index.html")).LinkTooltip("See documentation to learn more");
            _debugToolstripControls = new[]
            {
                _toolstrip.AddSeparator(),
                _toolstrip.AddButton(editor.Icons.Play32, OnDebuggerContinue).LinkTooltip("Continue (F5)"),
                _toolstrip.AddButton(editor.Icons.Find32, OnDebuggerNavigateToCurrentNode).LinkTooltip("Navigate to the current stack trace node"),
                _toolstrip.AddButton(editor.Icons.ArrowRight32, OnDebuggerStepOver).LinkTooltip("Step Over (F10)"),
                _toolstrip.AddButton(editor.Icons.ArrowDown32, OnDebuggerStepInto).LinkTooltip("Step Into (F11)"),
                _toolstrip.AddButton(editor.Icons.ArrowUp32, OnDebuggerStepOut).LinkTooltip("Step Out (Shift+F11)"),
                _toolstrip.AddButton(editor.Icons.Stop32, OnDebuggerStop).LinkTooltip("Stop debugging"),
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
                Size = new Vector2(60.0f, _toolstrip.Height),
                Text = "Debug:",
                TooltipText = "The Visual Script object instance to debug. Can be used to filter the logic to debug a specific instance.",
            };
            _debugObjectPicker = new FlaxObjectRefPickerControl
            {
                Location = new Vector2(debugObjectPickerLabel.Right + 4.0f, 8.0f),
                Width = 140.0f,
                Type = item.ScriptType,
                Parent = debugObjectPickerContainer,
            };
            debugObjectPickerContainer.Enabled = debugObjectPickerContainer.Visible = isPlayMode;
            debugObjectPickerContainer.Size = new Vector2(_debugObjectPicker.Right + 2.0f, _toolstrip.Height);
            debugObjectPickerContainer.Parent = _toolstrip;

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
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
            for (int i = 0; i < Surface.Breakpoints.Count; i++)
            {
                if (Surface.Breakpoints[i].ID == flowInfo.NodeId)
                {
                    OnDebugBreakpointHit(ref flowInfo, Surface.Breakpoints[i]);
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
                state.Locals = Editor.Internal_GetVisualScriptLocals();
                Editor.Instance.Simulation.BreakpointHangTag = state;
            }
            return state;
        }

        internal static BreakpointHangState GetStackFrames()
        {
            var state = (BreakpointHangState)Editor.Instance.Simulation.BreakpointHangTag;
            if (state.StackFrames == null)
            {
                state.StackFrames = Editor.Internal_GetVisualScriptStackFrames();
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
            var frame = Editor.Internal_GetVisualScriptPreviousScopeFrame();
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
                {
                    var item = Editor.Instance.ContentDatabase.FindAsset(frame.Script.ID);
                    if (item != null)
                        vsWindow = Editor.Instance.ContentEditing.Open(item) as VisualScriptWindow;
                }
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
                if (Editor.ProjectCache.TryGetCustomData(_asset.ScriptTypeName + ".Breakpoints", out var breakpointsData))
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
                    // Error
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save Visual Script surface data");
                }
                _asset.Reload();
                SaveBreakpoints();

                // Asset got edited so the methods/properties types info could be changed
                Editor.CodeEditing.ClearTypes();
                Editor.CodeEditing.OnTypesChanged();
            }
        }

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
                // Error
                Editor.LogError("Failed to load Visual Script surface.");
                return true;
            }

            return false;
        }

        private bool SaveSurface()
        {
            _surface.Save();

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
            foreach (var child in _propertiesEditor.Panel.Children)
            {
                if (!(child is ScrollBar))
                    child.Enabled = canEdit;
            }
            UpdateToolstrip();
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
            writer.WriteAttributeString("Split", _split.SplitterValue.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("Split"), out float value1))
                _split.SplitterValue = value1;
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
    }
}
