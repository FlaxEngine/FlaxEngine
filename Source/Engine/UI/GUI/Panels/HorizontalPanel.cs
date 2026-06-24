// Copyright (c) Wojciech Figat. All rights reserved.

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
                Resize(ref size);
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

            if(!_autoSize)
                PerformExpansion();

        }

        /// <inheritdoc />
        protected override void PerformExpansion()
        {

            if (ForceChildExpand && _children.Count > 0)
            {
                // Calculate the available width for children (taking margins into account)
                float availableWidth = Width - _margin.Left - _margin.Right - (_children.Count - 1) * _spacing;

                // Calculate the width each child should take up
                float childWidth = availableWidth / _children.Count;

                // Adjust for the first and last child to prevent being cut off
                float firstChildX = _margin.Left;
                float lastChildX = Width - _margin.Right - childWidth;

                // Loop through each child and set their width and position
                float left = _margin.Left;
                for (int i = 0; i < _children.Count; i++)
                {
                    Control child = _children[i];
                    if (child.Visible)
                    {
                        // Adjust the width and position of the first and last children
                        if (i == 0)
                        {
                            // First child, position it at the start and set its width                           
                            child.Width = childWidth;
                            child.X = firstChildX;
                        }
                        else if (i == _children.Count - 1)
                        {
                            // Last child, position it near the right edge and set its width                           
                            child.Width = childWidth;
                            child.X = lastChildX;
                        }
                        else
                        {
                            // For all other children, distribute the width evenly                       
                            child.Width = childWidth;
                            child.X = left;
                        }

                        // Move the `left` pointer to the right for the next child
                        left = child.Right + _spacing;
                    }
                    child.PerformLayout(true);
                }
            }

        }

    }
}
