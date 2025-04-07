// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Interface for Visject Surface parent objects.
    /// </summary>
    [HideInEditor]
    public interface IVisjectSurfaceOwner : ISurfaceContext
    {
        /// <summary>
        /// Gets the undo system for actions (can be null if not used).
        /// </summary>
        FlaxEditor.Undo Undo { get; }

        /// <summary>
        /// On surface edited state gets changed
        /// </summary>
        void OnSurfaceEditedChanged();

        /// <summary>
        /// On surface graph edited
        /// </summary>
        void OnSurfaceGraphEdited();

        /// <summary>
        /// Called when surface wants to close the tool window (due to user interaction or sth else).
        /// </summary>
        void OnSurfaceClose();
    }
}
