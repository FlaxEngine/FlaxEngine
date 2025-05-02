// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Popup menu control.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    [HideInEditor]
    public class ContextMenu : ContextMenuBase
    {
        /// <summary>
        /// The items container.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Panel" />
        [HideInEditor]
        protected class ItemsPanel : Panel
        {
            private readonly ContextMenu _menu;

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemsPanel"/> class.
            /// </summary>
            /// <param name="menu">The menu.</param>
            public ItemsPanel(ContextMenu menu)
            : base(ScrollBars.Vertical)
            {
                _menu = menu;
            }

            /// <inheritdoc />
            protected override void Arrange()
            {
                base.Arrange();

                // Arrange controls
                Margin margin = _menu._itemsMargin;
                float y = 0;
                float width = Width - margin.Width;
                for (int i = 0; i < _children.Count; i++)
                {
                    if (_children[i] is ContextMenuItem item && item.Visible)
                    {
                        var height = item.Height;
                        item.Bounds = new Rectangle(margin.Left, y, width, height);
                        y += height + margin.Height;
                    }
                }
            }
        }

        /// <summary>
        /// The items area margin.
        /// </summary>
        protected Margin _itemsAreaMargin = new Margin(0, 0, 3, 3);

        /// <summary>
        /// The items margin.
        /// </summary>
        protected Margin _itemsMargin = new Margin(16, 0, 2, 0);

        /// <summary>
        /// The items panel.
        /// </summary>
        protected ItemsPanel _panel;

        /// <summary>
        /// Gets or sets the items area margin (items container area margin).
        /// </summary>
        public Margin ItemsAreaMargin
        {
            get => _itemsAreaMargin;
            set
            {
                _itemsAreaMargin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the items margin.
        /// </summary>
        public Margin ItemsMargin
        {
            get => _itemsMargin;
            set
            {
                _itemsMargin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the minimum popup width.
        /// </summary>
        public float MinimumWidth { get; set; }

        /// <summary>
        /// Gets or sets the maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.
        /// </summary>
        public int MaximumItemsInViewCount { get; set; }

        /// <summary>
        /// Gets the items (readonly).
        /// </summary>
        public IEnumerable<ContextMenuItem> Items => _panel.Children.OfType<ContextMenuItem>();

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public event Action<ContextMenuButton> ButtonClicked;

        /// <summary>
        /// Gets the context menu items container control.
        /// </summary>
        public Panel ItemsContainer => _panel;

        /// <summary>
        /// The auto sort.
        /// </summary>
        private bool _autosort;

        /// <summary>
        /// The auto sort property.
        /// </summary>
        public bool AutoSort
        {
            get => _autosort;
            set
            {
                _autosort = value;
                if (_autosort)
                    SortButtons();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenu"/> class.
        /// </summary>
        public ContextMenu()
        {
            MinimumWidth = 10;
            MaximumItemsInViewCount = 20;

            _panel = new ItemsPanel(this)
            {
                ClipChildren = false,
                Parent = this,
            };
        }

        /// <summary>
        /// Sorts all <see cref="ContextMenuButton"/> alphabetically.
        /// </summary>
        /// <param name="force">Overrides <see cref="AutoSort"/> property.</param>
        public void SortButtons(bool force = false)
        {
            if (!_autosort && !force)
                return;
            _panel.Children.Sort((control, control1) =>
            {
                if (control is ContextMenuButton cmb && control1 is ContextMenuButton cmb1)
                    return string.Compare(cmb.Text, cmb1.Text, StringComparison.OrdinalIgnoreCase);
                if (!(control is ContextMenuButton))
                    return 1;
                return -1;
            });
        }

        /// <summary>
        /// Removes all the added items (buttons, separators, etc.).
        /// </summary>
        public void DisposeAllItems()
        {
            for (int i = _panel.ChildrenCount - 1; _panel.ChildrenCount > 0 && i >= 0; i--)
            {
                if (_panel.Children[i] is ContextMenuItem)
                    _panel.Children[i].Dispose();
            }
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text)
        {
            var item = new ContextMenuButton(this, text)
            {
                Parent = _panel
            };
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The short keys.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, string shortKeys)
        {
            var item = new ContextMenuButton(this, text, shortKeys)
            {
                Parent = _panel
            };
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, Action clicked)
        {
            var item = new ContextMenuButton(this, text)
            {
                Parent = _panel
            };
            item.Clicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, Action<ContextMenuButton> clicked)
        {
            var item = new ContextMenuButton(this, text)
            {
                Parent = _panel
            };
            item.ButtonClicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The shortKeys.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, string shortKeys, Action clicked)
        {
            var item = new ContextMenuButton(this, text, shortKeys)
            {
                Parent = _panel
            };
            item.Clicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="binding">The input binding.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, InputBinding binding, Action clicked)
        {
            var item = new ContextMenuButton(this, text, binding.ToString())
            {
                Parent = _panel
            };
            item.Clicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Gets the child menu (with that name).
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control or null if missing.</returns>
        public ContextMenuChildMenu GetChildMenu(string text)
        {
            for (int i = 0; i < _panel.ChildrenCount; i++)
            {
                if (_panel.Children[i] is ContextMenuChildMenu menu && menu.Text == text)
                    return menu;
            }
            return null;
        }

        /// <summary>
        /// Adds the child menu or gets it if already created (with that name).
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuChildMenu GetOrAddChildMenu(string text)
        {
            var item = GetChildMenu(text);
            if (item == null)
            {
                item = new ContextMenuChildMenu(this, text)
                {
                    Parent = _panel
                };
            }
            return item;
        }

        /// <summary>
        /// Adds the child menu.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuChildMenu AddChildMenu(string text)
        {
            var item = new ContextMenuChildMenu(this, text)
            {
                Parent = _panel
            };
            return item;
        }

        /// <summary>
        /// Adds the separator.
        /// </summary>
        public void AddSeparator()
        {
            var item = new ContextMenuSeparator(this)
            {
                Parent = _panel
            };
        }

        /// <summary>
        /// Called when button get clicked.
        /// </summary>
        /// <param name="button">The button.</param>
        public virtual void OnButtonClicked(ContextMenuButton button)
        {
            ButtonClicked?.Invoke(button);
        }

        /// <inheritdoc />
        public override void Show(Control parent, Float2 location, ContextMenuDirection? direction = null)
        {
            // Remove last separator to make context menu look better
            int lastIndex = _panel.Children.Count - 1;
            if (lastIndex >= 0 && _panel.Children[lastIndex] is ContextMenuSeparator separator)
                separator.Dispose();

            base.Show(parent, location, direction);
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise)
        {
            if (base.ContainsPoint(ref location, precise))
                return true;

            var cLocation = location - Location;
            for (int i = 0; i < _panel.Children.Count; i++)
            {
                if (_panel.Children[i].ContainsPoint(ref cLocation, precise))
                    return true;
            }

            return false;
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            var prevSize = Size;

            // Calculate size of the context menu (items only)
            float maxWidth = 0;
            float height = _itemsAreaMargin.Height;
            int itemsLeft = MaximumItemsInViewCount;
            int overflowItemCount = 0;
            int itemsCount = 0;
            for (int i = 0; i < _panel.Children.Count; i++)
            {
                if (_panel.Children[i] is ContextMenuItem item && item.Visible)
                {
                    itemsCount++;
                    if (itemsLeft > 0)
                    {
                        height += item.Height + _itemsMargin.Height;
                        itemsLeft--;
                    }
                    else
                    {
                        overflowItemCount++;
                    }
                    maxWidth = Mathf.Max(maxWidth, item.MinimumWidth);
                }
            }
            if (itemsCount != 0)
                height -= _itemsMargin.Height; // Remove item margin from top and bottom
            maxWidth = Mathf.Max(maxWidth + 20, MinimumWidth);

            // Move child arrows to accommodate scroll bar showing 
            if (overflowItemCount > 0)
            {
                foreach (var child in _panel.Children)
                {
                    if (child is ContextMenuButton item && item.Visible)
                    {
                        item.ExtraAdjustmentAmount = -_panel.VScrollBar.Width;
                    }
                }
            }

            // Resize container
            Size = new Float2(Mathf.Ceil(maxWidth), Mathf.Ceil(height));

            // Arrange items view panel
            var panelBounds = new Rectangle(Float2.Zero, Size);
            _itemsAreaMargin.ShrinkRectangle(ref panelBounds);
            _panel.Bounds = panelBounds;

            // Check if is visible size get changed
            if (Visible && prevSize != Size)
            {
                // Update window dimensions
                UpdateWindowSize();
            }
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
                return true;

            // Find the item that starts with that character
            if (char.IsLetterOrDigit(c) && _panel.VScrollBar != null && _panel.VScrollBar.Visible)
            {
                int startIndex = 0;
                for (int i = 0; i < _panel.Children.Count; i++)
                {
                    if (_panel.Children[i] is ContextMenuButton item && item.Visible && item.IsFocused)
                    {
                        // Start searching from the last hit item
                        startIndex = i + 1;
                        break;
                    }
                }
                for (int i = startIndex; i < _panel.Children.Count; i++)
                {
                    if (_panel.Children[i] is ContextMenuButton item && item.Visible)
                    {
                        bool startsWith = false;
                        for (int j = 0; j < item.Text.Length; j++)
                        {
                            var k = item.Text[j];
                            if (char.ToLower(k) == char.ToLower(c))
                            {
                                startsWith = true;
                                break;
                            }
                            if (!char.IsWhiteSpace(k) && k != '>')
                                break;
                        }
                        if (startsWith)
                        {
                            // Focus found item
                            item.Focus();
                            _panel.ScrollViewTo(item);
                            return true;
                        }
                    }
                }
                if (startIndex > 0 && startIndex <= _panel.Children.Count)
                {
                    // No more items found so start from the top if there are matching items
                    _panel.Children[startIndex - 1].Defocus();
                    return OnCharInput(c);
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.ArrowDown:
                for (int i = 0; i < _panel.Children.Count; i++)
                {
                    if (_panel.Children[i] is ContextMenuButton item && item.Visible && item.Enabled)
                    {
                        item.Focus();
                        _panel.ScrollViewTo(item);
                        return true;
                    }
                }
                break;
            case KeyboardKeys.ArrowUp:
                for (int i = _panel.Children.Count - 1; i >= 0; i--)
                {
                    if (_panel.Children[i] is ContextMenuButton item && item.Visible && item.Enabled)
                    {
                        item.Focus();
                        _panel.ScrollViewTo(item);
                        return true;
                    }
                }
                break;
            }

            return false;
        }
    }
}
