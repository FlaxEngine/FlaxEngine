// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Events timeline control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Panel" />
    public class Timeline : Panel
    {
        /// <summary>
        /// Single timeline event control.
        /// </summary>
        /// <seealso cref="ContainerControl" />
        public class Event : ContainerControl
        {
            private static readonly Color[] Colors =
            {
                new Color(0.8f, 0.894117653f, 0.709803939f, 1f),
                new Color(0.1254902f, 0.698039234f, 0.6666667f, 1f),
                new Color(0.4831376f, 0.6211768f, 0.0219608f, 1f),
                new Color(0.3827448f, 0.2886272f, 0.5239216f, 1f),
                new Color(0.8f, 0.4423528f, 0f, 1f),
                new Color(0.4486272f, 0.4078432f, 0.050196f, 1f),
                new Color(0.4831376f, 0.6211768f, 0.0219608f, 1f),
                new Color(0.4831376f, 0.6211768f, 0.0219608f, 1f),
                new Color(0.2070592f, 0.5333336f, 0.6556864f, 1f),
                new Color(0.8f, 0.4423528f, 0f, 1f),
                new Color(0.4486272f, 0.4078432f, 0.050196f, 1f),
                new Color(0.7749016f, 0.6368624f, 0.0250984f, 1f),
                new Color(0.5333336f, 0.16f, 0.0282352f, 1f),
                new Color(0.3827448f, 0.2886272f, 0.5239216f, 1f),
                new Color(0.478431374f, 0.482352942f, 0.117647059f, 1f),
                new Color(0.9411765f, 0.5019608f, 0.5019608f, 1f),
                new Color(0.6627451f, 0.6627451f, 0.6627451f, 1f),
                new Color(0.545098066f, 0f, 0.545098066f, 1f),
            };

            private Color _color;
            private string _name;
            private float _nameLength = -1;
            private bool _leftClick;
            private bool _isRightDown;

            /// <summary>
            /// The double click event.
            /// </summary>
            public Action DoubleClick;

            /// <summary>
            /// The left mouse button click event.
            /// </summary>
            public Action LeftClick;

            /// <summary>
            /// The right mouse button click event.
            /// </summary>
            public Action RightClick;

            /// <summary>
            /// The double click event.
            /// </summary>
            public Action<Event> EventDoubleClick;

            /// <summary>
            /// The left mouse button click event.
            /// </summary>
            public Action<Event> EventLeftClick;

            /// <summary>
            /// The right mouse button click event.
            /// </summary>
            public Action<Event> EventRightClick;

            /// <summary>
            /// The default height of the event.
            /// </summary>
            public const float DefaultHeight = 25.0f;

            /// <summary>
            /// Gets or sets the event name.
            /// </summary>
            public string Name
            {
                get => _name;
                set
                {
                    _name = value;
                    _nameLength = -1;
                }
            }

            /// <inheritdoc />
            protected override void OnParentChangedInternal()
            {
                base.OnParentChangedInternal();

                int key = (HasParent ? Parent.GetChildIndex(this) : 1) * (string.IsNullOrEmpty(Name) ? 1 : Name[0]);
                _color = Colors[key % Colors.Length] * 0.8f;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var bounds = new Rectangle(Float2.Zero, Size);
                Color color = EnabledInHierarchy ? _color : _color * 0.35f;
                if (IsMouseOver)
                    color *= 1.1f;

                Render2D.FillRectangle(bounds, color);
                Render2D.DrawRectangle(bounds, color * 0.5f);

                if (_nameLength < 0 && style.FontMedium)
                    _nameLength = style.FontMedium.MeasureText(_name).X;

                if (_nameLength < bounds.Width + 4)
                {
                    Render2D.PushClip(bounds);
                    Render2D.DrawText(style.FontMedium, _name, bounds, EnabledInHierarchy ? style.Foreground : style.ForegroundGrey, TextAlignment.Center, TextAlignment.Center);
                    Render2D.PopClip();
                }
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (InteractBounds.Contains(ref location))
                {
                    if (button == MouseButton.Left)
                        _leftClick = true;
                    else if (button == MouseButton.Right)
                        _isRightDown = true;
                    Focus();
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _leftClick && InteractBounds.Contains(ref location))
                {
                    _leftClick = false;
                    LeftClick?.Invoke();
                    EventLeftClick?.Invoke(this);
                    return true;
                }
                if (button == MouseButton.Right && _isRightDown && InteractBounds.Contains(ref location))
                {
                    _isRightDown = false;
                    RightClick?.Invoke();
                    EventRightClick?.Invoke(this);
                    return true;
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && InteractBounds.Contains(ref location))
                {
                    _leftClick = false;
                    _isRightDown = false;
                    DoubleClick?.Invoke();
                    EventDoubleClick?.Invoke(this);
                    return true;
                }

                return base.OnMouseDoubleClick(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                _leftClick = false;
                _isRightDown = false;

                base.OnMouseLeave();
            }

            /// <summary>Captured duration used to build the tooltip only when it is requested.</summary>
            public double DurationMilliseconds = double.NaN;

            /// <inheritdoc />
            protected override bool ShowTooltip => !double.IsNaN(DurationMilliseconds) || base.ShowTooltip;

            /// <inheritdoc />
            public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
            {
                bool show = base.OnShowTooltip(out text, out location, out area);
                if (!double.IsNaN(DurationMilliseconds))
                    text = Enabled ? string.Format("{0}, {1:0.###} ms", Name, DurationMilliseconds) : "Unused entry retained from a recent capture.";
                return show;
            }

            private Rectangle InteractBounds => new Rectangle(Float2.Zero, Mathf.Max(Width, 4), Height);
        }

        /// <summary>
        /// Timeline track label
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        public class TrackLabel : ContainerControl
        {
            /// <summary>
            /// Gets or sets the name.
            /// </summary>
            public string Name { get; set; }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var rect = new Rectangle(Float2.Zero, Size);
                Render2D.PushClip(rect);
                Render2D.DrawText(style.FontMedium, Name, rect, EnabledInHierarchy ? style.Foreground : style.ForegroundGrey, TextAlignment.Center, TextAlignment.Center, TextWrapping.WrapChars);
                Render2D.PopClip();
            }
        }

        /// <summary>Horizontal time scale used by profiler samples.</summary>
        public float PixelsPerMillisecond { get; private set; } = 100;

        /// <summary>Width reserved for thread labels.</summary>
        public float LabelWidth;

        /// <summary>Called when samples must be repositioned after zooming.</summary>
        public event Action ScaleChanged;

        /// <inheritdoc />
        protected override bool ShouldDrawChild(Control child, ref Rectangle bounds, ref Rectangle clipping)
        {
            // Subpixel samples stay in the capture and can be inspected by zooming in.
            return base.ShouldDrawChild(child, ref bounds, ref clipping) && (!(child is Event) || bounds.Width >= 0.75f);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (Input.GetKey(KeyboardKeys.Control))
            {
                Zoom(delta, location.X);
                return true;
            }
            return base.OnMouseWheel(location, delta);
        }

        private void Zoom(float delta, float position)
        {
            float time = (position - ViewOffset.X - LabelWidth) / PixelsPerMillisecond;
            PixelsPerMillisecond = Mathf.Clamp(PixelsPerMillisecond * Mathf.Pow(1.25f, delta), 1, 65536);
            ScaleChanged?.Invoke();
            float scroll = time * PixelsPerMillisecond + LabelWidth - position;
            scroll = HScrollBar != null && HScrollBar.Enabled ? Mathf.Clamp(scroll, HScrollBar.Minimum, HScrollBar.Maximum) : 0;
            ViewOffset = new Float2(-scroll, ViewOffset.Y);
            FastScroll();
        }

        /// <summary>
        /// Gets the events container control. Use it to remove/add events to the timeline.
        /// </summary>
        public ContainerControl EventsContainer => this;

        /// <summary>
        /// Initializes a new instance of the <see cref="Timeline"/> class.
        /// </summary>
        public Timeline()
        : base(ScrollBars.Both)
        {
            // Parent mouse-enter runs after the event's handler. A tooltip on the
            // timeline would replace the event's lazily generated name and duration.
            if (HScrollBar != null)
                HScrollBar.TooltipText = "Ctrl + mouse wheel to zoom. Subpixel samples remain recorded and appear when zoomed in.";
        }
    }
}
