// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// One of the main editor windows used to present workspace content and user scripts.
    /// Provides various functionalities for asset operations.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public sealed partial class ContentWindow : EditorWindow
    {
        private const string ProjectDataLastViewedFolder = "LastViewedFolder";
        private const string ProjectDataExpandedFolders = "ExpandedFolders";
        private bool _isWorkspaceDirty;
        private string _workspaceRebuildLocation;
        private string _lastViewedFolderBeforeReload;
        private SplitPanel _split;
        private Panel _treeOnlyPanel;
        private ContainerControl _treePanelRoot;
        private ContainerControl _treeHeaderPanel;
        private Panel _contentItemsSearchPanel;
        private Panel _contentViewPanel;
        private Panel _contentTreePanel;
        private ContentView _view;

        private readonly ToolStrip _toolStrip;
        private readonly ToolStripButton _importButton;
        private readonly ToolStripButton _createNewButton;
        private readonly ToolStripButton _navigateBackwardButton;
        private readonly ToolStripButton _navigateForwardButton;
        private readonly ToolStripButton _navigateUpButton;

        private NavigationBar _navigationBar;
        private Panel _viewDropdownPanel;
        private Tree _tree;
        private TextBox _foldersSearchBox;
        private TextBox _itemsSearchBox;
        private ViewDropdown _viewDropdown;
        private SortType _sortType;
        private bool _showEngineFiles = true, _showPluginsFiles = true, _showAllFiles = true, _showGeneratedFiles = false;
        private bool _showAllContentInTree;
        private bool _suppressExpandedStateSave;
        private readonly HashSet<string> _expandedFolderPaths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        private bool _renameInTree;

        private RootContentFolderTreeNode _root;

        private bool _navigationUnlocked;
        private readonly Stack<ContentFolderTreeNode> _navigationUndo = new Stack<ContentFolderTreeNode>(32);
        private readonly Stack<ContentFolderTreeNode> _navigationRedo = new Stack<ContentFolderTreeNode>(32);

        private NewItem _newElement;

        /// <summary>
        /// Gets the toolstrip.
        /// </summary>
        public ToolStrip Toolstrip => _toolStrip;

        /// <summary>
        /// Gets the assets view.
        /// </summary>
        public ContentView View => _view;

        internal bool ShowEngineFiles
        {
            get => _showEngineFiles;
            set
            {
                if (_showEngineFiles != value)
                {
                    _showEngineFiles = value;
                    if (Editor.ContentDatabase.Engine != null)
                    {
                        Editor.ContentDatabase.Engine.Visible = value;
                        Editor.ContentDatabase.Engine.Folder.Visible = value;
                        RefreshView();
                        _tree.PerformLayout();
                    }
                }
            }
        }

        internal bool ShowPluginsFiles
        {
            get => _showPluginsFiles;
            set
            {
                if (_showPluginsFiles != value)
                {
                    _showPluginsFiles = value;
                    foreach (var project in Editor.ContentDatabase.Projects)
                    {
                        if (project == Editor.ContentDatabase.Game || project == Editor.ContentDatabase.Engine)
                            continue;
                        project.Visible = value;
                        project.Folder.Visible = value;
                        RefreshView();
                        _tree.PerformLayout();
                    }
                }
            }
        }

        internal bool ShowGeneratedFiles
        {
            get => _showGeneratedFiles;
            set
            {
                if (_showGeneratedFiles != value)
                {
                    _showGeneratedFiles = value;
                    RefreshView();
                }
            }
        }

        internal bool ShowAllFiles
        {
            get => _showAllFiles;
            set
            {
                if (_showAllFiles != value)
                {
                    _showAllFiles = value;
                    RefreshView();
                }
            }
        }

        internal bool IsTreeOnlyMode => _showAllContentInTree;
        internal SortType CurrentSortType => _sortType;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ContentWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Content";
            Icon = editor.Icons.Folder32;
            var style = Style.Current;

            FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);

            var options = Editor.Options;
            options.OptionsChanged += OnOptionsChanged;

            // Toolstrip
            _toolStrip = new ToolStrip(34.0f)
            {
                Parent = this,
            };
            _importButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Import64, () => Editor.ContentImporting.ShowImportFileDialog(CurrentViewFolder)).LinkTooltip("Import content.");
            _createNewButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Add64, OnCreateNewItemButtonClicked).LinkTooltip("Create a new asset. Shift + left click to create a new folder.");
            _toolStrip.AddSeparator();
            _navigateBackwardButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Left64, NavigateBackward).LinkTooltip("Navigate backward.");
            _navigateForwardButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Right64, NavigateForward).LinkTooltip("Navigate forward.");
            _navigateUpButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Up64, NavigateUp).LinkTooltip("Navigate up.");
            _toolStrip.AddSeparator();

            // Split panel
            _split = new SplitPanel(options.Options.Interface.ContentWindowOrientation, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolStrip.Bottom, 0),
                SplitterValue = 0.2f,
                Parent = this,
            };

            // Tree-only panel (used when showing all content in the tree)
            _treeOnlyPanel = new Panel(ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolStrip.Bottom, 0),
                Visible = false,
                Parent = this,
            };

            // Tree host panel
            _treePanelRoot = new ContainerControl
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _split.Panel1,
            };

            // Content structure tree searching query input box
            _treeHeaderPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = style.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 18 + 6),
                Parent = _treePanelRoot,
            };

            _foldersSearchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = _treeHeaderPanel,
                Bounds = new Rectangle(4, 4, _treeHeaderPanel.Width - 8, 18),
            };
            _foldersSearchBox.TextChanged += OnFoldersSearchBoxTextChanged;

            // Content tree panel
            _contentTreePanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _treeHeaderPanel.Bottom, 0),
                IsScrollable = true,
                ScrollBars = ScrollBars.Both,
                Parent = _treePanelRoot,
            };

            // Content structure tree
            _tree = new Tree(true)
            {
                DrawRootTreeLine = false,
                Parent = _contentTreePanel,
            };
            _tree.SelectedChanged += OnTreeSelectionChanged;

            // Content items searching query input box and filters selector
            _contentItemsSearchPanel = new Panel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 8),
                Parent = _split.Panel2,
            };

            _itemsSearchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = _contentItemsSearchPanel,
                Bounds = new Rectangle(4, 4, _contentItemsSearchPanel.Width - 8, 18),
            };
            _itemsSearchBox.TextChanged += UpdateItemsSearch;

            _viewDropdownPanel = new Panel
            {
                Width = 50.0f,
                Parent = this,
                AnchorPreset = AnchorPresets.TopLeft,
                BackgroundColor = Color.Transparent,
            };

            _viewDropdown = new ViewDropdown
            {
                SupportMultiSelect = true,
                TooltipText = "Change content view and filter options",
                Offsets = Margin.Zero,
                Width = 46.0f,
                Height = 18.0f,
                Parent = _viewDropdownPanel,
            };
            _viewDropdown.LocalX += 2.0f;
            _viewDropdown.LocalY += _toolStrip.ItemsHeight * 0.5f - 9.0f;
            _viewDropdown.SelectedIndexChanged += e => UpdateItemsSearch();
            for (int i = 0; i <= (int)ContentItemSearchFilter.Other; i++)
                _viewDropdown.Items.Add(((ContentItemSearchFilter)i).ToString());
            _viewDropdown.PopupCreate += OnViewDropdownPopupCreate;

            // Navigation bar (after view dropdown so layout order stays stable)
            _navigationBar = new NavigationBar
            {
                Parent = _toolStrip,
                ScrollbarTrackColor = style.Background,
                ScrollbarThumbColor = style.ForegroundGrey,
            };

            // Content view panel
            _contentViewPanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _contentItemsSearchPanel.Bottom + 4, 0),
                IsScrollable = true,
                ScrollBars = ScrollBars.Vertical,
                Parent = _split.Panel2,
            };

            // Content View
            _view = new ContentView
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, 0),
                IsScrollable = true,
                Parent = _contentViewPanel,
            };
            _view.OnOpen += Open;
            _view.OnNavigateBack += NavigateBackward;
            _view.OnRename += Rename;
            _view.OnDelete += Delete;
            _view.OnDuplicate += Duplicate;
            _view.OnPaste += Paste;
            _view.ViewScaleChanged += ApplyTreeViewScale;

            _view.InputActions.Add(options => options.Search, () => _itemsSearchBox.Focus());
            InputActions.Add(options => options.Search, () => _itemsSearchBox.Focus());

            LoadExpandedFolders();
            UpdateViewDropdownBounds();
            ApplyTreeViewScale();
        }

        private void OnCreateNewItemButtonClicked()
        {
            if (Input.GetKey(KeyboardKeys.Shift) && CanCreateFolder())
            {
                NewFolder();
                return;
            }

            var menu = new ContextMenu();
            
            InterfaceOptions interfaceOptions = Editor.Instance.Options.Options.Interface;
            bool disableUnavaliable = interfaceOptions.UnavaliableContentCreateOptions == InterfaceOptions.DisabledHidden.Disabled;

            CreateNewFolderMenu(menu, CurrentViewFolder, disableUnavaliable);
            CreateNewModuleMenu(menu, CurrentViewFolder, disableUnavaliable);
            menu.AddSeparator();
            CreateNewContentItemMenu(menu, CurrentViewFolder, false, disableUnavaliable);
            // Hack: Show the menu once to get the direction, then show it above or below the button depending on the direction.
            menu.Show(this, _createNewButton.UpperLeft);
            var direction = menu.Direction;
            menu.Hide();
            bool below = false;
            switch (direction)
            {
                case ContextMenuDirection.RightDown:
                case ContextMenuDirection.LeftDown:
                    below = true;
                    break;
                case ContextMenuDirection.RightUp:
                case ContextMenuDirection.LeftUp:
                    below = false;
                    break;
            }
            menu.Show(this, below ? _createNewButton.BottomLeft : _createNewButton.UpperLeft, direction);
        }

        private ContextMenu OnViewDropdownPopupCreate(ComboBox comboBox)
        {
            var menu = new ContextMenu();

            var viewScale = menu.AddButton("View Scale");
            viewScale.CloseMenuOnClick = false;
            var scaleValue = new FloatValueBox(1, 75, 2, 50.0f, 0.3f, 3.0f, 0.01f)
            {
                Parent = viewScale
            };
            scaleValue.ValueChanged += () => View.ViewScale = scaleValue.Value;
            menu.VisibleChanged += control => { scaleValue.Value = View.ViewScale; };

            var viewType = menu.AddChildMenu("View Type");
            viewType.ContextMenu.AddButton("Tiles", OnViewTypeButtonClicked).Tag = ContentViewType.Tiles;
            viewType.ContextMenu.AddButton("List", OnViewTypeButtonClicked).Tag = ContentViewType.List;
            viewType.ContextMenu.AddButton("Tree View", OnViewTypeButtonClicked).Tag = "Tree";
            viewType.ContextMenu.VisibleChanged += control =>
            {
                if (!control.Visible)
                    return;
                foreach (var item in ((ContextMenu)control).Items)
                {
                    if (item is ContextMenuButton button)
                    {
                        if (button.Tag is ContentViewType type)
                            button.Checked = View.ViewType == type && !_showAllContentInTree;
                        else
                            button.Checked = _showAllContentInTree;
                    }
                }
            };

            var show = menu.AddChildMenu("Show");
            {
                var b = show.ContextMenu.AddButton("File extensions", () =>
                {
                    View.ShowFileExtensions = !View.ShowFileExtensions;
                    if (_showAllContentInTree)
                        UpdateTreeItemNames(_root);
                });
                b.TooltipText = "Shows all files with extensions";
                b.Checked = View.ShowFileExtensions;
                b.CloseMenuOnClick = false;
                b.AutoCheck = true;

                b = show.ContextMenu.AddButton("Engine files", () => ShowEngineFiles = !ShowEngineFiles);
                b.TooltipText = "Shows in-built engine content";
                b.Checked = ShowEngineFiles;
                b.CloseMenuOnClick = false;
                b.AutoCheck = true;

                b = show.ContextMenu.AddButton("Plugins files", () => ShowPluginsFiles = !ShowPluginsFiles);
                b.TooltipText = "Shows plugin projects content";
                b.Checked = ShowPluginsFiles;
                b.CloseMenuOnClick = false;
                b.AutoCheck = true;

                b = show.ContextMenu.AddButton("Generated files", () => ShowGeneratedFiles = !ShowGeneratedFiles);
                b.TooltipText = "Shows generated files";
                b.Checked = ShowGeneratedFiles;
                b.CloseMenuOnClick = false;
                b.AutoCheck = true;

                b = show.ContextMenu.AddButton("All files", () => ShowAllFiles = !ShowAllFiles);
                b.TooltipText = "Shows all files including other than assets and source code";
                b.Checked = ShowAllFiles;
                b.CloseMenuOnClick = false;
                b.AutoCheck = true;
            }

            var filters = menu.AddChildMenu("Filters");
            for (int i = 0; i < _viewDropdown.Items.Count; i++)
            {
                var filterButton = filters.ContextMenu.AddButton(_viewDropdown.Items[i], OnFilterClicked);
                filterButton.CloseMenuOnClick = false;
                filterButton.Tag = i;
            }
            filters.ContextMenu.ButtonClicked += button =>
            {
                foreach (var item in (filters.ContextMenu).Items)
                {
                    if (item is ContextMenuButton filterButton)
                        filterButton.Checked = _viewDropdown.IsSelected(filterButton.Text);
                }
            };
            filters.ContextMenu.VisibleChanged += control =>
            {
                if (!control.Visible)
                    return;
                foreach (var item in ((ContextMenu)control).Items)
                {
                    if (item is ContextMenuButton filterButton)
                        filterButton.Checked = _viewDropdown.IsSelected(filterButton.Text);
                }
            };

            var sortBy = menu.AddChildMenu("Sort by");
            sortBy.ContextMenu.AddButton("Alphabetic Order", OnSortByButtonClicked).Tag = SortType.AlphabeticOrder;
            sortBy.ContextMenu.AddButton("Alphabetic Reverse", OnSortByButtonClicked).Tag = SortType.AlphabeticReverse;
            sortBy.ContextMenu.VisibleChanged += control =>
            {
                if (!control.Visible)
                    return;
                foreach (var item in ((ContextMenu)control).Items)
                {
                    if (item is ContextMenuButton button)
                        button.Checked = _sortType == (SortType)button.Tag;
                }
            };

            return menu;
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            _split.Orientation = options.Interface.ContentWindowOrientation;

            RefreshView();
        }

        private void SetShowAllContentInTree(bool value)
        {
            if (_showAllContentInTree == value)
                return;

            _showAllContentInTree = value;
            ApplyTreeViewMode();
        }

        private void ApplyTreeViewMode()
        {
            if (_treeOnlyPanel == null || _split == null || _treePanelRoot == null)
                return;

            if (_showAllContentInTree)
            {
                _split.Visible = false;
                _treeOnlyPanel.Visible = true;
                _treePanelRoot.Parent = _treeOnlyPanel;
                _treePanelRoot.Offsets = Margin.Zero;
                _contentItemsSearchPanel.Visible = false;
                _itemsSearchBox.Visible = false;
                _contentViewPanel.Visible = false;
                RefreshTreeItems();
            }
            else
            {
                _treeOnlyPanel.Visible = false;
                _split.Visible = true;
                _treePanelRoot.Parent = _split.Panel1;
                _treePanelRoot.Offsets = Margin.Zero;
                _contentItemsSearchPanel.Visible = true;
                _itemsSearchBox.Visible = true;
                _contentViewPanel.Visible = true;
                if (_tree.SelectedNode is ContentItemTreeNode itemNode && itemNode.Parent is TreeNode parentNode)
                    _tree.Select(parentNode);
                if (_root != null)
                    RemoveTreeAssetNodes(_root);
                RefreshView(SelectedNode);
            }

            PerformLayout();
            ApplyTreeViewScale();
            _tree.PerformLayout();
        }

        private void OnViewTypeButtonClicked(ContextMenuButton button)
        {
            if (button.Tag is ContentViewType viewType)
            {
                SetShowAllContentInTree(false);
                View.ViewType = viewType;
            }
            else
            {
                SetShowAllContentInTree(true);
            }
        }

        private void OnFilterClicked(ContextMenuButton filterButton)
        {
            var i = (int)filterButton.Tag;
            _viewDropdown.OnClicked(i);
        }

        private void OnSortByButtonClicked(ContextMenuButton button)
        {
            switch ((SortType)button.Tag)
            {
            case SortType.AlphabeticOrder:
                _sortType = SortType.AlphabeticOrder;
                break;
            case SortType.AlphabeticReverse:
                _sortType = SortType.AlphabeticReverse;
                break;
            }
            RefreshView(SelectedNode);
        }

        /// <summary>
        ///  Enables or disables vertical and horizontal scrolling on the content tree panel
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
        public void ScrollingOnTreeView(bool enabled)
        {
            if (_contentTreePanel.VScrollBar != null)
                _contentTreePanel.VScrollBar.ThumbEnabled = enabled;
            if (_contentTreePanel.HScrollBar != null)
                _contentTreePanel.HScrollBar.ThumbEnabled = enabled;
        }

        /// <summary>
        ///  Enables or disables vertical and horizontal scrolling on the content view panel
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
        public void ScrollingOnContentView(bool enabled)
        {
            if (_contentViewPanel.VScrollBar != null)
                _contentViewPanel.VScrollBar.ThumbEnabled = enabled;
            if (_contentViewPanel.HScrollBar != null)
                _contentViewPanel.HScrollBar.ThumbEnabled = enabled;
        }

        /// <summary>
        /// Shows popup dialog with UI to rename content item.
        /// </summary>
        /// <param name="item">The item to rename.</param>
        /// <returns>The created renaming popup.</returns>
        public void Rename(ContentItem item)
        {
            if (!item.CanRename)
                return;

            // Show element in the view
            Select(item, true);

            // Disable scrolling in proper view
            _renameInTree = _showAllContentInTree;
            if (_renameInTree)
            {
                if (_contentTreePanel.VScrollBar != null)
                    _contentTreePanel.VScrollBar.ThumbEnabled = false;
                if (_contentTreePanel.HScrollBar != null)
                    _contentTreePanel.HScrollBar.ThumbEnabled = false;
                ScrollingOnTreeView(false);
            }
            else
            {
                if (_contentViewPanel.VScrollBar != null)
                    _contentViewPanel.VScrollBar.ThumbEnabled = false;
                if (_contentViewPanel.HScrollBar != null)
                    _contentViewPanel.HScrollBar.ThumbEnabled = false;
                ScrollingOnContentView(false);
            }

            // Show rename popup
            RenamePopup popup;
            if (_renameInTree)
            {
                TreeNode node = null;
                if (item is ContentFolder folder)
                    node = folder.Node;
                else if (item.ParentFolder != null)
                    node = FindTreeItemNode(item.ParentFolder.Node, item);
                if (node == null)
                {
                    // Fallback to content view rename
                    popup = RenamePopup.Show(item, item.TextRectangle, item.ShortName, true);
                }
                else
                {
                    var area = node.TextRect;
                    const float minRenameWidth = 220.0f;
                    if (area.Width < minRenameWidth)
                    {
                        float expand = minRenameWidth - area.Width;
                        area.X -= expand * 0.5f;
                        area.Width = minRenameWidth;
                    }
                    area.Y -= 2;
                    area.Height += 4.0f;
                    popup = RenamePopup.Show(node, area, item.ShortName, true);
                }
            }
            else
            {
                popup = RenamePopup.Show(item, item.TextRectangle, item.ShortName, true);
            }
            popup.Tag = item;
            popup.Validate += OnRenameValidate;
            popup.Renamed += renamePopup => Rename((ContentItem)renamePopup.Tag, renamePopup.Text);
            popup.Closed += OnRenameClosed;

            // For new asset we want to mock the initial value so user can press just Enter to use default name
            if (_newElement != null)
            {
                popup.InitialValue = "?";
            }
        }

        private bool OnRenameValidate(RenamePopup popup, string value)
        {
            return Editor.ContentEditing.IsValidAssetName((ContentItem)popup.Tag, value, out _);
        }

        private void OnRenameClosed(RenamePopup popup)
        {
            // Restore scrolling in proper view
            if (_renameInTree)
            {
                if (_contentTreePanel.VScrollBar != null)
                    _contentTreePanel.VScrollBar.ThumbEnabled = true;
                if (_contentTreePanel.HScrollBar != null)
                    _contentTreePanel.HScrollBar.ThumbEnabled = true;
                ScrollingOnTreeView(true);
            }
            else
            {
                if (_contentViewPanel.VScrollBar != null)
                    _contentViewPanel.VScrollBar.ThumbEnabled = true;
                if (_contentViewPanel.HScrollBar != null)
                    _contentViewPanel.HScrollBar.ThumbEnabled = true;
                ScrollingOnContentView(true);
            }
            _renameInTree = false;

            // Check if was creating new element
            if (_newElement != null)
            {
                // Destroy mock control
                _newElement.ParentFolder = null;
                _newElement.Dispose();
                _newElement = null;
            }
        }

        /// <summary>
        /// Renames the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="newShortName">New name (without extension, just the filename).</param>
        public void Rename(ContentItem item, string newShortName)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Check if can rename this item
            if (!item.CanRename)
            {
                // Cannot
                MessageBox.Show("Cannot rename this item.", "Cannot rename", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Renaming a file to an extension it already has
            if (!item.IsFolder && StringUtils.NormalizeExtension(Path.GetExtension(newShortName)) == StringUtils.NormalizeExtension(Path.GetExtension(item.Path)))
            {
                newShortName = StringUtils.GetPathWithoutExtension(newShortName);
            }

            // Check if name is valid
            if (!Editor.ContentEditing.IsValidAssetName(item, newShortName, out string hint))
            {
                // Invalid name
                MessageBox.Show("Given asset name is invalid. " + hint, "Invalid name", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Ensure has parent
            if (item.ParentFolder == null)
            {
                Editor.LogWarning("Cannot rename root items. " + item.Path);
                return;
            }

            newShortName = newShortName.Trim();

            // Cache data
            string extension = item.IsFolder ? "" : Path.GetExtension(item.Path);
            var newPath = StringUtils.CombinePaths(item.ParentFolder.Path, newShortName + extension);

            // Check if was renaming mock element
            // Note: we create `_newElement` and then rename it to create new asset
            var itemFolder = item.ParentFolder;
            Action<ContentItem> endEvent = null;
            if (_newElement == item)
            {
                try
                {
                    endEvent = (Action<ContentItem>)_newElement.Tag;

                    // Create new asset
                    var proxy = _newElement.Proxy;
                    Editor.Log(string.Format("Creating asset {0} in {1}", proxy.Name, newPath));
                    proxy.Create(newPath, _newElement.Argument);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                    Editor.LogError("Failed to create asset.");
                }
            }
            else
            {
                // Validate state
                Assert.IsNull(_newElement);

                // Rename asset
                Editor.Log(string.Format("Renaming asset {0} to {1}", item.Path, newShortName));
                Editor.ContentDatabase.Move(item, newPath);
            }

            if (_newElement != null)
            {
                // Trigger compilation if need to
                if (_newElement.Proxy is ScriptProxy && Editor.Instance.Options.Options.General.AutoReloadScriptsOnMainWindowFocus)
                    ScriptsBuilder.MarkWorkspaceDirty();

                // Destroy mock control
                _newElement.ParentFolder = null;
                _newElement.Dispose();
                _newElement = null;

                // Focus content window
                Focus();
                RootWindow?.Focus();
            }

            // Refresh database and view now
            Editor.ContentDatabase.RefreshFolder(itemFolder, true);
            RefreshView();
            var newItem = itemFolder.FindChild(newPath);
            if (newItem == null)
            {
                Editor.LogWarning("Failed to find the created new item.");
                return;
            }

            // Auto-select item
            Select(newItem, true);

            // Custom post-action
            endEvent?.Invoke(newItem);
        }


        /// <summary>
        /// Deletes the specified item. Asks user first and uses some GUI.
        /// </summary>
        /// <param name="item">The item to delete.</param>
        public void Delete(ContentItem item)
        {
            var items = View.Selection;
            if (items.Count == 0)
                items = new List<ContentItem>() { item };
            Delete(items);
        }

        /// <summary>
        /// Deletes the specified items. Asks user first and uses some GUI.
        /// </summary>
        /// <param name="items">The items to delete.</param>
        public void Delete(List<ContentItem> items)
        {
            if (items.Count == 0)
                return;

            // Sort items to remove files first, then folders
            var toDelete = new List<ContentItem>(items);
            toDelete.Sort((a, b) => a.IsFolder ? 1 : b.IsFolder ? -1 : a.Compare(b));

            string singularPlural = toDelete.Count > 1 ? "s" : "";

            string msg = toDelete.Count == 1
                         ? string.Format("Delete \'{0}\'?\n\nThis action cannot be undone.\nFile will be deleted permanently.", items[0].Path)
                         : string.Format("Delete {0} selected items?\n\nThis action cannot be undone.\nFiles will be deleted permanently.", items.Count);

            // Ask user
            if (MessageBox.Show(msg, "Delete asset" + singularPlural, MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK)
                return;

            // Clear navigation
            // TODO: just remove invalid locations from the history (those are removed)
            NavigationClearHistory();

            // Delete items
            for (int i = 0; i < toDelete.Count; i++)
                Editor.ContentDatabase.Delete(toDelete[i], true);

            RefreshView();
        }

        private string GetClonedAssetPath(ContentItem item)
        {
            string sourcePath = item.Path;
            string sourceFolder = Path.GetDirectoryName(sourcePath);

            // Find new name for clone
            string destinationName;
            if (item.IsFolder)
            {
                destinationName = Utilities.Utils.IncrementNameNumber(item.ShortName, x => !Directory.Exists(StringUtils.CombinePaths(sourceFolder, x)));
            }
            else
            {
                string extension = Path.GetExtension(sourcePath);
                destinationName = Utilities.Utils.IncrementNameNumber(item.ShortName, x => !File.Exists(StringUtils.CombinePaths(sourceFolder, x + extension))) + extension;
            }

            return StringUtils.NormalizePath(StringUtils.CombinePaths(sourceFolder, destinationName));
        }

        /// <summary>
        /// Clones the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void Duplicate(ContentItem item)
        {
            // Skip null
            if (item == null)
                return;

            // TODO: don't allow to duplicate items without ParentFolder - like root items (Content, Source, Engine and Editor dirs)

            // Clone item
            var targetPath = GetClonedAssetPath(item);
            Editor.ContentDatabase.Copy(item, targetPath);

            // Refresh this folder now and try to find duplicated item
            Editor.ContentDatabase.RefreshFolder(item.ParentFolder, true);
            RefreshView();
            var targetItem = item.ParentFolder.FindChild(targetPath);

            // Start renaming it
            if (targetItem != null)
            {
                Select(targetItem);
                Rename(targetItem);
            }
        }

        /// <summary>
        /// Duplicates the specified items.
        /// </summary>
        /// <param name="items">The items.</param>
        public void Duplicate(List<ContentItem> items)
        {
            // Skip empty or null case
            if (items == null || items.Count == 0)
                return;

            // TODO: don't allow to duplicate items without ParentFolder - like root items (Content, Source, Engine and Editor dirs)

            // Check if it's just a single item
            if (items.Count == 1)
            {
                Duplicate(items[0]);
            }
            else
            {
                // TODO: remove items that depend on different items in the list: use wants to remove `folderA` and `folderA/asset.x`, we should just remove `folderA`
                var toDuplicate = new List<ContentItem>(items);

                // Duplicate every item
                for (int i = 0; i < toDuplicate.Count; i++)
                {
                    var item = toDuplicate[i];
                    Editor.ContentDatabase.Copy(item, GetClonedAssetPath(item));
                }
            }
        }

        /// <summary>
        /// Pastes the specified files.
        /// </summary>
        /// <param name="files">The files paths to import.</param>
        /// <param name="isCutting">Whether a cutting action is occuring.</param>
        public void Paste(string[] files, bool isCutting)
        {
            var importFiles = new List<string>();
            foreach (var sourcePath in files)
            {
                var item = Editor.ContentDatabase.Find(sourcePath);
                if (item != null)
                {
                    var newPath = StringUtils.NormalizePath(Path.Combine(CurrentViewFolder.Path, item.FileName));
                    if (sourcePath.Equals(newPath))
                        newPath = GetClonedAssetPath(item);
                    if (isCutting)
                        Editor.ContentDatabase.Move(item, newPath);
                    else
                        Editor.ContentDatabase.Copy(item, newPath);
                }
                else
                    importFiles.Add(sourcePath);
            }
            Editor.ContentImporting.Import(importFiles, CurrentViewFolder);
        }

        /// <summary>
        /// Starts creating the folder.
        /// </summary>
        public void NewFolder()
        {
            // Construct path
            var parentFolder = SelectedNode.Folder;
            string destinationPath;
            int i = 0;
            do
            {
                destinationPath = StringUtils.CombinePaths(parentFolder.Path, string.Format("New Folder ({0})", i++));
            } while (Directory.Exists(destinationPath));

            // Create new folder
            Directory.CreateDirectory(destinationPath);

            // Refresh parent folder now and try to find duplicated item
            // Note: we should spawn new items directly, content database should do it to propagate events in a proper way
            Editor.ContentDatabase.RefreshFolder(parentFolder, true);
            RefreshView();
            var targetItem = parentFolder.FindChild(destinationPath);

            // Start renaming it
            if (targetItem != null)
            {
                Rename(targetItem);
            }
        }

        /// <summary>
        /// Starts creating new item.
        /// </summary>
        /// <param name="proxy">The new item proxy.</param>
        /// <param name="argument">The argument passed to the proxy for the item creation. In most cases it is null.</param>
        /// <param name="created">The event called when the item is crated by the user. The argument is the new item.</param>
        /// <param name="initialName">The initial item name.</param>
        /// <param name="withRenaming">True if start initial item renaming by user, or tru to skip it.</param>
        public void NewItem(ContentProxy proxy, object argument = null, Action<ContentItem> created = null, string initialName = null, bool withRenaming = true)
        {
            Assert.IsNull(_newElement);
            if (proxy == null)
                throw new ArgumentNullException(nameof(proxy));

            // Setup name
            string name = initialName ?? proxy.NewItemName;
            if (!proxy.IsFileNameValid(name) || Utilities.Utils.HasInvalidPathChar(name))
                name = proxy.NewItemName;

            // If the proxy can not be created in the current folder, then navigate to the content folder
            if (!proxy.CanCreate(CurrentViewFolder))
                Navigate(Editor.Instance.ContentDatabase.Game.Content);

            ContentFolder parentFolder = CurrentViewFolder;
            string parentFolderPath = parentFolder.Path;

            // Create asset name
            string extension = '.' + proxy.FileExtension;
            string path = StringUtils.CombinePaths(parentFolderPath, name + extension);
            if (parentFolder.FindChild(path) != null)
            {
                int i = 0;
                do
                {
                    path = StringUtils.CombinePaths(parentFolderPath, string.Format("{0} {1}", name, i++) + extension);
                } while (parentFolder.FindChild(path) != null);
            }

            if (withRenaming)
            {
                // Create new asset proxy, add to view and rename it
                _newElement = new NewItem(path, proxy, argument)
                {
                    ParentFolder = parentFolder,
                    Tag = created,
                };
                RefreshView();
                Rename(_newElement);
            }
            else
            {
                // Create new asset
                try
                {
                    Editor.Log(string.Format("Creating asset {0} in {1}", proxy.Name, path));
                    proxy.Create(path, argument);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                    Editor.LogError("Failed to create asset.");
                    return;
                }

                // Focus content window
                Focus();
                RootWindow?.Focus();

                // Refresh database and view now
                Editor.ContentDatabase.RefreshFolder(parentFolder, false);
                RefreshView();
                var newItem = parentFolder.FindChild(path);
                if (newItem == null)
                {
                    Editor.LogWarning("Failed to find the created new item.");
                    return;
                }

                // Auto-select item
                Select(newItem, true);

                // Custom post-action
                created?.Invoke(newItem);
            }
        }

        private void OnContentDatabaseItemRemoved(ContentItem contentItem)
        {
            if (contentItem is ContentFolder folder)
            {
                var node = folder.Node;

                // Check if current location contains it as a parent
                if (contentItem.Find(CurrentViewFolder))
                {
                    // Navigate to root to prevent leaks
                    ShowRoot();
                }

                // Check if folder is in navigation
                if (_navigationRedo.Contains(node) || _navigationUndo.Contains(node))
                {
                    // Clear all to prevent leaks
                    NavigationClearHistory();
                }
            }
        }

        private void OnContentDatabaseItemAdded(ContentItem contentItem)
        {
            if (contentItem is ContentFolder folder && _expandedFolderPaths.Contains(StringUtils.NormalizePath(folder.Path)))
            {
                _suppressExpandedStateSave = true;
                folder.Node?.Expand(true);
                _suppressExpandedStateSave = false;
            }
        }

        /// <summary>
        /// Opens the specified content item.
        /// </summary>
        /// <param name="item">The content item.</param>
        public void Open(ContentItem item)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Check if it's a folder
            if (item.IsFolder)
            {
                // Show folder
                var folder = (ContentFolder)item;
                folder.Node.Expand();
                _tree.Select(folder.Node);
                if (!_showAllContentInTree)
                    _view.SelectFirstItem();
                return;
            }

            // Open it
            Editor.ContentEditing.Open(item);
        }

        /// <summary>
        /// Selects the specified asset in the content view.
        /// </summary>
        /// <param name="asset">The asset to select.</param>
        public void Select(Asset asset)
        {
            if (asset == null)
                throw new ArgumentNullException();

            var item = Editor.ContentDatabase.Find(asset.ID);
            if (item != null)
                Select(item);
        }

        /// <summary>
        /// Selects the specified item in the content view.
        /// </summary>
        /// <param name="item">The item to select.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void Select(ContentItem item, bool fastScroll = false)
        {
            if (item == null)
                throw new ArgumentNullException();

            if (!_navigationUnlocked)
                return;
            var parent = item.ParentFolder;
            if (parent == null || !parent.Visible)
                return;

            // Ensure that window is visible
            FocusOrShow();

            if (_showAllContentInTree)
            {
                var targetNode = item is ContentFolder folder ? folder.Node : parent.Node;
                if (targetNode != null)
                {
                    targetNode.ExpandAllParents();
                    if (item is ContentFolder)
                    {
                        _tree.Select(targetNode);
                        _contentTreePanel.ScrollViewTo(targetNode, fastScroll);
                        targetNode.Focus();
                    }
                    else
                    {
                        var itemNode = FindTreeItemNode(targetNode, item);
                        if (itemNode != null)
                        {
                            _tree.Select(itemNode);
                            _contentTreePanel.ScrollViewTo(itemNode, fastScroll);
                            itemNode.Focus();
                        }
                        else
                        {
                            _tree.Select(targetNode);
                        }
                    }
                }
                return;
            }

            // Navigate to the parent directory
            Navigate(parent.Node);

            // Select and scroll to cover in view
            _view.Select(item);
            _contentViewPanel.ScrollViewTo(item, fastScroll);

            // Focus
            _view.Focus();
        }

        private ContentItemTreeNode FindTreeItemNode(ContentFolderTreeNode parentNode, ContentItem item)
        {
            if (parentNode == null || item == null)
                return null;
            for (int i = 0; i < parentNode.ChildrenCount; i++)
            {
                if (parentNode.GetChild(i) is ContentItemTreeNode itemNode && itemNode.Item == item)
                    return itemNode;
            }
            return null;
        }

        /// <summary>
        /// Refreshes the current view items collection.
        /// </summary>
        public void RefreshView()
        {
            if (_showAllContentInTree)
                RefreshTreeItems();
            else if (_view.IsSearching)
                UpdateItemsSearch();
            else
                RefreshView(SelectedNode);

            return;
        }

        /// <summary>
        /// Refreshes the view.
        /// </summary>
        /// <param name="target">The target location.</param>
        public void RefreshView(ContentFolderTreeNode target)
        {
            if (_showAllContentInTree)
            {
                RefreshTreeItems();
                return;
            }

            _view.IsSearching = false;
            if (target == _root)
            {
                // Special case for root folder
                var items = new List<ContentItem>(8);
                for (int i = 0; i < _root.ChildrenCount; i++)
                {
                    if (_root.GetChild(i) is ContentFolderTreeNode node)
                    {
                        items.Add(node.Folder);
                    }
                }
                _view.ShowItems(items, _sortType, false, true);
            }
            else if (target != null)
            {
                // Show folder contents
                var items = target.Folder.Children;
                if (!_showAllFiles)
                    items = items.Where(x => !(x is FileItem)).ToList();
                if (!_showGeneratedFiles)
                    items = items.Where(x => !(x.Path.EndsWith(".Gen.cs", StringComparison.Ordinal) || x.Path.EndsWith(".Gen.h", StringComparison.Ordinal) || x.Path.EndsWith(".Gen.cpp", StringComparison.Ordinal) || x.Path.EndsWith(".csproj", StringComparison.Ordinal) || x.Path.Contains(".CSharp"))).ToList();
                _view.ShowItems(items, _sortType, false, true);
            }
        }

        private void RefreshTreeItems()
        {
            if (!_showAllContentInTree || _root == null)
                return;

            _root.LockChildrenRecursive();
            RemoveTreeAssetNodes(_root);
            AddTreeAssetNodes(_root);
            _root.UnlockChildrenRecursive();
            _tree.PerformLayout();
        }

        private void UpdateTreeItemNames(ContentFolderTreeNode node)
        {
            if (node == null)
                return;

            for (int i = 0; i < node.ChildrenCount; i++)
            {
                if (node.GetChild(i) is ContentFolderTreeNode childFolder)
                {
                    UpdateTreeItemNames(childFolder);
                }
                else if (node.GetChild(i) is ContentItemTreeNode itemNode)
                {
                    itemNode.UpdateDisplayedName();
                }
            }
        }

        internal void OnContentTreeNodeExpandedChanged(ContentFolderTreeNode node, bool isExpanded)
        {
            if (_suppressExpandedStateSave || node == null || node == _root)
                return;

            var path = node.Path;
            if (string.IsNullOrEmpty(path))
                return;
            path = StringUtils.NormalizePath(path);

            if (isExpanded)
                _expandedFolderPaths.Add(path);
            else
                // Remove all sub paths if parent folder is closed.
                _expandedFolderPaths.RemoveWhere(x => x.Contains(path));

            SaveExpandedFolders();
        }

        internal void TryAutoExpandContentNode(ContentFolderTreeNode node)
        {
            if (node == null || node == _root)
                return;

            var path = node.Path;
            if (string.IsNullOrEmpty(path))
                return;
            path = StringUtils.NormalizePath(path);

            if (!_expandedFolderPaths.Contains(path))
                return;

            _suppressExpandedStateSave = true;
            node.Expand(true);
            _suppressExpandedStateSave = false;
        }

        private void LoadExpandedFolders()
        {
            _expandedFolderPaths.Clear();
            if (Editor.ProjectCache.TryGetCustomData(ProjectDataExpandedFolders, out string data) && !string.IsNullOrWhiteSpace(data))
            {
                var entries = data.Split(new[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
                for (int i = 0; i < entries.Length; i++)
                {
                    var path = entries[i].Trim();
                    if (path.Length == 0)
                        continue;
                    _expandedFolderPaths.Add(StringUtils.NormalizePath(path));
                }
            }
        }

        private void SaveExpandedFolders()
        {
            if (_expandedFolderPaths.Count == 0)
            {
                Editor.ProjectCache.RemoveCustomData(ProjectDataExpandedFolders);
                return;
            }

            var data = string.Join("\n", _expandedFolderPaths);
            Editor.ProjectCache.SetCustomData(ProjectDataExpandedFolders, data);
        }

        private void ApplyExpandedFolders()
        {
            if (_root == null || _expandedFolderPaths.Count == 0)
                return;

            _suppressExpandedStateSave = true;
            foreach (var path in _expandedFolderPaths)
            {
                if (Editor.ContentDatabase.Find(path) is ContentFolder folder)
                {
                    folder.Node.ExpandAllParents(true);
                    folder.Node.Expand(true);
                }
            }
            _suppressExpandedStateSave = false;
        }

        private void RemoveTreeAssetNodes(ContentFolderTreeNode node)
        {
            for (int i = node.ChildrenCount - 1; i >= 0; i--)
            {
                if (node.GetChild(i) is ContentItemTreeNode itemNode)
                {
                    node.RemoveChild(itemNode);
                    itemNode.Dispose();
                }
                else if (node.GetChild(i) is ContentFolderTreeNode childFolder)
                {
                    RemoveTreeAssetNodes(childFolder);
                }
            }
        }

        private void AddTreeAssetNodes(ContentFolderTreeNode node)
        {
            if (node.Folder != null)
            {
                var children = node.Folder.Children;
                for (int i = 0; i < children.Count; i++)
                {
                    var child = children[i];
                    if (child is ContentFolder)
                        continue;
                    if (!ShouldShowTreeItem(child))
                        continue;

                    var itemNode = new ContentItemTreeNode(child)
                    {
                        Parent = node,
                    };
                }
            }

            for (int i = 0; i < node.ChildrenCount; i++)
            {
                if (node.GetChild(i) is ContentFolderTreeNode childFolder)
                {
                    AddTreeAssetNodes(childFolder);
                }
            }

            node.SortChildren();
        }

        private bool ShouldShowTreeItem(ContentItem item)
        {
            if (item == null || !item.Visible)
                return false;
            if (_viewDropdown != null && _viewDropdown.HasSelection)
            {
                var filterIndex = (int)item.SearchFilter;
                if (!_viewDropdown.Selection.Contains(filterIndex))
                    return false;
            }
            if (!_showAllFiles && item is FileItem)
                return false;
            if (!_showGeneratedFiles && IsGeneratedFile(item.Path))
                return false;
            return true;
        }

        private static bool IsGeneratedFile(string path)
        {
            return path.EndsWith(".Gen.cs", StringComparison.Ordinal) ||
                   path.EndsWith(".Gen.h", StringComparison.Ordinal) ||
                   path.EndsWith(".Gen.cpp", StringComparison.Ordinal) ||
                   path.EndsWith(".csproj", StringComparison.Ordinal) ||
                   path.Contains(".CSharp");
        }

        private void UpdateUI()
        {
            UpdateToolstrip();
            UpdateNavigationBar();
        }

        private void ApplyTreeViewScale()
        {
            if (_tree == null)
                return;

            var scale = _showAllContentInTree ? View.ViewScale : 1.0f;
            var headerHeight = Mathf.Clamp(16.0f * scale, 12.0f, 28.0f);
            var style = Style.Current;
            var fontSize = Mathf.Clamp(style.FontSmall.Size * scale, 8.0f, 28.0f);
            var fontRef = new FontReference(style.FontSmall.Asset, fontSize);
            var iconSize = Mathf.Clamp(16.0f * scale, 12.0f, 28.0f);
            var textMarginLeft = 2.0f + Mathf.Max(0.0f, iconSize - 16.0f);
            ApplyTreeNodeScale(_root, headerHeight, fontRef, textMarginLeft);
            _root?.PerformLayout(true);
            _tree.PerformLayout();
        }

        private void ApplyTreeNodeScale(ContentFolderTreeNode node, float headerHeight, FontReference fontRef, float textMarginLeft)
        {
            if (node == null)
                return;

            var margin = node.TextMargin;
            margin.Left = textMarginLeft;
            margin.Top = 2.0f;
            margin.Right = 2.0f;
            margin.Bottom = 2.0f;
            node.TextMargin = margin;
            node.CustomArrowRect = GetTreeArrowRect(node, headerHeight);
            node.HeaderHeight = headerHeight;
            node.TextFont = fontRef;
            for (int i = 0; i < node.ChildrenCount; i++)
            {
                if (node.GetChild(i) is ContentFolderTreeNode child)
                    ApplyTreeNodeScale(child, headerHeight, fontRef, textMarginLeft);
                else if (node.GetChild(i) is ContentItemTreeNode itemNode)
                {
                    var itemMargin = itemNode.TextMargin;
                    itemMargin.Left = textMarginLeft;
                    itemMargin.Top = 2.0f;
                    itemMargin.Right = 2.0f;
                    itemMargin.Bottom = 2.0f;
                    itemNode.TextMargin = itemMargin;
                    itemNode.HeaderHeight = headerHeight;
                    itemNode.TextFont = fontRef;
                }
            }
        }

        private static Rectangle GetTreeArrowRect(ContentFolderTreeNode node, float headerHeight)
        {
            if (node == null)
                return Rectangle.Empty;

            var scale = Editor.Instance?.Windows?.ContentWin?.IsTreeOnlyMode == true
                ? Editor.Instance.Windows.ContentWin.View.ViewScale
                : 1.0f;
            var arrowSize = Mathf.Clamp(12.0f * scale, 10.0f, 20.0f);
            var iconSize = Mathf.Clamp(16.0f * scale, 12.0f, 28.0f);
            var textRect = node.TextRect;
            var iconLeft = textRect.Left - iconSize - 2.0f;
            var x = iconLeft - arrowSize - 2.0f;
            var y = (headerHeight - arrowSize) * 0.5f;
            return new Rectangle(Mathf.Max(x, 0.0f), Mathf.Max(y, 0.0f), arrowSize, arrowSize);
        }

        private void UpdateToolstrip()
        {
            if (_toolStrip == null)
                return;

            // Update buttons
            var folder = CurrentViewFolder;
            _importButton.Enabled = folder != null && folder.CanHaveAssets;
            _navigateBackwardButton.Enabled = _navigationUndo.Count > 0;
            _navigateForwardButton.Enabled = _navigationRedo.Count > 0;
            _navigateUpButton.Enabled = folder != null && _tree.SelectedNode != _root;
        }

        private void UpdateNavigationBarBounds()
        {
            if (_navigationBar != null && _toolStrip != null)
            {
                var bottomPrev = _toolStrip.Bottom;
                _navigationBar.UpdateBounds(_toolStrip);
                if (_viewDropdownPanel != null && _viewDropdownPanel.Visible)
                {
                    var reserved = _viewDropdownPanel.Width + 8.0f;
                    _navigationBar.Width = Mathf.Max(_navigationBar.Width - reserved, 0.0f);
                }
                if (bottomPrev != _toolStrip.Bottom)
                {
                    // Navigation bar changed toolstrip height
                    _split.Offsets = new Margin(0, 0, _toolStrip.Bottom, 0);
                    if (_treeOnlyPanel != null)
                        _treeOnlyPanel.Offsets = new Margin(0, 0, _toolStrip.Bottom, 0);
                    PerformLayout();
                }
                UpdateViewDropdownBounds();
            }
        }

        private void UpdateViewDropdownBounds()
        {
            if (_viewDropdownPanel == null || _toolStrip == null)
                return;

            var margin = _toolStrip.ItemsMargin;
            var height = _toolStrip.ItemsHeight;
            var y = _toolStrip.Y + (_toolStrip.Height - height) * 0.5f;
            var width = _viewDropdownPanel.Width;
            var x = _toolStrip.Right - width - margin.Right;
            _viewDropdownPanel.Bounds = new Rectangle(x, y, width, height);
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            // Content database events
            Editor.ContentDatabase.WorkspaceModified += () => _isWorkspaceDirty = true;
            Editor.ContentDatabase.ItemAdded += OnContentDatabaseItemAdded;
            Editor.ContentDatabase.ItemRemoved += OnContentDatabaseItemRemoved;
            Editor.ContentDatabase.WorkspaceRebuilding += () => { _workspaceRebuildLocation = SelectedNode?.Path; };
            Editor.ContentDatabase.WorkspaceRebuilt += () =>
            {
                var selected = Editor.ContentDatabase.Find(_workspaceRebuildLocation);
                if (selected is ContentFolder selectedFolder)
                {
                    _navigationUnlocked = false;
                    RefreshView(selectedFolder.Node);
                    _tree.Select(selectedFolder.Node);
                    UpdateItemsSearch();
                    _navigationUnlocked = true;
                    UpdateUI();
                }
                else if (_root != null)
                    ShowRoot();
            };

            LoadExpandedFolders();
            Refresh();

            // Load last viewed folder
            if (Editor.ProjectCache.TryGetCustomData(ProjectDataLastViewedFolder, out string lastViewedFolder))
            {
                if (Editor.ContentDatabase.Find(lastViewedFolder) is ContentFolder folder)
                    _tree.Select(folder.Node);
            }

            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
        }

        private void OnScriptsReloadBegin()
        {
            var lastViewedFolder = _tree.Selection.Count == 1 ? _tree.SelectedNode as ContentFolderTreeNode : null;
            _lastViewedFolderBeforeReload = lastViewedFolder?.Path ?? string.Empty;

            _tree.RemoveChild(_root);
            _root = null;
        }

        private void OnScriptsReloadEnd()
        {
            Refresh();

            if (!string.IsNullOrEmpty(_lastViewedFolderBeforeReload))
            {
                if (Editor.ContentDatabase.Find(_lastViewedFolderBeforeReload) is ContentFolder folder)
                    _tree.Select(folder.Node);
            }

            OnFoldersSearchBoxTextChanged();
        }

        private void Refresh()
        {
            // Setup content root node
            _root = new RootContentFolderTreeNode
            {
                ChildrenIndent = 0
            };
            _root.Expand(true);

            // Add game project on top, plugins in the middle and engine at bottom
            _root.AddChild(Editor.ContentDatabase.Game);
            Editor.ContentDatabase.Projects.Sort();
            foreach (var project in Editor.ContentDatabase.Projects)
            {
                project.SortChildrenRecursive();
                if (project == Editor.ContentDatabase.Game || project == Editor.ContentDatabase.Engine)
                    continue;
                project.Visible = _showPluginsFiles;
                project.Folder.Visible = _showPluginsFiles;
                _root.AddChild(project);
            }
            Editor.ContentDatabase.Engine.Visible = _showEngineFiles;
            Editor.ContentDatabase.Engine.Folder.Visible = _showEngineFiles;
            _root.AddChild(Editor.ContentDatabase.Engine);

            Editor.ContentDatabase.Game?.Expand(true);
            _tree.Margin = new Margin(0.0f, 0.0f, -16.0f, ScrollBar.DefaultSize + 2); // Hide root node
            _tree.AddChild(_root);

            // Setup navigation
            _navigationUnlocked = true;
            _tree.Select(_root);
            NavigationClearHistory();

            // Update UI layout
            _isLayoutLocked = false;
            PerformLayout();
            ApplyExpandedFolders();
            ApplyTreeViewMode();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Handle workspace modification events but only once per frame
            if (_isWorkspaceDirty)
            {
                _isWorkspaceDirty = false;
                if (_showAllContentInTree)
                    RefreshTreeItems();
                else
                    RefreshView();
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            // Save last viewed folder
            ContentFolderTreeNode lastViewedFolder = null;
            if (_tree.Selection.Count == 1)
            {
                var selectedNode = _tree.SelectedNode;
                if (selectedNode is ContentItemTreeNode itemNode)
                    lastViewedFolder = itemNode.Item?.ParentFolder?.Node;
                else
                    lastViewedFolder = selectedNode as ContentFolderTreeNode;
            }
            Editor.ProjectCache.SetCustomData(ProjectDataLastViewedFolder, lastViewedFolder?.Path ?? string.Empty);

            // Clear view
            _view.ClearItems();

            // Unlink used directories
            if (_root != null)
            {
                while (_root.HasChildren)
                {
                    _root.RemoveChild((ContentFolderTreeNode)_root.GetChild(0));
                }
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            // Navigate through directories using the side mouse buttons
            if (button == MouseButton.Extended1)
                NavigateBackward();
            else if (button == MouseButton.Extended2)
                NavigateForward();

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Right)
            {
                // Find control that is under the mouse
                var c = GetChildAtRecursive(location);

                if (c is ContentItem item)
                {
                    if (_view.IsSelected(item) == false)
                        _view.Select(item);
                    ShowContextMenuForItem(item, ref location, false);
                }
                else if (c is ContentView)
                {
                    ShowContextMenuForItem(null, ref location, false);
                }
                else if (c is ContentItemTreeNode itemNode)
                {
                    _tree.Select(itemNode);
                    ShowContextMenuForItem(itemNode.Item, ref location, false);
                }
                else if (c is ContentFolderTreeNode node)
                {
                    _tree.Select(node);
                    ShowContextMenuForItem(node.Folder, ref location, true);
                }

                return true;
            }

            if (button == MouseButton.Left)
            {
                // Find control that is under the mouse
                var c = GetChildAtRecursive(location);
                if (c is ContentView)
                {
                    _view.ClearSelection();
                    return true;
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            base.PerformLayoutAfterChildren();
            UpdateNavigationBarBounds();
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split", _split);
            writer.WriteAttributeString("Scale", _view.ViewScale.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("ShowFileExtensions", _view.ShowFileExtensions.ToString());
            writer.WriteAttributeString("ShowEngineFiles", ShowEngineFiles.ToString());
            writer.WriteAttributeString("ShowPluginsFiles", ShowPluginsFiles.ToString());
            writer.WriteAttributeString("ShowAllFiles", ShowAllFiles.ToString());
            writer.WriteAttributeString("ShowGeneratedFiles", ShowGeneratedFiles.ToString());
            writer.WriteAttributeString("ViewType", _view.ViewType.ToString());
            writer.WriteAttributeString("TreeViewAllContent", _showAllContentInTree.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split", _split);
            if (float.TryParse(node.GetAttribute("Scale"), CultureInfo.InvariantCulture, out var value1))
                _view.ViewScale = value1;
            if (bool.TryParse(node.GetAttribute("ShowFileExtensions"), out bool value2))
                _view.ShowFileExtensions = value2;
            if (bool.TryParse(node.GetAttribute("ShowEngineFiles"), out value2))
                ShowEngineFiles = value2;
            if (bool.TryParse(node.GetAttribute("ShowPluginsFiles"), out value2))
                ShowPluginsFiles = value2;
            if (bool.TryParse(node.GetAttribute("ShowAllFiles"), out value2))
                ShowAllFiles = value2;
            if (bool.TryParse(node.GetAttribute("ShowGeneratedFiles"), out value2))
                ShowGeneratedFiles = value2;
            if (Enum.TryParse(node.GetAttribute("ViewType"), out ContentViewType viewType))
                _view.ViewType = viewType;
            if (bool.TryParse(node.GetAttribute("TreeViewAllContent"), out value2))
                _showAllContentInTree = value2;
            ApplyTreeViewMode();
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.2f;
            _view.ViewScale = 1.0f;
            _showAllContentInTree = false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _foldersSearchBox = null;
            _itemsSearchBox = null;
            _viewDropdown = null;
            _viewDropdownPanel = null;
            _treePanelRoot = null;
            _treeHeaderPanel = null;
            _treeOnlyPanel = null;
            _contentItemsSearchPanel = null;

            Editor.Options.OptionsChanged -= OnOptionsChanged;
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;
            if (Editor?.ContentDatabase != null)
                Editor.ContentDatabase.ItemAdded -= OnContentDatabaseItemAdded;

            base.OnDestroy();
        }
    }
}
