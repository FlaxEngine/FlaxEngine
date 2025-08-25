// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEditor.Tools;
using FlaxEditor.Tools.Foliage;
using FlaxEditor.Tools.Terrain;
using FlaxEditor.Utilities;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Objects spawning tab. Supports searching actor types and prefabs for spawning them into the level.
    /// </summary>
    /// <seealso cref="Tab" />
    public class SpawnTab : Tab
    {
        private class Item : TreeNode
        {
            private DragData _dragData;
            private List<Rectangle> _highlights;

            public Item(string text, DragData dragData = null)
            : this(text, dragData, SpriteHandle.Invalid)
            {
            }

            private Item(string text, DragData dragData, SpriteHandle icon)
            : base(false, icon, icon)
            {
                Text = text;
                Height = 20;
                TextMargin = new Margin(-5.0f, 2.0f, 2.0f, 2.0f);
                _dragData = dragData;
            }

            public void SetHighlights(List<Rectangle> highlights)
            {
                _highlights = highlights;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw all highlights
                if (_highlights != null)
                {
                    var style = Style.Current;
                    var color = style.ProgressNormal * 0.6f;
                    for (int i = 0; i < _highlights.Count; i++)
                        Render2D.FillRectangle(_highlights[i], color);
                }
            }

            /// <inheritdoc />
            protected override void DoDragDrop()
            {
                if (_dragData != null)
                    DoDragDrop(_dragData);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _dragData = null;
                _highlights = null;

                base.OnDestroy();
            }
        }

        private sealed class ScriptTypeItem : Item
        {
            private ScriptType _type;

            public ScriptTypeItem(string text, ScriptType type, DragData dragData = null)
            : base(text, dragData)
            {
                _type = type;
            }

            protected override bool ShowTooltip => true;

            public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
            {
                if (TooltipText == null)
                    TooltipText = Editor.Instance.CodeDocs.GetTooltip(_type);
                return base.OnShowTooltip(out text, out location, out area);
            }
        }

        [Flags]
        private enum SearchFilter
        {
            UI = 1,
            Actors = 2,
            Primitives = 4,
            [HideInEditor]
            Default = UI | Actors | Primitives,
        }

        private TextBox _searchBox;
        private ContainerControl _groupSearch;
        private Tabs _actorGroups;
        private ContainerControl groupPrimitives;
        private Button _viewDropdown;
        private int _searchFilterMask = (int)SearchFilter.Default;

        /// <summary>
        /// The editor instance.
        /// </summary>
        public readonly Editor Editor;

        /// <summary>
        /// Initializes a new instance of the <see cref="SpawnTab"/> class.
        /// </summary>
        /// <param name="icon">The icon.</param>
        /// <param name="editor">The editor instance.</param>
        public SpawnTab(SpriteHandle icon, Editor editor)
        : base(string.Empty, icon)
        {
            Editor = editor;
            Selected += tab => Editor.Windows.EditWin.Viewport.Gizmos.SetActiveMode<TransformGizmoMode>();
            ScriptsBuilder.ScriptsReload += OnScriptsReload;
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;

            _actorGroups = new Tabs
            {
                Orientation = Orientation.Vertical,
                UseScroll = true,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(90, 32),
                Parent = this,
            };

            _groupSearch = CreateGroupWithList(_actorGroups, "Search", 26);

            _viewDropdown = new Button(2, 2, 45.0f, TextBoxBase.DefaultHeight)
            {
                TooltipText = "Change search filter options.",
                Text = "Filters",
                Parent = _groupSearch.Parent.Parent,
            };
            _viewDropdown.Clicked += OnViewButtonClicked;

            _searchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Parent = _groupSearch.Parent.Parent,
                Bounds = new Rectangle(_viewDropdown.Right + 2, 2, _actorGroups.Width - 4, TextBoxBase.DefaultHeight),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            RefreshActorTabs();

            _actorGroups.SelectedTabIndex = 1;
        }

        private void OnViewButtonClicked()
        {
            var menu = new ContextMenu();
            AddSearchFilterButton(menu, SearchFilter.UI, "UI");
            AddSearchFilterButton(menu, SearchFilter.Actors, "Actors");
            AddSearchFilterButton(menu, SearchFilter.Primitives, "Primitives");
            menu.Show(_viewDropdown.Parent, _viewDropdown.BottomLeft);
        }

        private void AddSearchFilterButton(ContextMenu menu, SearchFilter value, string name)
        {
            var button = menu.AddButton(name);
            button.AutoCheck = true;
            button.Checked = (_searchFilterMask & (int)value) != 0;
            button.Clicked += () =>
            {
                _searchFilterMask ^= (int)value;
                OnSearchBoxTextChanged();
            };
        }

        /// <inheritdoc/>
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            _searchBox.Width = _groupSearch.Width - _viewDropdown.Right - 4;
        }

        private void OnScriptsReload()
        {
            // Prevent any references to actor types from the game assemblies that will be reloaded
            _groupSearch.DisposeChildren();
            _groupSearch.PerformLayout();

            // Remove tabs
            var tabs = new List<Tab>();
            foreach (var child in _actorGroups.Children)
            {
                if (child is Tab tab)
                {
                    if (tab.Text != "Search")
                        tabs.Add(tab);
                }
            }
            foreach (var tab in tabs)
            {
                var group = _actorGroups.Children.Find(T => T == tab);
                group.Dispose();
            }
        }

        private void OnScriptsReloadEnd()
        {
            RefreshActorTabs();
            OnSearchBoxTextChanged();
        }

        private void RefreshActorTabs()
        {
            // Remove tabs
            var tabs = new List<Tab>();
            foreach (var child in _actorGroups.Children)
            {
                if (child is Tab tab)
                {
                    if (tab.Text != "Search")
                        tabs.Add(tab);
                }
            }
            foreach (var tab in tabs)
            {
                var group = _actorGroups.Children.Find(T => T == tab);
                group.Dispose();
            }

            // Add primitives to primtives and search tab
            groupPrimitives = CreateGroupWithList(_actorGroups, "Primitives");

            groupPrimitives.AddChild(CreateEditorAssetItem("Cube", "Primitives/Cube.flax"));
            _groupSearch.AddChild(CreateEditorAssetItem("Cube", "Primitives/Cube.flax"));
            groupPrimitives.AddChild(CreateEditorAssetItem("Sphere", "Primitives/Sphere.flax"));
            _groupSearch.AddChild(CreateEditorAssetItem("Sphere", "Primitives/Sphere.flax"));
            groupPrimitives.AddChild(CreateEditorAssetItem("Plane", "Primitives/Plane.flax"));
            _groupSearch.AddChild(CreateEditorAssetItem("Plane", "Primitives/Plane.flax"));
            groupPrimitives.AddChild(CreateEditorAssetItem("Cylinder", "Primitives/Cylinder.flax"));
            _groupSearch.AddChild(CreateEditorAssetItem("Cylinder", "Primitives/Cylinder.flax"));
            groupPrimitives.AddChild(CreateEditorAssetItem("Cone", "Primitives/Cone.flax"));
            _groupSearch.AddChild(CreateEditorAssetItem("Cone", "Primitives/Cone.flax"));
            groupPrimitives.AddChild(CreateEditorAssetItem("Capsule", "Primitives/Capsule.flax"));
            _groupSearch.AddChild(CreateEditorAssetItem("Capsule", "Primitives/Capsule.flax"));

            // Created first to order specific tabs
            CreateGroupWithList(_actorGroups, "Lights");
            CreateGroupWithList(_actorGroups, "Visuals");
            CreateGroupWithList(_actorGroups, "Physics");
            CreateGroupWithList(_actorGroups, "GUI");
            CreateGroupWithList(_actorGroups, "Other");

            // Add control types to tabs
            foreach (var controlType in Editor.Instance.CodeEditing.Controls.Get())
            {
                if (controlType.IsAbstract)
                    continue;
                _groupSearch.AddChild(CreateControlItem(Utilities.Utils.GetPropertyNameUI(controlType.Name), controlType));
                ActorToolboxAttribute attribute = null;
                foreach (var e in controlType.GetAttributes(false))
                {
                    if (e is ActorToolboxAttribute actorToolboxAttribute)
                    {
                        attribute = actorToolboxAttribute;
                        break;
                    }
                }
                if (attribute == null)
                    continue;
                var groupName = attribute.Group.Trim();

                // Check if tab already exists and add it to the tab
                var actorTabExists = false;
                foreach (var child in _actorGroups.Children)
                {
                    if (child is Tab tab)
                    {
                        if (string.Equals(tab.Text, groupName, StringComparison.OrdinalIgnoreCase))
                        {
                            var tree = tab.GetChild<Panel>().GetChild<Tree>();
                            if (tree != null)
                            {
                                tree.AddChild(string.IsNullOrEmpty(attribute.Name) ? CreateControlItem(Utilities.Utils.GetPropertyNameUI(controlType.Name), controlType) : CreateControlItem(attribute.Name, controlType));
                                tree.SortChildren();
                            }
                            actorTabExists = true;
                            break;
                        }
                    }
                }
                if (actorTabExists)
                    continue;

                var group = CreateGroupWithList(_actorGroups, groupName);
                group.AddChild(string.IsNullOrEmpty(attribute.Name) ? CreateControlItem(Utilities.Utils.GetPropertyNameUI(controlType.Name), controlType) : CreateControlItem(attribute.Name, controlType));
                group.SortChildren();
            }

            // Add other actor types to respective tab based on attribute
            foreach (var actorType in Editor.CodeEditing.Actors.Get())
            {
                if (actorType.IsAbstract)
                    continue;
                _groupSearch.AddChild(CreateActorItem(Utilities.Utils.GetPropertyNameUI(actorType.Name), actorType));
                ActorToolboxAttribute attribute = null;
                foreach (var e in actorType.GetAttributes(false))
                {
                    if (e is ActorToolboxAttribute actorToolboxAttribute)
                    {
                        attribute = actorToolboxAttribute;
                        break;
                    }
                }
                if (attribute == null)
                    continue;
                var groupName = attribute.Group.Trim();

                // Check if tab already exists and add it to the tab
                var actorTabExists = false;
                foreach (var child in _actorGroups.Children)
                {
                    if (child is Tab tab)
                    {
                        if (string.Equals(tab.Text, groupName, StringComparison.OrdinalIgnoreCase))
                        {
                            var tree = tab.GetChild<Panel>().GetChild<Tree>();
                            if (tree != null)
                            {
                                tree.AddChild(string.IsNullOrEmpty(attribute.Name) ? CreateActorItem(Utilities.Utils.GetPropertyNameUI(actorType.Name), actorType) : CreateActorItem(attribute.Name, actorType));
                                tree.SortChildren();
                            }
                            actorTabExists = true;
                            break;
                        }
                    }
                }
                if (actorTabExists)
                    continue;

                var group = CreateGroupWithList(_actorGroups, groupName);
                group.AddChild(string.IsNullOrEmpty(attribute.Name) ? CreateActorItem(Utilities.Utils.GetPropertyNameUI(actorType.Name), actorType) : CreateActorItem(attribute.Name, actorType));
                group.SortChildren();
            }
            _groupSearch.SortChildren();
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            var filterText = _searchBox.Text;
            _groupSearch.LockChildrenRecursive();
            _groupSearch.DisposeChildren();

            if (((int)SearchFilter.Actors & _searchFilterMask) != 0)
            {
                foreach (var actorType in Editor.CodeEditing.Actors.Get())
                {
                    ActorToolboxAttribute attribute = null;
                    foreach (var e in actorType.GetAttributes(false))
                    {
                        if (e is ActorToolboxAttribute actorToolboxAttribute)
                        {
                            attribute = actorToolboxAttribute;
                            break;
                        }
                    }

                    var text = (attribute == null) ? actorType.Name : string.IsNullOrEmpty(attribute.Name) ? actorType.Name : attribute.Name;

                    // Display all actors on no search
                    if (string.IsNullOrEmpty(filterText))
                        _groupSearch.AddChild(CreateActorItem(Utilities.Utils.GetPropertyNameUI(text), actorType));

                    if (!QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
                        continue;

                    var item = CreateActorItem(Utilities.Utils.GetPropertyNameUI(text), actorType);
                    SearchFilterHighlights(item, text, ranges);
                }
            }

            if (((int)SearchFilter.Primitives & _searchFilterMask) != 0)
            {
                // Hack primitive models into the search results
                foreach (var child in groupPrimitives.Children)
                {
                    if (child is Item primitiveAssetItem)
                    {
                        var text = primitiveAssetItem.Text;

                        // Rebuild the path based on item name (it would be better to convert the drag data back to a string somehow)
                        string path = $"Primitives/{text}.flax";

                        // Display all primitives on no search
                        if (string.IsNullOrEmpty(filterText))
                            _groupSearch.AddChild(CreateEditorAssetItem(text, path));

                        if (!QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
                            continue;

                        var item = CreateEditorAssetItem(text, path);
                        SearchFilterHighlights(item, text, ranges);
                    }
                }
            }

            if (((int)SearchFilter.UI & _searchFilterMask) != 0)
            { 
                foreach (var controlType in Editor.Instance.CodeEditing.Controls.Get())
                {
                    if (controlType.IsAbstract)
                        continue;

                    ActorToolboxAttribute attribute = null;
                    foreach (var e in controlType.GetAttributes(false))
                    {
                        if (e is ActorToolboxAttribute actorToolboxAttribute)
                        {
                            attribute = actorToolboxAttribute;
                            break;
                        }
                    }

                    var text = (attribute == null) ? controlType.Name : string.IsNullOrEmpty(attribute.Name) ? controlType.Name : attribute.Name;

                    // Display all controls on no search
                    if (string.IsNullOrEmpty(filterText))
                        _groupSearch.AddChild(CreateControlItem(Utilities.Utils.GetPropertyNameUI(controlType.Name), controlType));

                    if (!QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
                        continue;

                    var item = CreateControlItem(Utilities.Utils.GetPropertyNameUI(controlType.Name), controlType);
                    SearchFilterHighlights(item, text, ranges);
                }
            }

            // Sort the search results alphabetically
            _groupSearch.SortChildren();

            _groupSearch.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        /// <inheritdoc/>
        public override void Draw()
        {
            base.Draw();

            // Show a text to hint the user that either no filter is active or the search does not return any results
            bool noSearchResults = _groupSearch.Children.Count == 0 && !string.IsNullOrEmpty(_searchBox.Text);
            bool showHint = _searchFilterMask == 0 || noSearchResults;
            if (showHint)
            {
                string hint = noSearchResults ? "No results" : "No search filter active, please enable at least one filter";
                var textRect = _groupSearch.Parent.Parent.Bounds;
                var style = Style.Current;
                Render2D.DrawText(style.FontMedium, hint, textRect, style.ForegroundGrey, TextAlignment.Center, TextAlignment.Center, TextWrapping.WrapWords);
            }
        }

        private void SearchFilterHighlights(Item item, string text, QueryFilterHelper.Range[] ranges)
        {
            _groupSearch.AddChild(item);
            var highlights = new List<Rectangle>(ranges.Length);
            var font = Style.Current.FontSmall;
            var textRect = item.TextRect;
            for (int i = 0; i < ranges.Length; i++)
            {
                var start = font.GetCharPosition(text, ranges[i].StartIndex);
                var end = font.GetCharPosition(text, ranges[i].EndIndex);
                highlights.Add(new Rectangle(start.X + textRect.X, textRect.Y, end.X - start.X, textRect.Height));
            }
            item.SetHighlights(highlights);
        }

        private Item CreateEditorAssetItem(string name, string path)
        {
            string globalPath = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor", path);
            Item item = new Item(name, GUI.Drag.DragItems.GetDragData(globalPath));
            item.TooltipText = $"{path}\nActor with a {name} asset.";
            return item;
        }

        private Item CreateActorItem(string name, ScriptType type)
        {
            return new ScriptTypeItem(name, type, GUI.Drag.DragActorType.GetDragData(type));
        }

        private Item CreateControlItem(string name, ScriptType type)
        {
            return new ScriptTypeItem(name, type, GUI.Drag.DragControlType.GetDragData(type));
        }

        private ContainerControl CreateGroupWithList(Tabs parentTabs, string title, float topOffset = 0)
        {
            var tab = parentTabs.AddTab(new Tab(title));
            var panel = new Panel(ScrollBars.Both)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, topOffset, 0),
                Parent = tab
            };
            var tree = new Tree(false)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Margin = new Margin(0, 0, 0, panel.ScrollBarsSize),
                IsScrollable = true,
                Parent = panel
            };
            return tree;
        }
    }

    /// <summary>
    /// A helper utility window with bunch of tools used during scene editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public class ToolboxWindow : EditorWindow
    {
        /// <summary>
        /// Gets the tabs control used by this window. Can be used to add custom toolbox modes.
        /// </summary>
        public Tabs TabsControl { get; private set; }

        /// <summary>
        /// The objects spawning tab.
        /// </summary>
        public SpawnTab Spawn;

        /// <summary>
        /// The vertex painting tab.
        /// </summary>
        public VertexPaintingTab VertexPaint;

        /// <summary>
        /// The foliage editing tab.
        /// </summary>
        public FoliageTab Foliage;

        /// <summary>
        /// The terrain editing tab.
        /// </summary>
        public CarveTab Carve;

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolboxWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ToolboxWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Toolbox";
            Icon = editor.Icons.Toolbox96;

            FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            float tabSize = 48 * Editor.Options.Options.Interface.IconsScale;
            TabsControl = new Tabs
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(tabSize, tabSize),
                Parent = this
            };

            TabsControl.AddTab(Spawn = new SpawnTab(Editor.Icons.Toolbox96, Editor));
            TabsControl.AddTab(VertexPaint = new VertexPaintingTab(Editor.Icons.Paint96, Editor));
            TabsControl.AddTab(Foliage = new FoliageTab(Editor.Icons.Foliage96, Editor));
            TabsControl.AddTab(Carve = new CarveTab(Editor.Icons.Terrain96, Editor));

            TabsControl.SelectedTabIndex = 0;
        }
    }
}
