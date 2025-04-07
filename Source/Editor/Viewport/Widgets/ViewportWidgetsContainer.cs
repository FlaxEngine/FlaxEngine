// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Widgets
{
    /// <summary>
    /// The viewport widget location.
    /// </summary>
    [HideInEditor]
    public enum ViewportWidgetLocation
    {
        /// <summary>
        /// The upper left corner of the parent container.
        /// </summary>
        UpperLeft,

        /// <summary>
        /// The upper right corner of the parent container.
        /// </summary>
        UpperRight,
    }

    /// <summary>
    /// Viewport Widgets Container control
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class ViewportWidgetsContainer : ContainerControl
    {
        /// <summary>
        /// The widgets margin.
        /// </summary>
        public const float WidgetsMargin = 4;

        /// <summary>
        /// The widgets height.
        /// </summary>
        public const float WidgetsHeight = 18;

        /// <summary>
        /// The widgets icon size.
        /// </summary>
        public const float WidgetsIconSize = 16;

        /// <summary>
        /// Gets the widget location.
        /// </summary>
        public ViewportWidgetLocation WidgetLocation { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewportWidgetsContainer"/> class.
        /// </summary>
        /// <param name="location">The location.</param>
        public ViewportWidgetsContainer(ViewportWidgetLocation location)
        : base(0, WidgetsMargin, 64, WidgetsHeight + 2)
        {
            AutoFocus = false;
            WidgetLocation = location;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            var clientRect = new Rectangle(Float2.Zero, Size);

            // Draw background
            Render2D.FillRectangle(clientRect, style.LightBackground * (IsMouseOver ? 0.3f : 0.2f));

            base.Draw();

            // Draw frame
            Render2D.DrawRectangle(clientRect, style.BackgroundSelected * (IsMouseOver ? 1.0f : 0.6f));
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            float x = 1;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];
                var w = c.Width;

                c.Bounds = new Rectangle(x, 1, w, Height - 2);

                x += w;
            }

            Width = x + 1;
        }

        /// <summary>
        /// Arranges the widgets of the control.
        /// </summary>
        /// <param name="control">The control.</param>
        public static void ArrangeWidgets(ContainerControl control)
        {
            // Arrange viewport widgets
            const float margin = ViewportWidgetsContainer.WidgetsMargin;
            float left = margin;
            float right = control.Width - margin;
            for (int i = 0; i < control.ChildrenCount; i++)
            {
                if (control.Children[i] is ViewportWidgetsContainer widget && widget.Visible)
                {
                    float x;
                    switch (widget.WidgetLocation)
                    {
                    case ViewportWidgetLocation.UpperLeft:
                        x = left;
                        left += widget.Width + margin;
                        break;
                    case ViewportWidgetLocation.UpperRight:
                        x = right - widget.Width;
                        right = x - margin;
                        break;
                    default:
                        x = 0;
                        break;
                    }
                    widget.Location = new Float2(x, margin);
                }
            }
        }
    }
}
