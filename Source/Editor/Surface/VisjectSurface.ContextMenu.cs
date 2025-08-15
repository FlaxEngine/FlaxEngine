// Copyright (c) Wojciech Figat. All rights reserved.

//#define DEBUG_SEARCH_TIME

using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        /// <summary>
        /// Utility for easy nodes archetypes generation for Visject Surface based on scripting types.
        /// </summary>
        [HideInEditor]
        public class NodesCache
        {
            /// <summary>
            /// Delegate for scripting types filtering into cache.
            /// </summary>
            /// <param name="scriptType">The input type to process.</param>
            /// <param name="cache">Node groups cache that can be used for reusing groups for different nodes.</param>
            /// <param name="version">The cache version number. Can be used to reject any cached data after.<see cref="NodesCache"/> rebuilt.</param>
            public delegate void IterateType(ScriptType scriptType, Dictionary<KeyValuePair<string, ushort>, GroupArchetype> cache, int version);

            internal static readonly List<NodesCache> Caches = new List<NodesCache>(8);

            private readonly object _locker = new object();
            private readonly IterateType _iterator;
            private int _version;
            private Task _task;
            private VisjectCM _taskContextMenu;
            private Dictionary<KeyValuePair<string, ushort>, GroupArchetype> _cache;

            /// <summary>
            /// Initializes a new instance of the <see cref="NodesCache"/> class.
            /// </summary>
            /// <param name="iterator">The iterator callback to build node types from Scripting.</param>
            public NodesCache(IterateType iterator)
            {
                _iterator = iterator;
            }

            /// <summary>
            /// Waits for the async caching job to finish.
            /// </summary>
            public void Wait()
            {
                if (_task != null)
                {
                    Profiler.BeginEvent("Setup Context Menu");
                    _task.Wait();
                    Profiler.EndEvent();
                }
            }

            /// <summary>
            /// Clears cache.
            /// </summary>
            public void Clear()
            {
                Wait();

                if (_cache != null && _cache.Count != 0)
                {
                    OnCodeEditingTypesCleared();
                }
                lock (_locker)
                {
                    Caches.Remove(this);
                }
            }

            /// <summary>
            /// Updates the Visject Context Menu to contain current nodes.
            /// </summary>
            /// <param name="contextMenu">The output context menu to setup.</param>
            public void Get(VisjectCM contextMenu)
            {
                Profiler.BeginEvent("Setup Context Menu");

                Wait();

                lock (_locker)
                {
                    if (_cache == null)
                    {
                        if (!Caches.Contains(this))
                            Caches.Add(this);
                        _cache = new Dictionary<KeyValuePair<string, ushort>, GroupArchetype>();
                    }
                    contextMenu.LockChildrenRecursive();

                    // Check if has cached groups
                    if (_cache.Count != 0)
                    {
                        // Check if context menu doesn't have the recent cached groups
                        if (!contextMenu.Groups.Any(g => g.Archetypes[0].Tag is int asInt && asInt == _version))
                        {
                            var groups = contextMenu.Groups.Where(g => g.Archetypes.Count != 0 && g.Archetypes[0].Tag is int).ToArray();
                            foreach (var g in groups)
                                contextMenu.RemoveGroup(g);
                            foreach (var g in _cache.Values)
                                contextMenu.AddGroup(g);
                        }
                    }
                    else
                    {
                        // Remove any old groups from context menu
                        var groups = contextMenu.Groups.Where(g => g.Archetypes.Count != 0 && g.Archetypes[0].Tag is int).ToArray();
                        foreach (var g in groups)
                            contextMenu.RemoveGroup(g);

                        // Register for scripting types reload
                        Editor.Instance.CodeEditing.TypesCleared += OnCodeEditingTypesCleared;

                        // Run caching on an async
                        _task = Task.Run(OnActiveContextMenuShowAsync);
                        _taskContextMenu = contextMenu;
                    }

                    contextMenu.UnlockChildrenRecursive();
                }

                Profiler.EndEvent();
            }

            private void OnActiveContextMenuShowAsync()
            {
                Profiler.BeginEvent("Setup Context Menu (async)");
#if DEBUG_SEARCH_TIME
                var searchStartTime = DateTime.Now;
#endif

                foreach (var scriptType in Editor.Instance.CodeEditing.AllWithStd.Get())
                {
                    if (SurfaceUtils.IsValidVisualScriptType(scriptType))
                    {
                        _iterator(scriptType, _cache, _version);
                    }
                }

                // Add group to context menu (on a main thread)
                FlaxEngine.Scripting.InvokeOnUpdate(() =>
                {
#if DEBUG_SEARCH_TIME
                    var addStartTime = DateTime.Now;
#endif
                    lock (_locker)
                    {
                        _taskContextMenu.AddGroups(_cache.Values);
                        _taskContextMenu = null;
                    }
#if DEBUG_SEARCH_TIME
                    Editor.LogError($"Added items to VisjectCM in: {(DateTime.Now - addStartTime).TotalMilliseconds} ms");
#endif
                });

#if DEBUG_SEARCH_TIME
                Editor.LogError($"Collected items in: {(DateTime.Now - searchStartTime).TotalMilliseconds} ms");
#endif
                Profiler.EndEvent();

                lock (_locker)
                {
                    _task = null;
                }
            }

            private void OnCodeEditingTypesCleared()
            {
                Wait();

                lock (_locker)
                {
                    _cache.Clear();
                    _version++;
                }

                Editor.Instance.CodeEditing.TypesCleared -= OnCodeEditingTypesCleared;
            }
        }

        private ContextMenuButton _cmCopyButton;
        private ContextMenuButton _cmDuplicateButton;
        private ContextMenuChildMenu _cmFormatNodesMenu;
        private ContextMenuButton _cmFormatNodesConnectionButton;
        private ContextMenuButton _cmAlignNodesTopButton;
        private ContextMenuButton _cmAlignNodesMiddleButton;
        private ContextMenuButton _cmAlignNodesBottomButton;
        private ContextMenuButton _cmAlignNodesLeftButton;
        private ContextMenuButton _cmAlignNodesCenterButton;
        private ContextMenuButton _cmAlignNodesRightButton;
        private ContextMenuButton _cmDistributeNodesHorizontallyButton;
        private ContextMenuButton _cmDistributeNodesVerticallyButton;
        private ContextMenuButton _cmRemoveNodeConnectionsButton;
        private ContextMenuButton _cmRemoveBoxConnectionsButton;
        private readonly Float2 ContextMenuOffset = new Float2(5);

        /// <summary>
        /// Gets a value indicating whether the primary surface context menu is being opened (eg. user is adding nodes).
        /// </summary>
        public virtual bool IsPrimaryMenuOpened => _activeVisjectCM != null && _activeVisjectCM.Visible;

        /// <summary>
        /// Sets the primary menu for the Visject nodes spawning. Can be overriden per surface or surface context. Set to null to restore the default menu.
        /// </summary>
        /// <param name="menu">The menu to override with (use null if restore the default value).</param>
        protected virtual void SetPrimaryMenu(VisjectCM menu)
        {
            menu = menu ?? _cmPrimaryMenu;

            if (menu == _activeVisjectCM)
                return;

            if (_activeVisjectCM != null)
            {
                _activeVisjectCM.ItemClicked -= OnPrimaryMenuButtonClick;
                _activeVisjectCM.VisibleChanged -= OnPrimaryMenuVisibleChanged;
            }

            _activeVisjectCM = menu;

            if (_activeVisjectCM != null)
            {
                _activeVisjectCM.ItemClicked += OnPrimaryMenuButtonClick;
                _activeVisjectCM.VisibleChanged += OnPrimaryMenuVisibleChanged;
            }
        }

        /// <summary>
        /// Creates the default primary context menu for the surface. Override this to provide the custom implementation.
        /// </summary>
        /// <remarks>This method is being called in <see cref="ShowPrimaryMenu"/> on first time when need to show the default menu (no overrides specified for the surface context).</remarks>
        /// <returns>The created menu.</returns>
        protected virtual VisjectCM CreateDefaultPrimaryMenu()
        {
            return new VisjectCM(new VisjectCM.InitInfo
            {
                CanSetParameters = CanSetParameters,
                UseDescriptionPanel = UseContextMenuDescriptionPanel,
                Groups = NodeArchetypes,
                CanSpawnNode = CanUseNodeType,
                ParametersGetter = () => Parameters,
                CustomNodesGroup = GetCustomNodes(),
                ParameterGetNodeArchetype = GetParameterGetterNodeArchetype(out _),
                Style = Style,
            });
        }

        /// <summary>
        /// Called when showing primary context menu.
        /// </summary>
        /// <param name="activeCM">The active context menu to show.</param>
        /// <param name="location">The display location on the surface control.</param>
        /// <param name="startBox">The start box.</param>
        protected virtual void OnShowPrimaryMenu(VisjectCM activeCM, Float2 location, Box startBox)
        {
            activeCM.Show(this, location, startBox);
        }

        /// <summary>
        /// Shows the primary menu.
        /// </summary>
        /// <param name="location">The location in the Surface Space.</param>
        /// <param name="moveSurface">If the surface should be moved to accommodate for the menu.</param>
        /// <param name="input">The user text input for nodes search.</param>
        public virtual void ShowPrimaryMenu(Float2 location, bool moveSurface = false, string input = null)
        {
            if (!CanEdit)
                return;

            // Check if need to create default context menu (no override specified)
            if (_activeVisjectCM == null && _cmPrimaryMenu == null)
            {
                _activeVisjectCM = _cmPrimaryMenu = CreateDefaultPrimaryMenu();

                _activeVisjectCM.ItemClicked += OnPrimaryMenuButtonClick;
                _activeVisjectCM.VisibleChanged += OnPrimaryMenuVisibleChanged;
            }

            if (moveSurface)
            {
                const float leftPadding = 20;
                var delta = Float2.Min(location - leftPadding, Float2.Zero) + Float2.Max((location + _activeVisjectCM.Size) - Size, Float2.Zero);

                location -= delta;
                _rootControl.Location -= delta;
            }

            _cmStartPos = location;

            // Offset added in case the user doesn't like the box and wants to quickly get rid of it by clicking
            OnShowPrimaryMenu(_activeVisjectCM, _cmStartPos + ContextMenuOffset, _connectionInstigator as Box);

            if (!string.IsNullOrEmpty(input))
            {
                foreach (char character in input)
                {
                    // OnKeyDown -> VisjectCM focuses on the text-thingy
                    _activeVisjectCM.OnKeyDown(KeyboardKeys.None);
                    _activeVisjectCM.OnCharInput(character);
                    _activeVisjectCM.OnKeyUp(KeyboardKeys.None);
                }
            }
        }

        /// <summary>
        /// Shows the secondary context menu.
        /// </summary>
        /// <param name="location">The location in the Surface Space.</param>
        /// <param name="controlUnderMouse">The Surface Control that is under the cursor. Used to customize the menu.</param>
        public virtual void ShowSecondaryCM(Float2 location, SurfaceControl controlUnderMouse)
        {
            var selection = SelectedNodes;
            if (selection.Count == 0)
                return;

            // Create secondary context menu
            var menu = new FlaxEditor.GUI.ContextMenu.ContextMenu();

            if (_onSave != null)
            {
                menu.AddButton("Save", _onSave).Enabled = CanEdit;
                menu.AddSeparator();
            }
            _cmCopyButton = menu.AddButton("Copy", Copy);
            menu.AddButton("Paste", Paste).Enabled = CanEdit && CanPaste();
            _cmDuplicateButton = menu.AddButton("Duplicate", Duplicate);
            _cmDuplicateButton.Enabled = CanEdit && selection.Any(node => (node.Archetype.Flags & NodeFlags.NoSpawnViaPaste) == 0);
            var canRemove = CanEdit && selection.All(node => (node.Archetype.Flags & NodeFlags.NoRemove) == 0);
            menu.AddButton("Cut", Cut).Enabled = canRemove;
            menu.AddButton("Delete", Delete).Enabled = canRemove;

            if (_supportsDebugging)
            {
                menu.AddSeparator();
                if (selection.Count == 1)
                {
                    menu.AddButton(selection[0].Breakpoint.Set ? "Delete breakpoint" : "Add breakpoint", () =>
                    {
                        foreach (var node in Nodes)
                        {
                            if (node.IsSelected)
                            {
                                node.Breakpoint.Set = !node.Breakpoint.Set;
                                node.Breakpoint.Enabled = true;
                                OnNodeBreakpointEdited(node);
                                break;
                            }
                        }
                    });
                    menu.AddButton("Toggle breakpoint", () =>
                    {
                        foreach (var node in Nodes)
                        {
                            if (node.IsSelected)
                            {
                                node.Breakpoint.Enabled = !node.Breakpoint.Enabled;
                                OnNodeBreakpointEdited(node);
                                break;
                            }
                        }
                    }).Enabled = selection[0].Breakpoint.Set;
                }
                menu.AddSeparator();
                menu.AddButton("Delete all breakpoints", () =>
                {
                    foreach (var node in Nodes)
                    {
                        if (node.Breakpoint.Set)
                        {
                            node.Breakpoint.Set = false;
                            OnNodeBreakpointEdited(node);
                        }
                    }
                }).Enabled = Nodes.Any(x => x.Breakpoint.Set);
                menu.AddButton("Enable all breakpoints", () =>
                {
                    foreach (var node in Nodes)
                    {
                        if (!node.Breakpoint.Enabled)
                        {
                            node.Breakpoint.Enabled = true;
                            OnNodeBreakpointEdited(node);
                        }
                    }
                }).Enabled = Nodes.Any(x => x.Breakpoint.Set && !x.Breakpoint.Enabled);
                menu.AddButton("Disable all breakpoints", () =>
                {
                    foreach (var node in Nodes)
                    {
                        if (node.Breakpoint.Enabled)
                        {
                            node.Breakpoint.Enabled = false;
                            OnNodeBreakpointEdited(node);
                        }
                    }
                }).Enabled = Nodes.Any(x => x.Breakpoint.Set && x.Breakpoint.Enabled);
            }
            menu.AddSeparator();

            _cmFormatNodesMenu = menu.AddChildMenu("Format node(s)");
            _cmFormatNodesMenu.Enabled = CanEdit && HasNodesSelection;

            _cmFormatNodesConnectionButton = _cmFormatNodesMenu.ContextMenu.AddButton("Auto format", Editor.Instance.Options.Options.Input.NodesAutoFormat, () => { FormatGraph(SelectedNodes); });
            _cmFormatNodesConnectionButton = _cmFormatNodesMenu.ContextMenu.AddButton("Straighten connections", Editor.Instance.Options.Options.Input.NodesStraightenConnections, () => { StraightenGraphConnections(SelectedNodes); });

            _cmFormatNodesMenu.ContextMenu.AddSeparator();
            _cmAlignNodesTopButton = _cmFormatNodesMenu.ContextMenu.AddButton("Align top", Editor.Instance.Options.Options.Input.NodesAlignTop, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Top); });
            _cmAlignNodesMiddleButton = _cmFormatNodesMenu.ContextMenu.AddButton("Align middle", Editor.Instance.Options.Options.Input.NodesAlignMiddle, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Middle); });
            _cmAlignNodesBottomButton = _cmFormatNodesMenu.ContextMenu.AddButton("Align bottom", Editor.Instance.Options.Options.Input.NodesAlignBottom, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Bottom); });

            _cmFormatNodesMenu.ContextMenu.AddSeparator();
            _cmAlignNodesLeftButton = _cmFormatNodesMenu.ContextMenu.AddButton("Align left", Editor.Instance.Options.Options.Input.NodesAlignLeft, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Left); });
            _cmAlignNodesCenterButton = _cmFormatNodesMenu.ContextMenu.AddButton("Align center", Editor.Instance.Options.Options.Input.NodesAlignCenter, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Center); });
            _cmAlignNodesRightButton = _cmFormatNodesMenu.ContextMenu.AddButton("Align right", Editor.Instance.Options.Options.Input.NodesAlignRight, () => { AlignNodes(SelectedNodes, NodeAlignmentType.Right); });

            _cmFormatNodesMenu.ContextMenu.AddSeparator();
            _cmDistributeNodesHorizontallyButton = _cmFormatNodesMenu.ContextMenu.AddButton("Distribute horizontally", Editor.Instance.Options.Options.Input.NodesDistributeHorizontal, () => { DistributeNodes(SelectedNodes, false); });
            _cmDistributeNodesVerticallyButton = _cmFormatNodesMenu.ContextMenu.AddButton("Distribute vertically", Editor.Instance.Options.Options.Input.NodesDistributeVertical, () => { DistributeNodes(SelectedNodes, true); });

            _cmRemoveNodeConnectionsButton = menu.AddButton("Remove all connections", () =>
            {
                var nodes = ((List<SurfaceNode>)menu.Tag);

                if (Undo != null)
                {
                    var actions = new List<IUndoAction>(nodes.Count);
                    foreach (var node in nodes)
                    {
                        var action = new EditNodeConnections(Context, node);
                        node.RemoveConnections();
                        action.End();
                        actions.Add(action);
                    }
                    Undo.AddAction(new MultiUndoAction(actions, actions[0].ActionString));
                }
                else
                {
                    foreach (var node in nodes)
                    {
                        node.RemoveConnections();
                    }
                }

                MarkAsEdited();
            });
            bool anyConnection = SelectedNodes.Any(n => n.GetBoxes().Any(b => b.HasAnyConnection));
            _cmRemoveNodeConnectionsButton.Enabled = CanEdit && anyConnection;

            _cmRemoveBoxConnectionsButton = menu.AddButton("Remove all socket connections", () =>
            {
                var boxUnderMouse = (Box)_cmRemoveBoxConnectionsButton.Tag;
                if (Undo != null)
                {
                    var action = new EditNodeConnections(Context, boxUnderMouse.ParentNode);
                    boxUnderMouse.RemoveConnections();
                    action.End();
                    Undo.AddAction(action);
                }
                else
                {
                    boxUnderMouse.RemoveConnections();
                }
                MarkAsEdited();
            });
            _cmRemoveBoxConnectionsButton.Enabled = CanEdit;
            {
                var boxUnderMouse = GetChildAtRecursive(location) as Box;
                _cmRemoveBoxConnectionsButton.Enabled = boxUnderMouse != null && boxUnderMouse.HasAnyConnection;
                _cmRemoveBoxConnectionsButton.Tag = boxUnderMouse;

                if (boxUnderMouse != null)
                {
                    boxUnderMouse.ParentNode.highlightBox = boxUnderMouse; 
                    menu.VisibleChanged += (c) =>
                    {
                        if (!c.Visible)
                            boxUnderMouse.ParentNode.highlightBox = null;
                    };
                }
            }

            controlUnderMouse?.OnShowSecondaryContextMenu(menu, controlUnderMouse.PointFromParent(location));

            OnShowSecondaryContextMenu(menu, controlUnderMouse);

            // Show secondary context menu
            _cmStartPos = location;
            menu.Tag = selection;
            menu.MaximumItemsInViewCount = 24;
            menu.Show(this, location);
        }

        /// <summary>
        /// Called when editor is showing secondary context menu. Can be used to inject custom options for surface logic.
        /// </summary>
        /// <param name="controlUnderMouse">The Surface Control that is under the cursor. Used to customize the menu.</param>
        /// <param name="menu">The menu.</param>
        protected virtual void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, SurfaceControl controlUnderMouse)
        {
        }

        private void OnPrimaryMenuVisibleChanged(Control primaryMenu)
        {
            if (!primaryMenu.Visible)
            {
                _connectionInstigator = null;
            }
        }

        /// <summary>
        /// Handles Visject CM item click event by spawning the selected item.
        /// </summary>
        /// <param name="visjectCmItem">The item.</param>
        /// <param name="selectedBox">The selected box.</param>
        protected virtual void OnPrimaryMenuButtonClick(VisjectCMItem visjectCmItem, Box selectedBox)
        {
            if (!CanEdit)
                return;
            var node = Context.SpawnNode(
                                         visjectCmItem.GroupArchetype,
                                         visjectCmItem.NodeArchetype,
                                         _rootControl.PointFromParent(ref _cmStartPos),
                                         visjectCmItem.Data
                                        );
            if (node == null)
                return;

            // If the user entered a comment
            if (node is SurfaceComment surfaceComment && HasNodesSelection)
            {
                // Note how the user input exactly mimics the other comment creation way. This is very much desired.
                // Select node --> Type // --> Type the comment text --> Hit Enter
                string title = surfaceComment.Title;
                Delete(node, false);
                CommentSelection(title);
                return;
            }

            // Auto select new node
            Select(node);

            if (selectedBox != null)
            {
                Box endBox = null;
                foreach (var box in node.GetBoxes().Where(box => box.IsOutput != selectedBox.IsOutput))
                {
                    if (selectedBox.IsOutput)
                    {
                        if (box.CanUseType(selectedBox.CurrentType))
                        {
                            endBox = box;
                            break;
                        }
                    }
                    else
                    {
                        if (selectedBox.CanUseType(box.CurrentType))
                        {
                            endBox = box;
                            break;
                        }
                    }

                    if (endBox == null && selectedBox.CanUseType(box.CurrentType))
                    {
                        endBox = box;
                    }
                }
                TryConnect(selectedBox, endBox);
            }
        }

        private void TryConnect(Box startBox, Box endBox)
        {
            if (startBox == null || endBox == null)
            {
                if (IsConnecting)
                {
                    ConnectingEnd(null);
                }
                return;
            }

            // If the user is patiently waiting for his box to get connected to the newly created one fulfill his wish!

            _connectionInstigator = startBox;

            if (!IsConnecting)
            {
                ConnectingStart(startBox);
            }
            ConnectingEnd(endBox);

            // Smart-Select next box
            /*
             * Output and Output => undefined
             * Output and Input => Connect and move to next on input-node
             * Input and Output => Connect and move to next on input-node
             * Input and Input => undefined, cannot happen
             */
            Box inputBox = endBox.IsOutput ? startBox : endBox;
            Box nextBox = inputBox.ParentNode.GetNextBox(inputBox);

            // If we are going backwards and the end-node has an input box we want to edit backwards
            if (!startBox.IsOutput)
            {
                Box endNodeInputBox = endBox.ParentNode.GetBoxes().DefaultIfEmpty(null).FirstOrDefault(b => !b.IsOutput);
                if (endNodeInputBox != null)
                {
                    nextBox = endNodeInputBox;
                }
            }

            // TODO: What if we reached the end (nextBox == null)? Do we travel along the nodes?
            /*
             * while (nextBox == null && _inputBoxStack.Count > 0)
                {
                    // We found the last box on this node but there are still boxes on previous nodes on the stack
                    nextBox = GetNextBox(_inputBoxStack.Pop());
                }
                */

            if (nextBox != null)
            {
                Select(nextBox.ParentNode);
                nextBox.ParentNode.SelectBox(nextBox);

                var padding = new Float2(20);
                var delta = Float2.Min(_rootControl.PointToParent(nextBox.ParentNode.Location) - padding, Float2.Zero) + Float2.Max((_rootControl.PointToParent(nextBox.ParentNode.BottomRight) + padding) - Size, Float2.Zero);

                _rootControl.Location -= delta;
            }
        }
    }
}
