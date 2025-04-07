// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Context Menu control that cn be expanded to the child popup menu.
    /// </summary>
    /// <seealso cref="ContextMenuItem" />
    [HideInEditor]
    public class ContextMenuChildMenu : ContextMenuButton
    {
        /// <summary>
        /// The child context menu.
        /// </summary>
        public readonly ContextMenu ContextMenu = new ContextMenu();

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenuChildMenu"/> class.
        /// </summary>
        /// <param name="parent">The parent context menu.</param>
        /// <param name="text">The text.</param>
        public ContextMenuChildMenu(ContextMenu parent, string text)
        : base(parent, text)
        {
            Text = text;
            CloseMenuOnClick = false;
        }

        private void ShowChild(ContextMenu parentContextMenu)
        {
            // Hide parent CM popups and set itself as child
            var vAlign = parentContextMenu.ItemsAreaMargin.Top;
            var location = new Float2(Width, -vAlign);
            location = PointToParent(parentContextMenu, location);
            parentContextMenu.ShowChild(ContextMenu, location);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var backgroundRect = new Rectangle(-X + 3, 0, Parent.Width - 6, Height);
            bool isCMopened = ContextMenu.IsOpened;

            // Draw background
            if (isCMopened)
                Render2D.FillRectangle(backgroundRect, style.LightBackground);

            base.Draw();

            // Draw arrow
            if (ContextMenu.HasChildren)
                Render2D.DrawSprite(style.ArrowRight, new Rectangle(Width - 15 + ExtraAdjustmentAmount, (Height - 12) / 2, 12, 12), Enabled ? isCMopened ? style.BackgroundSelected : style.Foreground : style.ForegroundDisabled);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            // Skip if has no children
            if (ContextMenu.HasChildren == false)
                return;

            // Skip if already shown
            var parentContextMenu = ParentContextMenu;
            if (parentContextMenu == ContextMenu)
                return;
            if (ContextMenu.IsOpened)
                return;

            base.OnMouseEnter(location);

            ShowChild(parentContextMenu);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Skip if already shown
            var parentContextMenu = ParentContextMenu;
            if (parentContextMenu == ContextMenu)
                return true;
            if (ContextMenu.IsOpened)
                return true;

            ShowChild(parentContextMenu);
            return base.OnMouseUp(location, button);
        }
    }
}
