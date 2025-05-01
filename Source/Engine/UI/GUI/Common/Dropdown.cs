// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Dropdown menu control allows to choose one item from the provided collection of options.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [ActorToolbox("GUI")]
    public class Dropdown : ContainerControl
    {
        /// <summary>
        /// The root control used by the <see cref="Dropdown"/> to show the items collections and track item selecting event.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Panel" />
        [HideInEditor]
        protected class DropdownRoot : Panel
        {
            /// <summary>
            /// Occurs when popup lost focus.
            /// </summary>
            public Action LostFocus;

            /// <summary>
            /// The selected control. Used to scroll to the control on popup creation.
            /// </summary>
            public Control SelectedControl = null;

            /// <summary>
            /// The main panel used to hold the items.
            /// </summary>
            public Panel MainPanel = null;

            /// <inheritdoc />
            public override void OnEndContainsFocus()
            {
                base.OnEndContainsFocus();

                // Dont lose focus when using panel. Does prevent LostFocus even from being called if clicking inside of the panel.
                if (MainPanel != null && MainPanel.IsMouseOver && !MainPanel.ContainsFocus)
                {
                    MainPanel.Focus();
                    return;
                }
                // Call event after this 'focus contains flag' propagation ends to prevent focus issues
                if (LostFocus != null)
                    Scripting.RunOnUpdate(LostFocus);
            }

            private static DropdownLabel FindItem(Control control)
            {
                if (control is DropdownLabel item)
                    return item;
                if (control is ContainerControl containerControl)
                {
                    foreach (var child in containerControl.Children)
                    {
                        item = FindItem(child);
                        if (item != null)
                            return item;
                    }
                }
                return null;
            }

            /// <inheritdoc />
            public override Control OnNavigate(NavDirection direction, Float2 location, Control caller, List<Control> visited)
            {
                if (IsFocused)
                {
                    // Dropdown root is focused
                    if (direction == NavDirection.Down)
                    {
                        // Pick the first item
                        return FindItem(this);
                    }

                    // Close popup
                    Defocus();
                    return null;
                }

                return base.OnNavigate(direction, location, caller, visited);
            }

            /// <inheritdoc />
            public override void OnSubmit()
            {
                Defocus();

                base.OnSubmit();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (key == KeyboardKeys.Escape)
                {
                    Defocus();
                    return true;
                }

                return base.OnKeyDown(key);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;

                // Close on click outside the popup
                if (!new Rectangle(Float2.Zero, Size).Contains(ref location))
                {
                    Defocus();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override bool OnTouchDown(Float2 location, int pointerId)
            {
                if (base.OnTouchDown(location, pointerId))
                    return true;

                // Close on touch outside the popup
                if (!new Rectangle(Float2.Zero, Size).Contains(ref location))
                {
                    Defocus();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                LostFocus = null;
                MainPanel = null;
                SelectedControl = null;

                base.OnDestroy();
            }
        }

        [HideInEditor]
        private class DropdownLabel : Label
        {
            public Action<Label> ItemClicked;

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;
                ItemClicked?.Invoke(this);
                return true;
            }

            public override bool OnTouchDown(Float2 location, int pointerId)
            {
                if (base.OnTouchDown(location, pointerId))
                    return true;
                ItemClicked?.Invoke(this);
                return true;
            }

            /// <inheritdoc />
            public override void OnSubmit()
            {
                ItemClicked?.Invoke(this);

                base.OnSubmit();
            }

            public override void OnDestroy()
            {
                ItemClicked = null;

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
        private bool _hadNavFocus;

        /// <summary>
        /// The selected index of the item (-1 for no selection).
        /// </summary>
        protected int _selectedIndex = -1;

        /// <summary>
        /// Gets or sets the items collection.
        /// </summary>
        [EditorOrder(1)]
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
            get => _selectedIndex > -1 && _selectedIndex < _items.Count ? _items[_selectedIndex].ToString() : string.Empty;
            set => SelectedIndex = _items.IndexOf(value);
        }

        /// <summary>
        /// Gets or sets the selected item (returns <see cref="LocalizedString.Empty"/> if no item is being selected).
        /// </summary>
        [HideInEditor, NoSerialize]
        public LocalizedString SelectedItemLocalized
        {
            get => _selectedIndex > -1 && _selectedIndex < _items.Count ? _items[_selectedIndex] : LocalizedString.Empty;
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
        /// Gets or sets whether to show all the items in the dropdown.
        /// </summary>
        [EditorOrder(3)]
        public bool ShowAllItems { get; set; } = true;

        /// <summary>
        /// Gets or sets the maximum number of items to show at once. Only used if ShowAllItems is false.
        /// </summary>
        [EditorOrder(4), VisibleIf(nameof(ShowAllItems), true), Limit(1)]
        public int ShowMaxItemsCount { get; set; } = 5;

        /// <summary>
        /// Event fired when selected item gets changed.
        /// </summary>
        public event Action SelectedItemChanged;

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
        [EditorDisplay("Text Style"), EditorOrder(2020)]
        public FontReference Font { get; set; }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2021)]
        public MaterialBase FontMaterial { get; set; }

        /// <summary>
        /// Gets or sets the custom text format for selected item displaying. Can be used to prefix or/and postfix actual selected value within the drowpdown control text where '{0}' is used to insert selected value text. Example: 'Selected: {0}'. Leave empty if unussed.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2022)]
        public LocalizedString TextFormat { get; set; }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2023), ExpandGroups]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the horizontal text alignment within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2027)]
        public TextAlignment HorizontalAlignment { get; set; } = TextAlignment.Near;

        /// <summary>
        /// Gets or sets the vertical text alignment within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2028)]
        public TextAlignment VerticalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2010), ExpandGroups]
        public Color BorderColor { get; set; }

        /// <summary>
        /// Gets or sets the background color when dropdown popup is opened.
        /// </summary>
        [EditorDisplay("Background Style"), EditorOrder(2002)]
        public Color BackgroundColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the border color when dropdown popup is opened.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2012)]
        public Color BorderColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the background color when dropdown is highlighted.
        /// </summary>
        [EditorDisplay("Background Style"), EditorOrder(2001), ExpandGroups]
        public Color BackgroundColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the border color when dropdown is highlighted.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2013)]
        public Color BorderColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the image used to render dropdown drop arrow icon.
        /// </summary>
        [EditorDisplay("Icon Style"), EditorOrder(2033), Tooltip("The image used to render dropdown drop arrow icon.")]
        public IBrush ArrowImage { get; set; }

        /// <summary>
        /// Gets or sets the color used to render dropdown drop arrow icon.
        /// </summary>
        [EditorDisplay("Icon Style"), EditorOrder(2030), Tooltip("The color used to render dropdown drop arrow icon."), ExpandGroups]
        public Color ArrowColor { get; set; }

        /// <summary>
        /// Gets or sets the color used to render dropdown drop arrow icon (menu is opened).
        /// </summary>
        [EditorDisplay("Icon Style"), EditorOrder(2032), Tooltip("The color used to render dropdown drop arrow icon (menu is opened).")]
        public Color ArrowColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the color used to render dropdown drop arrow icon (menu is highlighted).
        /// </summary>
        [EditorDisplay("Icon Style"), EditorOrder(2031), Tooltip("The color used to render dropdown drop arrow icon (menu is highlighted).")]
        public Color ArrowColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the image used to render dropdown checked item icon.
        /// </summary>
        [EditorDisplay("Icon Style"), EditorOrder(2034), Tooltip("The image used to render dropdown checked item icon.")]
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
            SelectedItemChanged?.Invoke();
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
            // Create popup
            var popup = CreatePopupRoot();
            if (popup == null)
                throw new NullReferenceException("Missing popup.");
            if (popup.MainPanel == null)
                throw new NullReferenceException("Missing popup MainPanel.");
            CreatePopupBackground(popup);

            // Create items container
            var itemContainer = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                BackgroundColor = Color.Transparent,
                Pivot = Float2.Zero,
                IsScrollable = true,
                AutoSize = true,
                Parent = popup.MainPanel,
            };

            var itemsHeight = 20.0f;
            var itemsMargin = 20.0f;

            // Scale height and margin with text height if needed
            var textHeight = Font.GetFont().Height;
            if (textHeight > itemsHeight)
            {
                itemsHeight = textHeight;
                itemsMargin = textHeight;
            }
            /*
            var itemsWidth = 40.0f;
            var font = Font.GetFont();
            for (int i = 0; i < _items.Count; i++)
            {
                itemsWidth = Mathf.Max(itemsWidth, itemsMargin + 4 + font.MeasureText(_items[i]).X);
            }
            */

            var itemsWidth = Width;
            var height = itemContainer.Margin.Height;

            for (int i = 0; i < _items.Count; i++)
            {
                var item = CreatePopupItem(i, new Float2(itemsWidth, itemsHeight), itemsMargin);
                item.Parent = itemContainer;
                height += itemsHeight;
                if (i != 0)
                    height += itemContainer.Spacing;
                if (_selectedIndex == i)
                    popup.SelectedControl = item;
            }

            if (ShowAllItems || _items.Count < ShowMaxItemsCount)
            {
                popup.Size = new Float2(itemsWidth, height);
                popup.MainPanel.Size = popup.Size;
            }
            else
            {
                popup.Size = new Float2(itemsWidth, (itemsHeight + itemContainer.Spacing) * ShowMaxItemsCount);
                popup.MainPanel.Size = popup.Size;
            }

            return popup;
        }

        /// <summary>
        /// Creates the popup root. Called by default implementation of <see cref="CreatePopup"/> and allows to customize popup base.
        /// </summary>
        /// <returns>Custom popup root control.</returns>
        protected virtual DropdownRoot CreatePopupRoot()
        {
            var popup = new DropdownRoot();

            var panel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                BackgroundColor = BackgroundColor,
                ScrollBars = ScrollBars.Vertical,
                AutoFocus = true,
                Parent = popup,
            };
            popup.MainPanel = panel;

            return popup;
        }

        /// <summary>
        /// Creates the popup background. Called by default implementation of <see cref="CreatePopup"/> and allows to customize popup background by adding controls to it.
        /// </summary>
        /// <param name="popup">The popup control where background controls can be added.</param>
        protected virtual void CreatePopupBackground(DropdownRoot popup)
        {
            // Default background outline
            var border = new Border
            {
                BorderColor = BorderColorHighlighted,
                Width = 4.0f,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = popup,
            };
        }

        /// <summary>
        /// Creates the popup item. Called by default implementation of <see cref="CreatePopup"/> and allows to customize popup item.
        /// </summary>
        /// <param name="i">The item index.</param>
        /// <param name="size">The item control size</param>
        /// <param name="margin">The item control left-side margin</param>
        /// <returns>Custom popup item control.</returns>
        protected virtual Control CreatePopupItem(int i, Float2 size, float margin)
        {
            // Default item with label
            var item = new ContainerControl
            {
                AutoFocus = false,
                Size = size,
            };
            var label = new DropdownLabel
            {
                AutoFocus = true,
                X = margin,
                Size = new Float2(size.X - margin, size.Y),
                Font = Font,
                TextColor = Color.White * 0.9f,
                TextColorHighlighted = Color.White,
                HorizontalAlignment = HorizontalAlignment,
                VerticalAlignment = VerticalAlignment,
                Text = _items[i],
                Parent = item,
                Tag = i,
            };
            label.ItemClicked += c =>
            {
                OnItemClicked((int)c.Tag);
                DestroyPopup();
            };

            if (_selectedIndex == i)
            {
                // Add icon to the selected item
                var icon = new Image
                {
                    Brush = CheckedImage,
                    Size = new Float2(margin, size.Y),
                    Margin = new Margin(4.0f, 6.0f, 4.0f, 4.0f),
                    //AnchorPreset = AnchorPresets.VerticalStretchLeft,
                    Parent = item,
                };
            }

            return item;
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
                _popup.EndMouseCapture();
                _popup.Dispose();
                _popup = null;
                if (_hadNavFocus)
                    NavigationFocus();
                else
                    Focus();
            }
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        public void ShowPopup()
        {
            // Find canvas scalar and set as root if it exists.
            ContainerControl c = Parent;
            while (c.Parent != Root && c.Parent != null)
            {
                c = c.Parent;
                if (c is CanvasScaler scalar)
                    break;
            }
            var root = c is CanvasScaler ? c : Root;

            if (_items.Count == 0 || root == null)
                return;

            // Setup popup
            DestroyPopup();
            _hadNavFocus = IsNavFocused;
            _popup = CreatePopup();
            _popup.UnlockChildrenRecursive();
            _popup.PerformLayout();
            _popup.LostFocus += DestroyPopup;

            // Show dropdown popup
            var locationRootSpace = Location + new Float2(0, Height - Height * (1 - Scale.Y) / 2);
            var parent = Parent;
            while (parent != null && parent != root)
            {
                locationRootSpace = parent.PointToParent(ref locationRootSpace);
                parent = parent.Parent;
            }
            _popup.Scale = Scale;
            _popup.Location = locationRootSpace - new Float2(0, _popup.Height * (1 - _popup.Scale.Y) / 2);
            _popup.Parent = root;
            _popup.Focus();
            _popup.StartMouseCapture();
            if (_popup.SelectedControl != null && _popup.MainPanel != null)
                _popup.MainPanel.ScrollViewTo(_popup.SelectedControl, true);
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
            var clientRect = new Rectangle(Float2.Zero, Size);
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
            else if (IsMouseOver || IsNavFocused)
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
                string text = _items[_selectedIndex];
                string format = TextFormat != null ? TextFormat : null;
                if (!string.IsNullOrEmpty(format))
                    text = string.Format(format, text);
                Render2D.DrawText(Font.GetFont(), FontMaterial, text, textRect, enabled ? textColor : textColor * 0.5f, HorizontalAlignment, VerticalAlignment);
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
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _touchDown = true;
                if (!IsPopupOpened)
                    Focus();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (_touchDown && button == MouseButton.Left)
            {
                _touchDown = false;
                ShowPopup();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

            if (_touchDown && button == MouseButton.Left)
            {
                _touchDown = false;
                ShowPopup();
                return true;
            }

            if (button == MouseButton.Left)
            {
                _touchDown = true;
                if (!IsPopupOpened)
                    Focus();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (base.OnTouchDown(location, pointerId))
                return true;

            _touchDown = true;
            return true;
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
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

        /// <inheritdoc />
        public override void OnSubmit()
        {
            ShowPopup();

            base.OnSubmit();
        }
    }
}
