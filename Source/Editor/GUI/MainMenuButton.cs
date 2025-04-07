// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Main menu button control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class MainMenuButton : Control
    {
        /// <summary>
        /// The button text.
        /// </summary>
        public string Text;

        /// <summary>
        /// The context menu.
        /// </summary>
        public readonly ContextMenu.ContextMenu ContextMenu = new ContextMenu.ContextMenu();

        /// <summary>
        /// The background color when mouse is over.
        /// </summary>
        public Color BackgroundColorMouseOver;

        /// <summary>
        /// The background color when mouse is over and context menu is opened.
        /// </summary>
        public Color BackgroundColorMouseOverOpened;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenuButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        public MainMenuButton(string text)
        : base(0, 0, 32, 16)
        {
            Text = text;

            var style = Style.Current;
#if PLATFORM_WINDOWS
            if (Editor.Instance.Options.Options.Interface.UseNativeWindowSystem)
            {
                BackgroundColorMouseOver = style.BackgroundHighlighted;
                BackgroundColorMouseOverOpened = style.Background;
            }
            else
#endif
            {
                BackgroundColorMouseOver = BackgroundColorMouseOverOpened = style.LightBackground * 1.3f;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            var clientRect = new Rectangle(0, 0, Width, Height);
            var hasChildItems = ContextMenu.HasChildren;
            var isOpened = ContextMenu.IsOpened;
            bool enabled = EnabledInHierarchy;

            // Draw background
            if (enabled && hasChildItems && (isOpened || IsMouseOver))
            {
                Render2D.FillRectangle(clientRect, isOpened ? BackgroundColorMouseOverOpened : BackgroundColorMouseOver);
            }

            // Draw text
            Render2D.DrawText(style.FontMedium, Text, clientRect, enabled && hasChildItems ? style.Foreground : style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();

            if (Parent is MainMenu menu)
                menu.Selected = this;

            return true;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            if (Parent is MainMenu menu && menu.Selected != null)
                menu.Selected = this;
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            var style = Style.Current;
            float width = 18;

            if (style.FontMedium)
                width += style.FontMedium.MeasureText(Text).X;

            Width = width;
        }
    }
}
