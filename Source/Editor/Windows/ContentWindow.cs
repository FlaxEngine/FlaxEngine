// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        private bool _isWorkspaceDirty;
        private string _workspaceRebuildLocation;
        private SplitPanel _split;
        private Panel _contentViewPanel;
        private Panel _contentTreePanel;
        private ContentView _view;

        private readonly ToolStrip _toolStrip;
        private readonly ToolStripButton _importButton;
        private readonly ToolStripButton _navigateBackwardButton;
        private readonly ToolStripButton _navigateForwardButton;
        private readonly ToolStripButton _navigateUpButton;

        private NavigationBar _navigationBar;
        private Tree _tree;
        private TextBox _foldersSearchBox;
        private TextBox _itemsSearchBox;
        private ViewDropdown _viewDropdown;
        private SortType _sortType;
        private bool _showEngineFiles = true, _showPluginsFiles = true, _showAllFiles = true, _showGeneratedFiles = false;

        private RootContentTreeNode _root;

        private bool _navigationUnlocked;
        private readonly Stack<ContentTreeNode> _navigationUndo = new Stack<ContentTreeNode>(32);
        private readonly Stack<ContentTreeNode> _navigationRedo = new Stack<ContentTreeNode>(32);

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

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ContentWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Content";
            Icon = editor.Icons.Folder32;

            FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);

            // Content database events
            editor.ContentDatabase.WorkspaceModified += () => _isWorkspaceDirty = true;
            editor.ContentDatabase.ItemRemoved += OnContentDatabaseItemRemoved;
            editor.ContentDatabase.WorkspaceRebuilding += () => { _workspaceRebuildLocation = SelectedNode?.Path; };
            editor.ContentDatabase.WorkspaceRebuilt += () =>
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
                else
                    ShowRoot();
            };

            var options = Editor.Options;
            options.OptionsChanged += OnOptionsChanged;

            // Toolstrip
            _toolStrip = new ToolStrip(34.0f)
            {
                Parent = this,
            };
            _importButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Import64, () => Editor.ContentImporting.ShowImportFileDialog(CurrentViewFolder)).LinkTooltip("Import content");
            _toolStrip.AddSeparator();
            _navigateBackwardButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Left64, NavigateBackward).LinkTooltip("Navigate backward");
            _navigateForwardButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Right64, NavigateForward).LinkTooltip("Navigate forward");
            _navigateUpButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Up64, NavigateUp).LinkTooltip("Navigate up");
            _toolStrip.AddSeparator();

            // Navigation bar
            _navigationBar = new NavigationBar
            {
                Parent = _toolStrip,
            };

            // Split panel
            _split = new SplitPanel(options.Options.Interface.ContentWindowOrientation, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolStrip.Bottom, 0),
                SplitterValue = 0.2f,
                Parent = this,
            };

            // Content structure tree searching query input box
            var headerPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = Style.Current.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 18 + 6),
            };
            _foldersSearchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _foldersSearchBox.TextChanged += OnFoldersSearchBoxTextChanged;

            // Content tree panel
            _contentTreePanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, headerPanel.Bottom, 0),
                IsScrollable = true,
                ScrollBars = ScrollBars.Both,
                Parent = _split.Panel1,
            };

            // Content structure tree
            _tree = new Tree(false)
            {
                Parent = _contentTreePanel,
            };
            _tree.SelectedChanged += OnTreeSelectionChanged;
            headerPanel.Parent = _split.Panel1;

            // Content items searching query input box and filters selector
            var contentItemsSearchPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 8),
                Parent = _split.Panel2,
            };
            const float viewDropdownWidth = 50.0f;
            _itemsSearchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = contentItemsSearchPanel,
                Bounds = new Rectangle(viewDropdownWidth + 8, 4, contentItemsSearchPanel.Width - 12 - viewDropdownWidth, 18),
            };
            _itemsSearchBox.TextChanged += UpdateItemsSearch;
            _viewDropdown = new ViewDropdown
            {
                AnchorPreset = AnchorPresets.MiddleLeft,
                SupportMultiSelect = true,
                TooltipText = "Change content view and filter options",
                Parent = contentItemsSearchPanel,
                Offsets = new Margin(4, viewDropdownWidth, -9, 18),
            };
            _viewDropdown.SelectedIndexChanged += e => UpdateItemsSearch();
            for (int i = 0; i <= (int)ContentItemSearchFilter.Other; i++)
                _viewDropdown.Items.Add(((ContentItemSearchFilter)i).ToString());
            _viewDropdown.PopupCreate += OnViewDropdownPopupCreate;

            // Content view panel
            _contentViewPanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, contentItemsSearchPanel.Bottom + 4, 0),
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

            _view.InputActions.Add(options => options.Search, () => _itemsSearchBox.Focus());
            InputActions.Add(options => options.Search, () => _itemsSearchBox.Focus());
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
            viewType.ContextMenu.VisibleChanged += control =>
            {
                if (!control.Visible)
                    return;
                foreach (var item in ((ContextMenu)control).Items)
                {
                    if (item is ContextMenuButton button)
                        button.Checked = View.ViewType == (ContentViewType)button.Tag;
                }
            };

            var show = menu.AddChildMenu("Show");
            {
                var b = show.ContextMenu.AddButton("File extensions", () => View.ShowFileExtensions = !View.ShowFileExtensions);
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

        private void OnViewTypeButtonClicked(ContextMenuButton button)
        {
            View.ViewType = (ContentViewType)button.Tag;
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

            // Disable scrolling in content view
            if (_contentViewPanel.VScrollBar != null)
                _contentViewPanel.VScrollBar.ThumbEnabled = false;
            if (_contentViewPanel.HScrollBar != null)
                _contentViewPanel.HScrollBar.ThumbEnabled = false;
            ScrollingOnContentView(false);

            // Show rename popup
            var popup = RenamePopup.Show(item, item.TextRectangle, item.ShortName, true);
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
            // Restore scrolling in content view
            if (_contentViewPanel.VScrollBar != null)
                _contentViewPanel.VScrollBar.ThumbEnabled = true;
            if (_contentViewPanel.HScrollBar != null)
                _contentViewPanel.HScrollBar.ThumbEnabled = true;
            ScrollingOnContentView(true);

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

            // Navigate to the parent directory
            Navigate(parent.Node);

            // Select and scroll to cover in view
            _view.Select(item);
            _contentViewPanel.ScrollViewTo(item, fastScroll);

            // Focus
            _view.Focus();
        }

        /// <summary>
        /// Refreshes the current view items collection.
        /// </summary>
        public void RefreshView()
        {
            if (_view.IsSearching)
                UpdateItemsSearch();
            else
                RefreshView(SelectedNode);
        }

        /// <summary>
        /// Refreshes the view.
        /// </summary>
        /// <param name="target">The target location.</param>
        public void RefreshView(ContentTreeNode target)
        {
            _view.IsSearching = false;
            if (target == _root)
            {
                // Special case for root folder
                var items = new List<ContentItem>(8);
                for (int i = 0; i < _root.ChildrenCount; i++)
                {
                    if (_root.GetChild(i) is ContentTreeNode node)
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

        private void UpdateUI()
        {
            UpdateToolstrip();
            UpdateNavigationBar();
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

        /// <inheritdoc />
        public override void OnInit()
        {
            // Setup content root node
            _root = new RootContentTreeNode
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
            _tree.Margin = new Margin(0.0f, 0.0f, -16.0f, 2.0f); // Hide root node
            _tree.AddChild(_root);

            // Setup navigation
            _navigationUnlocked = true;
            _tree.Select(_root);
            NavigationClearHistory();

            // Update UI layout
            _isLayoutLocked = false;
            PerformLayout();

            // Load last viewed folder
            if (Editor.ProjectCache.TryGetCustomData(ProjectDataLastViewedFolder, out string lastViewedFolder))
            {
                if (Editor.ContentDatabase.Find(lastViewedFolder) is ContentFolder folder)
                    _tree.Select(folder.Node);
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Handle workspace modification events but only once per frame
            if (_isWorkspaceDirty)
            {
                _isWorkspaceDirty = false;
                RefreshView();
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            // Save last viewed folder
            var lastViewedFolder = _tree.Selection.Count == 1 ? _tree.SelectedNode as ContentTreeNode : null;
            Editor.ProjectCache.SetCustomData(ProjectDataLastViewedFolder, lastViewedFolder?.Path ?? string.Empty);

            // Clear view
            _view.ClearItems();

            // Unlink used directories
            if (_root != null)
            {
                while (_root.HasChildren)
                {
                    _root.RemoveChild((ContentTreeNode)_root.GetChild(0));
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
                else if (c is ContentTreeNode node)
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

            _navigationBar?.UpdateBounds(_toolStrip);
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
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.2f;
            _view.ViewScale = 1.0f;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _foldersSearchBox = null;
            _itemsSearchBox = null;
            _viewDropdown = null;

            Editor.Options.OptionsChanged -= OnOptionsChanged;

            base.OnDestroy();
        }
    }
}
