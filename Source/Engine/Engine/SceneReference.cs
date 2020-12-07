// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Represents the reference to the scene asset. Stores the unique ID of the scene to reference. Can be used to load the selected scene.
    /// </summary>
    public struct SceneReference
    {
        /// <summary>
        /// The identifier of the scene asset (and the scene object).
        /// </summary>
        public Guid ID;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneReference"/> class.
        /// </summary>
        /// <param name="id">The identifier of the scene asset.</param>
        public SceneReference(Guid id)
        {
            ID = id;
        }
    }
}
