// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Surface;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Visual Scripting debugger utility window.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public sealed class VisualScriptDebuggerWindow : EditorWindow
    {
        private abstract class Tab : GUI.Tabs.Tab
        {
            protected class Node : TreeNode
            {
                private struct NodeInfo
                {
                    public VisualScript Script;
                    public uint NodeId;

                    public NodeInfo(VisualScript script, uint nodeId)
                    {
                        Script = script;
                        NodeId = nodeId;
                    }
                }

                public Node(VisualScript script, uint nodeId)
                : this(script, nodeId, SpriteHandle.Invalid, SpriteHandle.Invalid)
                {
                }

                public Node(VisualScript script, uint nodeId, SpriteHandle iconCollapsed, SpriteHandle iconOpened)
                : base(false, iconCollapsed, iconOpened)
                {
                    Tag = new NodeInfo(script, nodeId);
                }

                public static SurfaceNode GetNode(object tag)
                {
                    if (tag is NodeInfo nodeInfo)
                    {
                        VisualScriptWindow vsWindow = null;
                        foreach (var window in Editor.Instance.Windows.Windows)
                        {
                            if (window is VisualScriptWindow w && w.Asset == nodeInfo.Script)
                            {
                                vsWindow = (VisualScriptWindow)window;
                                break;
                            }
                        }
                        if (vsWindow == null)
                            vsWindow = Editor.Instance.ContentEditing.Open(nodeInfo.Script) as VisualScriptWindow;
                        return vsWindow?.Surface.FindNode(nodeInfo.NodeId);
                    }
                    return null;
                }

                /// <inheritdoc />
                protected override bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
                {
                    var node = GetNode(Tag);
                    ((VisualScriptWindow)node?.Surface.Owner)?.ShowNode(node);
                    return true;
                }
            }

            private Tree _tree;
            protected TreeNode _rootNode;

            protected Tab(string text)
            : base(text)
            {
                var scrollPanel = new Panel(ScrollBars.Vertical)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = this
                };
                _tree = new Tree(false)
                {
                    AnchorPreset = AnchorPresets.HorizontalStretchTop,
                    IsScrollable = true,
                    Parent = scrollPanel
                };
                _rootNode = new TreeNode(false);
                _rootNode.Expand();
                _rootNode.ChildrenIndent = 0;
                _rootNode.Parent = _tree;
                _tree.Margin = new Margin(0.0f, 0.0f, -14.0f, 2.0f); // Hide root node
                _tree.SelectedChanged += OnTreeSelectedChanged;
                _tree.RightClick += OnTreeRightClick;
            }

            private void OnTreeSelectedChanged(List<TreeNode> before, List<TreeNode> after)
            {
                if (after != null && after.Count == 1 && after[0] is Node treeNode)
                {
                    var node = Node.GetNode(treeNode.Tag);
                    ((VisualScriptWindow)node?.Surface.Owner)?.ShowNode(node);
                }
            }

            private void OnTreeRightClick(TreeNode treeNode, Float2 location)
            {
                var menu = new ContextMenu
                {
                    Tag = treeNode.Tag
                };

                menu.AddButton("Show node", button =>
                {
                    var node = Node.GetNode(button.ParentContextMenu.Tag);
                    ((VisualScriptWindow)node?.Surface.Owner)?.ShowNode(node);
                }).Icon = Editor.Instance.Icons.Search12;
                OnTreeNodeRightClick(menu);
                menu.Show(treeNode, location);
            }

            protected virtual void OnTreeNodeRightClick(ContextMenu menu)
            {
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _tree = null;

                base.OnDestroy();
            }
        }

        private class CallStackTab : Tab
        {
            public CallStackTab()
            : base("Call Stack")
            {
                Editor.Instance.Simulation.BreakpointHangBegin += OnBreakpointHangBegin;
                Editor.Instance.Simulation.BreakpointHangEnd += OnBreakpointHangEnd;
            }

            private void OnBreakpointHangBegin()
            {
                var state = VisualScriptWindow.GetStackFrames();
                if (state.StackFrames != null)
                {
                    for (int i = 0; i < state.StackFrames.Length; i++)
                    {
                        ref var stackFrame = ref state.StackFrames[i];
                        VisualScriptWindow vsWindow = null;
                        foreach (var window in Editor.Instance.Windows.Windows)
                        {
                            if (window is VisualScriptWindow w && w.Asset == stackFrame.Script)
                            {
                                vsWindow = (VisualScriptWindow)window;
                                break;
                            }
                        }
                        var node = vsWindow?.Surface.FindNode(stackFrame.NodeId);
                        string text;
                        if (node != null)
                            text = $"{Path.GetFileNameWithoutExtension(stackFrame.Script.Path)} in node {node.Title}";
                        else
                            text = $"{Path.GetFileNameWithoutExtension(stackFrame.Script.Path)} in nodeId {stackFrame.NodeId}";
                        var icon = Editor.Instance.Icons.ArrowRight12;
                        new Node(stackFrame.Script, stackFrame.NodeId, icon, icon)
                        {
                            Text = text,
                            Parent = _rootNode,
                            IconColor = i == 0 ? Color.Yellow : Color.Transparent,
                        };
                    }
                }
            }

            private void OnBreakpointHangEnd()
            {
                _rootNode.DisposeChildren();
            }

            public override void OnDestroy()
            {
                Editor.Instance.Simulation.BreakpointHangBegin -= OnBreakpointHangBegin;
                Editor.Instance.Simulation.BreakpointHangEnd -= OnBreakpointHangEnd;

                base.OnDestroy();
            }
        }

        private class LocalsTab : Tab
        {
            public LocalsTab()
            : base("Locals")
            {
                Editor.Instance.Simulation.BreakpointHangBegin += OnBreakpointHangBegin;
                Editor.Instance.Simulation.BreakpointHangEnd += OnBreakpointHangEnd;
            }

            protected override void OnTreeNodeRightClick(ContextMenu menu)
            {
                menu.AddSeparator();
                menu.AddButton("Copy value", button =>
                {
                    var node = (SurfaceNode)button.ParentContextMenu.Tag;
                    var state = VisualScriptWindow.GetLocals();
                    var local = state.Locals.First(x => x.NodeId == node.ID);
                    Clipboard.Text = local.Value ?? string.Empty;
                });
            }

            private void OnBreakpointHangBegin()
            {
                var state = VisualScriptWindow.GetLocals();
                if (state.Locals != null)
                {
                    for (int i = 0; i < state.Locals.Length; i++)
                    {
                        ref var local = ref state.Locals[i];
                        var node = state.Window.Surface.FindNode(local.NodeId);
                        var box = node?.GetBox(local.BoxId);
                        if (box != null)
                        {
                            var name = string.IsNullOrEmpty(box.Text) ? node.Title : node.Title + " : " + box.Text;
                            var value = local.Value ?? string.Empty;
                            var type = TypeUtils.GetType(local.ValueTypeName);
                            node.Surface.Style.GetConnectionColor(type, node.Archetype.ConnectionsHints, out var typeColor);
                            var icon = node.Surface.Style.Icons.BoxClose;
                            new Node(state.Window.Asset, local.NodeId, icon, icon)
                            {
                                Text = name + " = " + value,
                                Parent = _rootNode,
                                IconColor = typeColor,
                                TooltipText = type.TypeName,
                            };
                        }
                    }
                }
            }

            private void OnBreakpointHangEnd()
            {
                _rootNode.DisposeChildren();
            }

            public override void OnDestroy()
            {
                Editor.Instance.Simulation.BreakpointHangBegin -= OnBreakpointHangBegin;
                Editor.Instance.Simulation.BreakpointHangEnd -= OnBreakpointHangEnd;

                base.OnDestroy();
            }
        }

        private class BreakpointsTab : Tab
        {
            public BreakpointsTab()
            : base("Breakpoints")
            {
                foreach (var window in Editor.Instance.Windows.Windows)
                    OnWindowAdded(window);

                Editor.Instance.Windows.WindowAdded += OnWindowAdded;
                Editor.Instance.Windows.WindowRemoved += OnWindowRemoved;
            }

            protected override void OnTreeNodeRightClick(ContextMenu menu)
            {
                menu.AddSeparator();
                menu.AddButton("Remove breakpoint", button =>
                {
                    var node = (SurfaceNode)button.ParentContextMenu.Tag;
                    node.Breakpoint.Set = !node.Breakpoint.Set;
                    node.Breakpoint.Enabled = true;
                    node.Surface.OnNodeBreakpointEdited(node);
                }).Icon = Editor.Instance.Icons.Cross12;
                menu.AddButton("Toggle breakpoint", button =>
                {
                    var node = (SurfaceNode)button.ParentContextMenu.Tag;
                    node.Breakpoint.Enabled = !node.Breakpoint.Enabled;
                    node.Surface.OnNodeBreakpointEdited(node);
                });
                menu.AddSeparator();
                menu.AddButton("Delete all breakpoints", () =>
                {
                    foreach (var child in _rootNode.Children.ToArray())
                    {
                        if (child is TreeNode n && n.Tag is SurfaceNode node)
                        {
                            node.Breakpoint.Set = false;
                            node.Surface.OnNodeBreakpointEdited(node);
                        }
                    }
                });
                menu.AddButton("Enable all breakpoints", () =>
                {
                    foreach (var child in _rootNode.Children)
                    {
                        if (child is TreeNode n && n.Tag is SurfaceNode node && !node.Breakpoint.Enabled)
                        {
                            node.Breakpoint.Enabled = true;
                            node.Surface.OnNodeBreakpointEdited(node);
                        }
                    }
                });
                menu.AddButton("Disable all breakpoints", () =>
                {
                    foreach (var child in _rootNode.Children.ToArray())
                    {
                        if (child is TreeNode n && n.Tag is SurfaceNode node && node.Breakpoint.Enabled)
                        {
                            node.Breakpoint.Enabled = false;
                            node.Surface.OnNodeBreakpointEdited(node);
                        }
                    }
                });
            }

            private void OnWindowAdded(EditorWindow window)
            {
                if (window is VisualScriptWindow vsWindow)
                {
                    FlaxEngine.Scripting.RunOnUpdate(() =>
                    {
                        vsWindow.Surface.NodeBreakpointEdited += OnSurfaceNodeBreakpointEdited;
                        if (vsWindow.Surface.Nodes != null)
                        {
                            foreach (var node in vsWindow.Surface.Nodes)
                                if (node.Breakpoint.Set)
                                    OnSurfaceNodeBreakpointEdited(node);
                        }
                    });
                }
            }

            private void OnSurfaceNodeBreakpointEdited(SurfaceNode node)
            {
                var vsWindow = (VisualScriptWindow)node.Surface.Owner;
                var bpNode = _rootNode.Children.FirstOrDefault(x => x.Tag == node) as TreeNode;
                if (node.Breakpoint.Set)
                {
                    if (bpNode == null)
                    {
                        bpNode = new Node(vsWindow.Asset, node.ID, node.Surface.Style.Icons.BoxOpen, node.Surface.Style.Icons.BoxClose)
                        {
                            Text = vsWindow.Item.ShortName + " at " + node.Title,
                            Parent = _rootNode,
                            IconColor = new Color(0.894117647f, 0.0784313725f, 0.0f),
                        };
                    }
                    bpNode.IsExpanded = node.Breakpoint.Enabled;
                }
                else
                    bpNode?.Dispose();
            }

            private void OnWindowRemoved(EditorWindow window)
            {
                if (window is VisualScriptWindow vsWindow)
                {
                    vsWindow.Surface.NodeBreakpointEdited -= OnSurfaceNodeBreakpointEdited;
                    for (var i = _rootNode.Children.Count - 1; i >= 0; i--)
                    {
                        var bpNode = _rootNode.Children[i];
                        if (bpNode.Tag is SurfaceNode node && node.Surface.Owner == vsWindow)
                        {
                            _rootNode.RemoveChild(bpNode);
                        }
                    }
                }
            }

            public override void OnDestroy()
            {
                Editor.Instance.Windows.WindowAdded -= OnWindowAdded;
                Editor.Instance.Windows.WindowRemoved -= OnWindowRemoved;

                base.OnDestroy();
            }
        }

        private Control[] _debugToolstripControls;
        private Tab[] _tabs;

        /// <inheritdoc />
        public VisualScriptDebuggerWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Visual Script Debugger";

            var inputOptions = editor.Options.Options.Input;

            var toolstrip = new ToolStrip
            {
                Parent = this
            };
            toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/scripting/visual/index.html")).LinkTooltip("See documentation to learn more");
            _debugToolstripControls = new[]
            {
                toolstrip.AddSeparator(),
                toolstrip.AddButton(editor.Icons.Play64, OnDebuggerContinue).LinkTooltip("Continue", ref inputOptions.DebuggerContinue),
                toolstrip.AddButton(editor.Icons.Search64, OnDebuggerNavigateToCurrentNode).LinkTooltip("Navigate to the current stack trace node"),
                toolstrip.AddButton(editor.Icons.Stop64, OnDebuggerStop).LinkTooltip("Stop debugging"),
            };
            foreach (var control in _debugToolstripControls)
                control.Visible = Editor.Simulation.IsDuringBreakpointHang;

            var tabs = new Tabs
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, toolstrip.Bottom, 0),
                TabsSize = new Float2(80, 20),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                UseScroll = true,
                Parent = this
            };
            _tabs = new Tab[]
            {
                new CallStackTab(),
                new LocalsTab(),
                new BreakpointsTab(),
            };
            foreach (var tab in _tabs)
                tabs.AddTab(tab);

            InputActions.Add(options => options.DebuggerContinue, OnDebuggerContinue);

            Editor.Simulation.BreakpointHangBegin += OnBreakpointHangBegin;
            Editor.Simulation.BreakpointHangEnd += OnBreakpointHangEnd;
        }

        private void OnBreakpointHangBegin()
        {
            foreach (var control in _debugToolstripControls)
                control.Visible = true;
        }

        private void OnBreakpointHangEnd()
        {
            foreach (var control in _debugToolstripControls)
                control.Visible = false;
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
            var state = (VisualScriptWindow.BreakpointHangState)Editor.Simulation.BreakpointHangTag;
            state.Window.SelectTab();
            state.Window.RootWindow.Focus();
            state.Window.Surface.Focus();
            state.Window.Surface.FocusNode(state.Node);
        }

        private void OnDebuggerStop()
        {
            if (!Editor.Simulation.IsDuringBreakpointHang)
                return;

            // Stop play
            Editor.Simulation.RequestStopPlay();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _debugToolstripControls = null;
            _tabs = null;

            Editor.Simulation.BreakpointHangBegin -= OnBreakpointHangBegin;
            Editor.Simulation.BreakpointHangEnd -= OnBreakpointHangEnd;

            base.OnDestroy();
        }
    }
}
