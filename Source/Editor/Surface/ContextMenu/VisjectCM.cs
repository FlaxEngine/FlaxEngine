// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

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
        /// <param name="groupArch">The nodes group archetype to check.</param>
        /// <param name="arch">The node archetype to check.</param>
        /// <returns>True if can use this node to spawn it on a surface, otherwise false..</returns>
        public delegate bool NodeSpawnCheckDelegate(GroupArchetype groupArch, NodeArchetype arch);

        /// <summary>
        /// Visject Surface parameters getter delegate.
        /// </summary>
        /// <returns>TThe list of surface parameters or null if failed (readonly).</returns>
        public delegate List<SurfaceParameter> ParameterGetterDelegate();

        private const float DefaultWidth = 300;
        private const float DefaultHeight = 400;

        private readonly List<VisjectCMGroup> _groups = new List<VisjectCMGroup>(16);
        private CheckBox _contextSensitiveToggle;
        private bool _contextSensitiveSearchEnabled = true;
        private readonly TextBox _searchBox;
        private bool _waitingForInput;
        private VisjectCMGroup _surfaceParametersGroup;
        private Panel _panel1;
        private VerticalPanel _groupsPanel;
        private readonly ParameterGetterDelegate _parametersGetter;
        private Elements.Box _selectedBox;
        private NodeArchetype _parameterGetNodeArchetype;
        private NodeArchetype _parameterSetNodeArchetype;

        // Description panel elements
        private readonly bool _useDescriptionPanel;
        private bool _descriptionPanelVisible;
        private readonly Panel _descriptionPanel;
        private readonly Panel _descriptionPanelSeparator;
        private readonly Image _descriptionDeclaringClassImage;
        private readonly Label _descriptionSignatureLabel;
        private readonly Label _descriptionLabel;
        private readonly VerticalPanel _descriptionInputPanel;
        private readonly VerticalPanel _descriptionOutputPanel;
        private readonly SurfaceStyle _surfaceStyle;
        private VisjectCMItem _selectedItem;

        /// <summary>
        /// The selected item
        /// </summary>
        public VisjectCMItem SelectedItem
        {
            get => _selectedItem;
            private set
            {
                _selectedItem = value;
                _selectedItem?.OnSelect();
            }
        }

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
            /// True if the surface should make use of a description panel drawn at the bottom of the context menu
            /// </summary>
            public bool UseDescriptionPanel;

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

            /// <summary>
            /// The surface style to use to draw images in the description panel
            /// </summary>
            public SurfaceStyle Style;
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
            _useDescriptionPanel = info.UseDescriptionPanel;
            _surfaceStyle = info.Style;

            // Context menu dimensions
            Size = new Float2(_useDescriptionPanel ? DefaultWidth + 50f : DefaultWidth, DefaultHeight);

            var headerPanel = new Panel(ScrollBars.None)
            {
                Parent = this,
                Height = 20,
                Width = Width - 4,
                X = 2,
                Y = 1,
            };

            // Title bar
            var titleFontReference = new FontReference(Style.Current.FontLarge.Asset, 10);
            var titleLabel = new Label
            {
                Width = Width * 0.5f - 8f,
                Height = 20,
                X = 4,
                Parent = headerPanel,
                Text = "Select Node",
                HorizontalAlignment = TextAlignment.Near,
                Font = titleFontReference,
            };

            // Context sensitive toggle
            var contextSensitiveLabel = new Label
            {
                Width = Width * 0.5f - 28,
                Height = 20,
                X = Width * 0.5f,
                Parent = headerPanel,
                Text = "Context Sensitive",
                TooltipText = "Should the nodes be filtered to only show those that can be connected in the current context?",
                HorizontalAlignment = TextAlignment.Far,
                Font = titleFontReference,
            };

            _contextSensitiveToggle = new CheckBox
            {
                Width = 20,
                Height = 20,
                X = Width - 24,
                Parent = headerPanel,
                Checked = _contextSensitiveSearchEnabled,
            };
            _contextSensitiveToggle.StateChanged += OnContextSensitiveToggleStateChanged;

            // Search box
            _searchBox = new SearchBox(false, 2, 22)
            {
                Width = Width - 4,
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
                Pivot = Float2.Zero,
                IsScrollable = true,
            };
            _groupsPanel = panel2;

            // Create description panel elements only when description panel is about to be used
            if (_useDescriptionPanel)
            {
                _descriptionPanel = new Panel(ScrollBars.None)
                {
                    Parent = this,
                    Bounds = new Rectangle(0, Height, Width, 0),
                    BackgroundColor = Style.Current.BackgroundNormal,
                };

                _descriptionDeclaringClassImage = new Image(8, 12, 20, 20)
                {
                    Parent = _descriptionPanel,
                    Brush = new SpriteBrush(info.Style.Icons.BoxClose),
                };

                var descriptionFontReference = new FontReference(Style.Current.FontMedium.Asset, 9f);
                _descriptionSignatureLabel = new Label(32, 8, Width - 40, 0)
                {
                    Parent = _descriptionPanel,
                    HorizontalAlignment = TextAlignment.Near,
                    VerticalAlignment = TextAlignment.Near,
                    Wrapping = TextWrapping.WrapWords,
                    Font = descriptionFontReference,
                    Bold = true,
                    AutoHeight = true,
                };
                _descriptionSignatureLabel.SetAnchorPreset(AnchorPresets.TopLeft, true);

                _descriptionLabel = new Label(32, 0, Width - 40, 0)
                {
                    Parent = _descriptionPanel,
                    HorizontalAlignment = TextAlignment.Near,
                    VerticalAlignment = TextAlignment.Near,
                    Wrapping = TextWrapping.WrapWords,
                    Font = descriptionFontReference,
                    AutoHeight = true,
                };
                _descriptionLabel.SetAnchorPreset(AnchorPresets.TopLeft, true);

                _descriptionPanelSeparator = new Panel(ScrollBars.None)
                {
                    Parent = _descriptionPanel,
                    Bounds = new Rectangle(8, Height, Width - 16, 2),
                    BackgroundColor = Style.Current.BackgroundHighlighted,
                };

                _descriptionInputPanel = new VerticalPanel()
                {
                    Parent = _descriptionPanel,
                    X = 8,
                    Width = Width * 0.5f - 16,
                    AutoSize = true,
                    Pivot = Float2.Zero,
                };

                _descriptionOutputPanel = new VerticalPanel()
                {
                    Parent = _descriptionPanel,
                    X = Width * 0.5f + 8,
                    Width = Width * 0.5f - 16,
                    AutoSize = true,
                    Pivot = Float2.Zero,
                };
            }

            // Init groups
            var nodes = new List<NodeArchetype>();
            foreach (var groupArchetype in info.Groups)
            {
                // Get valid nodes
                nodes.Clear();
                foreach (var nodeArchetype in groupArchetype.Archetypes)
                {
                    if ((nodeArchetype.Flags & NodeFlags.NoSpawnViaGUI) == 0 && info.CanSpawnNode(groupArchetype, nodeArchetype))
                    {
                        nodes.Add(nodeArchetype);
                    }
                }

                // Check if can create group for them
                if (nodes.Count > 0)
                {
                    var group = CreateGroup(groupArchetype);
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
                        if (string.Equals(_groups[j].Name, groupName, StringComparison.OrdinalIgnoreCase))
                        {
                            group = _groups[j];
                            break;
                        }
                    }

                    // Create new group if name is unique
                    if (group == null)
                    {
                        group = CreateGroup(info.CustomNodesGroup, true, groupName);
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
        /// <param name="withGroupMerge">True if merge group items into any existing group of the same name.</param>
        public void AddGroup(GroupArchetype groupArchetype, bool withGroupMerge = true)
        {
            // Check if can create group for them to be spawned via GUI
            if (groupArchetype.Archetypes.Any(x => (x.Flags & NodeFlags.NoSpawnViaGUI) == 0))
            {
                Profiler.BeginEvent("VisjectCM.AddGroup");

                var group = CreateGroup(groupArchetype, withGroupMerge);
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
                    if (_contextSensitiveSearchEnabled && _selectedBox != null)
                        UpdateFilters();
                    else
                        SortGroups();
                    if (ShowExpanded)
                        group.Open(false);
                    group.PerformLayout();
                    if (_searchBox.TextLength != 0)
                    {
                        OnSearchFilterChanged();
                    }
                }
                else if (_contextSensitiveSearchEnabled)
                {
                    group.EvaluateVisibilityWithBox(_selectedBox);
                }

                Profiler.EndEvent();
            }
        }

        /// <summary>
        /// Adds the group archetypes to add to the menu.
        /// </summary>
        /// <param name="groupArchetypes">The groups.</param>
        /// <param name="withGroupMerge">True if merge group items into any existing group of the same name.</param>
        public void AddGroups(IEnumerable<GroupArchetype> groupArchetypes, bool withGroupMerge = true)
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
                    var group = CreateGroup(groupArchetype, withGroupMerge);
                    group.Close(false);

                    foreach (var nodeArchetype in groupArchetype.Archetypes)
                    {
                        var item = new VisjectCMItem(group, groupArchetype, nodeArchetype)
                        {
                            Parent = group
                        };
                    }
                    if (_contextSensitiveSearchEnabled)
                        group.EvaluateVisibilityWithBox(_selectedBox);
                    group.SortChildren();
                    if (ShowExpanded)
                        group.Open(false);
                    group.Parent = _groupsPanel;
                    _groups.Add(group);

                    groups.Add(group);
                }
                Profiler.EndEvent();

                if (!isLayoutLocked)
                {
                    if (_contextSensitiveSearchEnabled && _selectedBox != null)
                        UpdateFilters();
                    else
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
                var group = _groups[i];
                if (group.Archetypes.Remove(groupArchetype))
                {
                    Profiler.BeginEvent("VisjectCM.RemoveGroup");
                    if (group.Archetypes.Count == 0)
                    {
                        _groups.RemoveAt(i);
                        group.Dispose();
                    }
                    else
                    {
                        var children = group.Children.ToArray();
                        foreach (var child in children)
                        {
                            if (child is VisjectCMItem item && item.GroupArchetype == groupArchetype)
                                item.Dispose();
                        }
                    }
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

        private VisjectCMGroup CreateGroup(GroupArchetype groupArchetype, bool withGroupMerge = true, string name = null)
        {
            if (name == null)
                name = groupArchetype.Name;
            if (withGroupMerge)
            {
                for (int i = 0; i < _groups.Count; i++)
                {
                    if (string.Equals(_groups[i].HeaderText, name, StringComparison.Ordinal))
                        return _groups[i];
                }
            }
            return new VisjectCMGroup(this, groupArchetype)
            {
                HeaderText = name
            };
        }

        private void OnSearchFilterChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            Profiler.BeginEvent("VisjectCM.OnSearchFilterChanged");
            UpdateFilters();
            _searchBox.Focus();
            Profiler.EndEvent();
        }

        private void OnContextSensitiveToggleStateChanged(CheckBox checkBox)
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            Profiler.BeginEvent("VisjectCM.OnContextSensitiveToggleStateChanged");
            _contextSensitiveSearchEnabled = checkBox.Checked;
            UpdateFilters();
            Profiler.EndEvent();
        }

        private void UpdateFilters()
        {
            if (string.IsNullOrEmpty(_searchBox.Text) && _selectedBox == null)
            {
                ResetView();
                Profiler.EndEvent();
                return;
            }

            // Update groups
            LockChildrenRecursive();
            var contextSensitiveSelectedBox = _contextSensitiveSearchEnabled ? _selectedBox : null;
            for (int i = 0; i < _groups.Count; i++)
            {
                _groups[i].UpdateFilter(_searchBox.Text, contextSensitiveSelectedBox);
                _groups[i].UpdateItemSort(contextSensitiveSelectedBox);
            }
            SortGroups();
            UnlockChildrenRecursive();

            // If no item is selected (or it's not visible anymore), select the top one
            Profiler.BeginEvent("VisjectCM.Layout");
            SelectedItem = _groups.Find(g => g.Visible)?.Children.Find(c => c.Visible && c is VisjectCMItem) as VisjectCMItem;
            PerformLayout();
            if (SelectedItem != null)
                _panel1.ScrollViewTo(SelectedItem);
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
            {
                _groups[i].ResetView();
                if (_contextSensitiveSearchEnabled)
                    _groups[i].EvaluateVisibilityWithBox(_selectedBox);
            }
            UnlockChildrenRecursive();

            if (_contextSensitiveSearchEnabled && _selectedBox != null)
                UpdateFilters();
            else
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
            int count = parameters?.Count ?? 0;
            if (count > 0)
            {
                // TODO: cache the allocated memory to reduce dynamic allocations
                if (_parameterSetNodeArchetype != null)
                    count *= 2;
                var archetypes = new NodeArchetype[count];
                int archetypeIndex = 0;

                var groupArchetype = new GroupArchetype
                {
                    GroupID = 6,
                    Name = "Surface Parameters",
                    Color = new Color(52, 73, 94),
                    Archetypes = archetypes
                };

                var group = CreateGroup(groupArchetype, false);
                group.ArrowImageOpened = new SpriteBrush(Style.Current.ArrowDown);
                group.ArrowImageClosed = new SpriteBrush(Style.Current.ArrowRight);
                group.Close(false);

                // ReSharper disable once PossibleNullReferenceException
                for (int i = 0; i < parameters.Count; i++)
                {
                    var param = parameters[i];

                    // Define Getter node and create CM item
                    var node = (NodeArchetype)_parameterGetNodeArchetype.Clone();
                    node.Title = "Get " + param.Name;
                    node.DefaultValues[0] = param.ID;
                    archetypes[archetypeIndex++] = node;

                    var item = new VisjectCMItem(group, groupArchetype, node)
                    {
                        Parent = group
                    };

                    // Define Setter node and create CM item if parameter has a setter
                    if (_parameterSetNodeArchetype != null)
                    {
                        node = (NodeArchetype)_parameterSetNodeArchetype.Clone();
                        node.Title = "Set " + param.Name;
                        node.DefaultValues[0] = param.ID;
                        node.DefaultValues[1] = TypeUtils.GetDefaultValue(param.Type);
                        archetypes[archetypeIndex++] = node;

                        item = new VisjectCMItem(group, groupArchetype, node)
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
        public override void Show(Control parent, Float2 location, ContextMenuDirection? direction = null)
        {
            Show(parent, location, null);
        }

        /// <summary>
        /// Show context menu over given control.
        /// </summary>
        /// <param name="parent">Parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        /// <param name="startBox">The currently selected box that the new node will get connected to. Can be null</param>
        public void Show(Control parent, Float2 location, Elements.Box startBox)
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

            if (_useDescriptionPanel)
                HideDescriptionPanel();

            base.Hide();
        }

        /// <summary>
        /// Updates the description panel and shows information about the set archetype
        /// </summary>
        /// <param name="archetype">The node archetype</param>
        public void SetDescriptionPanelArchetype(NodeArchetype archetype)
        {
            if (!_useDescriptionPanel)
                return;

            if (archetype == null || !Editor.Instance.Options.Options.Interface.NodeDescriptionPanel)
            {
                HideDescriptionPanel();
                return;
            }

            Profiler.BeginEvent("VisjectCM.SetDescriptionPanelArchetype");

            _descriptionInputPanel.RemoveChildren();
            _descriptionOutputPanel.RemoveChildren();

            // Fetch description and information from the memberInfo - mainly from nodes that got fetched asynchronously by the visual scripting editor
            ScriptType declaringType;
            if (archetype.Tag is ScriptMemberInfo memberInfo)
            {
                var name = memberInfo.Name;
                if (memberInfo.IsMethod && memberInfo.Name.StartsWith("get_") || memberInfo.Name.StartsWith("set_"))
                {
                    name = memberInfo.Name.Substring(4);
                }

                declaringType = memberInfo.DeclaringType;
                _descriptionSignatureLabel.Text = memberInfo.DeclaringType + "." + name;

                // We have to add the Instance information manually for members that aren't static
                if (!memberInfo.IsStatic)
                    AddInputOutputElement(archetype, declaringType, false, $"Instance ({memberInfo.DeclaringType.Name})");

                // We also have to manually add the Return information as well.
                if (memberInfo.ValueType != ScriptType.Null && memberInfo.ValueType != ScriptType.Void)
                {
                    // When a field has a setter we don't want it to show the input as a return output.
                    if (memberInfo.IsField && archetype.Title.StartsWith("Set "))
                        AddInputOutputElement(archetype, memberInfo.ValueType, false, $"({memberInfo.ValueType.Name})");
                    else
                        AddInputOutputElement(archetype, memberInfo.ValueType, true, $"Return ({memberInfo.ValueType.Name})");
                }

                for (int i = 0; i < memberInfo.ParametersCount; i++)
                {
                    var param = memberInfo.GetParameters()[i];
                    AddInputOutputElement(archetype, param.Type, param.IsOut, $"{param.Name} ({param.Type.Name})");
                }
            }
            else
            {
                // Try to fetch as many informations as possible from the predefined hardcoded nodes
                _descriptionSignatureLabel.Text = string.IsNullOrEmpty(archetype.Signature) ? archetype.Title : archetype.Signature;
                declaringType = archetype.DefaultType;

                // Some nodes are more delicate. Like Arrays or Dictionaries. In this case we let them fetch the inputs/outputs for us.
                // Otherwise, it is not possible to show a proper return type on some nodes.
                if (archetype.GetInputOutputDescription != null)
                {
                    archetype.GetInputOutputDescription.Invoke(archetype, out var inputs, out var outputs);

                    foreach (var input in inputs ?? [])
                    {
                        AddInputOutputElement(archetype, input.Type, false, $"{input.Name} ({input.Type.Name})");
                    }

                    foreach (var output in outputs ?? [])
                    {
                        AddInputOutputElement(archetype, output.Type, true, $"{output.Name} ({output.Type.Name})");
                    }
                }
                else if (archetype.Elements != null) // Skip if no Elements (ex: Comment node)
                {
                    foreach (var element in archetype.Elements)
                    {
                        if (element.Type is not (NodeElementType.Input or NodeElementType.Output))
                            continue;

                        var typeText = element.ConnectionsType ? element.ConnectionsType.Name : archetype.ConnectionsHints.ToString();
                        AddInputOutputElement(archetype, element.ConnectionsType, element.Type == NodeElementType.Output, $"{element.Text} ({typeText})");
                    }
                }
            }

            // Set declaring type icon color
            _surfaceStyle.GetConnectionColor(declaringType, archetype.ConnectionsHints, out var declaringTypeColor);
            _descriptionDeclaringClassImage.Color = declaringTypeColor;
            _descriptionDeclaringClassImage.MouseOverColor = declaringTypeColor;

            // Calculate the description panel height. (I am doing this manually since working with autoSize on horizontal/vertical panels didn't work, especially with nesting - Nils)
            float panelHeight = _descriptionSignatureLabel.Height;

            // If thee is no description we move the signature label down a bit to align it with the icon. Just a cosmetic check
            if (string.IsNullOrEmpty(archetype.Description))
            {
                _descriptionSignatureLabel.Y = 15;
                _descriptionLabel.Text = "";
            }
            else
            {
                _descriptionSignatureLabel.Y = 8;
                _descriptionLabel.Y = _descriptionSignatureLabel.Bounds.Bottom + 6f;
                // Replacing multiple whitespaces with a linebreak for better readability. (Mainly doing this because the ToolTip fetching system replaces linebreaks with whitespaces - Nils)
                _descriptionLabel.Text = Regex.Replace(archetype.Description, @"\s{2,}", "\n");
            }

            // Some padding and moving elements around
            _descriptionPanelSeparator.Y = _descriptionLabel.Bounds.Bottom + 8f;
            panelHeight += _descriptionLabel.Height + 32f;
            _descriptionInputPanel.Y = panelHeight;
            _descriptionOutputPanel.Y = panelHeight;
            panelHeight += Mathf.Max(_descriptionInputPanel.Height, _descriptionOutputPanel.Height);

            // Forcing the description panel to at least have a minimum height to not make the window size change too much in order to reduce jittering
            // TODO: Remove the Mathf.Max and just set the height to panelHeight once the window jitter issue is fixed - Nils
            _descriptionPanel.Height = Mathf.Max(140f, panelHeight);
            Height = DefaultHeight + _descriptionPanel.Height;
            UpdateWindowSize();
            _descriptionPanelVisible = true;

            Profiler.EndEvent();
        }

        private void AddInputOutputElement(NodeArchetype nodeArchetype, ScriptType type, bool isOutput, string text)
        {
            // Using a panel instead of a vertical panel because auto sizing is unreliable - so we are doing this manually here as well
            var elementPanel = new Panel()
            {
                Parent = isOutput ? _descriptionOutputPanel : _descriptionInputPanel,
                Width = Width * 0.5f,
                Height = 16,
                AnchorPreset = AnchorPresets.TopLeft
            };

            _surfaceStyle.GetConnectionColor(type, nodeArchetype.ConnectionsHints, out var typeColor);
            elementPanel.AddChild(new Image(2, 0, 12, 12)
            {
                Brush = new SpriteBrush(_surfaceStyle.Icons.BoxOpen),
                Color = typeColor,
                MouseOverColor = typeColor,
                AutoFocus = false,
            }).SetAnchorPreset(AnchorPresets.TopLeft, true);

            // Forcing the first letter to be capital and removing the '&' char from pointer-references
            text = (char.ToUpper(text[0]) + text.Substring(1)).Replace("&", "");
            var elementText = new Label(16, 0, Width * 0.5f - 32, 16)
            {
                Text = text,
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
                AutoHeight = true,
            };
            elementText.SetAnchorPreset(AnchorPresets.TopLeft, true);
            elementPanel.AddChild(elementText);
            elementPanel.Height = elementText.Height;
        }

        /// <summary>
        /// Hides the description panel and resets the context menu to its original size
        /// </summary>
        private void HideDescriptionPanel()
        {
            if (!_descriptionPanelVisible)
                return;

            _descriptionInputPanel.RemoveChildren();
            _descriptionOutputPanel.RemoveChildren();
            Height = DefaultHeight;
            UpdateWindowSize();
            _descriptionPanelVisible = false;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Escape)
            {
                Hide();
                return true;
            }
            else if (key == KeyboardKeys.Return || key == KeyboardKeys.Tab)
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
                    _panel1.ScrollViewTo(SelectedItem, true);
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

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _contextSensitiveToggle.StateChanged -= OnContextSensitiveToggleStateChanged;
            base.OnDestroy();
        }
    }
}
