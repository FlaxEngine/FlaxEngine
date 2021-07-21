// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Combo box control allows to choose one item or set of items from the provided collection of options.
    /// </summary>
    /// <remarks>
    /// Difference between <see cref="ComboBox"/> and <see cref="Dropdown"/> is that ComboBox uses native window to show the items list while Dropdown uses a custom panel added to parent window.
    /// This means that Dropdown will work on all platforms that don't support multiple native windows (eg. Android, PS4, Xbox One).
    /// </remarks>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class ComboBox : Control
    {
        /// <summary>
        /// The default height of the control.
        /// </summary>
        public const float DefaultHeight = 18.0f;

        /// <summary>
        /// The items.
        /// </summary>
        protected List<string> _items = new List<string>();

        /// <summary>
        /// The item tooltips (optional).
        /// </summary>
        protected string[] _tooltips;

        /// <summary>
        /// The popup menu. May be null if has not been used yet.
        /// </summary>
        protected ContextMenu.ContextMenu _popupMenu;

        /// <summary>
        /// The mouse down flag.
        /// </summary>
        protected bool _mouseDown;

        /// <summary>
        /// The block popup flag.
        /// </summary>
        protected bool _blockPopup;

        /// <summary>
        /// The selected indices.
        /// </summary>
        protected List<int> _selectedIndices = new List<int>(4);

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
        /// Gets or sets the items tooltips (optional).
        /// </summary>
        [NoSerialize, HideInEditor]
        public string[] Tooltips
        {
            get => _tooltips;
            set => _tooltips = value;
        }

        /// <summary>
        /// True if sort items before showing the list, otherwise present them in the unchanged order.
        /// </summary>
        [EditorOrder(40), Tooltip("If checked, items will be sorted before showing the list, otherwise present them in the unchanged order.")]
        public bool Sorted { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether support multi items selection.
        /// </summary>
        [EditorOrder(41), Tooltip("If checked, combobox will support multi items selection. Otherwise it will be single item picking.")]
        public bool SupportMultiSelect { get; set; }

        /// <summary>
        /// Gets or sets the maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.
        /// </summary>
        [EditorOrder(42), Limit(1, 1000), Tooltip("The maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.")]
        public int MaximumItemsInViewCount { get; set; }

        /// <summary>
        /// Gets or sets the selected item (returns <see cref="string.Empty"/> if no item is being selected or more than one item is selected).
        /// </summary>
        [HideInEditor, NoSerialize]
        public string SelectedItem
        {
            get => _selectedIndices.Count == 1 ? _items[_selectedIndices[0]] : string.Empty;
            set => SelectedIndex = _items.IndexOf(value);
        }

        /// <summary>
        /// Gets a value indicating whether this combobox has any item selected.
        /// </summary>
        public bool HasSelection => _selectedIndices.Count != 0;

        /// <summary>
        /// Gets or sets the index of the selected. If combobox has more than 1 item selected then it returns invalid index (value -1).
        /// </summary>
        [EditorOrder(2), Tooltip("The index of the selected item from the list.")]
        public int SelectedIndex
        {
            get => _selectedIndices.Count == 1 ? _selectedIndices[0] : -1;
            set
            {
                // Clamp index
                value = Mathf.Min(value, _items.Count - 1);

                // Check if index will change
                if (value != SelectedIndex)
                {
                    // Select
                    _selectedIndices.Clear();
                    if (value != -1)
                        _selectedIndices.Add(value);
                    OnSelectedIndexChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets the selection.
        /// </summary>
        [NoSerialize, HideInEditor]
        public List<int> Selection
        {
            get => _selectedIndices;
            set
            {
                if (value == null)
                    throw new ArgumentNullException();
                if (!SupportMultiSelect && value.Count > 1)
                    throw new InvalidOperationException();
                for (int i = 0; i < value.Count; i++)
                {
                    var index = value[i];
                    if (index < 0 || index >= _items.Count)
                        throw new ArgumentOutOfRangeException();
                }

                if (!_selectedIndices.SequenceEqual(value))
                {
                    // Select
                    _selectedIndices.Clear();
                    _selectedIndices.AddRange(value);
                    OnSelectedIndexChanged();
                }
            }
        }

        /// <summary>
        /// Event fired when selected index gets changed.
        /// </summary>
        public event Action<ComboBox> SelectedIndexChanged;

        /// <summary>
        /// Occurs when popup is showing (before event). Can be used to update items collection before showing it to the user.
        /// </summary>
        public event Action<ComboBox> PopupShowing;

        /// <summary>
        /// Custom popup creation function.
        /// </summary>
        public event Func<ComboBox, ContextMenu.ContextMenu> PopupCreate;

        /// <summary>
        /// Gets the popup menu (it may be null if not used - lazy init).
        /// </summary>
        public ContextMenu.ContextMenu Popup => _popupMenu;

        /// <summary>
        /// Gets a value indicating whether this popup menu is opened.
        /// </summary>
        public bool IsPopupOpened => _popupMenu != null && _popupMenu.IsOpened;

        /// <summary>
        /// Gets or sets the font used to draw text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public FontReference Font { get; set; }

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
        /// Gets or sets the background color when combobox popup is opened.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2010)]
        public Color BackgroundColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the border color when combobox popup is opened.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2020)]
        public Color BorderColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the background color when combobox is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BackgroundColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the border color when combobox is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BorderColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the image used to render combobox drop arrow icon.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The image used to render combobox drop arrow icon.")]
        public IBrush ArrowImage { get; set; }

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color used to render combobox drop arrow icon.")]
        public Color ArrowColor { get; set; }

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon (menu is opened).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color used to render combobox drop arrow icon (menu is opened).")]
        public Color ArrowColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon (menu is highlighted).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color used to render combobox drop arrow icon (menu is highlighted).")]
        public Color ArrowColorHighlighted { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ComboBox"/> class.
        /// </summary>
        public ComboBox()
        : this(0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ComboBox"/> class.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="width">The width.</param>
        public ComboBox(float x, float y, float width = 120.0f)
        : base(x, y, width, DefaultHeight)
        {
            MaximumItemsInViewCount = 20;

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
        /// Determines whether the specified item is selected.
        /// </summary>
        /// <param name="item">The item to check.</param>
        /// <returns><c>true</c> if the item is selected; otherwise, <c>false</c>.</returns>
        public bool IsSelected(string item)
        {
            return IsSelected(_items.IndexOf(item));
        }

        /// <summary>
        /// Determines whether the item at the specified index is selected.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns><c>true</c> if the item is selected; otherwise, <c>false</c>.</returns>
        public bool IsSelected(int index)
        {
            return index != -1 && _selectedIndices.Contains(index);
        }

        /// <summary>
        /// Called when selected item index gets changed.
        /// </summary>
        protected virtual void OnSelectedIndexChanged()
        {
            if (_tooltips != null && _tooltips.Length == _items.Count)
                TooltipText = _selectedIndices.Count == 1 ? _tooltips[_selectedIndices[0]] : null;
            SelectedIndexChanged?.Invoke(this);
        }

        /// <summary>
        /// Called when item is clicked.
        /// </summary>
        /// <param name="index">The index.</param>
        protected virtual void OnItemClicked(int index)
        {
            if (SupportMultiSelect)
            {
                if (_selectedIndices.Contains(index))
                    _selectedIndices.Remove(index);
                else
                    _selectedIndices.Add(index);
                OnSelectedIndexChanged();
            }
            else
            {
                SelectedIndex = index;
            }
        }

        /// <summary>
        /// Creates the popup menu.
        /// </summary>
        protected virtual ContextMenu.ContextMenu OnCreatePopup()
        {
            if (PopupCreate != null)
                return PopupCreate(this);
            return new ContextMenu.ContextMenu();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_popupMenu != null)
            {
                _popupMenu.Hide();
                _popupMenu.Dispose();
                _popupMenu = null;
            }

            _selectedIndices.Clear();
            _selectedIndices = null;
            _items.Clear();
            _items = null;
            _tooltips = null;

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
            Render2D.DrawRectangle(clientRect.MakeExpanded(-2.0f), borderColor);

            // Check if has selected item
            if (_selectedIndices.Count > 0)
            {
                string text = _selectedIndices.Count == 1 ? _items[_selectedIndices[0]] : "Multiple Values";

                // Draw text of the selected item
                float textScale = Height / DefaultHeight;
                var textRect = new Rectangle(margin, 0, clientRect.Width - boxSize - 2.0f * margin, clientRect.Height);
                Render2D.PushClip(textRect);
                var textColor = TextColor;
                Render2D.DrawText(Font.GetFont(), text, textRect, enabled ? textColor : textColor * 0.5f, TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, textScale);
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
            _blockPopup = false;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flags
            _mouseDown = false;
            _blockPopup = false;

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
            if (_mouseDown && !_blockPopup)
            {
                // Clear flag
                _mouseDown = false;

                // Ensure to have valid menu
                if (_popupMenu == null)
                {
                    _popupMenu = OnCreatePopup();
                    _popupMenu.MaximumItemsInViewCount = MaximumItemsInViewCount;

                    // Bind events
                    _popupMenu.VisibleChanged += cm =>
                    {
                        var win = Root;
                        _blockPopup = win != null && new Rectangle(Vector2.Zero, Size).Contains(PointFromWindow(win.MousePosition));
                        if (!_blockPopup)
                            Focus();
                    };
                    _popupMenu.ButtonClicked += btn =>
                    {
                        OnItemClicked((int)btn.Tag);
                        _popupMenu?.Hide();
                    };
                }

                // Check if menu hs been already shown
                if (_popupMenu.Visible)
                {
                    // Hide
                    _popupMenu.Hide();
                    return true;
                }

                PopupShowing?.Invoke(this);

                // Check if has any items
                if (_items.Count > 0)
                {
                    // Setup items list
                    var itemControls = _popupMenu.Items.ToArray();
                    foreach (var e in itemControls)
                        e.Dispose();
                    if (Sorted)
                        _items.Sort();
                    var style = Style.Current;
                    for (int i = 0; i < _items.Count; i++)
                    {
                        var btn = _popupMenu.AddButton(_items[i]);
                        if (_selectedIndices.Contains(i))
                        {
                            btn.Icon = style.CheckBoxTick;
                        }
                        if (_tooltips != null && _tooltips.Length > i)
                        {
                            btn.TooltipText = _tooltips[i];
                        }

                        btn.Tag = i;
                    }

                    // Show dropdown list
                    _popupMenu.MinimumWidth = Width;
                    _popupMenu.Show(this, new Vector2(1, Height));
                }
            }
            else
            {
                _blockPopup = false;
            }

            return true;
        }
    }
}
