// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine.Utilities;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// This panel arranges child controls horizontally.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.PanelWithMargins" />
    [ActorToolbox("GUI")]
    public class HorizontalPanel : PanelWithMargins
    {

        
        
        private int direction;
        private float prevWidthss;
        private bool minSizeInitialized = false;
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

           
            PerformExpansion();

        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            var prevBounds = Bounds.Width ;
            base.OnSizeChanged();
            direction = prevWidthss < Bounds.Width ? 1 : -1;
            prevWidthss = prevBounds;
            


        }

        private void PerformExpansion()
        {

            if (ChildForceExpandWidth && _children.Count > 0)
            {
                // Calculate the available width for children (taking margins into account)
                float availableWidth = Width - _margin.Left - _margin.Right;
                // Calculate the width each child should take up
                float childWidth = availableWidth / _children.Count;

                // Loop through each visible child and set their width
                foreach (var child in _children)
                {
                    if (child.Visible)
                    {
                        // Set each child's width to be equal (no shrinking or growing logic needed)
                        child.Width = childWidth;

                        //// Ensure no child becomes smaller than its MinSize.X if set
                        //if (child.MinSize.X > 0 && child.Width < child.MinSize.X)
                        //{
                        //    child.Width = child.MinSize.X;
                        //}
                    }
                }

            }

        }

      
    }
}
