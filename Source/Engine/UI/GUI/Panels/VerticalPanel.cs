// Copyright (c) Wojciech Figat. All rights reserved.

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
            float maxWidth = w;
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
                    maxWidth = Mathf.Max(maxWidth, cw);
                }
            }
            if (hasAnyTop)
                top -= _spacing;
            if (hasAnyBottom)
                bottom -= _spacing;

            // Update size
            if (_autoSize)
            {
                var size = Size;
                size.Y = top + bottom;
                if (!ControlChildSize)
                    size.X = maxWidth;
                Resize(ref size);
            }
            else if (_alignment != TextAlignment.Near && hasAnyTop)
            {
                // Apply layout alignment
                var offset = Height - top - _margin.Bottom;
                if (_alignment == TextAlignment.Center)
                    offset *= 0.5f;
                for (int i = 0; i < _children.Count; i++)
                {
                    Control c = _children[i];
                    if (c.Visible)
                    {
                        if (Mathf.IsZero(c.AnchorMin.Y) && Mathf.IsZero(c.AnchorMax.Y))
                        {
                            c.Y += offset;
                        }
                    }
                }
            }

            if (!_autoSize)
                PerformExpansion();
        }

        /// <inheritdoc />
        protected override void PerformExpansion()
        {

            if (ForceChildExpand && _children.Count > 0)
            {
                // Calculate the available height for children (taking margins into account)
                float availableHeight = Height - _margin.Top - _margin.Bottom - (_children.Count - 1) * _spacing;

                // Calculate the height each child should take up
                float childHeight = availableHeight / _children.Count;

                // Adjust for the first and last child to prevent being cut off
                float firstChildY = _margin.Top;
                float lastChildY = Height - _margin.Bottom - childHeight;

                // Loop through each child and set their height and position
                float top = _margin.Top;
                for (int i = 0; i < _children.Count; i++)
                {
                    Control child = _children[i];
                    if (child.Visible)
                    {
                        // Adjust the height and position of the first and last children
                        if (i == 0)
                        {
                            // First child, position it at the start and set its height                            
                            child.Height = childHeight;
                            child.Y = firstChildY;
                        }
                        else if (i == _children.Count - 1)
                        {
                            // Last child, position it near the bottom edge and set its height                            
                            child.Height = childHeight;
                            child.Y = lastChildY;
                        }
                        else
                        {
                            // For all other children, distribute the height evenly                            
                            child.Height = childHeight;
                            child.Y = top;
                        }

                        // Move the `top` pointer to the bottom for the next child
                        top = child.Bottom + _spacing;
                    }
                    child.PerformLayout(true);
                }
            }
        }

    }
}
