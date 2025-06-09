// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Modules;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Search
{
    /// <summary>
    /// The content finder popup. Allows to search for project items and quickly navigate to.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ContextMenu.ContextMenuBase" />
    [HideInEditor]
    internal class ContentFinder : ContextMenuBase
    {
        private Panel _resultPanel;
        private TextBox _searchBox;
        private SearchItem _selectedItem;
        private List<SearchItem> _matchedItems = new List<SearchItem>();

        /// <summary>
        /// Gets or sets the height per item.
        /// </summary>
        public float ItemHeight { get; set; } = 20;

        /// <summary>
        /// Gets or sets the number of item to show.
        /// </summary>
        public int VisibleItemCount { get; set; } = 12;

        /// <summary>
        /// Gets or sets the selected item.
        /// </summary>
        internal SearchItem SelectedItem
        {
            get => _selectedItem;
            set
            {
                if (value == _selectedItem || (value != null && !_matchedItems.Contains(value)))
                    return;

                // Restore the previous selected item to the non-selected color
                if (_selectedItem != null)
                {
                    _selectedItem.BackgroundColor = Color.Transparent;
                }

                _selectedItem = value;

                if (_selectedItem != null)
                {
                    _selectedItem.BackgroundColor = Style.Current.BackgroundSelected;
                    if (_matchedItems.Count > VisibleItemCount)
                    {
                        _selectedItem.Focus();
                        _resultPanel.ScrollViewTo(_selectedItem, true);
                    }
                }
            }
        }

        /// <summary>
        /// Gets actual matched item list.
        /// </summary>
        internal bool Hand;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentFinder"/> class.
        /// </summary>
        /// <param name="width">The finder width.</param>
        public ContentFinder(float width = 440.0f)
        {
            Size = new Float2(width, TextBox.DefaultHeight + 2.0f);

            _searchBox = new TextBox
            {
                X = 1,
                Y = 1,
                Width = width - 2.0f,
                WatermarkText = "Type to search...",
                Parent = this
            };
            _searchBox.TextChanged += OnTextChanged;

            _resultPanel = new Panel
            {
                Location = new Float2(1, _searchBox.Height + 1),
                Size = new Float2(width - 2.0f, Height - (_searchBox.Height + 1 + 1)),
                Parent = this
            };
        }

        private void OnTextChanged()
        {
            _matchedItems.Clear();
            SelectedItem = null;

            var results = Editor.Instance.ContentFinding.Search(_searchBox.Text);
            BuildList(results);
        }

        private void BuildList(List<SearchResult> items)
        {
            _resultPanel.DisposeChildren();
            LockChildrenRecursive();

            var dpiScale = DpiScale;
            var window = RootWindow.Window;

            if (items.Count == 0)
            {
                Height = _searchBox.Height + 1;
                _resultPanel.ScrollBars = ScrollBars.None;
                window.ClientSize = new Float2(window.ClientSize.X, Height * dpiScale);
                UnlockChildrenRecursive();
                PerformLayout();
                return;
            }

            // Setup items container
            if (items.Count <= VisibleItemCount)
            {
                Height = 1 + _searchBox.Height + 1 + ItemHeight * items.Count;
                _resultPanel.ScrollBars = ScrollBars.None;
            }
            else
            {
                Height = 1 + _searchBox.Height + 1 + ItemHeight * VisibleItemCount;
                _resultPanel.ScrollBars = ScrollBars.Vertical;
            }

            _resultPanel.Height = ItemHeight * VisibleItemCount;
            var itemsWidth = _resultPanel.GetClientArea().Width;
            var itemHeight = ItemHeight;

            // Spawn items
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                SearchItem searchItem;
                if (item.Item is ContentItem contentItem)
                    searchItem = new ContentSearchItem(item.Name, item.Type, contentItem, this, itemsWidth, itemHeight);
                else
                    searchItem = new SearchItem(item.Name, item.Type, item.Item, this, itemsWidth, itemHeight);
                searchItem.Y = i * itemHeight;
                searchItem.Parent = _resultPanel;
                _matchedItems.Add(searchItem);
            }

            window.ClientSize = new Float2(window.ClientSize.X, Height * dpiScale);

            UnlockChildrenRecursive();
            PerformLayout();
        }

        /// <inheritdoc />
        public override void Show(Control parent, Float2 location, ContextMenuDirection? direction = null)
        {
            base.Show(parent, location, direction);

            // Setup
            _resultPanel.ScrollViewTo(Float2.Zero);
            // Select the text in the search bar so that the user can just start typing
            _searchBox.SelectAll();
            _searchBox.Focus();
        }

        /// <inheritdoc />
        public override void Update(float delta)
        {
            Hand = false;

            base.Update(delta);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.ArrowDown:
            case KeyboardKeys.ArrowUp:
            {
                if (_matchedItems.Count == 0)
                    return true;

                var focusedIndex = _matchedItems.IndexOf(_selectedItem);
                int delta = key == KeyboardKeys.ArrowDown ? -1 : 1;
                int nextIndex = Mathf.Wrap(focusedIndex - delta, 0, _matchedItems.Count - 1);
                var nextItem = _matchedItems[nextIndex];

                SelectedItem = nextItem;
                return true;
            }
            case KeyboardKeys.Return:
            {
                if (_selectedItem != null)
                {
                    Hide();
                    Editor.Instance.ContentFinding.Open(_selectedItem.Item);
                }
                else if (_selectedItem == null && _searchBox.TextLength != 0 && _matchedItems.Count != 0)
                {
                    Hide();
                    Editor.Instance.ContentFinding.Open(_matchedItems[0].Item);
                }
                return true;
            }
            case KeyboardKeys.Escape:
            {
                Hide();
                return true;
            }
            case KeyboardKeys.Backspace:
            { 
                // Alow the user to quickly focus the searchbar
                if (_searchBox != null && !_searchBox.IsFocused)
                {
                    _searchBox.Focus();
                    _searchBox.SelectAll();
                    return true;
                }
                break;
            }
            }

            return base.OnKeyDown(key);
        }
    }
}
