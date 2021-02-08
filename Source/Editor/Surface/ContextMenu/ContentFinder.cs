// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Modules;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.ContextMenu
{
    /// <summary>
    /// The content finder popup. Allows to search for project items and quickly navigate to.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ContextMenu.ContextMenuBase" />
    [HideInEditor]
    public class ContentFinder : ContextMenuBase
    {
        private Panel _resultPanel;
        private TextBox _searchBox;
        private SearchItem _selectedItem;

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
        public SearchItem SelectedItem
        {
            get => _selectedItem;
            set
            {
                if (value == _selectedItem || (value != null && !MatchedItems.Contains(value)))
                    return;

                if (_selectedItem != null)
                {
                    _selectedItem.BackgroundColor = Color.Transparent;
                }

                _selectedItem = value;

                if (_selectedItem != null)
                {
                    _selectedItem.BackgroundColor = Style.Current.BackgroundSelected;

                    if (MatchedItems.Count > VisibleItemCount)
                    {
                        _resultPanel.VScrollBar.SmoothingScale = 0;
                        _resultPanel.ScrollViewTo(_selectedItem);
                        _resultPanel.VScrollBar.SmoothingScale = 1;
                    }
                }
            }
        }

        /// <summary>
        /// Gets actual matched item list.
        /// </summary>
        public List<SearchItem> MatchedItems { get; } = new List<SearchItem>();

        internal bool Hand;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentFinder"/> class.
        /// </summary>
        /// <param name="width">The finder width.</param>
        public ContentFinder(float width = 440.0f)
        {
            Size = new Vector2(width, TextBox.DefaultHeight + 2.0f);

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
                Location = new Vector2(1, _searchBox.Height + 1),
                Size = new Vector2(width - 2.0f, Height - (_searchBox.Height + 1 + 1)),
                Parent = this
            };
        }

        private void OnTextChanged()
        {
            MatchedItems.Clear();
            SelectedItem = null;

            var results = Editor.Instance.ContentFinding.Search(_searchBox.Text);
            BuildList(results);
        }

        private void BuildList(List<SearchResult> items)
        {
            _resultPanel.DisposeChildren();

            var dpiScale = RootWindow.DpiScale;

            if (items.Count == 0)
            {
                Height = _searchBox.Height + 1;
                _resultPanel.ScrollBars = ScrollBars.None;
                RootWindow.Window.ClientSize = new Vector2(RootWindow.Window.ClientSize.X, Height * dpiScale);
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
                if (item.Item is AssetItem assetItem)
                    searchItem = new AssetSearchItem(item.Name, item.Type, assetItem, this, itemsWidth, itemHeight);
                else
                    searchItem = new SearchItem(item.Name, item.Type, item.Item, this, itemsWidth, itemHeight);
                searchItem.Y = i * itemHeight;
                searchItem.Parent = _resultPanel;
                MatchedItems.Add(searchItem);
            }

            RootWindow.Window.ClientSize = new Vector2(RootWindow.Window.ClientSize.X, Height * dpiScale);

            PerformLayout();
        }

        /// <inheritdoc />
        public override void Show(Control parent, Vector2 location)
        {
            base.Show(parent, location);

            // Setup
            _resultPanel.ScrollViewTo(Vector2.Zero);
            _searchBox.Text = string.Empty;
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
            {
                if (MatchedItems.Count == 0)
                    return true;
                int currentPos;
                if (_selectedItem != null)
                {
                    currentPos = MatchedItems.IndexOf(_selectedItem) + 1;
                    if (currentPos >= MatchedItems.Count)
                        currentPos--;
                }
                else
                {
                    currentPos = 0;
                }
                SelectedItem = MatchedItems[currentPos];
                return true;
            }
            case KeyboardKeys.ArrowUp:
            {
                if (MatchedItems.Count == 0)
                    return true;
                int currentPos;
                if (_selectedItem != null)
                {
                    currentPos = MatchedItems.IndexOf(_selectedItem) - 1;
                    if (currentPos < 0)
                        currentPos = 0;
                }
                else
                {
                    currentPos = 0;
                }
                SelectedItem = MatchedItems[currentPos];
                return true;
            }
            case KeyboardKeys.Return:
            {
                if (_selectedItem != null)
                {
                    Hide();
                    Editor.Instance.ContentFinding.Open(_selectedItem.Item);
                }
                else if (_selectedItem == null && _searchBox.TextLength != 0 && MatchedItems.Count != 0)
                {
                    Hide();
                    Editor.Instance.ContentFinding.Open(MatchedItems[0].Item);
                }
                return true;
            }
            case KeyboardKeys.Escape:
            {
                Hide();
                return true;
            }
            }

            return base.OnKeyDown(key);
        }
    }
}
