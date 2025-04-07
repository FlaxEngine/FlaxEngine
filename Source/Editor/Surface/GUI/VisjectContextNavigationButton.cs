// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEngine;

namespace FlaxEditor.Surface.GUI
{
    /// <summary>
    /// A <see cref="VisjectSurface"/> context navigation bar button. Allows to change the current context and view the context path.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Button" />
    [HideInEditor]
    public class VisjectContextNavigationButton : NavigationButton
    {
        /// <summary>
        /// Gets the parent surface.
        /// </summary>
        public VisjectSurface Surface { get; }

        /// <summary>
        /// Gets the target context.
        /// </summary>
        public ISurfaceContext Context { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectContextNavigationButton"/> class.
        /// </summary>
        /// <param name="surface">The parent surface.</param>
        /// <param name="context">The target context.</param>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        public VisjectContextNavigationButton(VisjectSurface surface, ISurfaceContext context, float x, float y, float height)
        : base(x, y, height)
        {
            Surface = surface;
            Context = context;
            Text = context.SurfaceName + "/";
        }

        /// <inheritdoc />
        protected override void OnClick()
        {
            // Navigate
            Surface.ChangeContext(Context);

            base.OnClick();
        }
    }
}
