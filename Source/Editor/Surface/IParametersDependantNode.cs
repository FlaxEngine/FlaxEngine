// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
        /// On new parameter created
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamCreated(SurfaceParameter param);

        /// <summary>
        /// On new parameter renamed
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamRenamed(SurfaceParameter param);

        /// <summary>
        /// On new parameter deleted
        /// </summary>
        /// <param name="param">The parameter.</param>
        void OnParamDeleted(SurfaceParameter param);
    }
}
