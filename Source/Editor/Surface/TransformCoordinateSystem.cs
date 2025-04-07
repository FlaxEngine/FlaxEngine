// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Vector transformation coordinate systems.
    /// </summary>
    [HideInEditor]
    public enum TransformCoordinateSystem
    {
        /// <summary>
        /// The world space. It's absolute world space coordinate system.
        /// </summary>
        World = 0,

        /// <summary>
        /// The tangent space. It's relative to the surface (tangent frame defined by normal, tangent and bitangent vectors).
        /// </summary>
        Tangent = 1,

        /// <summary>
        /// The view space. It's relative to the current rendered viewport orientation.
        /// </summary>
        View = 2,

        /// <summary>
        /// The local space. It's relative to the rendered object (aka object space).
        /// </summary>
        Local = 3,
    }
}
