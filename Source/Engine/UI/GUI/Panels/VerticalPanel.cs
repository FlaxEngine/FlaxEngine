// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// This panel arranges child controls vertically.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.PanelWithMargins" />
    [ActorToolbox("GUI")]
    public class VerticalPanel : PanelWithMargins
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VerticalPanel"/> class.
        /// </summary>
        public VerticalPanel()
        {
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            // Pre-set width of all controls
            if (!ControlChildSize)
                return;
            float w = Width - _margin.Width;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (c.Visible && Mathf.IsZero(c.AnchorMin.X) && Mathf.IsZero(c.AnchorMax.X))
                {
                    c.Width = w;
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            // Sort controls vertically
            float top = _margin.Top;
            float bottom = _margin.Bottom;
            float w = Width - _margin.Width;
            bool hasAnyTop = false, hasAnyBottom = false;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (c.Visible)
                {
                    var h = c.Height;
                    var cw = ControlChildSize ? w : c.Width;
                    if (Mathf.IsZero(c.AnchorMin.Y) && Mathf.IsZero(c.AnchorMax.Y))
                    {
                        c.Bounds = new Rectangle(_margin.Left + _offset.X, top + _offset.Y, cw, h);
                        top = c.Bottom + _spacing;
                        hasAnyTop = true;
                    }
                    else if (Mathf.IsOne(c.AnchorMin.Y) && Mathf.IsOne(c.AnchorMax.Y))
                    {
                        bottom += h + _spacing;
                        c.Bounds = new Rectangle(_margin.Left + _offset.X, Height - bottom + _offset.Y, cw, h);
                        hasAnyBottom = true;
                    }
                }
            }
            if (hasAnyTop)
                top -= _spacing;
            if (hasAnyBottom)
                bottom -= _spacing;

            // Update size
            if (_autoSize)
                Height = top + bottom;
        }
    }
}
