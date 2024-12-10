// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Panel UI control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ScrollableControl" />
    [ActorToolbox("GUI")]
    public class Panel : ScrollableControl
    {
        private bool _layoutChanged;
        private bool _alwaysShowScrollbars;
        private int _layoutUpdateLock;
        private ScrollBars _scrollBars;
        private float _scrollBarsSize = ScrollBar.DefaultSize;
        private Margin _scrollMargin;
        private Color _scrollbarTrackColor;
        private Color _scrollbarThumbColor;
        private Color _scrollbarThumbSelectedColor;

        /// <summary>
        /// The cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        [HideInEditor, NoSerialize]
        protected Rectangle _controlsBounds;

        /// <summary>
        /// The vertical scroll bar.
        /// </summary>
        [HideInEditor, NoSerialize]
        public VScrollBar VScrollBar;

        /// <summary>
        /// The horizontal scroll bar.
        /// </summary>
        [HideInEditor, NoSerialize]
        public HScrollBar HScrollBar;

        /// <summary>
        /// Gets the view bottom.
        /// </summary>
        public Float2 ViewBottom => Size + _viewOffset;

        /// <summary>
        /// Gets the cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        public Rectangle ControlsBounds => _controlsBounds;

        /// <summary>
        /// Gets or sets the scroll bars usage by this panel.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1500), Tooltip("The scroll bars usage.")]
        public ScrollBars ScrollBars
        {
            get => _scrollBars;
            set
            {
                if (_scrollBars == value)
                    return;

                _scrollBars = value;

                if ((value & ScrollBars.Vertical) == ScrollBars.Vertical)
                {
                    if (VScrollBar == null)
                        VScrollBar = GetChild<VScrollBar>();
                    if (VScrollBar == null)
                    {
                        VScrollBar = new VScrollBar(this, Width - _scrollBarsSize, Height)
                        {
                            AnchorPreset = AnchorPresets.TopLeft
                        };
                        //VScrollBar.X += VScrollBar.Width;
                        VScrollBar.ValueChanged += () => SetViewOffset(Orientation.Vertical, VScrollBar.Value);
                    }
                    if (VScrollBar != null)
                    {
                        VScrollBar.TrackColor = _scrollbarTrackColor;
                        VScrollBar.ThumbColor = _scrollbarThumbColor;
                        VScrollBar.ThumbSelectedColor = _scrollbarThumbSelectedColor;
                    }
                }
                else if (VScrollBar != null)
                {
                    VScrollBar.Dispose();
                    VScrollBar = null;
                }

                if ((value & ScrollBars.Horizontal) == ScrollBars.Horizontal)
                {
                    if (HScrollBar == null)
                        HScrollBar = GetChild<HScrollBar>();
                    if (HScrollBar == null)
                    {
                        HScrollBar = new HScrollBar(this, Height - _scrollBarsSize, Width)
                        {
                            AnchorPreset = AnchorPresets.TopLeft
                        };
                        //HScrollBar.Y += HScrollBar.Height;
                        //HScrollBar.Offsets += new Margin(0, 0, HScrollBar.Height * 0.5f, 0);
                        HScrollBar.ValueChanged += () => SetViewOffset(Orientation.Horizontal, HScrollBar.Value);
                    }
                    if (HScrollBar != null)
                    {
                        HScrollBar.TrackColor = _scrollbarTrackColor;
                        HScrollBar.ThumbColor = _scrollbarThumbColor;
                        HScrollBar.ThumbSelectedColor = _scrollbarThumbSelectedColor;
                    }
                }
                else if (HScrollBar != null)
                {
                    HScrollBar.Dispose();
                    HScrollBar = null;
                }

                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the size of the scroll bars.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1501), Tooltip("Scroll bars size.")]
        public float ScrollBarsSize
        {
            get => _scrollBarsSize;
            set
            {
                if (Mathf.NearEqual(_scrollBarsSize, value))
                    return;
                _scrollBarsSize = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether always show scrollbars. Otherwise show them only if scrolling is available.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1502), Tooltip("Whether always show scrollbars. Otherwise show them only if scrolling is available.")]
        public bool AlwaysShowScrollbars
        {
            get => _alwaysShowScrollbars;
            set
            {
                if (_alwaysShowScrollbars != value)
                {
                    _alwaysShowScrollbars = value;
                    switch (_scrollBars)
                    {
                    case ScrollBars.None:
                        break;
                    case ScrollBars.Horizontal:
                        HScrollBar.Visible = value;
                        break;
                    case ScrollBars.Vertical:
                        VScrollBar.Visible = value;
                        break;
                    case ScrollBars.Both:
                        HScrollBar.Visible = value;
                        VScrollBar.Visible = value;
                        break;
                    default: break;
                    }
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1503), Tooltip("Scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.")]
        public Margin ScrollMargin
        {
            get => _scrollMargin;
            set
            {
                if (_scrollMargin != value)
                {
                    _scrollMargin = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// The color of the scroll bar track.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1600), ExpandGroups]
        public Color ScrollbarTrackColor
        {
            get => _scrollbarTrackColor;
            set
            {
                _scrollbarTrackColor = value;
                if (VScrollBar != null)
                    VScrollBar.TrackColor = _scrollbarTrackColor;
                if (HScrollBar != null)
                    HScrollBar.TrackColor = _scrollbarTrackColor;
            }
        }

        /// <summary>
        /// The color of the scroll bar thumb.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1601)]
        public Color ScrollbarThumbColor
        {
            get => _scrollbarThumbColor;
            set
            {
                _scrollbarThumbColor = value;
                if (VScrollBar != null)
                    VScrollBar.ThumbColor = _scrollbarThumbColor;
                if (HScrollBar != null)
                    HScrollBar.ThumbColor = _scrollbarThumbColor;
            }
        }

        /// <summary>
        /// The color of the scroll bar thumb when selected.
        /// </summary>
        [EditorDisplay("Scrollbar Style"), EditorOrder(1602)]
        public Color ScrollbarThumbSelectedColor
        {
            get => _scrollbarThumbSelectedColor;
            set
            {
                _scrollbarThumbSelectedColor = value;
                if (VScrollBar != null)
                    VScrollBar.ThumbSelectedColor = _scrollbarThumbSelectedColor;
                if (HScrollBar != null)
                    HScrollBar.ThumbSelectedColor = _scrollbarThumbSelectedColor;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Panel"/> class.
        /// </summary>
        public Panel()
        : this(ScrollBars.None)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Panel"/> class.
        /// </summary>
        /// <param name="scrollBars">The scroll bars.</param>
        /// <param name="autoFocus">True if control can accept user focus</param>
        public Panel(ScrollBars scrollBars, bool autoFocus = false)
        {
            AutoFocus = autoFocus;
            var style = Style.Current;
            _scrollbarTrackColor = style.BackgroundHighlighted;
            _scrollbarThumbColor = style.BackgroundNormal;
            _scrollbarThumbSelectedColor = style.BackgroundSelected;
            ScrollBars = scrollBars;
        }

        /// <inheritdoc />
        protected override void SetViewOffset(ref Float2 value)
        {
            // Update scroll bars but with locked layout
            bool wasLocked = _isLayoutLocked;
            int layoutUpdateLock = _layoutUpdateLock;
            _isLayoutLocked = true;
            _layoutUpdateLock = 999;
            if (HScrollBar != null)
                HScrollBar.TargetValue = -value.X;
            if (VScrollBar != null)
                VScrollBar.TargetValue = -value.Y;
            _layoutUpdateLock = layoutUpdateLock;
            _isLayoutLocked = wasLocked;

            base.SetViewOffset(ref value);

            PerformLayout();
        }

        /// <summary>
        /// Cuts the scroll bars value smoothing and imminently goes to the target scroll value.
        /// </summary>
        public void FastScroll()
        {
            HScrollBar?.FastScroll();
            VScrollBar?.FastScroll();
        }

        /// <summary>
        /// Scrolls the view to the given control area.
        /// </summary>
        /// <param name="c">The control.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Control c, bool fastScroll = false)
        {
            if (c == null)
                throw new ArgumentNullException();

            var location = c.Location;
            var size = c.Size;
            while (c.HasParent && c.Parent != this)
            {
                c = c.Parent;
                location = c.PointToParent(ref location);
            }

            if (c.HasParent)
            {
                ScrollViewTo(new Rectangle(location, size), fastScroll);
            }
        }

        /// <summary>
        /// Scrolls the view to the given location.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Float2 location, bool fastScroll = false)
        {
            ScrollViewTo(new Rectangle(location, Float2.Zero), fastScroll);
        }

        /// <summary>
        /// Scrolls the view to the given area.
        /// </summary>
        /// <param name="bounds">The bounds.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Rectangle bounds, bool fastScroll = false)
        {
            bool wasLocked = _isLayoutLocked;
            _isLayoutLocked = true;

            if (HScrollBar != null && HScrollBar.Enabled)
                HScrollBar.ScrollViewTo(bounds.Left, bounds.Right, fastScroll);
            if (VScrollBar != null && VScrollBar.Enabled)
                VScrollBar.ScrollViewTo(bounds.Top, bounds.Bottom, fastScroll);

            _isLayoutLocked = wasLocked;
            PerformLayout();
        }

        internal void SetViewOffset(Orientation orientation, float value)
        {
            if (orientation == Orientation.Vertical)
                _viewOffset.Y = -value;
            else
                _viewOffset.X = -value;
            OnViewOffsetChanged();
            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;
            return AutoFocus && Focus(this);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // Base
            if (base.OnMouseWheel(location, delta))
                return true;

            if (Input.GetKey(KeyboardKeys.Shift))
            {
                if (HScrollBar != null && HScrollBar.Enabled && HScrollBar.OnMouseWheel(HScrollBar.PointFromParent(ref location), delta))
                    return true;
            }

            // Roll back to scroll bars
            if (VScrollBar != null && VScrollBar.Enabled && VScrollBar.OnMouseWheel(VScrollBar.PointFromParent(ref location), delta))
                return true;

            // No event handled
            return false;
        }

        /// <inheritdoc />
        public override void RemoveChildren()
        {
            // Keep scroll bars alive
            if (VScrollBar != null)
                _children.Remove(VScrollBar);
            if (HScrollBar != null)
                _children.Remove(HScrollBar);

            base.RemoveChildren();

            // Restore scrollbars
            if (VScrollBar != null)
                _children.Add(VScrollBar);
            if (HScrollBar != null)
                _children.Add(HScrollBar);
            PerformLayout();
        }

        /// <inheritdoc />
        public override void DisposeChildren()
        {
            // Keep scrollbars alive
            if (VScrollBar != null)
                _children.Remove(VScrollBar);
            if (HScrollBar != null)
                _children.Remove(HScrollBar);

            base.DisposeChildren();

            // Restore scrollbars
            if (VScrollBar != null)
                _children.Add(VScrollBar);
            if (HScrollBar != null)
                _children.Add(HScrollBar);
            PerformLayout();
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            if (control.IsScrollable)
            {
                PerformLayout();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw scrollbars manually (they are outside the clipping bounds)
            if (VScrollBar != null && VScrollBar.Visible)
            {
                Render2D.PushTransform(ref VScrollBar._cachedTransform);
                VScrollBar.Draw();
                Render2D.PopTransform();
            }

            if (HScrollBar != null && HScrollBar.Visible)
            {
                Render2D.PushTransform(ref HScrollBar._cachedTransform);
                HScrollBar.Draw();
                Render2D.PopTransform();
            }
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise && BackgroundColor.A <= 0.0f) // Go through transparency
                return false;
            return base.ContainsPoint(ref location, precise);
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            // For not scroll bars we want to reject any collisions
            if (child != VScrollBar && child != HScrollBar)
            {
                // Check if has v scroll bar to reject points on it
                if (VScrollBar != null && VScrollBar.Enabled)
                {
                    var pos = VScrollBar.PointFromParent(ref location);
                    if (VScrollBar.ContainsPoint(ref pos))
                    {
                        childSpaceLocation = Float2.Zero;
                        return false;
                    }
                }

                // Check if has h scroll bar to reject points on it
                if (HScrollBar != null && HScrollBar.Enabled)
                {
                    var pos = HScrollBar.PointFromParent(ref location);
                    if (HScrollBar.ContainsPoint(ref pos))
                    {
                        childSpaceLocation = Float2.Zero;
                        return false;
                    }
                }
            }

            return base.IntersectsChildContent(child, location, out childSpaceLocation);
        }

        /// <inheritdoc />
        internal override void AddChildInternal(Control child)
        {
            base.AddChildInternal(child);

            if (child.IsScrollable)
            {
                PerformLayout();
            }
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (_layoutUpdateLock > 2)
                return;
            _layoutUpdateLock++;

            if (!_isLayoutLocked)
            {
                _layoutChanged = false;
            }

            base.PerformLayout(force);

            if (!_isLayoutLocked && _layoutChanged)
            {
                _layoutChanged = false;
                PerformLayout(true);
            }

            _layoutUpdateLock--;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Arrange controls and get scroll bounds
            ArrangeAndGetBounds();

            // Update scroll bars
            var controlsBounds = _controlsBounds;
            var scrollBounds = controlsBounds;
            _scrollMargin.ExpandRectangle(ref scrollBounds);
            if (VScrollBar != null)
            {
                float height = Height;
                bool vScrollEnabled = (controlsBounds.Bottom > height + 0.01f || controlsBounds.Y < 0.0f) && height > _scrollBarsSize;

                if (VScrollBar.Enabled != vScrollEnabled)
                {
                    // Set scroll bar visibility 
                    VScrollBar.Enabled = vScrollEnabled;
                    VScrollBar.Visible = vScrollEnabled || _alwaysShowScrollbars;
                    _layoutChanged = true;

                    // Clear scroll state
                    VScrollBar.Reset();
                    _viewOffset.Y = 0;
                    OnViewOffsetChanged();

                    // Get the new bounds after changing scroll
                    ArrangeAndGetBounds();
                }

                if (vScrollEnabled)
                {
                    float max;
                    if (scrollBounds.Top < 0)
                        max = Mathf.Max(scrollBounds.Bottom, scrollBounds.Top + scrollBounds.Height - height);
                    else
                        max = Mathf.Max(scrollBounds.Top, scrollBounds.Height - height);
                    VScrollBar.SetScrollRange(scrollBounds.Top, max);
                }
                VScrollBar.Bounds = new Rectangle(Width - _scrollBarsSize, 0, _scrollBarsSize, Height);
            }
            if (HScrollBar != null)
            {
                float width = Width;
                bool hScrollEnabled = (controlsBounds.Right > width + 0.01f || controlsBounds.X < 0.0f) && width > _scrollBarsSize;

                if (HScrollBar.Enabled != hScrollEnabled)
                {
                    // Set scroll bar visibility
                    HScrollBar.Enabled = hScrollEnabled;
                    HScrollBar.Visible = hScrollEnabled || _alwaysShowScrollbars;
                    _layoutChanged = true;

                    // Clear scroll state
                    HScrollBar.Reset();
                    _viewOffset.X = 0;
                    OnViewOffsetChanged();

                    // Get the new bounds after changing scroll
                    ArrangeAndGetBounds();
                }

                if (hScrollEnabled)
                {
                    float max;
                    if (scrollBounds.Left < 0)
                        max = Mathf.Max(scrollBounds.Right, scrollBounds.Left + scrollBounds.Width - width);
                    else
                        max = Mathf.Max(scrollBounds.Left, scrollBounds.Width - width);
                    HScrollBar.SetScrollRange(scrollBounds.Left, max);
                }
                HScrollBar.Bounds = new Rectangle(0, Height - _scrollBarsSize, Width - (VScrollBar != null && VScrollBar.Visible ? VScrollBar.Width : 0), _scrollBarsSize);
            }
        }

        /// <summary>
        /// Arranges the child controls and gets their bounds.
        /// </summary>
        protected virtual void ArrangeAndGetBounds()
        {
            Arrange();

            // Calculate scroll area bounds
            var totalMin = Float2.Zero;
            var totalMax = Float2.Zero;
            var hasTotal = false;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];
                if (c.Visible && c.IsScrollable)
                {
                    var upperLeft = Float2.Zero;
                    var bottomRight = c.Size;
                    Matrix3x3.Transform2D(ref upperLeft, ref c._cachedTransform, out upperLeft);
                    Matrix3x3.Transform2D(ref bottomRight, ref c._cachedTransform, out bottomRight);
                    Float2.Min(ref upperLeft, ref bottomRight, out var min);
                    Float2.Max(ref upperLeft, ref bottomRight, out var max);
                    if (hasTotal)
                    {
                        Float2.Min(ref min, ref totalMin, out totalMin);
                        Float2.Max(ref max, ref totalMax, out totalMax);
                    }
                    else
                    {
                        totalMin = min;
                        totalMax = max;
                        hasTotal = true;
                    }
                }
            }

            // Cache result
            _controlsBounds = new Rectangle(totalMin, totalMax - totalMin);
        }

        /// <summary>
        /// Arranges the child controls.
        /// </summary>
        protected virtual void Arrange()
        {
            base.PerformLayoutBeforeChildren();
        }

        /// <inheritdoc />
        public override void GetDesireClientArea(out Rectangle rect)
        {
            rect = new Rectangle(Float2.Zero, Size);

            if (VScrollBar != null && VScrollBar.Visible)
            {
                rect.Width -= VScrollBar.Width;
            }

            if (HScrollBar != null && HScrollBar.Visible)
            {
                rect.Height -= HScrollBar.Height;
            }
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);

            float width = Width;
            float height = Height;
            float MinSize = 70;
            float AreaSize = 25;
            float MoveScale = 4.0f;
            var viewOffset = -_viewOffset;

            if (VScrollBar != null && VScrollBar.Enabled && height > MinSize)
            {
                if (new Rectangle(0, 0, width, AreaSize).Contains(ref location))
                {
                    viewOffset.Y -= MoveScale;
                }
                else if (new Rectangle(0, height - AreaSize, width, AreaSize).Contains(ref location))
                {
                    viewOffset.Y += MoveScale;
                }

                viewOffset.Y = Mathf.Clamp(viewOffset.Y, VScrollBar.Minimum, VScrollBar.Maximum);
                VScrollBar.Value = viewOffset.Y;
            }

            if (HScrollBar != null && HScrollBar.Enabled && width > MinSize)
            {
                if (new Rectangle(0, 0, AreaSize, height).Contains(ref location))
                {
                    viewOffset.X -= MoveScale;
                }
                else if (new Rectangle(width - AreaSize, 0, AreaSize, height).Contains(ref location))
                {
                    viewOffset.X += MoveScale;
                }

                viewOffset.X = Mathf.Clamp(viewOffset.X, HScrollBar.Minimum, HScrollBar.Maximum);
                HScrollBar.Value = viewOffset.X;
            }

            viewOffset *= -1;

            if (viewOffset != _viewOffset)
            {
                _viewOffset = viewOffset;
                OnViewOffsetChanged();
                PerformLayout();
            }

            return result;
        }
    }
}
