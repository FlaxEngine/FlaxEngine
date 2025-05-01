// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Defines the context data and logic for the Visject Surface editor.
    /// </summary>
    [HideInEditor]
    public interface ISurfaceContext
    {
        /// <summary>
        /// Gets the asset containing the surface (optional, null by default).
        /// </summary>
        Asset SurfaceAsset { get; }

        /// <summary>
        /// Gets the name of the surface (for UI).
        /// </summary>
        string SurfaceName { get; }

        /// <summary>
        /// Gets or sets the surface data. Used to load or save the surface to the data source.
        /// </summary>
        byte[] SurfaceData { get; set; }

        /// <summary>
        /// Gets the context which owns this surface context (null for root).
        /// </summary>
        VisjectSurfaceContext ParentContext { get; }

        /// <summary>
        /// Called when Visject Surface context gets created for this surface data source. Can be used to link for some events.
        /// </summary>
        /// <param name="context">The context.</param>
        void OnContextCreated(VisjectSurfaceContext context);
    }
}
