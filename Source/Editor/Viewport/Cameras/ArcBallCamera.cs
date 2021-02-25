// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Viewport.Cameras
{
    /// <summary>
    /// Implementation of <see cref="ViewportCamera"/> that orbits around the fixed location.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Cameras.ViewportCamera" />
    [HideInEditor]
    public class ArcBallCamera : ViewportCamera
    {
        private Vector3 _orbitCenter;
        private float _orbitRadius;

        /// <summary>
        /// Gets or sets the orbit center.
        /// </summary>
        public Vector3 OrbitCenter
        {
            get => _orbitCenter;
            set
            {
                _orbitCenter = value;
                UpdatePosition();
            }
        }

        /// <summary>
        /// Gets or sets the orbit radius.
        /// </summary>
        public float OrbitRadius
        {
            get => _orbitRadius;
            set
            {
                _orbitRadius = Mathf.Max(value, 0.0001f);
                UpdatePosition();
            }
        }

        /// <inheritdoc />
        public override bool UseMovementSpeed => false;

        /// <summary>
        /// Initializes a new instance of the <see cref="ArcBallCamera"/> class.
        /// </summary>
        public ArcBallCamera()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ArcBallCamera"/> class.
        /// </summary>
        /// <param name="orbitCenter">The orbit center.</param>
        /// <param name="radius">The orbit radius.</param>
        public ArcBallCamera(Vector3 orbitCenter, float radius)
        {
            _orbitCenter = orbitCenter;
            _orbitRadius = radius;
        }

        /// <summary>
        /// Sets view direction.
        /// </summary>
        /// <param name="direction">The view direction.</param>
        public void SetView(Vector3 direction)
        {
            // Rotate
            Viewport.ViewDirection = direction;

            // Update view position
            UpdatePosition();
        }

        /// <summary>
        /// Sets view orientation.
        /// </summary>
        /// <param name="orientation">The view rotation.</param>
        public void SetView(Quaternion orientation)
        {
            // Rotate
            Viewport.ViewOrientation = orientation;

            // Update view position
            UpdatePosition();
        }

        private void UpdatePosition()
        {
            // Move camera to look at orbit center point
            Vector3 localPosition = Viewport.ViewDirection * (-1 * _orbitRadius);
            Viewport.ViewPosition = _orbitCenter + localPosition;
        }

        /// <inheritdoc />
        public override void UpdateView(float dt, ref Vector3 moveDelta, ref Vector2 mouseDelta, out bool centerMouse)
        {
            centerMouse = true;

            Viewport.GetInput(out EditorViewport.Input input);

            // Rotate
            Viewport.YawPitch += mouseDelta;

            // Zoom
            if (input.IsZooming)
            {
                _orbitRadius = Mathf.Clamp(_orbitRadius - (Viewport.MouseWheelZoomSpeedFactor * input.MouseWheelDelta * 25.0f), 0.001f, 10000.0f);
            }

            // Update view position
            UpdatePosition();
        }
    }
}
