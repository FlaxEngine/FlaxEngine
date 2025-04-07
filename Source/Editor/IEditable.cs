// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEditor
{
    /// <summary>
    /// Interface for all objects that can be modified (dirty state) and expose some functionalities and events.
    /// </summary>
    public interface IEditable
    {
        /// <summary>
        /// Occurs when object gets edited.
        /// </summary>
        event Action OnEdited;

        /// <summary>
        /// Gets a value indicating whether this object is edited (dirty state).
        /// </summary>
        bool IsEdited { get; }

        /// <summary>
        /// Marks object as edited (sets dirty flag).
        /// </summary>
        void MarkAsEdited();
    }
}
