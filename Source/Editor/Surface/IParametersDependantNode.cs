// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Interface for surface nodes that depend on surface parameters collection.
    /// </summary>
    [HideInEditor]
    public interface IParametersDependantNode
    {
        /// <summary>
        /// On new parameter created.
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamCreated(SurfaceParameter param);

        /// <summary>
        /// On parameter renamed.
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamRenamed(SurfaceParameter param);

        /// <summary>
        /// On parameter modified (eg. type changed).
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamEdited(SurfaceParameter param);

        /// <summary>
        /// On parameter deleted.
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamDeleted(SurfaceParameter param);

        /// <summary>
        /// Get if the parameter is referenced in a graph. Referenced in this case means in a graph and at least one node in-/output connected to another node.
        /// </summary>
        /// <param name="param">The parameter.</param>
        /// <returns>If the parameter is referenced.</returns>
        bool IsParamUsed(SurfaceParameter param);
    }
}
