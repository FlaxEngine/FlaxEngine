// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Viewport.Cameras
{
    /// <summary>
    /// Base class for <see cref="EditorViewport"/> view controllers.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Cameras.IViewportCamera" />
    [HideInEditor]
    public abstract class ViewportCamera : IViewportCamera
    {
        private EditorViewport _viewport;

        /// <summary>
        /// Gets the parent viewport.
        /// </summary>
        public EditorViewport Viewport
        {
            get => _viewport;
            internal set => _viewport = value;
        }

        /// <summary>
        /// Gets a value indicating whether the viewport camera uses movement speed.
        /// </summary>
        public virtual bool UseMovementSpeed => true;

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given target object bounds.
        /// </summary>
        /// <param name="objectBounds">The target object bounds.</param>
        /// <param name="marginDistanceScale">The margin distance scale of the orbit radius.</param>
        public void SetArcBallView(BoundingBox objectBounds, float marginDistanceScale = 2.0f)
        {
            SetArcBallView(BoundingSphere.FromBox(objectBounds), marginDistanceScale);
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given target object bounds.
        /// </summary>
        /// <param name="objectBounds">The target object bounds.</param>
        /// <param name="marginDistanceScale">The margin distance scale of the orbit radius.</param>
        public void SetArcBallView(BoundingSphere objectBounds, float marginDistanceScale = 2.0f)
        {
            SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), objectBounds.Center, objectBounds.Radius * marginDistanceScale);
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given orbit radius.
        /// </summary>
        /// <param name="orbitRadius">The orbit radius.</param>
        public void SetArcBallView(float orbitRadius)
        {
            SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), Vector3.Zero, orbitRadius);
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view.
        /// </summary>
        /// <param name="orientation">The view rotation.</param>
        /// <param name="orbitCenter">The orbit center location.</param>
        /// <param name="orbitRadius">The orbit radius.</param>
        public void SetArcBallView(Quaternion orientation, Vector3 orbitCenter, float orbitRadius)
        {
            // Rotate
            Viewport.ViewOrientation = orientation;

            // Move camera to look at orbit center point
            Vector3 localPosition = Viewport.ViewDirection * (-1 * orbitRadius);
            Viewport.ViewPosition = orbitCenter + localPosition;
        }

        /// <inheritdoc />
        public virtual void Update(float deltaTime)
        {
        }

        /// <inheritdoc />
        public abstract void UpdateView(float dt, ref Vector3 moveDelta, ref Vector2 mouseDelta, out bool centerMouse);
    }
}
