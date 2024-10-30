// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Modules;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Search
{
    // BONUS TODO: Make it perform the whole search to display list loop when created to always show entries, even when there hasen't been a search yet

    /// <summary>
    /// The content finder popup. Allows to search for project items and quickly navigate to.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ContextMenu.ContextMenuBase" />
    [HideInEditor]
    internal abstract class SearchableEditorPopup : ContextMenuBase
    {
        private SpriteHandle _icon;
        private float iconWidth;
        private Margin iconMargin = new Margin(3, 3, 2, 2);

        private TextBox _searchBox;
        private PopupItemBase _selectedItem;
        private List<PopupItemBase> _matchedItems = new List<PopupItemBase>();

        /// <summary>
        /// Gets or sets the height per item.
        /// </summary>
        public float ItemHeight { get; set; } = 20;

        /// <summary>
        /// Gets or sets the number of item to show.
        /// </summary>
        public int VisibleItemCount { get; set; }

        /// <summary>
        /// The panel that displays the search results.
        /// </summary>
        public Panel ResultPanel;

        /// <summary>
        /// Gets or sets the selected item.
        /// </summary>
        internal PopupItemBase SelectedItem
        {
            get => _selectedItem;
            set
            {
                if (value == _selectedItem || (value != null && !_matchedItems.Contains(value)))
                    return;

                if (_selectedItem != null)
                    _selectedItem.BackgroundColor = Color.Transparent;

                _selectedItem = value;

                if (_selectedItem != null)
                {
                    _selectedItem.BackgroundColor = Style.Current.BackgroundSelected;
                    if (_matchedItems.Count > VisibleItemCount)
                        ResultPanel.ScrollViewTo(_selectedItem, true);

                    // TODO: Find a better fix for defocus than this vv
                    //if (!_searchBox.IsEditing)
                        //_selectedItem.Focus();
                    
                }
            }
        }

        /// <summary>
        /// Gets actual matched item list.
        /// </summary>
        internal bool Hand;

        /// <summary>
        /// Initializes a new instance of the <see cref="SearchableEditorPopup"/> class.
        /// </summary>
        /// <param name="width">The finder width.</param>
        /// <param name="searchBarPrompt">The prompt displayed in the search bar.</param>
        /// <param name="visibleItemsCount">How many items the list displays.</param>
        /// <param name="icon">The icon to display alongside the popup.</param>
        public SearchableEditorPopup(SpriteHandle icon, float width = 440.0f, int visibleItemsCount = 12, string searchBarPrompt = "Type to search...")
        {
            Size = new Float2(width, TextBox.DefaultHeight + 2.0f);
            VisibleItemCount = visibleItemsCount;

            _icon = icon;
            iconWidth = 15;

            _searchBox = new TextBox
            {
                X = iconWidth + iconMargin.Width,
                Y = 1,
                Width = width - iconWidth - iconMargin.Width,
                WatermarkText = searchBarPrompt,
                Parent = this,
                //AutoFocus
            };
            _searchBox.TextChanged += OnTextChanged;

            ResultPanel = new Panel
            {
                Location = new Float2(1, _searchBox.Height + 1),
                Size = new Float2(width - 2.0f, Height - (_searchBox.Height + 1 + 1)),
                Parent = this,
            };

            Focus();

            // TODO: Perform search
            //Search(_searchBox.Text);
        }

        private void OnTextChanged()
        {
            _matchedItems.Clear();
            SelectedItem = null;

            var results = Search(_searchBox.Text);
            BuildList(results);
        }

        public override void Draw()
        {
            base.Draw();

            Rectangle iconRect = new Rectangle(iconMargin.Left, iconMargin.Top, iconWidth, iconWidth);
            Render2D.DrawSprite(_icon, iconRect);
        }

        public abstract List<SearchResult> Search(string prompt);

        private void BuildList(List<SearchResult> results)
        {
            ResultPanel.DisposeChildren();
            LockChildrenRecursive();

            var dpiScale = DpiScale;
            var window = RootWindow.Window;

            if (results.Count == 0)
            {
                Height = _searchBox.Height + 1;
                ResultPanel.ScrollBars = ScrollBars.None;
                window.ClientSize = new Float2(window.ClientSize.X, Height * dpiScale);
                UnlockChildrenRecursive();
                PerformLayout();
                return;
            }

            // Setup items container
            if (results.Count <= VisibleItemCount)
            {
                Height = 1 + _searchBox.Height + 1 + ItemHeight * results.Count;
                ResultPanel.ScrollBars = ScrollBars.None;
            }
            else
            {
                Height = 1 + _searchBox.Height + 1 + ItemHeight * VisibleItemCount;
                ResultPanel.ScrollBars = ScrollBars.Vertical;
            }

            ResultPanel.Height = ItemHeight * VisibleItemCount;
            var itemsWidth = ResultPanel.GetClientArea().Width;
            var itemHeight = ItemHeight;

            // Spawn items
            for (var i = 0; i < results.Count; i++)
            {
                var item = results[i];

                PopupItemBase searchItem = CreateSearchItemFromResult(item, itemsWidth, itemHeight, i);

                searchItem.Parent = ResultPanel;
                _matchedItems.Add(searchItem);
            }

            window.ClientSize = new Float2(window.ClientSize.X, Height * dpiScale);

            UnlockChildrenRecursive();
            PerformLayout();
        }

        public abstract PopupItemBase CreateSearchItemFromResult(SearchResult searchResult, float width, float height, int index);

        // TODO: Could get rid of this function with a type wildcard <T>, making the `SelectedItem` the base class again then then checking if the cast to <T> fails or not
        // TODO: ^^ Technically I could... (Only try this once the other stuff 100% works)
        public abstract void SelectItem();

        /// <inheritdoc />
        public override void Show(Control parent, Float2 location)
        {
            base.Show(parent, location);

            // Setup
            ResultPanel.ScrollViewTo(Float2.Zero);
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
                        if (_matchedItems.Count == 0)
                            return true;
                        int currentPos;
                        if (_selectedItem != null)
                        {
                            currentPos = _matchedItems.IndexOf(_selectedItem) + 1;
                            if (currentPos >= _matchedItems.Count)
                                currentPos--;
                        }
                        else
                            currentPos = 0;
                        SelectedItem = _matchedItems[currentPos];
                        return true;
                    }
                case KeyboardKeys.ArrowUp:
                    {
                        if (_matchedItems.Count == 0)
                            return true;
                        int currentPos;
                        if (_selectedItem != null)
                        {
                            currentPos = _matchedItems.IndexOf(_selectedItem) - 1;
                            if (currentPos < 0)
                                currentPos = 0;
                        }
                        else
                            currentPos = 0;
                        SelectedItem = _matchedItems[currentPos];
                        return true;
                    }
                case KeyboardKeys.Return:
                    {
                        if (SelectedItem != null)
                            SelectItem();

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
