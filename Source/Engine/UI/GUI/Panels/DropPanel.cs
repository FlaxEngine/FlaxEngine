// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Drop Panel arranges control vertically and provides feature to collapse contents.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class DropPanel : ContainerControl
    {
        /// <summary>
        /// The header height.
        /// </summary>
        protected float _headerHeight = 14.0f;

        /// <summary>
        /// The header text margin.
        /// </summary>
        protected Margin _headerTextMargin = new Margin(2.0f);

        /// <summary>
        /// The 'is closed' flag.
        /// </summary>
        protected bool _isClosed;

        /// <summary>
        /// The 'mouse over header' flag (over header).
        /// </summary>
        protected bool _mouseOverHeader;

        /// <summary>
        /// The 'mouse down' flag (over header) for the left mouse button.
        /// </summary>
        protected bool _mouseButtonLeftDown;

        /// <summary>
        /// The 'mouse down' flag (over header) for the right mouse button.
        /// </summary>
        protected bool _mouseButtonRightDown;

        /// <summary>
        /// The animation progress (normalized).
        /// </summary>
        protected float _animationProgress = 1.0f;

        /// <summary>
        /// The cached height of the control.
        /// </summary>
        protected float _cachedHeight = 16.0f;

        /// <summary>
        /// The items margin.
        /// </summary>
        protected Margin _itemsMargin = new Margin(2.0f, 2.0f, 2.0f, 2.0f);

        /// <summary>
        /// Gets or sets the header text.
        /// </summary>
        [EditorOrder(10), Tooltip("The text to show on a panel header.")]
        public string HeaderText { get; set; }

        /// <summary>
        /// Gets or sets the height of the header.
        /// </summary>
        [EditorOrder(20), Limit(1, 10000, 0.1f), Tooltip("The height of the panel header.")]
        public float HeaderHeight
        {
            get => _headerHeight;
            set
            {
                if (!Mathf.NearEqual(_headerHeight, value))
                {
                    _headerHeight = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the header margin.
        /// </summary>
        [EditorOrder(30), Tooltip("The margin of the header text area.")]
        public Margin HeaderTextMargin
        {
            get => _headerTextMargin;
            set
            {
                if (_headerTextMargin != value)
                {
                    _headerTextMargin = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the color used to draw header text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color HeaderTextColor;

        /// <summary>
        /// Gets or sets the color of the header.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color HeaderColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the header when mouse is over.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color HeaderColorMouseOver { get; set; }

        /// <summary>
        /// Gets or sets the font used to render panel header text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public FontReference HeaderTextFont { get; set; }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("Custom material used to render the header text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.")]
        public MaterialBase HeaderTextMaterial { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether enable drop down icon drawing.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public bool EnableDropDownIcon { get; set; }

        /// <summary>
        /// Occurs when mouse right-clicks over the header.
        /// </summary>
        public event Action<DropPanel, Vector2> MouseButtonRightClicked;

        /// <summary>
        /// Occurs when drop panel is opened or closed.
        /// </summary>
        public event Action<DropPanel> IsClosedChanged;

        /// <summary>
        /// Gets or sets a value indicating whether this panel is closed.
        /// </summary>
        [EditorOrder(0), Tooltip("If checked, the panel is closed, otherwise is open.")]
        public bool IsClosed
        {
            get => _isClosed;
            set
            {
                if (_isClosed != value)
                {
                    if (value)
                        Close(false);
                    else
                        Open(false);
                }
            }
        }

        /// <summary>
        /// Gets or sets the item slots margin (the space between items).
        /// </summary>
        [EditorOrder(10), Tooltip("The item slots margin (the space between items).")]
        public Margin ItemsMargin
        {
            get => _itemsMargin;
            set
            {
                if (_itemsMargin != value)
                {
                    _itemsMargin = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the panel close/open animation duration (in seconds).
        /// </summary>
        [EditorOrder(10), Limit(0, 100, 0.01f), Tooltip("The panel close/open animation duration (in seconds).")]
        public float CloseAnimationTime { get; set; } = 0.2f;

        /// <summary>
        /// Gets or sets the image used to render drop panel drop arrow icon when panel is opened.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The image used to render drop panel drop arrow icon when panel is opened.")]
        public IBrush ArrowImageOpened { get; set; }

        /// <summary>
        /// Gets or sets the image used to render drop panel drop arrow icon when panel is closed.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The image used to render drop panel drop arrow icon when panel is closed.")]
        public IBrush ArrowImageClosed { get; set; }

        /// <summary>
        /// Gets the header rectangle.
        /// </summary>
        protected Rectangle HeaderRectangle => new Rectangle(0, 0, Width, HeaderHeight);

        /// <inheritdoc />
        protected override bool ShowTooltip => base.ShowTooltip && _mouseOverHeader;

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Vector2 location, out Rectangle area)
        {
            var result = base.OnShowTooltip(out text, out location, out area);

            // Change the position
            location = new Vector2(Width * 0.5f, HeaderHeight);

            return result;
        }

        /// <inheritdoc />
        public override bool OnTestTooltipOverControl(ref Vector2 location)
        {
            return HeaderRectangle.Contains(ref location);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DropPanel"/> class.
        /// </summary>
        public DropPanel()
        : base(0, 0, 64, 16.0f)
        {
            AutoFocus = false;

            var style = Style.Current;
            HeaderColor = style.BackgroundNormal;
            HeaderColorMouseOver = style.BackgroundHighlighted;
            HeaderTextFont = new FontReference(style.FontMedium);
            HeaderTextColor = style.Foreground;
            ArrowImageOpened = new SpriteBrush(style.ArrowDown);
            ArrowImageClosed = new SpriteBrush(style.ArrowRight);
        }

        /// <summary>
        /// Opens the group.
        /// </summary>
        /// <param name="animate">Enable/disable animation feature.</param>
        public void Open(bool animate = true)
        {
            // Check if state will change
            if (_isClosed)
            {
                // Set flag
                _isClosed = false;
                if (animate)
                    _animationProgress = 1 - _animationProgress;
                else
                    _animationProgress = 1.0f;

                // Update
                PerformLayout();

                // Fire event
                IsClosedChanged?.Invoke(this);
            }
        }

        /// <summary>
        /// Closes the group.
        /// </summary>
        /// <param name="animate">Enable/disable animation feature.</param>
        public void Close(bool animate = true)
        {
            // Check if state will change
            if (!_isClosed)
            {
                // Set flag
                _isClosed = true;
                if (animate)
                    _animationProgress = 1 - _animationProgress;
                else
                    _animationProgress = 1.0f;

                // Update
                PerformLayout();

                // Fire event
                IsClosedChanged?.Invoke(this);
            }
        }

        /// <summary>
        /// Toggles open state
        /// </summary>
        public void Toggle()
        {
            if (_isClosed)
                Open();
            else
                Close();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Drop/down animation
            if (_animationProgress < 1.0f)
            {
                float openCloseAnimationTime = CloseAnimationTime;
                bool isDeltaSlow = deltaTime > (1 / 20.0f);

                // Update progress
                if (isDeltaSlow || openCloseAnimationTime < Mathf.Epsilon)
                {
                    _animationProgress = 1.0f;
                }
                else
                {
                    _animationProgress += deltaTime / openCloseAnimationTime;
                    if (_animationProgress > 1.0f)
                        _animationProgress = 1.0f;
                }

                // Arrange controls
                PerformLayout();
            }
            else
            {
                base.Update(deltaTime);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var enabled = EnabledInHierarchy;

            // Paint Background
            var backgroundColor = BackgroundColor;
            if (backgroundColor.A > 0.0f)
            {
                Render2D.FillRectangle(new Rectangle(Vector2.Zero, Size), backgroundColor);
            }

            // Header
            var color = _mouseOverHeader ? HeaderColorMouseOver : HeaderColor;
            if (color.A > 0.0f)
            {
                Render2D.FillRectangle(new Rectangle(0, 0, Width, HeaderHeight), color);
            }

            // Drop down icon
            float textLeft = 0;
            if (EnableDropDownIcon)
            {
                textLeft += 14;
                var dropDownRect = new Rectangle(2, (HeaderHeight - 12) / 2, 12, 12);
                var arrowColor = _mouseOverHeader ? style.Foreground : style.ForegroundGrey;
                if (_isClosed)
                    ArrowImageClosed?.Draw(dropDownRect, arrowColor);
                else
                    ArrowImageOpened?.Draw(dropDownRect, arrowColor);
            }

            // Text
            var textRect = new Rectangle(textLeft, 0, Width - textLeft, HeaderHeight);
            _headerTextMargin.ShrinkRectangle(ref textRect);
            var textColor = HeaderTextColor;
            if (!enabled)
            {
                textColor *= 0.6f;
            }

            Render2D.DrawText(HeaderTextFont.GetFont(), HeaderTextMaterial, HeaderText, textRect, textColor, TextAlignment.Near, TextAlignment.Center);

            // Children
            DrawChildren();
        }

        /// <inheritdoc />
        protected override void DrawChildren()
        {
            // Draw child controls that are not arranged (pined to the header, etc.)
            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i];
                if (!child.IsScrollable && child.Visible)
                {
                    Render2D.PushTransform(ref child._cachedTransform);
                    child.Draw();
                    Render2D.PopTransform();
                }
            }

            // Check if isn't fully closed
            if (!_isClosed || _animationProgress < 0.998f)
            {
                // Draw children with clipping mask (so none of the controls will override the header)
                Rectangle clipMask = new Rectangle(0, HeaderHeight, Width, Height - HeaderHeight);
                Render2D.PushClip(ref clipMask);
                if (CullChildren)
                {
                    Render2D.PeekClip(out var globalClipping);
                    Render2D.PeekTransform(out var globalTransform);
                    for (int i = 0; i < _children.Count; i++)
                    {
                        var child = _children[i];
                        if (child.IsScrollable && child.Visible)
                        {
                            Matrix3x3.Multiply(ref child._cachedTransform, ref globalTransform, out var globalChildTransform);
                            var childGlobalRect = new Rectangle(globalChildTransform.M31, globalChildTransform.M32, child.Width * globalChildTransform.M11, child.Height * globalChildTransform.M22);
                            if (globalClipping.Intersects(ref childGlobalRect))
                            {
                                Render2D.PushTransform(ref child._cachedTransform);
                                child.Draw();
                                Render2D.PopTransform();
                            }
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < _children.Count; i++)
                    {
                        var child = _children[i];
                        if (child.IsScrollable && child.Visible)
                        {
                            Render2D.PushTransform(ref child._cachedTransform);
                            child.Draw();
                            Render2D.PopTransform();
                        }
                    }
                }

                Render2D.PopClip();
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            _mouseOverHeader = HeaderRectangle.Contains(location);
            if (button == MouseButton.Left && _mouseOverHeader)
            {
                _mouseButtonLeftDown = true;
                return true;
            }
            if (button == MouseButton.Right && _mouseOverHeader)
            {
                _mouseButtonRightDown = true;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            _mouseOverHeader = HeaderRectangle.Contains(location);

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            _mouseOverHeader = HeaderRectangle.Contains(location);
            if (button == MouseButton.Left && _mouseButtonLeftDown)
            {
                _mouseButtonLeftDown = false;
                if (_mouseOverHeader)
                    Toggle();
                return true;
            }
            if (button == MouseButton.Right && _mouseButtonRightDown)
            {
                _mouseButtonRightDown = false;
                MouseButtonRightClicked?.Invoke(this, location);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mouseButtonLeftDown = false;
            _mouseButtonRightDown = false;
            _mouseOverHeader = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        public override void GetDesireClientArea(out Rectangle rect)
        {
            var topMargin = HeaderHeight;
            rect = new Rectangle(0, topMargin, Width, Height - topMargin);
        }

        private void Arrange()
        {
            // Arrange anchored controls
            GetDesireClientArea(out var clientArea);
            float dropOffset = _cachedHeight * (_isClosed ? _animationProgress : 1.0f - _animationProgress);
            clientArea.Location.Y -= dropOffset;
            UpdateChildrenBounds();

            // Arrange scrollable controls
            var slotsMargin = _itemsMargin;
            var slotsLeft = clientArea.Left + slotsMargin.Left;
            var slotsWidth = clientArea.Width - slotsMargin.Width;
            float minHeight = HeaderHeight;
            float y = clientArea.Top;
            float height = clientArea.Top + dropOffset;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (c.IsScrollable && c.Visible)
                {
                    var h = c.Height;
                    y += slotsMargin.Top;

                    c.Bounds = new Rectangle(slotsLeft, y, slotsWidth, h);

                    h += slotsMargin.Bottom;
                    y += h;
                    height += h + slotsMargin.Top;
                }
            }

            // Update panel height
            _cachedHeight = height;
            if (_animationProgress >= 1.0f && _isClosed)
                y = minHeight;
            Height = Mathf.Max(minHeight, y);
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            Arrange();
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            Arrange();
        }
    }
}
