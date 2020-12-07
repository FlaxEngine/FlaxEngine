// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Interface for scene objects that unifies various properties used across actors and scripts.
    /// </summary>
    public interface ISceneObject
    {
        /// <summary>
        /// Gets the scene object which contains this object.
        /// </summary>
        Scene Scene { get; }

        /// <summary>
        /// Gets a value indicating whether this object has a valid linkage to the prefab asset.
        /// </summary>
        bool HasPrefabLink { get; }

        /// <summary>
        /// Gets the prefab asset ID. Empty if no prefab link exists.
        /// </summary>
        Guid PrefabID { get; }

        /// <summary>
        /// Gets the ID of the object within a object that is used for synchronization with this object. Empty if no prefab link exists.
        /// </summary>
        Guid PrefabObjectID { get; }

        /// <summary>
        /// Breaks the prefab linkage for this object (including all children).
        /// </summary>
        void BreakPrefabLink();
    }
}
