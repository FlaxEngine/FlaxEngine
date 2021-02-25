// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.ContextMenu
{
    /// <summary>
    /// The Visject Surface dedicated context menu for nodes spawning.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    [HideInEditor]
    public class VisjectCM : ContextMenuBase
    {
        /// <summary>
        /// Visject context menu item clicked delegate.
        /// </summary>
        /// <param name="clickedItem">The item that was clicked</param>
        /// <param name="selectedBox">The currently user-selected box. Can be null.</param>
        public delegate void ItemClickedDelegate(VisjectCMItem clickedItem, Elements.Box selectedBox);

        /// <summary>
        /// Visject Surface node archetype spawn ability checking delegate.
        /// </summary>
        /// <param name="arch">The node archetype to check.</param>
        /// <returns>True if can use this node to spawn it on a surface, otherwise false..</returns>
        public delegate bool NodeSpawnCheckDelegate(NodeArchetype arch);

        /// <summary>
        /// Visject Surface parameters getter delegate.
        /// </summary>
        /// <returns>TThe list of surface parameters or null if failed (readonly).</returns>
        public delegate List<SurfaceParameter> ParameterGetterDelegate();

        private readonly List<VisjectCMGroup> _groups = new List<VisjectCMGroup>(16);
        private readonly TextBox _searchBox;
        private bool _waitingForInput;
        private VisjectCMGroup _surfaceParametersGroup;
        private Panel _panel1;
        private VerticalPanel _groupsPanel;
        private readonly ParameterGetterDelegate _parametersGetter;
        private Elements.Box _selectedBox;
        private NodeArchetype _parameterGetNodeArchetype;
        private NodeArchetype _parameterSetNodeArchetype;

        /// <summary>
        /// The selected item
        /// </summary>
        public VisjectCMItem SelectedItem;

        /// <summary>
        /// Event fired when any item in this popup menu gets clicked.
        /// </summary>
        public event ItemClickedDelegate ItemClicked;

        /// <summary>
        /// Gets or sets a value indicating whether show groups expanded or collapsed.
        /// </summary>
        public bool ShowExpanded { get; set; }

        /// <summary>
        /// Gets the groups (read-only).
        /// </summary>
        public IList<VisjectCMGroup> Groups => _groups;

        /// <summary>
        /// The surface context menu initialization parameters.
        /// </summary>
        public struct InitInfo
        {
            /// <summary>
            /// True if surface parameters are not read-only and can be modified via setter node.
            /// </summary>
            public bool CanSetParameters;

            /// <summary>
            /// The groups archetypes. Cannot be null.
            /// </summary>
            public List<GroupArchetype> Groups;

            /// <summary>
            /// The custom callback to handle node types validation for spawning. Cannot be null.
            /// </summary>
            public NodeSpawnCheckDelegate CanSpawnNode;

            /// <summary>
            /// The surface parameters getter. Can be null.
            /// </summary>
            public ParameterGetterDelegate ParametersGetter;

            /// <summary>
            /// The group with custom nodes group. Can be null.
            /// </summary>
            public GroupArchetype CustomNodesGroup;

            /// <summary>
            /// The parameter getter node archetype to spawn when adding the parameter getter. Can be null.
            /// </summary>
            public NodeArchetype ParameterGetNodeArchetype;

            /// <summary>
            /// The parameter setter node archetype to spawn when adding the parameter getter. Can be null.
            /// </summary>
            public NodeArchetype ParameterSetNodeArchetype;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectCM"/> class.
        /// </summary>
        /// <param name="info">The initialization info data.</param>
        public VisjectCM(InitInfo info)
        {
            if (info.Groups == null)
                throw new ArgumentNullException(nameof(info.Groups));
            if (info.CanSpawnNode == null)
                throw new ArgumentNullException(nameof(info.CanSpawnNode));
            _parametersGetter = info.ParametersGetter;
            _parameterGetNodeArchetype = info.ParameterGetNodeArchetype ?? Archetypes.Parameters.Nodes[0];
            if (info.CanSetParameters)
                _parameterSetNodeArchetype = info.ParameterSetNodeArchetype ?? Archetypes.Parameters.Nodes[3];

            // Context menu dimensions
            Size = new Vector2(320, 220);

            // Search box
            _searchBox = new TextBox(false, 1, 1)
            {
                Width = Width - 3,
                WatermarkText = "Search...",
                Parent = this
            };
            _searchBox.TextChanged += OnSearchFilterChanged;

            // Create first panel (for scrollbar)
            var panel1 = new Panel(ScrollBars.Vertical)
            {
                Bounds = new Rectangle(0, _searchBox.Bottom + 1, Width, Height - _searchBox.Bottom - 2),
                Parent = this
            };

            _panel1 = panel1;

            // Create second panel (for groups arrangement)
            var panel2 = new VerticalPanel
            {
                Parent = panel1,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
            };
            _groupsPanel = panel2;

            // Init groups
            var nodes = new List<NodeArchetype>();
            foreach (var groupArchetype in info.Groups)
            {
                // Get valid nodes
                nodes.Clear();
                foreach (var nodeArchetype in groupArchetype.Archetypes)
                {
                    if ((nodeArchetype.Flags & NodeFlags.NoSpawnViaGUI) == 0 && info.CanSpawnNode(nodeArchetype))
                    {
                        nodes.Add(nodeArchetype);
                    }
                }

                // Check if can create group for them
                if (nodes.Count > 0)
                {
                    var group = new VisjectCMGroup(this, groupArchetype)
                    {
                        HeaderText = groupArchetype.Name
                    };

                    group.Close(false);
                    for (int i = 0; i < nodes.Count; i++)
                    {
                        var item = new VisjectCMItem(group, groupArchetype, nodes[i])
                        {
                            Parent = group
                        };
                    }
                    group.SortChildren();
                    group.Parent = panel2;

                    _groups.Add(group);
                }
            }

            // Add custom nodes (special handling)
            if (info.CustomNodesGroup?.Archetypes != null)
            {
                foreach (var nodeArchetype in info.CustomNodesGroup.Archetypes)
                {
                    if ((nodeArchetype.Flags & NodeFlags.NoSpawnViaGUI) != 0)
                        continue;

                    var groupName = Archetypes.Custom.GetNodeGroup(nodeArchetype);

                    // Find group to reuse
                    VisjectCMGroup group = null;
                    for (int j = 0; j < _groups.Count; j++)
                    {
                        if (string.Equals(_groups[j].Archetype.Name, groupName, StringComparison.OrdinalIgnoreCase))
                        {
                            group = _groups[j];
                            break;
                        }
                    }

                    // Create new group if name is unique
                    if (group == null)
                    {
                        group = new VisjectCMGroup(this, info.CustomNodesGroup)
                        {
                            HeaderText = groupName
                        };

                        group.Close(false);
                        group.Parent = _groupsPanel;
                        _groups.Add(group);
                    }

                    // Add new item
                    var item = new VisjectCMItem(group, info.CustomNodesGroup, nodeArchetype)
                    {
                        Parent = group
                    };

                    // Order items
                    group.SortChildren();
                }
            }
        }

        /// <summary>
        /// Adds the group archetype to add to the menu.
        /// </summary>
        /// <param name="groupArchetype">The group.</param>
        public void AddGroup(GroupArchetype groupArchetype)
        {
            // Check if can create group for them to be spawned via GUI
            if (groupArchetype.Archetypes.Any(x => (x.Flags & NodeFlags.NoSpawnViaGUI) == 0))
            {
                Profiler.BeginEvent("VisjectCM.AddGroup");

                var group = new VisjectCMGroup(this, groupArchetype)
                {
                    HeaderText = groupArchetype.Name
                };

                group.Close(false);

                foreach (var nodeArchetype in groupArchetype.Archetypes)
                {
                    var item = new VisjectCMItem(group, groupArchetype, nodeArchetype)
                    {
                        Parent = group
                    };
                }
                group.SortChildren();
                group.Parent = _groupsPanel;

                _groups.Add(group);

                if (!IsLayoutLocked)
                {
                    group.UnlockChildrenRecursive();
                    SortGroups();
                    group.PerformLayout();
                    if (_searchBox.TextLength != 0)
                    {
                        OnSearchFilterChanged();
                    }
                }

                Profiler.EndEvent();
            }
        }

        /// <summary>
        /// Adds the group archetypes to add to the menu.
        /// </summary>
        /// <param name="groupArchetypes">The groups.</param>
        public void AddGroups(IEnumerable<GroupArchetype> groupArchetypes)
        {
            // Check if can create group for them to be spawned via GUI
            if (groupArchetypes.Any(g => g.Archetypes.Any(x => (x.Flags & NodeFlags.NoSpawnViaGUI) == 0)))
            {
                Profiler.BeginEvent("VisjectCM.AddGroups");
                var isLayoutLocked = IsLayoutLocked;
                LockChildrenRecursive();

                Profiler.BeginEvent("Create Groups");
                var groups = new List<VisjectCMGroup>();
                foreach (var groupArchetype in groupArchetypes)
                {
                    var group = new VisjectCMGroup(this, groupArchetype)
                    {
                        HeaderText = groupArchetype.Name
                    };

                    group.Close(false);

                    foreach (var nodeArchetype in groupArchetype.Archetypes)
                    {
                        var item = new VisjectCMItem(group, groupArchetype, nodeArchetype)
                        {
                            Parent = group
                        };
                    }
                    group.SortChildren();
                    group.Parent = _groupsPanel;
                    _groups.Add(group);

                    groups.Add(group);
                }
                Profiler.EndEvent();

                if (!isLayoutLocked)
                {
                    SortGroups();
                    Profiler.BeginEvent("Perform Layout");
                    UnlockChildrenRecursive();
                    foreach (var group in groups)
                        group.PerformLayout();
                    PerformLayout();
                    Profiler.EndEvent();

                    if (_searchBox.TextLength != 0)
                    {
                        OnSearchFilterChanged();
                    }
                }

                Profiler.EndEvent();
            }
        }

        /// <summary>
        /// Removes the group archetype to from to the menu.
        /// </summary>
        /// <param name="groupArchetype">The group.</param>
        public void RemoveGroup(GroupArchetype groupArchetype)
        {
            for (int i = 0; i < _groups.Count; i++)
            {
                if (_groups[i].Archetype == groupArchetype)
                {
                    Profiler.BeginEvent("VisjectCM.RemoveGroup");
                    _groups[i].Dispose();
                    _groups.RemoveAt(i);
                    Profiler.EndEvent();
                    break;
                }
            }
        }

        /// <summary>
        /// Removes the group to from to the menu.
        /// </summary>
        /// <param name="group">The group.</param>
        public void RemoveGroup(VisjectCMGroup group)
        {
            Profiler.BeginEvent("VisjectCM.RemoveGroup");
            group.Dispose();
            _groups.Remove(group);
            Profiler.EndEvent();
        }

        private void OnSearchFilterChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            Profiler.BeginEvent("VisjectCM.OnSearchFilterChanged");

            // Update groups
            LockChildrenRecursive();
            for (int i = 0; i < _groups.Count; i++)
            {
                _groups[i].UpdateFilter(_searchBox.Text);
                _groups[i].UpdateItemSort(_selectedBox);
            }
            SortGroups();
            UnlockChildrenRecursive();

            // If no item is selected (or it's not visible anymore), select the top one
            Profiler.BeginEvent("VisjectCM.Layout");
            if (SelectedItem == null || !SelectedItem.VisibleInHierarchy)
                SelectedItem = _groups.Find(g => g.Visible)?.Children.Find(c => c.Visible && c is VisjectCMItem) as VisjectCMItem;
            if (SelectedItem != null)
                _panel1.ScrollViewTo(SelectedItem);
            PerformLayout();
            _searchBox.Focus();
            Profiler.EndEvent();

            Profiler.EndEvent();
        }

        /// <summary>
        /// Sort the groups and keeps <see cref="_groups"/> in sync
        /// </summary>
        private void SortGroups()
        {
            Profiler.BeginEvent("VisjectCM.SortGroups");

            // Sort groups
            _groupsPanel.SortChildren();

            // Synchronize with _groups[]
            for (int i = 0, groupsIndex = 0; i < _groupsPanel.ChildrenCount; i++)
            {
                if (_groupsPanel.Children[i] is VisjectCMGroup group)
                {
                    _groups[groupsIndex] = group;
                    groupsIndex++;
                }
            }

            Profiler.EndEvent();
        }

        /// <summary>
        /// Called when user clicks on an item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void OnClickItem(VisjectCMItem item)
        {
            Hide();
            ItemClicked?.Invoke(item, _selectedBox);
        }

        /// <summary>
        /// Expands all the groups.
        /// </summary>
        /// <param name="animate">Enable/disable animation feature.</param>
        public void ExpandAll(bool animate = false)
        {
            for (int i = 0; i < _groups.Count; i++)
                _groups[i].Open(animate);
        }

        /// <summary>
        /// Resets the view.
        /// </summary>
        public void ResetView()
        {
            Profiler.BeginEvent("VisjectCM.ResetView");

            LockChildrenRecursive();
            _searchBox.Clear();
            SelectedItem = null;
            for (int i = 0; i < _groups.Count; i++)
                _groups[i].ResetView();
            UnlockChildrenRecursive();

            SortGroups();
            PerformLayout();

            Profiler.EndEvent();
        }

        /// <summary>
        /// Updates the surface parameters group.
        /// </summary>
        private void UpdateSurfaceParametersGroup()
        {
            Profiler.BeginEvent("VisjectCM.UpdateSurfaceParametersGroup");

            // Remove the old one
            if (_surfaceParametersGroup != null)
            {
                _groups.Remove(_surfaceParametersGroup);
                _surfaceParametersGroup.Dispose();
                _surfaceParametersGroup = null;
            }

            // Check if surface has any parameters
            var parameters = _parametersGetter?.Invoke();
            int count = parameters?.Count(x => x.IsPublic) ?? 0;
            if (count > 0)
            {
                // TODO: cache the allocated memory to reduce dynamic allocations
                if (_parameterSetNodeArchetype != null)
                    count *= 2;
                var archetypes = new NodeArchetype[count];
                int archetypeIndex = 0;

                // ReSharper disable once PossibleNullReferenceException
                for (int i = 0; i < parameters.Count; i++)
                {
                    var param = parameters[i];
                    if (!param.IsPublic)
                        continue;

                    var node = (NodeArchetype)_parameterGetNodeArchetype.Clone();
                    node.Title = "Get " + param.Name;
                    node.DefaultValues[0] = param.ID;
                    archetypes[archetypeIndex++] = node;

                    if (_parameterSetNodeArchetype != null)
                    {
                        node = (NodeArchetype)_parameterSetNodeArchetype.Clone();
                        node.Title = "Set " + param.Name;
                        node.DefaultValues[0] = param.ID;
                        node.DefaultValues[1] = TypeUtils.GetDefaultValue(param.Type);
                        archetypes[archetypeIndex++] = node;
                    }
                }

                var groupArchetype = new GroupArchetype
                {
                    GroupID = 6,
                    Name = "Surface Parameters",
                    Color = new Color(52, 73, 94),
                    Archetypes = archetypes
                };

                var group = new VisjectCMGroup(this, groupArchetype)
                {
                    HeaderText = groupArchetype.Name
                };

                group.Close(false);
                archetypeIndex = 0;
                for (int i = 0; i < parameters.Count; i++)
                {
                    var param = parameters[i];
                    if (!param.IsPublic)
                        continue;

                    var item = new VisjectCMItem(group, groupArchetype, archetypes[archetypeIndex++])
                    {
                        Parent = group
                    };

                    if (_parameterSetNodeArchetype != null)
                    {
                        item = new VisjectCMItem(group, groupArchetype, archetypes[archetypeIndex++])
                        {
                            Parent = group
                        };
                    }
                }
                group.SortChildren();
                group.UnlockChildrenRecursive();
                group.Parent = _groupsPanel;
                _groups.Add(group);
                _surfaceParametersGroup = group;
            }

            Profiler.EndEvent();
        }

        /// <inheritdoc />
        public override void Show(Control parent, Vector2 location)
        {
            Show(parent, location, null);
        }

        /// <summary>
        /// Show context menu over given control.
        /// </summary>
        /// <param name="parent">Parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        /// <param name="startBox">The currently selected box that the new node will get connected to. Can be null</param>
        public void Show(Control parent, Vector2 location, Elements.Box startBox)
        {
            _selectedBox = startBox;
            base.Show(parent, location);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Prepare
            UpdateSurfaceParametersGroup();
            ResetView();
            _panel1.VScrollBar.TargetValue = 0;
            Focus();
            _waitingForInput = true;

            base.OnShow();
        }

        /// <inheritdoc />
        public override void Hide()
        {
            Focus(null);

            base.Hide();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Escape)
            {
                Hide();
                return true;
            }
            else if (key == KeyboardKeys.Return)
            {
                if (SelectedItem != null)
                    OnClickItem(SelectedItem);
                else
                    Hide();
                return true;
            }
            else if (key == KeyboardKeys.ArrowUp)
            {
                if (SelectedItem == null)
                    return true;

                var previousSelectedItem = GetPreviousSiblings<VisjectCMItem>(SelectedItem).FirstOrDefault(c => c.Visible) ??
                                           (GetPreviousSiblings<VisjectCMGroup>(SelectedItem.Group).FirstOrDefault(c => c.Visible)?.Children
                                                                                                   .FindLast(c => c.Visible && c is VisjectCMItem) as VisjectCMItem);

                if (previousSelectedItem != null)
                {
                    SelectedItem = previousSelectedItem;

                    // Scroll into view (without smoothing)
                    _panel1.VScrollBar.SmoothingScale = 0;
                    _panel1.ScrollViewTo(SelectedItem);
                    _panel1.VScrollBar.SmoothingScale = 1;
                }
                return true;
            }
            else if (key == KeyboardKeys.ArrowDown)
            {
                if (SelectedItem == null)
                    return true;

                var nextSelectedItem = GetNextSiblings<VisjectCMItem>(SelectedItem).FirstOrDefault(c => c.Visible) ??
                                       (GetNextSiblings<VisjectCMGroup>(SelectedItem.Group).FirstOrDefault(c => c.Visible)?.Children
                                                                                           .OfType<VisjectCMItem>().FirstOrDefault(c => c.Visible));

                if (nextSelectedItem != null)
                {
                    SelectedItem = nextSelectedItem;
                    _panel1.ScrollViewTo(SelectedItem);
                }
                return true;
            }

            if (_waitingForInput)
            {
                _waitingForInput = false;
                _searchBox.Focus();
                return _searchBox.OnKeyDown(key);
            }

            return base.OnKeyDown(key);
        }

        /// <summary>
        /// Gets the next siblings of a control.
        /// </summary>
        /// <param name="item">A control that is attached to a parent</param>
        /// <returns>An <see cref="IEnumerable{Control}"/> with the siblings that come after the current one.</returns>
        private IEnumerable<Control> GetNextSiblings(Control item)
        {
            if (item?.Parent == null)
                yield break;

            var parent = item.Parent;
            for (int i = item.IndexInParent + 1; i < parent.ChildrenCount; i++)
            {
                yield return parent.GetChild(i);
            }
        }

        /// <summary>
        /// Gets the next siblings of a control that have a specific type.
        /// </summary>
        /// <typeparam name="T">The type that the controls should have.</typeparam>
        /// <param name="item">A control that is attached to a parent</param>
        /// <returns>An <see cref="IEnumerable{T}"/> with the siblings that come after the current one.</returns>
        private IEnumerable<T> GetNextSiblings<T>(Control item) where T : Control
        {
            return GetNextSiblings(item).OfType<T>();
        }

        /// <summary>
        /// Gets the previous siblings of a control.
        /// </summary>
        /// <param name="item">A control that is attached to a parent</param>
        /// <returns>An <see cref="IEnumerable{Control}"/> with the siblings that come before the current one.</returns>
        private IEnumerable<Control> GetPreviousSiblings(Control item)
        {
            if (item?.Parent == null)
                yield break;

            var parent = item.Parent;
            for (int i = item.IndexInParent - 1; i >= 0; i--)
            {
                yield return parent.GetChild(i);
            }
        }

        /// <summary>
        /// Gets the previous sibling of a control that have a specific type.
        /// </summary>
        /// <typeparam name="T">The type that the controls should have.</typeparam>
        /// <param name="item">A control that is attached to a parent</param>
        /// <returns>An <see cref="IEnumerable{T}"/> with the siblings that come before the current one.</returns>
        private IEnumerable<T> GetPreviousSiblings<T>(Control item) where T : Control
        {
            return GetPreviousSiblings(item).OfType<T>();
        }
    }
}
