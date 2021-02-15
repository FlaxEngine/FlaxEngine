// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content folder tree node.
    /// </summary>
    /// <seealso cref="TreeNode" />
    public class ContentTreeNode : TreeNode
    {
        private DragItems _dragOverItems;
        private List<Rectangle> _highlights;

        /// <summary>
        /// The folder.
        /// </summary>
        protected ContentFolder _folder;

        /// <summary>
        /// Gets the content folder item.
        /// </summary>
        public ContentFolder Folder => _folder;

        /// <summary>
        /// Gets the type of the folder.
        /// </summary>
        public ContentFolderType FolderType => _folder.FolderType;

        /// <summary>
        /// Returns true if that folder can import/manage scripts.
        /// </summary>
        public bool CanHaveScripts => _folder.CanHaveScripts;

        /// <summary>
        /// Returns true if that folder can import/manage assets.
        /// </summary>
        /// <returns>True if can contain assets for project, otherwise false</returns>
        public bool CanHaveAssets => _folder.CanHaveAssets;

        /// <summary>
        /// Gets the parent node.
        /// </summary>
        public ContentTreeNode ParentNode => Parent as ContentTreeNode;

        /// <summary>
        /// Gets the folder path.
        /// </summary>
        public string Path => _folder.Path;

        /// <summary>
        /// Gets the navigation button label.
        /// </summary>
        public virtual string NavButtonLabel => _folder.ShortName;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent node.</param>
        /// <param name="path">The folder path.</param>
        public ContentTreeNode(ContentTreeNode parent, string path)
        : this(parent, parent?.FolderType ?? ContentFolderType.Other, path)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent node.</param>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The folder path.</param>
        protected ContentTreeNode(ContentTreeNode parent, ContentFolderType type, string path)
        : base(false, Editor.Instance.Icons.FolderClosed12, Editor.Instance.Icons.FolderOpened12)
        {
            _folder = new ContentFolder(type, path, this);
            Text = _folder.ShortName;
            if (parent != null)
            {
                Folder.ParentFolder = parent.Folder;
                Parent = parent;
            }
        }

        /// <summary>
        /// Shows the rename popup for the item.
        /// </summary>
        public void StartRenaming()
        {
            if (!_folder.CanRename)
                return;

            // Start renaming the folder
            var dialog = RenamePopup.Show(this, HeaderRect, _folder.ShortName, false);
            dialog.Tag = _folder;
            dialog.Renamed += popup => Editor.Instance.Windows.ContentWin.Rename((ContentFolder)popup.Tag, popup.Text);
        }

        /// <summary>
        /// Updates the query search filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        public void UpdateFilter(string filterText)
        {
            bool noFilter = string.IsNullOrWhiteSpace(filterText);

            // Update itself
            bool isThisVisible;
            if (noFilter)
            {
                // Clear filter
                _highlights?.Clear();
                isThisVisible = true;
            }
            else
            {
                QueryFilterHelper.Range[] ranges;
                var text = Text;
                if (QueryFilterHelper.Match(filterText, text, out ranges))
                {
                    // Update highlights
                    if (_highlights == null)
                        _highlights = new List<Rectangle>(ranges.Length);
                    else
                        _highlights.Clear();
                    var style = Style.Current;
                    var font = style.FontSmall;
                    var textRect = TextRect;
                    for (int i = 0; i < ranges.Length; i++)
                    {
                        var start = font.GetCharPosition(text, ranges[i].StartIndex);
                        var end = font.GetCharPosition(text, ranges[i].EndIndex);
                        _highlights.Add(new Rectangle(start.X + textRect.X, textRect.Y, end.X - start.X, textRect.Height));
                    }
                    isThisVisible = true;
                }
                else
                {
                    // Hide
                    _highlights?.Clear();
                    isThisVisible = false;
                }
            }

            // Update children
            bool isAnyChildVisible = false;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is ContentTreeNode child)
                {
                    child.UpdateFilter(filterText);
                    isAnyChildVisible |= child.Visible;
                }
            }

            bool isExpanded = isAnyChildVisible;

            if (isExpanded)
            {
                Expand(true);
            }
            else
            {
                Collapse(true);
            }

            Visible = isThisVisible | isAnyChildVisible;
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
        public override void OnDestroy()
        {
            // Delete folder item
            _folder.Dispose();

            base.OnDestroy();
        }

        private DragDropEffect GetDragEffect(DragData data)
        {
            if (data is DragDataFiles)
            {
                if (_folder.CanHaveAssets)
                    return DragDropEffect.Copy;
            }
            else
            {
                if (_dragOverItems.HasValidDrag)
                    return DragDropEffect.Move;
            }

            return DragDropEffect.None;
        }

        private bool ValidateDragItem(ContentItem item)
        {
            // Reject itself and any parent
            return item != _folder && !item.Find(_folder);
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            if (_dragOverItems == null)
                _dragOverItems = new DragItems(ValidateDragItem);

            _dragOverItems.OnDragEnter(data);
            return GetDragEffect(data);
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return GetDragEffect(data);
        }

        /// <inheritdoc />
        protected override void OnDragLeaveHeader()
        {
            _dragOverItems.OnDragLeave();
            base.OnDragLeaveHeader();
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            var result = DragDropEffect.None;

            // Check if drop element or files
            if (data is DragDataFiles files)
            {
                // Import files
                Editor.Instance.ContentImporting.Import(files.Files, _folder);
                result = DragDropEffect.Copy;

                Expand();
            }
            else if (_dragOverItems.HasValidDrag)
            {
                // Move items
                Editor.Instance.ContentDatabase.Move(_dragOverItems.Objects, _folder);
                result = DragDropEffect.Move;

                Expand();
            }

            _dragOverItems.OnDragDrop();

            return result;
        }

        /// <inheritdoc />
        protected override void DoDragDrop()
        {
            DoDragDrop(DragItems.GetDragData(_folder));
        }

        /// <inheritdoc />
        protected override void OnLongPress()
        {
            Select();

            StartRenaming();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (IsFocused)
            {
                switch (key)
                {
                case KeyboardKeys.F2:
                    StartRenaming();
                    return true;
                case KeyboardKeys.Delete:
                    if (Folder.Exists)
                        Editor.Instance.Windows.ContentWin.Delete(Folder);
                    return true;
                }
                if (RootWindow.GetKey(KeyboardKeys.Control))
                {
                    switch (key)
                    {
                    case KeyboardKeys.D:
                        if (Folder.Exists)
                            Editor.Instance.Windows.ContentWin.Duplicate(Folder);
                        return true;
                    }
                }
            }

            return base.OnKeyDown(key);
        }
    }
}
