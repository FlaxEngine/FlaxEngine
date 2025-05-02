// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Horizontal scroll bar control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ScrollBar" />
    [HideInEditor]
    public class HScrollBar : ScrollBar
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HScrollBar"/> class.
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="y">The y position.</param>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        public HScrollBar(ContainerControl parent, float y, float width, float height = DefaultSize)
        : base(Orientation.Horizontal)
        {
            AnchorPreset = AnchorPresets.HorizontalStretchBottom;
            Parent = parent;
            Bounds = new Rectangle(0, y, width, height);
        }

        /// <inheritdoc />
        protected override float TrackSize => Width;
    }
}
