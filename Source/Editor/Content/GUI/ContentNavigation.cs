// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content.GUI
{
    /// <summary>
    /// A navigation button for <see cref="Windows.ContentWindow"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.NavigationButton" />
    public class ContentNavigationButton : NavigationButton
    {
        private DragItems _dragOverItems;

        /// <summary>
        /// Gets the target node.
        /// </summary>
        public ContentTreeNode TargetNode { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentNavigationButton"/> class.
        /// </summary>
        /// <param name="targetNode">The target node.</param>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        public ContentNavigationButton(ContentTreeNode targetNode, float x, float y, float height)
        : base(x, y, height)
        {
            TargetNode = targetNode;
            Text = targetNode.NavButtonLabel;
        }

        /// <inheritdoc />
        protected override void OnClick()
        {
            // Navigate
            Editor.Instance.Windows.ContentWin.Navigate(TargetNode);

            base.OnClick();
        }

        private DragDropEffect GetDragEffect(DragData data)
        {
            if (data is DragDataFiles)
            {
                if (TargetNode.CanHaveAssets)
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
            return item != TargetNode.Folder && !item.Find(TargetNode.Folder) && !TargetNode.IsRoot;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            if (_dragOverItems == null)
                _dragOverItems = new DragItems(ValidateDragItem);
            _dragOverItems.OnDragEnter(data);
            var result = GetDragEffect(data);
            _validDragOver = result != DragDropEffect.None;
            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            return GetDragEffect(data);
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            base.OnDragLeave();

            _dragOverItems.OnDragLeave();
            _validDragOver = false;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = DragDropEffect.None;
            base.OnDragDrop(ref location, data);

            // Check if drop element or files
            if (data is DragDataFiles files)
            {
                // Import files
                Editor.Instance.ContentImporting.Import(files.Files, TargetNode.Folder);
                result = DragDropEffect.Copy;
            }
            else if (_dragOverItems.HasValidDrag)
            {
                // Move items
                Editor.Instance.ContentDatabase.Move(_dragOverItems.Objects, TargetNode.Folder);
                result = DragDropEffect.Move;
            }

            _dragOverItems.OnDragDrop();
            _validDragOver = false;

            return result;
        }
    }

    sealed class ContentNavigationSeparator : ComboBox
    {
        public ContentNavigationButton Target;

        public ContentNavigationSeparator(ContentNavigationButton target, float x, float y, float height)
        {
            Target = target;
            Bounds = new Rectangle(x, y, 16, height);
            Offsets = new Margin(Bounds.X, Bounds.Width, Bounds.Y, Bounds.Height);
            UpdateTransform();

            MaximumItemsInViewCount = 20;
            var style = Style.Current;
            BackgroundColor = style.BackgroundNormal;
            BackgroundColorHighlighted = BackgroundColor;
            BackgroundColorSelected = BackgroundColor;
        }

        protected override ContextMenu OnCreatePopup()
        {
            // Update items
            ClearItems();
            foreach (var child in Target.TargetNode.Children)
            {
                if (child is ContentTreeNode node)
                {
                    if (node.Folder.VisibleInHierarchy) // Respect the filter set by ContentFilterConfig.Filter(...)
                        AddItem(node.Folder.ShortName);
                }
            }

            return base.OnCreatePopup();
        }

        public override void Draw()
        {
            var style = Style.Current;
            var rect = new Rectangle(Float2.Zero, Size);
            var color = IsDragOver ? Color.Transparent : (_mouseDown ? style.BackgroundSelected : (IsMouseOver ? style.BackgroundHighlighted : Color.Transparent));
            Render2D.FillRectangle(rect, color);
            Render2D.DrawSprite(Editor.Instance.Icons.ArrowRight12, new Rectangle(rect.Location.X, rect.Y + rect.Size.Y * 0.25f, rect.Size.X, rect.Size.X), EnabledInHierarchy ? style.Foreground : style.ForegroundDisabled);
        }

        protected override void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct = false)
        {
            button.Icon = Editor.Instance.Icons.FolderClosed32;
            if (_tooltips != null && _tooltips.Length > index)
                button.TooltipText = _tooltips[index];
        }

        protected override void OnItemClicked(int index)
        {
            base.OnItemClicked(index);

            var item = _items[index];
            foreach (var child in Target.TargetNode.Children)
            {
                if (child is ContentTreeNode node && node.Folder.ShortName == item)
                {
                    Editor.Instance.Windows.ContentWin.Navigate(node);
                    return;
                }
            }
        }
    }
}
