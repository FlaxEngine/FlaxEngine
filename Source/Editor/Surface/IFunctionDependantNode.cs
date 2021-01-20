// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Interface for surface nodes that depend on surface function nodes collection.
    /// </summary>
    [HideInEditor]
    public interface IFunctionsDependantNode
    {
        /// <summary>
        /// On function created.
        /// </summary>
        /// <param name="node">The function node.</param>
        void OnFunctionCreated(SurfaceNode node);

        /// <summary>
        /// On function signature changed (new name or parameters change).
        /// </summary>
        /// <param name="node">The function node.</param>
        void OnFunctionEdited(SurfaceNode node);

        /// <summary>
        /// On function removed.
        /// </summary>
        /// <param name="node">The function node.</param>
        void OnFunctionDeleted(SurfaceNode node);
    }
}
