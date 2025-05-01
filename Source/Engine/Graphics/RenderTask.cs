// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class RenderTask
    {
        /// <summary>
        /// The custom tag object value linked to the rendering task.
        /// </summary>
        public object Tag;
    }

    partial class SceneRenderTask
    {
        /// <summary>
        /// Gets or sets the view flags (via <see cref="View"/> property).
        /// </summary>
        public ViewFlags ViewFlags
        {
            get => View.Flags;
            set
            {
                var view = View;
                view.Flags = value;
                View = view;
            }
        }

        /// <summary>
        /// Gets or sets the view mode (via <see cref="View"/> property).
        /// </summary>
        public ViewMode ViewMode
        {
            get => View.Mode;
            set
            {
                var view = View;
                view.Mode = value;
                View = view;
            }
        }

        /// <summary>
        /// The rendering mask for layers. Used to exclude objects from rendering (via <see cref="View"/> property).
        /// </summary>
        public LayersMask ViewLayersMask
        {
            get => View.RenderLayersMask;
            set
            {
                var view = View;
                view.RenderLayersMask = value;
                View = view;
            }
        }
    }
}
