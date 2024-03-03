// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The custom context menu that shows a list of items and supports searching by name and query results highlighting.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    public class ItemsListContextMenu : ContextMenuBase
    {
        /// <summary>
        /// The single list item control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        [HideInEditor]
        public class Item : Control
        {
            /// <summary>
            /// The is mouse down flag.
            /// </summary>
            protected bool _isMouseDown;

            /// <summary>
            /// The search query highlights.
            /// </summary>
            protected List<Rectangle> _highlights;

            /// <summary>
            /// The item name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The item category name (optional).
            /// </summary>
            public string Category;

            /// <summary>
            /// Occurs when items gets clicked by the user.
            /// </summary>
            public event Action<Item> Clicked;

            /// <summary>
            /// The tint color of the text.
            /// </summary>
            public Color TintColor = Color.White;

            /// <summary>
            /// Initializes a new instance of the <see cref="Item"/> class.
            /// </summary>
            public Item()
            : base(0, 0, 120, 12)
            {
            }

            /// <summary>
            /// Updates the filter.
            /// </summary>
            /// <param name="filterText">The filter text.</param>
            public void UpdateFilter(string filterText)
            {
                if (string.IsNullOrWhiteSpace(filterText))
                {
                    // Clear filter
                    _highlights?.Clear();
                    Visible = true;
                }
                else
                {
                    if (QueryFilterHelper.Match(filterText, Name, out var ranges))
                    {
                        // Update highlights
                        if (_highlights == null)
                            _highlights = new List<Rectangle>(ranges.Length);
                        else
                            _highlights.Clear();
                        var style = Style.Current;
                        var font = style.FontSmall;
                        for (int i = 0; i < ranges.Length; i++)
                        {
                            var start = font.GetCharPosition(Name, ranges[i].StartIndex);
                            var end = font.GetCharPosition(Name, ranges[i].EndIndex);
                            _highlights.Add(new Rectangle(start.X + 2, 0, end.X - start.X, Height));
                        }
                        Visible = true;
                    }
                    else
                    {
                        // Hide
                        _highlights?.Clear();
                        Visible = false;
                    }
                }
            }

            /// <summary>
            /// Gets the text rectangle.
            /// </summary>
            /// <param name="rect">The output rectangle.</param>
            protected virtual void GetTextRect(out Rectangle rect)
            {
                rect = new Rectangle(2, 0, Width - 4, Height);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;
                GetTextRect(out var textRect);

                base.Draw();

                // Overlay
                if (IsMouseOver || IsFocused)
                    Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.BackgroundHighlighted);

                // Indent for drop panel items is handled by drop panel margin
                if (Parent is not DropPanel)
                    textRect.Location += new Float2(Editor.Instance.Icons.ArrowRight12.Size.X + 2, 0);

                // Draw all highlights
                if (_highlights != null)
                {
                    var color = style.ProgressNormal * 0.6f;
                    for (int i = 0; i < _highlights.Count; i++)
                    {
                        var rect = _highlights[i];
                        rect.Location += textRect.Location;
                        rect.Height = textRect.Height;
                        Render2D.FillRectangle(rect, color);
                    }
                }

                // Draw name
                Render2D.DrawText(style.FontSmall, Name, textRect, TintColor * (Enabled ? style.Foreground : style.ForegroundDisabled), TextAlignment.Near, TextAlignment.Center);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    _isMouseDown = true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _isMouseDown)
                {
                    _isMouseDown = false;
                    Clicked?.Invoke(this);
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                _isMouseDown = false;

                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override int Compare(Control other)
            {
                if (other is Item otherItem)
                    return string.Compare(Name, otherItem.Name, StringComparison.Ordinal);
                return base.Compare(other);
            }
        }

        private readonly TextBox _searchBox;
        private readonly Panel _scrollPanel;
        private List<DropPanel> _categoryPanels;
        private bool _waitingForInput;

        /// <summary>
        /// Event fired when any item in this popup menu gets clicked.
        /// </summary>
        public event Action<Item> ItemClicked;

        /// <summary>
        /// Event fired when search text in this popup menu gets changed.
        /// </summary>
        public event Action<string> TextChanged;

        /// <summary>
        /// The panel control where you should add your items.
        /// </summary>
        public readonly VerticalPanel ItemsPanel;

        /// <summary>
        /// Initializes a new instance of the <see cref="ItemsListContextMenu"/> class.
        /// </summary>
        /// <param name="width">The control width.</param>
        /// <param name="height">The control height.</param>
        public ItemsListContextMenu(float width = 320, float height = 220)
        {
            // Context menu dimensions
            Size = new Float2(width, height);

            // Search box
            _searchBox = new SearchBox(false, 1, 1)
            {
                Parent = this,
                Width = Width - 3,
            };
            _searchBox.TextChanged += OnSearchFilterChanged;

            // Panel with scrollbar
            _scrollPanel = new Panel(ScrollBars.Vertical)
            {
                Parent = this,
                AnchorPreset = AnchorPresets.StretchAll,
                Bounds = new Rectangle(0, _searchBox.Bottom + 1, Width, Height - _searchBox.Bottom - 2),
            };

            // Items list panel
            ItemsPanel = new VerticalPanel
            {
                Parent = _scrollPanel,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
            };
        }

        private void OnSearchFilterChanged()
        {
            if (IsLayoutLocked)
                return;

            LockChildrenRecursive();

            var items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                if (items[i] is Item item)
                    item.UpdateFilter(_searchBox.Text);
            }
            if (_categoryPanels != null)
            {
                for (int i = 0; i < _categoryPanels.Count; i++)
                {
                    var category = _categoryPanels[i];
                    bool anyVisible = false;
                    for (int j = 0; j < category.Children.Count; j++)
                    {
                        if (category.Children[j] is Item item2)
                        {
                            item2.UpdateFilter(_searchBox.Text);
                            anyVisible |= item2.Visible;
                        }
                    }
                    category.Visible = anyVisible;
                    if (string.IsNullOrEmpty(_searchBox.Text))
                        category.Close(false);
                    else
                        category.Open(false);
                }
            }

            UnlockChildrenRecursive();
            PerformLayout(true);
            _searchBox.Focus();
            TextChanged?.Invoke(_searchBox.Text);
        }

        /// <summary>
        /// Scrolls the scroll panel to a specific Item
        /// </summary>
        /// <param name="item">The item to scroll to.</param>
        public void ScrollViewTo(Item item)
        {
            _scrollPanel.ScrollViewTo(item, true);
        }

        /// <summary>
        /// Scrolls to the item and focuses it by name.
        /// </summary>
        /// <param name="itemName">The item name.</param>
        public void ScrollToAndHighlightItemByName(string itemName)
        {
            foreach (var child in ItemsPanel.Children)
            {
                if (child is not ItemsListContextMenu.Item item)
                    continue;
                if (string.Equals(item.Name, itemName, StringComparison.Ordinal))
                {
                    // Highlight and scroll to item
                    item.Focus();
                    ScrollViewTo(item);
                    break;
                }
            }
        }

        /// <summary>
        /// Sorts the items list (by item name by default).
        /// </summary>
        public void SortItems()
        {
            ItemsPanel.SortChildren();
            if (_categoryPanels != null)
            {
                for (int i = 0; i < _categoryPanels.Count; i++)
                    _categoryPanels[i].SortChildren();
            }
        }

        /// <summary>
        /// Adds the item to the view and registers for the click event.
        /// </summary>
        /// <param name="item">The item.</param>
        public void AddItem(Item item)
        {
            item.Clicked += OnClickItem;
            ContainerControl parent = ItemsPanel;
            if (!string.IsNullOrEmpty(item.Category))
            {
                if (_categoryPanels == null)
                    _categoryPanels = new List<DropPanel>();
                for (int i = 0; i < _categoryPanels.Count; i++)
                {
                    if (string.Equals(_categoryPanels[i].HeaderText, item.Category, StringComparison.Ordinal))
                    {
                        parent = _categoryPanels[i];
                        break;
                    }
                }
                if (parent == ItemsPanel)
                {
                    var categoryPanel = new DropPanel
                    {
                        HeaderText = item.Category,
                        ArrowImageOpened = new SpriteBrush(Editor.Instance.Icons.ArrowDown12),
                        ArrowImageClosed = new SpriteBrush(Editor.Instance.Icons.ArrowRight12),
                        EnableDropDownIcon = true,
                        ItemsMargin = new Margin(28, 0, 2, 2),
                        HeaderColor = Style.Current.Background,
                        Parent = parent,
                    };
                    categoryPanel.Close(false);
                    _categoryPanels.Add(categoryPanel);
                    parent = categoryPanel;
                }
            }
            item.Parent = parent;
        }

        /// <summary>
        /// Called when user clicks on an item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void OnClickItem(Item item)
        {
            Hide();
            ItemClicked?.Invoke(item);
        }

        /// <summary>
        /// Resets the view.
        /// </summary>
        public void ResetView()
        {
            LockChildrenRecursive();

            var items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                if (items[i] is Item item)
                    item.UpdateFilter(null);
            }
            if (_categoryPanels != null)
            {
                for (int i = 0; i < _categoryPanels.Count; i++)
                {
                    var category = _categoryPanels[i];
                    for (int j = 0; j < category.Children.Count; j++)
                    {
                        if (category.Children[j] is Item item2)
                            item2.UpdateFilter(null);
                    }
                    category.Visible = true;
                    category.Close(false);
                }
            }

            _searchBox.Clear();
            UnlockChildrenRecursive();
            PerformLayout(true);
        }

        private List<Item> GetVisibleItems()
        {
            var result = new List<Item>();
            var items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                if (items[i] is Item item && item.Visible)
                    result.Add(item);
            }
            if (_categoryPanels != null)
            {
                for (int i = 0; i < _categoryPanels.Count; i++)
                {
                    var category = _categoryPanels[i];
                    if (!category.Visible)
                        continue;
                    for (int j = 0; j < category.Children.Count; j++)
                    {
                        if (category.Children[j] is Item item2 && item2.Visible)
                            result.Add(item2);
                    }
                }
            }
            return result;
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Prepare
            ResetView();
            Focus();
            _waitingForInput = true;

            base.OnShow();
        }

        /// <inheritdoc />
        public override void Hide()
        {
            Focus(null);

            base.Hide();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            var focusedItem = RootWindow.FocusedControl as Item;
            switch (key)
            {
            case KeyboardKeys.Escape:
                Hide();
                return true;
            case KeyboardKeys.ArrowDown:
            {
                if (RootWindow.FocusedControl == null)
                {
                    // Focus search box if nothing is focused
                    _searchBox.Focus();
                    return true;
                }

                //  Focus the first visible item or then next one
                var items = GetVisibleItems();
                var focusedIndex = items.IndexOf(focusedItem);
                if (focusedIndex == -1)
                    focusedIndex = -1;
                if (focusedIndex + 1 < items.Count)
                {
                    var item = items[focusedIndex + 1];
                    item.Focus();
                    _scrollPanel.ScrollViewTo(item);
                    return true;
                }
                break;
            }
            case KeyboardKeys.ArrowUp:
                if (focusedItem != null)
                {
                    //  Focus the previous visible item or the search box
                    var items = GetVisibleItems();
                    var focusedIndex = items.IndexOf(focusedItem);
                    if (focusedIndex == 0)
                    {
                        _searchBox.Focus();
                    }
                    else if (focusedIndex > 0)
                    {
                        var item = items[focusedIndex - 1];
                        item.Focus();
                        _scrollPanel.ScrollViewTo(item);
                        return true;
                    }
                }
                break;
            case KeyboardKeys.Return:
                if (focusedItem != null)
                {
                    OnClickItem(focusedItem);
                    return true;
                }
                break;
            }

            if (_waitingForInput)
            {
                _waitingForInput = false;
                _searchBox.Focus();
                return _searchBox.OnKeyDown(key);
            }

            return base.OnKeyDown(key);
        }
    }
}
