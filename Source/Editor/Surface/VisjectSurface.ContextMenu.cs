// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        private ContextMenuButton _cmCopyButton;
        private ContextMenuButton _cmDuplicateButton;
        private ContextMenuButton _cmFormatNodesConnectionButton;
        private ContextMenuButton _cmRemoveNodeConnectionsButton;
        private ContextMenuButton _cmRemoveBoxConnectionsButton;
        private readonly Vector2 ContextMenuOffset = new Vector2(5);

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
                Groups = NodeArchetypes,
                CanSpawnNode = CanUseNodeType,
                ParametersGetter = () => Parameters,
                CustomNodesGroup = GetCustomNodes(),
                ParameterGetNodeArchetype = GetParameterGetterNodeArchetype(out _),
            });
        }

        /// <summary>
        /// Called when showing primary context menu.
        /// </summary>
        /// <param name="activeCM">The active context menu to show.</param>
        /// <param name="location">The display location on the surface control.</param>
        /// <param name="startBox">The start box.</param>
        protected virtual void OnShowPrimaryMenu(VisjectCM activeCM, Vector2 location, Box startBox)
        {
            activeCM.Show(this, location, startBox);
        }

        /// <summary>
        /// Shows the primary menu.
        /// </summary>
        /// <param name="location">The location in the Surface Space.</param>
        /// <param name="moveSurface">If the surface should be moved to accommodate for the menu.</param>
        /// <param name="input">The user text input for nodes search.</param>
        public virtual void ShowPrimaryMenu(Vector2 location, bool moveSurface = false, string input = null)
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
                Vector2 delta = Vector2.Min(location - leftPadding, Vector2.Zero) +
                                Vector2.Max((location + _activeVisjectCM.Size) - Size, Vector2.Zero);

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
        public virtual void ShowSecondaryCM(Vector2 location, SurfaceControl controlUnderMouse)
        {
            var selection = SelectedNodes;
            if (selection.Count == 0)
                return;

            // Create secondary context menu
            var menu = new FlaxEditor.GUI.ContextMenu.ContextMenu();

            menu.AddButton("Save", _onSave).Enabled = CanEdit;
            menu.AddSeparator();
            _cmCopyButton = menu.AddButton("Copy", Copy);
            menu.AddButton("Paste", Paste).Enabled = CanEdit && CanPaste();
            _cmDuplicateButton = menu.AddButton("Duplicate", Duplicate);
            _cmDuplicateButton.Enabled = CanEdit;
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

            _cmFormatNodesConnectionButton = menu.AddButton("Format node(s)", () =>
            {
                FormatGraph(SelectedNodes);
            });
            _cmFormatNodesConnectionButton.Enabled = HasNodesSelection;

            menu.AddSeparator();
            _cmRemoveNodeConnectionsButton = menu.AddButton("Remove all connections to that node(s)", () =>
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
            _cmRemoveNodeConnectionsButton.Enabled = CanEdit;
            _cmRemoveBoxConnectionsButton = menu.AddButton("Remove all connections to that box", () =>
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
            }

            controlUnderMouse?.OnShowSecondaryContextMenu(menu, controlUnderMouse.PointFromParent(location));

            OnShowSecondaryContextMenu(menu, controlUnderMouse);

            // Show secondary context menu
            _cmStartPos = location;
            menu.Tag = selection;
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

                Vector2 padding = new Vector2(20);
                Vector2 delta = Vector2.Min(_rootControl.PointToParent(nextBox.ParentNode.Location) - padding, Vector2.Zero) +
                                Vector2.Max((_rootControl.PointToParent(nextBox.ParentNode.BottomRight) + padding) - Size, Vector2.Zero);

                _rootControl.Location -= delta;
            }
        }
    }
}
