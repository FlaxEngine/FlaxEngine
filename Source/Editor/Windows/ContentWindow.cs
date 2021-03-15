// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tree;
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
        private SplitPanel _split;
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

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ContentWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Content";
            Icon = editor.Icons.Folder64;

            // Content database events
            editor.ContentDatabase.WorkspaceModified += () => _isWorkspaceDirty = true;
            editor.ContentDatabase.ItemRemoved += ContentDatabaseOnItemRemoved;

            // Toolstrip
            _toolStrip = new ToolStrip(34.0f)
            {
                Parent = this,
            };
            _importButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.Import32, () => Editor.ContentImporting.ShowImportFileDialog(CurrentViewFolder)).LinkTooltip("Import content");
            _toolStrip.AddSeparator();
            _navigateBackwardButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.ArrowLeft32, NavigateBackward).LinkTooltip("Navigate backward");
            _navigateForwardButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.ArrowRight32, NavigateForward).LinkTooltip("Navigate forward");
            _navigateUpButton = (ToolStripButton)_toolStrip.AddButton(Editor.Icons.ArrowUp32, NavigateUp).LinkTooltip("Navigate up");

            // Navigation bar
            _navigationBar = new NavigationBar
            {
                Parent = this,
            };

            // Split panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.Both, ScrollBars.Vertical)
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
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 6),
                Parent = _split.Panel1,
            };
            _foldersSearchBox = new TextBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                WatermarkText = "Search...",
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _foldersSearchBox.TextChanged += OnFoldersSearchBoxTextChanged;

            // Content structure tree
            _tree = new Tree(false)
            {
                Y = headerPanel.Bottom,
                Parent = _split.Panel1,
            };
            _tree.SelectedChanged += OnTreeSelectionChanged;

            // Content items searching query input box and filters selector
            var contentItemsSearchPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 8),
                Parent = _split.Panel2,
            };
            const float viewDropdownWidth = 50.0f;
            _itemsSearchBox = new TextBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                WatermarkText = "Search...",
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

            // Content View
            _view = new ContentView
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, contentItemsSearchPanel.Bottom + 4, 0),
                IsScrollable = true,
                Parent = _split.Panel2,
            };
            _view.OnOpen += Open;
            _view.OnNavigateBack += NavigateBackward;
            _view.OnRename += Rename;
            _view.OnDelete += Delete;
            _view.OnDuplicate += Duplicate;
            _view.OnPaste += Paste;
        }

        private ContextMenu OnViewDropdownPopupCreate(ComboBox comboBox)
        {
            var menu = new ContextMenu();

            var showFileExtensionsButton = menu.AddButton("Show file extensions", () => View.ShowFileExtensions = !View.ShowFileExtensions);
            showFileExtensionsButton.Checked = View.ShowFileExtensions;
            showFileExtensionsButton.AutoCheck = true;

            var viewScale = menu.AddButton("View Scale");
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

            var filters = menu.AddChildMenu("Filters");
            for (int i = 0; i < _viewDropdown.Items.Count; i++)
            {
                var filterButton = filters.ContextMenu.AddButton(_viewDropdown.Items[i], OnFilterClicked);
                filterButton.Tag = i;
            }
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

            return menu;
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

        /// <summary>
        /// Shows popup dialog with UI to rename content item.
        /// </summary>
        /// <param name="item">The item to rename.</param>
        public void Rename(ContentItem item)
        {
            // Show element in the view
            Select(item, true);

            // Disable scrolling in content view
            if (_split.Panel2.VScrollBar != null)
                _split.Panel2.VScrollBar.ThumbEnabled = false;
            if (_split.Panel2.HScrollBar != null)
                _split.Panel2.HScrollBar.ThumbEnabled = false;

            // Show rename popup
            var popup = RenamePopup.Show(item, item.TextRectangle, item.ShortName, true);
            popup.Tag = item;
            popup.Validate += OnRenameValidate;
            popup.Renamed += renamePopup => Rename((ContentItem)renamePopup.Tag, renamePopup.Text);
            popup.Closed += renamePopup =>
            {
                // Restore scrolling in content view
                if (_split.Panel2.VScrollBar != null)
                    _split.Panel2.VScrollBar.ThumbEnabled = true;
                if (_split.Panel2.HScrollBar != null)
                    _split.Panel2.HScrollBar.ThumbEnabled = true;

                // Check if was creating new element
                if (_newElement != null)
                {
                    // Destroy mock control
                    _newElement.ParentFolder = null;
                    _newElement.Dispose();
                    _newElement = null;
                }
            };

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
                MessageBox.Show("Given asset name is invalid. " + hint,
                                "Invalid name",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                return;
            }

            // Ensure has parent
            if (item.ParentFolder == null)
            {
                // Error
                Editor.LogWarning("Cannot rename root items. " + item.Path);
                return;
            }

            // Cache data
            string extension = Path.GetExtension(item.Path);
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

            if (endEvent != null)
            {
                var newItem = itemFolder.FindChild(newPath);
                if (newItem != null)
                {
                    endEvent(newItem);
                }
                else
                {
                    Editor.LogWarning("Failed to find the created new item.");
                }
            }
        }


        /// <summary>
        /// Deletes the specified item. Asks user first and uses some GUI.
        /// </summary>
        /// <param name="item">The item to delete.</param>
        public void Delete(ContentItem item)
        {
            Delete(new List<ContentItem>(1) { item });
        }

        /// <summary>
        /// Deletes the specified items. Asks user first and uses some GUI.
        /// </summary>
        /// <param name="items">The items to delete.</param>
        public void Delete(List<ContentItem> items)
        {
            if (items.Count == 0) return;

            // TODO: remove items that depend on different items in the list: use wants to remove `folderA` and `folderA/asset.x`, we should just remove `folderA`
            var toDelete = new List<ContentItem>(items);

            string msg = toDelete.Count == 1 ? 
                  string.Format("Are you sure to delete \'{0}\'?\nThis action cannot be undone. Files will be deleted permanently.", items[0].Path)
                : string.Format("Are you sure to delete {0} selected items?\nThis action cannot be undone. Files will be deleted permanently.", items.Count);

            // Ask user
            if (MessageBox.Show(msg, "Delete asset(s)", MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK)
                return;

            // Clear navigation
            // TODO: just remove invalid locations from the history (those are removed)
            NavigationClearHistory();

            // Delete items
            for (int i = 0; i < toDelete.Count; i++)
                Editor.ContentDatabase.Delete(toDelete[i]);

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
                destinationName = StringUtils.IncrementNameNumber(item.ShortName, x => !Directory.Exists(StringUtils.CombinePaths(sourceFolder, x)));
            }
            else
            {
                string extension = Path.GetExtension(sourcePath);
                destinationName = StringUtils.IncrementNameNumber(item.ShortName, x => !File.Exists(StringUtils.CombinePaths(sourceFolder, x + extension))) + extension;
            }

            return StringUtils.CombinePaths(sourceFolder, destinationName);
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
        public void Paste(string[] files)
        {
            Editor.ContentImporting.Import(files, CurrentViewFolder);
        }

        /// <summary>
        /// Stars creating the folder.
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
        public void NewItem(ContentProxy proxy, object argument = null, Action<ContentItem> created = null)
        {
            Assert.IsNull(_newElement);
            if (proxy == null)
                throw new ArgumentNullException(nameof(proxy));

            string name = proxy.NewItemName;
            ContentFolder parentFolder = CurrentViewFolder;
            string parentFolderPath = parentFolder.Path;

            // Create asset name
            string path;
            string extension = '.' + proxy.FileExtension;
            int i = 0;
            do
            {
                path = StringUtils.CombinePaths(parentFolderPath, string.Format("{0} {1}", name, i++) + extension);
            } while (parentFolder.FindChild(path) != null);

            // Create new asset proxy, add to view and rename it
            _newElement = new NewItem(path, proxy, argument)
            {
                ParentFolder = parentFolder,
                Tag = created,
            };
            RefreshView();
            Rename(_newElement);
        }

        private void ContentDatabaseOnItemRemoved(ContentItem contentItem)
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
            _split.Panel2.ScrollViewTo(item, fastScroll);

            // Focus
            _view.Focus();
        }

        /// <summary>
        /// Refreshes the current view items collection.
        /// </summary>
        public void RefreshView()
        {
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
                List<ContentItem> items = new List<ContentItem>(8);
                for (int i = 0; i < _root.ChildrenCount; i++)
                {
                    if (_root.GetChild(i) is ContentTreeNode node)
                    {
                        items.Add(node.Folder);
                    }
                }
                _view.ShowItems(items);
            }
            else
            {
                // Show folder contents
                _view.ShowItems(target.Folder.Children);
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

        private void AddFolder2Root(ContentTreeNode node)
        {
            // Add to the root
            _root.AddChild(node);
        }

        private void RemoveFolder2Root(ContentTreeNode node)
        {
            // Remove from the root
            _root.RemoveChild(node);
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

            foreach (var project in Editor.ContentDatabase.Projects)
                AddFolder2Root(project);

            Editor.ContentDatabase.Game?.Expand(true);
            _tree.Margin = new Margin(0.0f, 0.0f, -16.0f, 2.0f); // Hide root node
            _tree.AddChild(_root);
            _root.SortChildrenRecursive();

            // Setup navigation
            _navigationUnlocked = true;
            _tree.Select(_root);
            NavigationClearHistory();

            // Update UI layout
            UnlockChildrenRecursive();
            PerformLayout();

            // Load last viewed folder
            if (Editor.ProjectCache.TryGetCustomData(ProjectDataLastViewedFolder, out var lastViewedFolder))
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
                    RemoveFolder2Root((ContentTreeNode)_root.GetChild(0));
                }
            }
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            // Check if it's a right mouse button
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
            writer.WriteAttributeString("Split", _split.SplitterValue.ToString());
            writer.WriteAttributeString("Scale", _view.ViewScale.ToString());
            writer.WriteAttributeString("ShowFileExtensions", _view.ShowFileExtensions.ToString());
            writer.WriteAttributeString("ViewType", _view.ViewType.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("Split"), out float value1))
                _split.SplitterValue = value1;

            if (float.TryParse(node.GetAttribute("Scale"), out value1))
                _view.ViewScale = value1;

            if (bool.TryParse(node.GetAttribute("ShowFileExtensions"), out bool value2))
                _view.ShowFileExtensions = value2;

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

            base.OnDestroy();
        }
    }
}
