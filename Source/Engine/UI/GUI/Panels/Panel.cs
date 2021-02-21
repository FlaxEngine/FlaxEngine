// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Panel UI control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ScrollableControl" />
    public class Panel : ScrollableControl
    {
        private bool _layoutChanged;
        private bool _alwaysShowScrollbars;
        private int _layoutUpdateLock;
        private ScrollBars _scrollBars;
        private Margin _scrollMargin;

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
        public Vector2 ViewBottom => Size + _viewOffset;

        /// <summary>
        /// Gets the cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        public Rectangle ControlsBounds => _controlsBounds;

        /// <summary>
        /// Gets or sets the scroll bars usage by this panel.
        /// </summary>
        [EditorOrder(0), Tooltip("The scroll bars usage.")]
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
                        VScrollBar = new VScrollBar(this, Width - ScrollBar.DefaultSize, Height)
                        {
                            AnchorPreset = AnchorPresets.TopLeft
                        };
                        //VScrollBar.X += VScrollBar.Width;
                        VScrollBar.ValueChanged += () => SetViewOffset(Orientation.Vertical, VScrollBar.Value);
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
                        HScrollBar = new HScrollBar(this, Height - ScrollBar.DefaultSize, Width)
                        {
                            AnchorPreset = AnchorPresets.TopLeft
                        };
                        //HScrollBar.Y += HScrollBar.Height;
                        //HScrollBar.Offsets += new Margin(0, 0, HScrollBar.Height * 0.5f, 0);
                        HScrollBar.ValueChanged += () => SetViewOffset(Orientation.Horizontal, HScrollBar.Value);
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
        /// Gets or sets a value indicating whether always show scrollbars. Otherwise show them only if scrolling is available.
        /// </summary>
        [EditorOrder(10), Tooltip("Whether always show scrollbars. Otherwise show them only if scrolling is available.")]
        public bool AlwaysShowScrollbars
        {
            get => _alwaysShowScrollbars;
            set
            {
                if (_alwaysShowScrollbars != value)
                {
                    _alwaysShowScrollbars = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.
        /// </summary>
        [EditorOrder(20), Tooltip("Scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.")]
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
        /// Initializes a new instance of the <see cref="Panel"/> class.
        /// </summary>
        /// <inheritdoc />
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
            ScrollBars = scrollBars;
        }

        /// <inheritdoc />
        protected override void SetViewOffset(ref Vector2 value)
        {
            bool wasLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            if (HScrollBar != null)
                HScrollBar.Value = -value.X;
            if (VScrollBar != null)
                VScrollBar.Value = -value.Y;

            IsLayoutLocked = wasLocked;
            base.SetViewOffset(ref value);
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

            Vector2 location = c.Location;
            Vector2 size = c.Size;
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
        public void ScrollViewTo(Vector2 location, bool fastScroll = false)
        {
            ScrollViewTo(new Rectangle(location, Vector2.Zero), fastScroll);
        }

        /// <summary>
        /// Scrolls the view to the given area.
        /// </summary>
        /// <param name="bounds">The bounds.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Rectangle bounds, bool fastScroll = false)
        {
            bool wasLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            if (HScrollBar != null && HScrollBar.Enabled)
                HScrollBar.ScrollViewTo(bounds.Left, bounds.Right, fastScroll);
            if (VScrollBar != null && VScrollBar.Enabled)
                VScrollBar.ScrollViewTo(bounds.Top, bounds.Bottom, fastScroll);

            IsLayoutLocked = wasLocked;
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
        public override bool OnMouseWheel(Vector2 location, float delta)
        {
            // Base
            if (base.OnMouseWheel(location, delta))
                return true;

            // Roll back to scroll bars
            if (VScrollBar != null && VScrollBar.Enabled && VScrollBar.OnMouseWheel(VScrollBar.PointFromParent(ref location), delta))
                return true;
            if (HScrollBar != null && HScrollBar.Enabled && HScrollBar.OnMouseWheel(HScrollBar.PointFromParent(ref location), delta))
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
        public override bool IntersectsChildContent(Control child, Vector2 location, out Vector2 childSpaceLocation)
        {
            // For not scroll bars we want to reject any collisions
            if (child != VScrollBar && child != HScrollBar)
            {
                // Check if has v scroll bar to reject points on it
                if (VScrollBar != null && VScrollBar.Enabled)
                {
                    Vector2 pos = VScrollBar.PointFromParent(ref location);
                    if (VScrollBar.ContainsPoint(ref pos))
                    {
                        childSpaceLocation = Vector2.Zero;
                        return false;
                    }
                }

                // Check if has h scroll bar to reject points on it
                if (HScrollBar != null && HScrollBar.Enabled)
                {
                    Vector2 pos = HScrollBar.PointFromParent(ref location);
                    if (HScrollBar.ContainsPoint(ref pos))
                    {
                        childSpaceLocation = Vector2.Zero;
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

            if (!IsLayoutLocked)
            {
                _layoutChanged = false;
            }

            base.PerformLayout(force);

            if (!IsLayoutLocked && _layoutChanged)
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
                bool vScrollEnabled = (controlsBounds.Bottom > height + 0.01f || controlsBounds.Y < 0.0f) && height > ScrollBar.DefaultMinimumSize;

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
                    VScrollBar.SetScrollRange(scrollBounds.Top, Mathf.Max(Mathf.Max(0, scrollBounds.Top), scrollBounds.Height - height));
                }
                VScrollBar.Bounds = new Rectangle(Width - ScrollBar.DefaultSize, 0, ScrollBar.DefaultSize, Height);
            }
            if (HScrollBar != null)
            {
                float width = Width;
                bool hScrollEnabled = (controlsBounds.Right > width + 0.01f || controlsBounds.X < 0.0f) && width > ScrollBar.DefaultMinimumSize;

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
                    HScrollBar.SetScrollRange(scrollBounds.Left, Mathf.Max(Mathf.Max(0, scrollBounds.Left), scrollBounds.Width - width));
                }
                HScrollBar.Bounds = new Rectangle(0, Height - ScrollBar.DefaultSize, Width - (VScrollBar != null && VScrollBar.Visible ? VScrollBar.Width : 0), ScrollBar.DefaultSize);
            }
        }

        /// <summary>
        /// Arranges the child controls and gets their bounds.
        /// </summary>
        protected virtual void ArrangeAndGetBounds()
        {
            Arrange();

            // Calculate scroll area bounds
            Vector2 totalMin = Vector2.Zero;
            Vector2 totalMax = Vector2.Zero;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];
                if (c.Visible && c.IsScrollable)
                {
                    Vector2 min = Vector2.Zero;
                    Vector2 max = c.Size;
                    Matrix3x3.Transform2D(ref min, ref c._cachedTransform, out min);
                    Matrix3x3.Transform2D(ref max, ref c._cachedTransform, out max);
                    Vector2.Min(ref min, ref totalMin, out totalMin);
                    Vector2.Max(ref max, ref totalMax, out totalMax);
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
            rect = new Rectangle(Vector2.Zero, Size);

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
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);

            float width = Width;
            float height = Height;
            float MinSize = 70;
            float AreaSize = 25;
            float MoveScale = 4.0f;
            Vector2 viewOffset = -_viewOffset;

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
