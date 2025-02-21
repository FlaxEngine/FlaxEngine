// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEditor.Gizmo;
using FlaxEngine;

namespace FlaxEditor.Viewport.Cameras
{
    /// <summary>
    /// Implementation of <see cref="ViewportCamera"/> that simulated the first-person camera which can fly though the scene.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Cameras.ViewportCamera" />
    [HideInEditor]
    public class FPSCamera : ViewportCamera
    {
        private Transform _startMove;
        private Transform _endMove;
        private float _moveStartTime = -1;

        /// <summary>
        /// Gets a value indicating whether this viewport is animating movement.
        /// </summary>
        public bool IsAnimatingMove => _moveStartTime > Mathf.Epsilon;

        /// <summary>
        /// The target point location. It's used to orbit around it when user clicks Alt+LMB.
        /// </summary>
        public Vector3 TargetPoint = new Vector3(-200);

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="position">The view position.</param>
        /// <param name="direction">The view direction.</param>
        public void SetView(Vector3 position, Vector3 direction)
        {
            if (IsAnimatingMove)
                return;

            // Rotate and move
            Viewport.ViewPosition = position;
            Viewport.ViewDirection = direction;
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="position">The view position.</param>
        /// <param name="orientation">The view rotation.</param>
        public void SetView(Vector3 position, Quaternion orientation)
        {
            if (IsAnimatingMove)
                return;

            // Rotate and move
            Viewport.ViewPosition = position;
            Viewport.ViewOrientation = orientation;
        }

        /// <summary>
        /// Start animating viewport movement to the target transformation.
        /// </summary>
        /// <param name="position">The target position.</param>
        /// <param name="orientation">The target orientation.</param>
        public void MoveViewport(Vector3 position, Quaternion orientation)
        {
            MoveViewport(new Transform(position, orientation));
        }

        /// <summary>
        /// Start animating viewport movement to the target transformation.
        /// </summary>
        /// <param name="target">The target transform.</param>
        public void MoveViewport(Transform target)
        {
            _startMove = Viewport.ViewTransform;
            _endMove = target;
            _moveStartTime = Time.UnscaledGameTime;
        }

        /// <inheritdoc />
        public override void ShowSphere(ref BoundingSphere sphere, ref Quaternion orientation)
        {
            Vector3 position;
            if (Viewport.UseOrthographicProjection)
            {
                position = sphere.Center + Vector3.Backward * orientation * (sphere.Radius * 5.0f);
                Viewport.OrthographicScale = (float)Vector3.Distance(position, sphere.Center) / 1000;
            }
            else
            {
                // calculate the min. distance so that the sphere fits roughly 70% in FOV
                // clip to far plane as a disappearing big object might be confusing
                var distance = Mathf.Min(1.4f * sphere.Radius / Mathf.Tan(Mathf.DegreesToRadians * Viewport.FieldOfView / 2), Viewport.FarPlane);
                position = sphere.Center - Vector3.Forward * orientation * distance;
            }
            TargetPoint = sphere.Center;
            MoveViewport(position, orientation);
        }

        /// <inheritdoc />
        public override void SetArcBallView(Quaternion orientation, Vector3 orbitCenter, Real orbitRadius)
        {
            base.SetArcBallView(orientation, orbitCenter, orbitRadius);

            TargetPoint = orbitCenter;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Update animated movement
            if (IsAnimatingMove)
            {
                // Calculate linear progress
                float animationDuration = 0.5f;
                float time = Time.UnscaledGameTime;
                float progress = (time - _moveStartTime) / animationDuration;

                // Check for end
                if (progress >= 1.0f)
                {
                    // Animation has been finished
                    _moveStartTime = -1;
                }

                // Animate camera
                try
                {
                    float a = Mathf.Saturate(progress);
                    a = a * a * a;
                    var targetTransform = Transform.Lerp(_startMove, _endMove, a);
                    if (progress >= 1.0f)
                        targetTransform = _endMove; // Be precise
                    targetTransform.Scale = Vector3.Zero;
                    Viewport.ViewPosition = targetTransform.Translation;
                    Viewport.ViewOrientation = targetTransform.Orientation;
                }
                catch
                {
                    // Fix camera if lerp failed (eg. large world with NaNs inside)
                    Viewport.ViewPosition = Vector3.Zero;
                    Viewport.ViewOrientation = Quaternion.Identity;
                }
            }
        }

        /// <inheritdoc />
        public override void UpdateView(float dt, ref Vector3 moveDelta, ref Float2 mouseDelta, out bool centerMouse)
        {
            centerMouse = true;

            if (IsAnimatingMove)
                return;

            Viewport.GetInput(out var input);
            Viewport.GetPrevInput(out var prevInput);
            var transformGizmo = (Viewport as EditorGizmoViewport)?.Gizmos.Active as TransformGizmoBase;
            var isUsingGizmo = transformGizmo != null && transformGizmo.ActiveAxis != TransformGizmoBase.Axis.None;

            // Get current view properties
            var yaw = Viewport.Yaw;
            var pitch = Viewport.Pitch;
            var position = Viewport.ViewPosition;
            var rotation = Viewport.ViewOrientation;

            // Compute base vectors for camera movement
            var forward = Vector3.Forward * rotation;
            var up = Vector3.Up * rotation;
            var right = Vector3.Cross(forward, up);

            // Dolly
            if (input.IsPanning || input.IsMoving || input.IsRotating)
            {
                Vector3.Transform(ref moveDelta, ref rotation, out Vector3 move);
                position += move;
            }

            // Pan
            if (input.IsPanning)
            {
                var panningSpeed = (Viewport.RelativePanning)
                    ? Mathf.Abs((position - TargetPoint).Length) * 0.005f
                    : Viewport.PanningSpeed;

                if (Viewport.InvertPanning)
                {
                    position += up * (mouseDelta.Y * panningSpeed);
                    position += right * (mouseDelta.X * panningSpeed);
                }
                else
                {
                    position -= right * (mouseDelta.X * panningSpeed);
                    position -= up * (mouseDelta.Y * panningSpeed);
                }
            }

            // Move
            if (input.IsMoving)
            {
                // Move camera over XZ plane
                var projectedForward = Vector3.Normalize(new Vector3(forward.X, 0, forward.Z));
                position -= projectedForward * mouseDelta.Y;
                yaw += mouseDelta.X;
            }

            // Rotate or orbit
            if (input.IsRotating || (input.IsOrbiting && !isUsingGizmo && prevInput.IsOrbiting))
            {
                yaw += mouseDelta.X;
                pitch += mouseDelta.Y;
            }

            // Zoom in/out
            if (input.IsZooming && !input.IsRotating)
            {
                position += forward * (Viewport.MouseWheelZoomSpeedFactor * input.MouseWheelDelta * 25.0f);
                if (input.IsAltDown)
                {
                    position += forward * (Viewport.MouseSpeed * 40 * Viewport.MousePositionDelta.ValuesSum);
                }
            }

            // Move camera with the gizmo
            if (input.IsOrbiting && isUsingGizmo)
            {
                centerMouse = false;
                Viewport.ViewPosition += transformGizmo.LastDelta.Translation;
                return;
            }

            // Update view
            Viewport.Yaw = yaw;
            Viewport.Pitch = pitch;
            if (input.IsOrbiting)
            {
                float orbitRadius = Mathf.Max((float)Vector3.Distance(ref position, ref TargetPoint), 0.0001f);
                Vector3 localPosition = Viewport.ViewDirection * (-1 * orbitRadius);
                Viewport.ViewPosition = TargetPoint + localPosition;
            }
            else
            {
                TargetPoint += position - Viewport.ViewPosition;
                Viewport.ViewPosition = position;
            }
        }
    }
}
