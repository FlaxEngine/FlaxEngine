// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Viewport.Cameras
{
    /// <summary>
    /// The interface for the editor viewport camera controllers. Handles the input logic updates and the preview rendering viewport.
    /// </summary>
    [HideInEditor]
    public interface IViewportCamera
    {
        /// <summary>
        /// Updates the camera.
        /// </summary>
        /// <param name="deltaTime">The delta time (in seconds).</param>
        void Update(float deltaTime);

        /// <summary>
        /// Updates the view.
        /// </summary>
        /// <param name="dt">The delta time (in seconds).</param>
        /// <param name="moveDelta">The move delta (scaled).</param>
        /// <param name="mouseDelta">The mouse delta (scaled).</param>
        /// <param name="centerMouse">True if center mouse after the update.</param>
        void UpdateView(float dt, ref Vector3 moveDelta, ref Float2 mouseDelta, out bool centerMouse);
    }
}
