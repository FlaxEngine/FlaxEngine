// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Vertical scroll bar control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ScrollBar" />
    [HideInEditor]
    public class VScrollBar : ScrollBar
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VScrollBar"/> class.
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="x">The x position.</param>
        /// <param name="height">The height.</param>
        /// <param name="width">The width.</param>
        public VScrollBar(ContainerControl parent, float x, float height, float width = DefaultSize)
        : base(Orientation.Vertical)
        {
            AnchorPreset = AnchorPresets.VerticalStretchRight;
            Parent = parent;
            Bounds = new Rectangle(x, 0, width, height);
        }

        /// <inheritdoc />
        protected override float TrackSize => Height;
    }
}
