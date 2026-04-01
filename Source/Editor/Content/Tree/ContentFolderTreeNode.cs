// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Tree;
using FlaxEditor.SceneGraph;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content;

/// <summary>
/// Content folder tree node.
/// </summary>
/// <seealso cref="TreeNode" />
public class ContentFolderTreeNode : TreeNode
{
    private DragItems _dragOverItems;
    private DragActors _dragActors;
    private List<Rectangle> _highlights;

    /// <summary>
    /// The folder.
    /// </summary>
    protected ContentFolder _folder;

    /// <summary>
    /// Whether this node can be deleted.
    /// </summary>
    public virtual bool CanDelete => true;
    
    /// <summary>
    /// Whether this node can be duplicated.
    /// </summary>
    public virtual bool CanDuplicate => true;

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
    public ContentFolderTreeNode ParentNode => Parent as ContentFolderTreeNode;

    /// <summary>
    /// Gets the folder path.
    /// </summary>
    public string Path => _folder.Path;

    /// <summary>
    /// Gets the navigation button label.
    /// </summary>
    public virtual string NavButtonLabel => _folder.ShortName;

    /// <summary>
    /// Initializes a new instance of the <see cref="ContentFolderTreeNode"/> class.
    /// </summary>
    /// <param name="parent">The parent node.</param>
    /// <param name="path">The folder path.</param>
    public ContentFolderTreeNode(ContentFolderTreeNode parent, string path)
    : this(parent, parent?.FolderType ?? ContentFolderType.Other, path)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ContentFolderTreeNode"/> class.
    /// </summary>
    /// <param name="parent">The parent node.</param>
    /// <param name="type">The folder type.</param>
    /// <param name="path">The folder path.</param>
    protected ContentFolderTreeNode(ContentFolderTreeNode parent, ContentFolderType type, string path)
    : base(false, Editor.Instance.Icons.FolderClosed32, Editor.Instance.Icons.FolderOpen32)
    {
        _folder = new ContentFolder(type, path, this);
        Text = _folder.ShortName;
        if (parent != null)
        {
            Folder.ParentFolder = parent.Folder;
            Parent = parent;
        }
        IconColor = Color.Transparent; // Hide default icon, we draw scaled icon manually
        UpdateCustomArrowRect();
        Editor.Instance?.Windows?.ContentWin?.TryAutoExpandContentNode(this);
    }
    
    /// <summary>
    /// Updates the custom arrow rectangle so it stays aligned with the current layout.
    /// </summary>
    private void UpdateCustomArrowRect()
    {
        var contentWindow = Editor.Instance?.Windows?.ContentWin;
        var scale = contentWindow != null && contentWindow.IsTreeOnlyMode ? contentWindow.View.ViewScale : 1.0f;
        var arrowSize = Mathf.Clamp(12.0f * scale, 10.0f, 20.0f);
        var iconSize = Mathf.Clamp(16.0f * scale, 12.0f, 28.0f);

        // Use the current text layout, not just cached values.
        var textRect = TextRect;
        var iconLeft = textRect.Left - iconSize - 2.0f;
        var x = Mathf.Max(iconLeft - arrowSize - 2.0f, 0.0f);
        var y = Mathf.Max((HeaderHeight - arrowSize) * 0.5f, 0.0f);

        CustomArrowRect = new Rectangle(x, y, arrowSize, arrowSize);
    }
    
    /// <inheritdoc />
    public override void PerformLayout(bool force = false)
    {
        base.PerformLayout(force);
        UpdateCustomArrowRect();
    }

    /// <summary>
    /// Shows the rename popup for the item.
    /// </summary>
    public void StartRenaming()
    {
        if (!_folder.CanRename)
            return;

        // Start renaming the folder
        Editor.Instance.Windows.ContentWin.ScrollingOnTreeView(false);
        var dialog = RenamePopup.Show(this, TextRect, _folder.ShortName, false);
        dialog.Tag = _folder;
        dialog.Renamed += popup =>
        {
            Editor.Instance.Windows.ContentWin.Rename((ContentFolder)popup.Tag, popup.Text);
            Editor.Instance.Windows.ContentWin.ScrollingOnTreeView(true);
        };
        dialog.Closed += popup => { Editor.Instance.Windows.ContentWin.ScrollingOnTreeView(true); };
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
            var text = Text;
            if (QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
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
            if (_children[i] is ContentFolderTreeNode child)
            {
                child.UpdateFilter(filterText);
                isAnyChildVisible |= child.Visible;
            }
            else if (_children[i] is ContentItemTreeNode itemNode)
            {
                itemNode.UpdateFilter(filterText);
                isAnyChildVisible |= itemNode.Visible;
            }
        }

        if (!noFilter)
        {
            bool isExpanded = isAnyChildVisible;
            if (isExpanded)
                Expand(true);
            else
                Collapse(true);
        }

        Visible = isThisVisible | isAnyChildVisible;
    }

    /// <inheritdoc />
    public override int Compare(Control other)
    {
        if (other is ContentItemTreeNode)
            return -1;
        if (other is ContentFolderTreeNode otherNode)
            return string.Compare(Text, otherNode.Text, StringComparison.Ordinal);
        return base.Compare(other);
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

        var contentWindow = Editor.Instance.Windows.ContentWin;
        var scale = contentWindow != null && contentWindow.IsTreeOnlyMode ? contentWindow.View.ViewScale : 1.0f;
        var icon = IsExpanded ? Editor.Instance.Icons.FolderOpen32 : Editor.Instance.Icons.FolderClosed32;
        var iconSize = Mathf.Clamp(16.0f * scale, 12.0f, 28.0f);
        var iconRect = new Rectangle(TextRect.Left - iconSize - 2.0f, (HeaderHeight - iconSize) * 0.5f, iconSize, iconSize);
        Render2D.DrawSprite(icon, iconRect);
    }

    /// <inheritdoc />
    public override void OnDestroy()
    {
        // Delete folder item
        _folder.Dispose();

        base.OnDestroy();
    }

    /// <inheritdoc />
    protected override void OnExpandedChanged()
    {
        base.OnExpandedChanged();

        Editor.Instance?.Windows?.ContentWin?.OnContentTreeNodeExpandedChanged(this, IsExpanded);
    }

    private DragDropEffect GetDragEffect(DragData data)
    {
        if (_dragActors != null && _dragActors.HasValidDrag)
            return DragDropEffect.Move;
        if (data is DragDataFiles)
        {
            if (_folder.CanHaveAssets)
                return DragDropEffect.Copy;
        }
        else
        {
            if (_dragOverItems != null && _dragOverItems.HasValidDrag)
                return DragDropEffect.Move;
        }

        return DragDropEffect.None;
    }

    private bool ValidateDragItem(ContentItem item)
    {
        // Reject itself and any parent
        return item != _folder && !item.Find(_folder);
    }

    private bool ValidateDragActors(ActorNode actor)
    {
        return actor.CanCreatePrefab && _folder.CanHaveAssets;
    }

    private void ImportActors(DragActors actors)
    {
        Select();
        foreach (var actorNode in actors.Objects)
        {
            var actor = actorNode.Actor;
            if (actors.Objects.Contains(actorNode.ParentNode as ActorNode))
                continue;

            Editor.Instance.Prefabs.CreatePrefab(actor, false);
        }
        Editor.Instance.Windows.ContentWin.RefreshView();
    }

    /// <inheritdoc />
    protected override DragDropEffect OnDragEnterHeader(DragData data)
    {
        if (data is DragDataFiles)
            return _folder.CanHaveAssets ? DragDropEffect.Copy : DragDropEffect.None;

        if (_dragActors == null)
            _dragActors = new DragActors(ValidateDragActors);
        if (_dragActors.OnDragEnter(data))
            return DragDropEffect.Move;

        if (_dragOverItems == null)
            _dragOverItems = new DragItems(ValidateDragItem);

        _dragOverItems.OnDragEnter(data);
        return GetDragEffect(data);
    }

    /// <inheritdoc />
    protected override DragDropEffect OnDragMoveHeader(DragData data)
    {
        if (data is DragDataFiles)
            return _folder.CanHaveAssets ? DragDropEffect.Copy : DragDropEffect.None;
        if (_dragActors != null && _dragActors.HasValidDrag)
            return DragDropEffect.Move;
        return GetDragEffect(data);
    }

    /// <inheritdoc />
    protected override void OnDragLeaveHeader()
    {
        _dragOverItems?.OnDragLeave();
        _dragActors?.OnDragLeave();
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
        else if (_dragActors != null && _dragActors.HasValidDrag)
        {
            ImportActors(_dragActors);
            _dragActors.OnDragDrop();
            result = DragDropEffect.Move;

            Expand();
        }
        else if (_dragOverItems != null && _dragOverItems.HasValidDrag)
        {
            // Move items
            Editor.Instance.ContentDatabase.Move(_dragOverItems.Objects, _folder);
            result = DragDropEffect.Move;

            Expand();
        }

        _dragOverItems?.OnDragDrop();

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
                if (Folder.Exists && CanDelete)
                    Editor.Instance.Windows.ContentWin.Delete(Folder);
                return true;
            }
            if (RootWindow.GetKey(KeyboardKeys.Control))
            {
                switch (key)
                {
                case KeyboardKeys.D:
                    if (Folder.Exists && CanDuplicate)
                        Editor.Instance.Windows.ContentWin.Duplicate(Folder);
                    return true;
                }
            }
        }

        return base.OnKeyDown(key);
    }
}
