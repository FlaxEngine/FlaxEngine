// Copyright (c) Wojciech Figat. All rights reserved.

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
            private bool _isStartsWithMatch;
            private bool _isFullMatch;

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
            /// A computed score for the context menu order
            /// </summary>
            public float SortScore;

            /// <summary>
            /// Occurs when items gets clicked by the user.
            /// </summary>
            public event Action<Item> Clicked;

            /// <summary>
            /// Occurs when items gets focused.
            /// </summary>
            public event Action<Item> Focused;

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
            /// Updates the <see cref="SortScore"/>
            /// </summary>
            public void UpdateScore()
            {
                SortScore = 0;

                if (!Visible)
                    return;

                if (_highlights is { Count: > 0 })
                    SortScore += 1;
                if (_isStartsWithMatch)
                    SortScore += 2;
                if (_isFullMatch)
                    SortScore += 5;
            }

            /// <summary>
            /// Updates the filter.
            /// </summary>
            /// <param name="filterText">The filter text.</param>
            public void UpdateFilter(string filterText)
            {
                _isStartsWithMatch = _isFullMatch = false;

                if (string.IsNullOrWhiteSpace(filterText))
                {
                    // Clear filter
                    _highlights?.Clear();
                    Visible = true;
                    return;
                }

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

                        if (ranges[i].StartIndex <= 0)
                        {
                            _isStartsWithMatch = true;
                            if (ranges[i].Length == Name.Length)
                                _isFullMatch = true;
                        }
                    }
                    Visible = true;
                    return;
                }

                // Hide
                _highlights?.Clear();
                Visible = false;
            }

            /// <summary>
            /// Gets the text rectangle.
            /// </summary>
            /// <param name="rect">The output rectangle.</param>
            protected virtual void GetTextRect(out Rectangle rect)
            {
                rect = new Rectangle(2, 0, Width - 4, Height);

                // Indent for drop panel items is handled by drop panel margin
                if (Parent is not DropPanel)
                    rect.Location += new Float2(Editor.Instance.Icons.ArrowRight12.Size.X + 2, 0);
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
            public override void OnGotFocus()
            {
                base.OnGotFocus();

                Focused?.Invoke(this);
            }

            /// <inheritdoc />
            public override int Compare(Control other)
            {
                if (other is Item otherItem)
                {
                    int order = -1 * SortScore.CompareTo(otherItem.SortScore);
                    if (order == 0)
                        order = string.Compare(Name, otherItem.Name, StringComparison.Ordinal);

                    return order;
                }
                return base.Compare(other);
            }
        }

        private readonly SearchBox _searchBox;
        private readonly Panel _scrollPanel;
        private List<DropPanel> _categoryPanels;
        private bool _waitingForInput;
        private string _customSearch;

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
        /// <param name="withSearch">Enables search field.</param>
        public ItemsListContextMenu(float width = 320, float height = 220, bool withSearch = true)
        {
            // Context menu dimensions
            Size = new Float2(width, height);

            if (withSearch)
            {
                // Search box
                _searchBox = new SearchBox(false, 1, 1)
                {
                    Parent = this,
                    Width = Width - 3,
                };
                _searchBox.TextChanged += OnSearchFilterChanged;
                _searchBox.ClearSearchButton.Clicked += () => PerformLayout();
            }

            // Panel with scrollbar
            _scrollPanel = new Panel(ScrollBars.Vertical)
            {
                Parent = this,
                AnchorPreset = AnchorPresets.StretchAll,
                Bounds = withSearch ? new Rectangle(0, _searchBox.Bottom + 1, Width, Height - _searchBox.Bottom - 2) : new Rectangle(Float2.Zero, Size),
            };

            // Items list panel
            ItemsPanel = new VerticalPanel
            {
                Parent = _scrollPanel,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Pivot = Float2.Zero,
            };
        }

        private void OnSearchFilterChanged()
        {
            if (IsLayoutLocked)
                return;

            LockChildrenRecursive();

            var searchText = _searchBox?.Text ?? _customSearch;
            var items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                if (items[i] is Item item)
                {
                    item.UpdateFilter(searchText);
                    item.UpdateScore();
                }
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
                            item2.UpdateFilter(searchText);
                            item2.UpdateScore();
                            anyVisible |= item2.Visible;
                        }
                    }
                    category.Visible = anyVisible;
                    if (string.IsNullOrEmpty(searchText))
                        category.Close(false);
                    else
                        category.Open(false);
                }
            }

            SortItems();

            UnlockChildrenRecursive();
            PerformLayout(true);
            _searchBox?.Focus();
            TextChanged?.Invoke(searchText);
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
        /// Removes all added items.
        /// </summary>
        public void ClearItems()
        {
            ItemsPanel.DisposeChildren();
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
        /// Focuses and scroll to the given item to be selected.
        /// </summary>
        /// <param name="item">The item to select.</param>
        public void SelectItem(Item item)
        {
            item.Focus();
            ScrollViewTo(item);
        }

        /// <summary>
        /// Applies custom search text query on the items list. Works even if search field is disabled
        /// </summary>
        /// <param name="text">The custom search text. Null to clear search.</param>
        public void Search(string text)
        {
            if (_searchBox != null)
            {
                _searchBox.SetText(text);
            }
            else
            {
                _customSearch = text;
                if (VisibleInHierarchy)
                    OnSearchFilterChanged();
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

            _searchBox?.Clear();
            UnlockChildrenRecursive();
            PerformLayout(true);
            if (_customSearch != null)
                OnSearchFilterChanged();
        }

        private List<Item> GetVisibleItems(bool ignoreFoldedCategories)
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
                    if (!category.Visible || (ignoreFoldedCategories && category is DropPanel panel && panel.IsClosed))
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

        private void ExpandToItem(Item item)
        {
            if (item.Parent is DropPanel dropPanel)
                dropPanel.Open(false);
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
            case KeyboardKeys.Backspace:
                // Allow the user to quickly focus the searchbar
                if (_searchBox != null && !_searchBox.IsFocused)
                {
                    _searchBox.Focus();
                    _searchBox.SelectAll();
                    return true;
                }
                break;
            case KeyboardKeys.ArrowDown:
            case KeyboardKeys.ArrowUp:
                if (RootWindow.FocusedControl == null)
                {
                    // Focus search box if nothing is focused
                    _searchBox?.Focus();
                    return true;
                }

                // Get the next item
                bool controlDown = Root.GetKey(KeyboardKeys.Control);
                var items = GetVisibleItems(!controlDown);
                var focusedIndex = items.IndexOf(focusedItem);

                // If the user hasn't selected anything yet and is holding control, focus first folded item
                if (focusedIndex == -1 && controlDown)
                    focusedIndex = GetVisibleItems(true).Count - 1;

                int delta = key == KeyboardKeys.ArrowDown ? -1 : 1;
                int nextIndex = Mathf.Wrap(focusedIndex - delta, 0, items.Count - 1);
                var nextItem = items[nextIndex];

                // Focus the next item
                nextItem.Focus();
            
                // Allow the user to expand groups while scrolling
                if (controlDown)
                    ExpandToItem(nextItem);
                
                _scrollPanel.ScrollViewTo(nextItem);
                return true;
            case KeyboardKeys.Return:
                if (focusedItem != null)
                {
                    OnClickItem(focusedItem);
                    return true;
                }
                else
                {
                    // Select first item if no item is focused (most likely to be the best result), saves the user from pressing arrow down first
                    var visibleItems = GetVisibleItems(true);
                    if (visibleItems.Count > 0)
                    {
                        OnClickItem(visibleItems[0]);
                        return true;
                    }
                }
                break;
            }

            if (_waitingForInput && _searchBox != null)
            {
                _waitingForInput = false;
                _searchBox.Focus();
                return _searchBox.OnKeyDown(key);
            }

            return base.OnKeyDown(key);
        }
    }
}
