// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Dropdown menu control allows to choose one item from the provided collection of options.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class Dropdown : ContainerControl
    {
        /// <summary>
        /// The root control used by the <see cref="Dropdown"/> to show the items collections and track item selecting event.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Panel" />
        [HideInEditor]
        protected class DropdownRoot : Panel
        {
            private bool _isMouseDown;

            /// <summary>
            /// Occurs when item gets clicked. Argument is item index.
            /// </summary>
            public Action<int> ItemClicked;

            /// <summary>
            /// Occurs when popup lost focus.
            /// </summary>
            public Action LostFocus;

            /// <summary>
            /// The items container control.
            /// </summary>
            public ContainerControl ItemsContainer;

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                _isMouseDown = true;
                var result = base.OnMouseDown(location, button);
                _isMouseDown = false;

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

                if (!_isMouseDown)
                {
                    LostFocus?.Invoke();
                }
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
        protected List<LocalizedString> _items = new List<LocalizedString>();

        /// <summary>
        /// The popup menu. May be null if has not been used yet.
        /// </summary>
        protected DropdownRoot _popup;

        private bool _touchDown;

        /// <summary>
        /// The selected index of the item (-1 for no selection).
        /// </summary>
        protected int _selectedIndex = -1;

        /// <summary>
        /// Gets or sets the items collection.
        /// </summary>
        [EditorOrder(1), Tooltip("The items collection.")]
        public List<LocalizedString> Items
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
            get => _selectedIndex != -1 ? _items[_selectedIndex].ToString() : string.Empty;
            set => SelectedIndex = _items.IndexOf(value);
        }

        /// <summary>
        /// Gets or sets the selected item (returns <see cref="LocalizedString.Empty"/> if no item is being selected).
        /// </summary>
        [HideInEditor, NoSerialize]
        public LocalizedString SelectedItemLocalized
        {
            get => _selectedIndex != -1 ? _items[_selectedIndex] : LocalizedString.Empty;
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
                value = Mathf.Min(value, _items.Count - 1);
                if (value != _selectedIndex)
                {
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
            AutoFocus = false;

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
            foreach (var item in items)
                _items.Add(item);
        }

        /// <summary>
        /// Sets the items.
        /// </summary>
        /// <param name="items">The items.</param>
        public void SetItems(IEnumerable<string> items)
        {
            SelectedIndex = -1;
            _items.Clear();
            foreach (var item in items)
                _items.Add(item);
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
            var height = container.Margin.Height;

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
                    Size = new Vector2(itemsWidth - itemsMargin, itemsHeight),
                    Font = Font,
                    TextColor = Color.White * 0.9f,
                    TextColorHighlighted = Color.White,
                    HorizontalAlignment = TextAlignment.Near,
                    Text = _items[i],
                    Parent = item,
                };
                height += itemsHeight;
                if (i != 0)
                    height += container.Spacing;

                if (_selectedIndex == i)
                {
                    var icon = new Image
                    {
                        Brush = CheckedImage,
                        Size = new Vector2(itemsMargin, itemsHeight),
                        Margin = new Margin(4.0f, 6.0f, 4.0f, 4.0f),
                        //AnchorPreset = AnchorPresets.VerticalStretchLeft,
                        Parent = item,
                    };
                }
            }

            popup.Size = new Vector2(itemsWidth, height);
            popup.ItemsContainer = container;

            return popup;
        }

        /// <summary>
        /// Called when popup menu gets shown.
        /// </summary>
        protected virtual void OnPopupShow()
        {
        }

        /// <summary>
        /// Called when popup menu gets hidden.
        /// </summary>
        protected virtual void OnPopupHide()
        {
        }

        /// <summary>
        /// Destroys the popup.
        /// </summary>
        protected virtual void DestroyPopup()
        {
            if (_popup != null)
            {
                OnPopupHide();
                _popup.Dispose();
                _popup = null;
            }
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        public void ShowPopup()
        {
            var root = Root;
            if (_items.Count == 0 || root == null)
                return;

            // Setup popup
            DestroyPopup();
            _popup = CreatePopup();

            // Update layout
            _popup.UnlockChildrenRecursive();
            _popup.PerformLayout();

            // Bind events
            _popup.ItemClicked += index =>
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
                locationRootSpace = parent.PointToParent(ref locationRootSpace);
                parent = parent.Parent;
            }
            _popup.Location = locationRootSpace;
            _popup.Parent = root;
            _popup.Focus();
            OnPopupShow();
        }

        /// <summary>
        /// Hides the popup.
        /// </summary>
        public void HidePopup()
        {
            DestroyPopup();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            DestroyPopup();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void DrawSelf()
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
            else if (isOpened || _touchDown)
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
            _touchDown = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _touchDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _touchDown = true;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (_touchDown && button == MouseButton.Left)
            {
                _touchDown = false;
                ShowPopup();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Vector2 location, int pointerId)
        {
            if (base.OnTouchDown(location, pointerId))
                return true;

            _touchDown = true;
            return true;
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Vector2 location, int pointerId)
        {
            if (base.OnTouchUp(location, pointerId))
                return true;

            if (_touchDown)
            {
                ShowPopup();
            }
            return true;
        }

        /// <inheritdoc />
        public override void OnTouchLeave(int pointerId)
        {
            _touchDown = false;

            base.OnTouchLeave(pointerId);
        }
    }
}
