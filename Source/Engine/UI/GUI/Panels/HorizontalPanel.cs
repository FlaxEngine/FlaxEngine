// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// This panel arranges child controls horizontally.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.PanelWithMargins" />
    [ActorToolbox("GUI")]
    public class HorizontalPanel : PanelWithMargins
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HorizontalPanel"/> class.
        /// </summary>
        public HorizontalPanel()
        {
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            // Pre-set height of all controls
            if (!ControlChildSize)
                return;
            float h = Height - _margin.Height;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (c.Visible && Mathf.IsZero(c.AnchorMin.Y) && Mathf.IsZero(c.AnchorMax.Y))
                {
                    c.Height = h;
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            // Sort controls horizontally
            float left = _margin.Left;
            float right = _margin.Right;
            float h = Height - _margin.Height;
            float maxHeight = h;
            bool hasAnyLeft = false, hasAnyRight = false;
            for (int i = 0; i < _children.Count; i++)
            {
                Control c = _children[i];
                if (c.Visible)
                {
                    var w = c.Width;
                    var ch = ControlChildSize ? h : c.Height;
                    if (Mathf.IsZero(c.AnchorMin.X) && Mathf.IsZero(c.AnchorMax.X))
                    {
                        c.Bounds = new Rectangle(left + _offset.X, _margin.Top + _offset.Y, w, ch);
                        left = c.Right + _spacing;
                        hasAnyLeft = true;
                    }
                    else if (Mathf.IsOne(c.AnchorMin.X) && Mathf.IsOne(c.AnchorMax.X))
                    {
                        right += w + _spacing;
                        c.Bounds = new Rectangle(Width - right + _offset.X, _margin.Top + _offset.Y, w, ch);
                        hasAnyRight = true;
                    }
                    maxHeight = Mathf.Max(maxHeight, ch);
                }
            }
            if (hasAnyLeft)
                left -= _spacing;
            if (hasAnyRight)
                right -= _spacing;

            // Update size
            if (_autoSize)
            {
                var size = Size;
                size.X = left + right;
                if (!ControlChildSize)
                    size.Y = maxHeight;
                Size = size;
            }
            else if (_alignment != TextAlignment.Near && hasAnyLeft)
            {
                // Apply layout alignment
                var offset = Width - left - _margin.Right;
                if (_alignment == TextAlignment.Center)
                    offset *= 0.5f;
                for (int i = 0; i < _children.Count; i++)
                {
                    Control c = _children[i];
                    if (c.Visible)
                    {
                        if (Mathf.IsZero(c.AnchorMin.X) && Mathf.IsZero(c.AnchorMax.X))
                        {
                            c.X += offset;
                        }
                    }
                }
            }
        }
    }
}
