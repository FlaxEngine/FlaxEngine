// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Dropdown menu control allows to choose one item from the provided collection of options.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class Dropdown : Control
    {
        /// <summary>
        /// The root control used by the <see cref="Dropdown"/> to show the items collections and track item selecting event.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Panel" />
        [HideInEditor]
        protected class DropdownRoot : Panel
        {
            private bool isMouseDown;

            /// <summary>
            /// Occurs when item gets clicked. Argument is item index.
            /// </summary>
            public Action<int> ItemClicked;

            /// <summary>
            /// Occurs when popup losts focus.
            /// </summary>
            public Action LostFocus;

            /// <summary>
            /// The items container control.
            /// </summary>
            public ContainerControl ItemsContainer;

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                isMouseDown = true;
                var result = base.OnMouseDown(location, button);
                isMouseDown = false;

                if (!result)
                    return false;

                var itemIndex = ItemsContainer?.GetChildIndexAt(location) ?? -1;
                if (itemIndex != -1)
                    ItemClicked(itemIndex);

                return true;
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                base.OnLostFocus();

                if (!isMouseDown)
                    LostFocus();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                ItemClicked = null;
                LostFocus = null;
                ItemsContainer = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// The items.
        /// </summary>
        protected List<string> _items = new List<string>();

        /// <summary>
        /// The popup menu. May be null if has not been used yet.
        /// </summary>
        protected DropdownRoot _popup;

        private bool _mouseDown;

        /// <summary>
        /// The selected index of the item (-1 for no selection).
        /// </summary>
        protected int _selectedIndex = -1;

        /// <summary>
        /// Gets or sets the items collection.
        /// </summary>
        [EditorOrder(1), Tooltip("The items collection.")]
        public List<string> Items
        {
            get => _items;
            set => _items = value;
        }

        /// <summary>
        /// Gets or sets the selected item (returns <see cref="string.Empty"/> if no item is being selected).
        /// </summary>
        [HideInEditor, NoSerialize]
        public string SelectedItem
        {
            get => _selectedIndex != -1 ? _items[_selectedIndex] : string.Empty;
            set => SelectedIndex = _items.IndexOf(value);
        }

        /// <summary>
        /// Gets or sets the index of the selected.
        /// </summary>
        [EditorOrder(2), Limit(-1), Tooltip("The index of the selected item from the list.")]
        public int SelectedIndex
        {
            get => _selectedIndex;
            set
            {
                // Clamp index
                value = Mathf.Min(value, _items.Count - 1);

                // Check if index will change
                if (value != _selectedIndex)
                {
                    // Select
                    _selectedIndex = value;
                    OnSelectedIndexChanged();
                }
            }
        }

        /// <summary>
        /// Event fired when selected index gets changed.
        /// </summary>
        public event Action<Dropdown> SelectedIndexChanged;

        /// <summary>
        /// Gets a value indicating whether this popup menu is opened.
        /// </summary>
        public bool IsPopupOpened => _popup != null;

        /// <summary>
        /// Gets or sets the font used to draw text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public FontReference Font { get; set; }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("Custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.")]
        public MaterialBase FontMaterial { get; set; }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BorderColor { get; set; }

        /// <summary>
        /// Gets or sets the background color when dropdown popup is opened.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2010)]
        public Color BackgroundColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the border color when dropdown popup is opened.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2020)]
        public Color BorderColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the background color when dropdown is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BackgroundColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the border color when dropdown is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BorderColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the image used to render dropdown drop arrow icon.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The image used to render dropdown drop arrow icon.")]
        public IBrush ArrowImage { get; set; }

        /// <summary>
        /// Gets or sets the color used to render dropdown drop arrow icon.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color used to render dropdown drop arrow icon.")]
        public Color ArrowColor { get; set; }

        /// <summary>
        /// Gets or sets the color used to render dropdown drop arrow icon (menu is opened).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color used to render dropdown drop arrow icon (menu is opened).")]
        public Color ArrowColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the color used to render dropdown drop arrow icon (menu is highlighted).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color used to render dropdown drop arrow icon (menu is highlighted).")]
        public Color ArrowColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the image used to render dropdown checked item icon.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The image used to render dropdown checked item icon.")]
        public IBrush CheckedImage { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="Dropdown"/> class.
        /// </summary>
        public Dropdown()
        : base(0, 0, 120, 18.0f)
        {
            var style = Style.Current;
            Font = new FontReference(style.FontMedium);
            TextColor = style.Foreground;
            BackgroundColor = style.BackgroundNormal;
            BackgroundColorHighlighted = BackgroundColor;
            BackgroundColorSelected = BackgroundColor;
            BorderColor = style.BorderNormal;
            BorderColorHighlighted = style.BorderSelected;
            BorderColorSelected = BorderColorHighlighted;
            ArrowImage = new SpriteBrush(style.ArrowDown);
            ArrowColor = style.Foreground * 0.6f;
            ArrowColorSelected = style.BackgroundSelected;
            ArrowColorHighlighted = style.Foreground;
            CheckedImage = new SpriteBrush(style.CheckBoxTick);
        }

        /// <summary>
        /// Clears the items.
        /// </summary>
        public void ClearItems()
        {
            SelectedIndex = -1;
            _items.Clear();
        }

        /// <summary>
        /// Adds the item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void AddItem(string item)
        {
            _items.Add(item);
        }

        /// <summary>
        /// Adds the items.
        /// </summary>
        /// <param name="items">The items.</param>
        public void AddItems(IEnumerable<string> items)
        {
            _items.AddRange(items);
        }

        /// <summary>
        /// Sets the items.
        /// </summary>
        /// <param name="items">The items.</param>
        public void SetItems(IEnumerable<string> items)
        {
            SelectedIndex = -1;
            _items.Clear();
            _items.AddRange(items);
        }

        /// <summary>
        /// Called when selected item index gets changed.
        /// </summary>
        protected virtual void OnSelectedIndexChanged()
        {
            SelectedIndexChanged?.Invoke(this);
        }

        /// <summary>
        /// Called when item is clicked.
        /// </summary>
        /// <param name="index">The index.</param>
        protected virtual void OnItemClicked(int index)
        {
            SelectedIndex = index;
        }

        /// <summary>
        /// Creates the popup menu (including items collection).
        /// </summary>
        protected virtual DropdownRoot CreatePopup()
        {
            // TODO: support using templates for the items collection container panel

            var popup = new DropdownRoot();

            // TODO: support item templates

            var container = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                BackgroundColor = BackgroundColor,
                AutoSize = false,
                Parent = popup,
            };
            var border = new Border
            {
                BorderColor = BorderColorHighlighted,
                Width = 4.0f,
                AnchorPreset = AnchorPresets.StretchAll,
                Parent = popup,
            };

            var itemsHeight = 20.0f;
            var itemsMargin = 20.0f;

            /*
            var itemsWidth = 40.0f;
            var font = Font.GetFont();
            for (int i = 0; i < _items.Count; i++)
            {
                itemsWidth = Mathf.Max(itemsWidth, itemsMargin + 4 + font.MeasureText(_items[i]).X);
            }
            */
            var itemsWidth = Width;

            for (int i = 0; i < _items.Count; i++)
            {
                var item = new Spacer
                {
                    Height = itemsHeight,
                    Width = itemsWidth,
                    Parent = container,
                };

                var label = new Label
                {
                    X = itemsMargin,
                    Width = itemsWidth - itemsMargin,
                    Font = Font,
                    TextColor = Color.White * 0.9f,
                    TextColorHighlighted = Color.White,
                    HorizontalAlignment = TextAlignment.Near,
                    AnchorPreset = AnchorPresets.VerticalStretchRight,
                    Text = _items[i],
                    Parent = item,
                };

                if (_selectedIndex == i)
                {
                    var icon = new Image
                    {
                        Brush = CheckedImage,
                        Width = itemsMargin,
                        Margin = new Margin(4.0f, 6.0f, 4.0f, 4.0f),
                        AnchorPreset = AnchorPresets.VerticalStretchLeft,
                        Parent = item,
                    };
                }
            }

            popup.Size = new Vector2(itemsWidth, (itemsHeight + container.Spacing) * _items.Count + container.Margin.Height);
            popup.ItemsContainer = container;

            return popup;
        }

        /// <summary>
        /// Destroys the popup.
        /// </summary>
        protected virtual void DestroyPopup()
        {
            if (_popup != null)
            {
                _popup.Dispose();
                _popup = null;
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            DestroyPopup();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var clientRect = new Rectangle(Vector2.Zero, Size);
            float margin = clientRect.Height * 0.2f;
            float boxSize = clientRect.Height - margin * 2;
            bool isOpened = IsPopupOpened;
            bool enabled = EnabledInHierarchy;
            Color backgroundColor = BackgroundColor;
            Color borderColor = BorderColor;
            Color arrowColor = ArrowColor;
            if (!enabled)
            {
                backgroundColor *= 0.5f;
                arrowColor *= 0.7f;
            }
            else if (isOpened || _mouseDown)
            {
                backgroundColor = BackgroundColorSelected;
                borderColor = BorderColorSelected;
                arrowColor = ArrowColorSelected;
            }
            else if (IsMouseOver)
            {
                backgroundColor = BackgroundColorHighlighted;
                borderColor = BorderColorHighlighted;
                arrowColor = ArrowColorHighlighted;
            }

            // Background
            Render2D.FillRectangle(clientRect, backgroundColor);
            Render2D.DrawRectangle(clientRect, borderColor);

            // Check if has selected item
            if (_selectedIndex > -1 && _selectedIndex < _items.Count)
            {
                // Draw text of the selected item
                var textRect = new Rectangle(margin, 0, clientRect.Width - boxSize - 2.0f * margin, clientRect.Height);
                Render2D.PushClip(textRect);
                var textColor = TextColor;
                Render2D.DrawText(Font.GetFont(), FontMaterial, _items[_selectedIndex], textRect, enabled ? textColor : textColor * 0.5f, TextAlignment.Near, TextAlignment.Center);
                Render2D.PopClip();
            }

            // Arrow
            ArrowImage?.Draw(new Rectangle(clientRect.Width - margin - boxSize, margin, boxSize, boxSize), arrowColor);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();

            // Clear flags
            _mouseDown = false;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flags
            _mouseDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            // Check mouse buttons
            if (button == MouseButton.Left)
            {
                // Set flag
                _mouseDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            // Check flags
            if (_mouseDown)
            {
                // Clear flag
                _mouseDown = false;

                var root = Root;
                if (_items.Count > 0 && root != null)
                {
                    // Setup popup
                    DestroyPopup();
                    _popup = CreatePopup();

                    // Update layout
                    _popup.UnlockChildrenRecursive();
                    _popup.PerformLayout();

                    // Bind events
                    _popup.ItemClicked += (index) =>
                    {
                        OnItemClicked(index);
                        DestroyPopup();
                    };
                    _popup.LostFocus += DestroyPopup;

                    // Show dropdown popup
                    Vector2 locationRootSpace = Location + new Vector2(0, Height);
                    var parent = Parent;
                    while (parent != null && parent != Root)
                    {
                        locationRootSpace = parent.PointToParent(ref location);
                        parent = parent.Parent;
                    }
                    _popup.Location = locationRootSpace;
                    _popup.Parent = root;
                    _popup.Focus();
                }
            }

            return true;
        }
    }
}
