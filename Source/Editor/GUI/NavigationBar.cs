// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// A navigation bar control. Shows the current location path with UI buttons to navigate around.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Panel" />
    public class NavigationBar : Panel
    {
        private float _toolstripHeight = 0;
        private Margin _toolstripMargin;

        /// <summary>
        /// The default buttons margin.
        /// </summary>
        public const float DefaultButtonsMargin = 2;

        /// <summary>
        /// Initializes a new instance of the <see cref="NavigationBar"/> class.
        /// </summary>
        public NavigationBar()
        : base(ScrollBars.Horizontal)
        {
        }

        /// <inheritdoc />
        protected override void Arrange()
        {
            base.Arrange();

            // Arrange buttons
            float x = DefaultButtonsMargin;
            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i];
                if (child.IsScrollable)
                {
                    child.X = x;
                    x += child.Width + DefaultButtonsMargin;
                }
            }
        }

        /// <summary>
        /// Updates the bar bounds and positions it after the last toolstrip button. Ensures to fit the toolstrip area (navigation bar horizontal scroll bar can be used to view the full path).
        /// </summary>
        /// <param name="toolstrip">The toolstrip.</param>
        public void UpdateBounds(ToolStrip toolstrip)
        {
            if (toolstrip == null)
                return;

            if (_toolstripHeight <= 0.0f)
            {
                // Cache initial toolstrip state
                _toolstripHeight = toolstrip.Height;
                _toolstripMargin = toolstrip.ItemsMargin;
            }

            // Control toolstrip bottom margin to prevent navigation bar scroll going over the buttons
            var toolstripLocked = toolstrip.IsLayoutLocked;
            toolstrip.IsLayoutLocked = true;
            var toolstripHeight = _toolstripHeight;
            var toolstripMargin = _toolstripMargin;
            if (HScrollBar.Visible)
            {
                float scrollMargin = 8;
                toolstripHeight += scrollMargin;
                toolstripMargin.Bottom += scrollMargin;
            }
            toolstrip.Height = toolstripHeight;
            toolstrip.IsLayoutLocked = toolstripLocked;
            toolstrip.ItemsMargin = toolstripMargin;

            var margin = toolstrip.ItemsMargin;
            float xOffset = margin.Left;
            bool hadChild = false;
            for (int i = 0; i < toolstrip.ChildrenCount; i++)
            {
                var child = toolstrip.GetChild(i);
                if (child == this || !child.Visible)
                    continue;
                hadChild = true;
                xOffset += child.Width + margin.Width;
            }

            var right = hadChild ? xOffset - margin.Width : margin.Left;
            var parentSize = Parent.Size;
            var x = right + 8.0f;
            var width = Mathf.Max(parentSize.X - x - 8.0f, 0.0f);
            Bounds = new Rectangle(x, 0, width, toolstrip.Height);
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            base.PerformLayout(force);

            // Stretch excluding toolstrip margin to fill the space
            if (Parent is ToolStrip toolStrip)
                Height = toolStrip.Height;
        }
    }
}
