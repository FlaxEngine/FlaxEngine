using FlaxEditor;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Collections.Generic;

namespace FlaxEditor.Content.GUI
{
    internal class ContentNavigationButtonSeparator : ComboBox
    {
        public ContentNavigationButton Target { get; }
        public ContentNavigationButtonSeparator(ContentNavigationButton target, float x, float y,float height)
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
            UpdateDropDownItems();
            return base.OnCreatePopup();
        }
        internal void UpdateDropDownItems()
        {
            ClearItems();
            _items = new();
            for (int i = 0; i < Target.TargetNode.Children.Count; i++)
            {
                if (Target.TargetNode.Children[i] is ContentTreeNode node)
                {
                    if (node.Folder.VisibleInHierarchy) // respect the filter set by ContentFilterConfig.Filter(...)
                        AddItem(node.Folder.ShortName);
                }
            }
        }
        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            var clientRect = new Rectangle(Float2.Zero, Size);
            // Draw background
            if (IsDragOver)
            {
                Render2D.FillRectangle(clientRect, Style.Current.BackgroundSelected * 0.6f);
            }
            else if (_mouseDown)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundSelected);
            }
            else if (IsMouseOver)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
            }

            Render2D.DrawSprite(Editor.Instance.Icons.ArrowRight12, new Rectangle(clientRect.Location.X, clientRect.Y + ((Size.Y / 2f)/2f), Size.X, Size.X), EnabledInHierarchy ? Style.Current.Foreground : Style.Current.ForegroundDisabled);
        }
        protected override void OnLayoutMenuButton(ref ContextMenuButton button, int index, bool construct = false)
        {
            var style = Style.Current;
            button.Icon = Editor.Instance.Icons.FolderClosed32;
            if (_tooltips != null && _tooltips.Length > index)
            {
                button.TooltipText = _tooltips[index];
            }
        }
        protected override void OnItemClicked(int index)
        {
            base.OnItemClicked(index);
            if (Target.TargetNode.Children[index] is ContentTreeNode node)
            {
                Editor.Instance.Windows.ContentWin.Navigate(node);
            }
            // Navigate calls the OnDestroy at some point dont place code below or editor will crash
        }
    }
}
