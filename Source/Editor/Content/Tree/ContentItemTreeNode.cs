// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content;

/// <summary>
/// Tree node for non-folder content items.
/// </summary>
public sealed class ContentItemTreeNode : TreeNode, IContentItemOwner
{
    private List<Rectangle> _highlights;

    /// <summary>
    /// The content item.
    /// </summary>
    public ContentItem Item { get; }

    /// <summary>
    /// Initializes a new instance of the <see cref="ContentItemTreeNode"/> class.
    /// </summary>
    /// <param name="item">The content item.</param>
    public ContentItemTreeNode(ContentItem item)
    : base(false, Editor.Instance.Icons.Document128, Editor.Instance.Icons.Document128)
    {
        Item = item ?? throw new ArgumentNullException(nameof(item));
        UpdateDisplayedName();
        IconColor = Color.Transparent; // Reserve icon space but draw custom thumbnail.
        Item.AddReference(this);
    }

    private static SpriteHandle GetIcon(ContentItem item)
    {
        if (item == null)
            return SpriteHandle.Invalid;
        var icon = item.Thumbnail;
        if (!icon.IsValid)
            icon = item.DefaultThumbnail;
        if (!icon.IsValid)
            icon = Editor.Instance.Icons.Document128;
        return icon;
    }

    /// <summary>
    /// Updates the query search filter.
    /// </summary>
    /// <param name="filterText">The filter text.</param>
    public void UpdateFilter(string filterText)
    {
        bool noFilter = string.IsNullOrWhiteSpace(filterText);
        bool isVisible;
        if (noFilter)
        {
            _highlights?.Clear();
            isVisible = true;
        }
        else
        {
            var text = Text;
            if (QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
            {
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
                isVisible = true;
            }
            else
            {
                _highlights?.Clear();
                isVisible = false;
            }
        }

        Visible = isVisible;
    }

    /// <inheritdoc />
    public override void Draw()
    {
        base.Draw();

        var icon = GetIcon(Item);
        if (icon.IsValid)
        {
            var contentWindow = Editor.Instance.Windows.ContentWin;
            var scale = contentWindow != null && contentWindow.IsTreeOnlyMode ? contentWindow.View.ViewScale : 1.0f;
            var iconSize = Mathf.Clamp(16.0f * scale, 12.0f, 28.0f);
            var textRect = TextRect;
            var iconRect = new Rectangle(textRect.Left - iconSize - 2.0f, (HeaderHeight - iconSize) * 0.5f, iconSize, iconSize);
            Render2D.DrawSprite(icon, iconRect);
        }

        if (_highlights != null)
        {
            var style = Style.Current;
            var color = style.ProgressNormal * 0.6f;
            for (int i = 0; i < _highlights.Count; i++)
                Render2D.FillRectangle(_highlights[i], color);
        }
    }

    /// <inheritdoc />
    protected override bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
    {
        if (button == MouseButton.Left)
        {
            Editor.Instance.Windows.ContentWin.Open(Item);
            return true;
        }

        return base.OnMouseDoubleClickHeader(ref location, button);
    }

    /// <inheritdoc />
    public override bool OnKeyDown(KeyboardKeys key)
    {
        if (IsFocused)
        {
            switch (key)
            {
            case KeyboardKeys.Return:
                Editor.Instance.Windows.ContentWin.Open(Item);
                return true;
            case KeyboardKeys.F2:
                Editor.Instance.Windows.ContentWin.Rename(Item);
                return true;
            case KeyboardKeys.Delete:
                Editor.Instance.Windows.ContentWin.Delete(Item);
                return true;
            }
        }

        return base.OnKeyDown(key);
    }

    /// <inheritdoc />
    protected override void DoDragDrop()
    {
        DoDragDrop(DragItems.GetDragData(Item));
    }

    /// <inheritdoc />
    public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
    {
        Item.UpdateTooltipText();
        TooltipText = Item.TooltipText;
        return base.OnShowTooltip(out text, out location, out area);
    }

    /// <inheritdoc />
    void IContentItemOwner.OnItemDeleted(ContentItem item)
    {
    }

    /// <inheritdoc />
    void IContentItemOwner.OnItemRenamed(ContentItem item)
    {
        UpdateDisplayedName();
    }

    /// <inheritdoc />
    void IContentItemOwner.OnItemReimported(ContentItem item)
    {
    }

    /// <inheritdoc />
    void IContentItemOwner.OnItemDispose(ContentItem item)
    {
    }

    /// <inheritdoc />
    public override int Compare(Control other)
    {
        if (other is ContentFolderTreeNode)
            return 1;
        if (other is ContentItemTreeNode otherItem)
            return ApplySortOrder(string.Compare(Text, otherItem.Text, StringComparison.InvariantCulture));
        return base.Compare(other);
    }

    /// <inheritdoc />
    public override void OnDestroy()
    {
        Item.RemoveReference(this);
        base.OnDestroy();
    }

    /// <summary>
    /// Updates the text of the node.
    /// </summary>
    public void UpdateDisplayedName()
    {
        var contentWindow = Editor.Instance?.Windows?.ContentWin;
        var showExtensions = contentWindow?.View?.ShowFileExtensions ?? true;
        Text = Item.ShowFileExtension || showExtensions ? Item.FileName : Item.ShortName;
    }

    private static SortType GetSortType()
    {
        return Editor.Instance?.Windows?.ContentWin?.CurrentSortType ?? SortType.AlphabeticOrder;
    }

    private static int ApplySortOrder(int result)
    {
        return GetSortType() == SortType.AlphabeticReverse ? -result : result;
    }
}
