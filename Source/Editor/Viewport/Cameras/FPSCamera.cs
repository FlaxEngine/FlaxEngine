// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEditor.Gizmo;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.Viewport.Cameras
{
    /// <summary>
    /// Implementation of <see cref="ViewportCamera"/> that simulated the first-person camera which can fly though the scene.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Cameras.ViewportCamera" />
    [HideInEditor]
    public class FPSCamera : ViewportCamera
    {
        private Vector3LongFloat _posStartMove;
        private Quaternion _rotStartMove;
        private Vector3LongFloat _posEndMove;
        private Quaternion _rotEndMove;
        private float _moveStartTime = -1;
        private float _additionalFOV;

        /// <summary>
        /// Gets a value indicating whether this viewport is animating movement.
        /// </summary>
        public bool IsAnimatingMove => _moveStartTime > Mathf.Epsilon;

        /// <summary>
        /// The target point location. It's used to orbit around it when user clicks Alt+LMB.
        /// </summary>
        public Vector3LongFloat TargetPoint = new Vector3LongFloat(-200);

        /// <summary>
        /// Additional field of view used for zooming the camera in and out.
        /// </summary>
        public float AdditionalZoomFOV
        {
            get => _additionalFOV;
            private set => _additionalFOV = Mathf.Clamp(value, 5 - Viewport.FieldOfView, 160f - Viewport.FieldOfView);
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="position">The view position.</param>
        /// <param name="direction">The view direction.</param>
        public void SetView(Vector3LongFloat position, Vector3 direction)
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
        public void SetView(Vector3LongFloat position, Quaternion orientation)
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
        public void MoveViewport(Vector3LongFloat position, Quaternion orientation)
        {
            _posStartMove = Viewport.ViewPosition;
            _rotStartMove = Viewport.ViewOrientation;
            _rotEndMove = orientation;
            _posEndMove = position;
            _moveStartTime = Time.UnscaledGameTime;
        }

        /// <inheritdoc />
        public override void ShowSphere(Vector3LongFloat Center,Real radius, ref Quaternion orientation)
        {
            Vector3LongFloat position;
            TargetPoint = Center;
            if (Viewport.UseOrthographicProjection)
            {
                position = TargetPoint + Vector3LongFloat.Expand(Vector3.Backward * orientation * (radius * 5.0f));
                Viewport.OrthographicScale = (Vector3LongFloat.Distance(position, TargetPoint) / new LongFloat(1000)).Quantize();
            }
            else
            {
                // calculate the min. distance so that the sphere fits roughly 70% in FOV
                // clip to far plane as a disappearing big object might be confusing
                var distance = Mathf.Min(1.4f * radius / Mathf.Tan(Mathf.DegreesToRadians * Viewport.FieldOfView / 2), Viewport.FarPlane);
                position = TargetPoint - Vector3LongFloat.Expand(Vector3.Forward * orientation * distance);
            }
            MoveViewport(position, orientation);
        }

        /// <inheritdoc />
        public override void SetArcBallView(Quaternion orientation, Vector3LongFloat orbitCenter, Real orbitRadius)
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
                    var targetRot = Quaternion.Lerp(_rotStartMove, _rotEndMove, a);
                    var translation = Vector3LongFloat.Lerp(_posStartMove, _posEndMove, new LongFloat(a));
                    if (progress >= 1.0f)
                    {
                        targetRot = _rotEndMove; // Be precise
                    }
                    Viewport.ViewPosition = translation;
                    Viewport.ViewOrientation = targetRot;
                }
                catch
                {
                    // Fix camera if lerp failed (eg. large world with NaNs inside)
                    Viewport.ViewPosition = Vector3LongFloat.Zero;
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
                position += Vector3LongFloat.Expand(move);
            }

            // Pan
            if (input.IsPanning)
            {
                var panningSpeed = (Viewport.RelativePanning)
                    ? Mathf.Abs((position - TargetPoint).Length.Quantize()) * 0.005f
                    : Viewport.PanningSpeed;

                if (Viewport.InvertPanning)
                {
                    position += Vector3LongFloat.Expand(up * (mouseDelta.Y * panningSpeed));
                    position += Vector3LongFloat.Expand(right * (mouseDelta.X * panningSpeed));
                }
                else
                {
                    position -= Vector3LongFloat.Expand(right * (mouseDelta.X * panningSpeed));
                    position -= Vector3LongFloat.Expand(up * (mouseDelta.Y * panningSpeed));
                }
            }

            // Move
            if (input.IsMoving)
            {
                // Move camera over XZ plane
                var projectedForward = Vector3.Normalize(new Vector3(forward.X, 0, forward.Z));
                position -= Vector3LongFloat.Expand(projectedForward * mouseDelta.Y);
                yaw += mouseDelta.X;
            }

            // Rotate or orbit
            if (input.IsRotating || (input.IsOrbiting && !isUsingGizmo && prevInput.IsOrbiting))
            {
                yaw += mouseDelta.X;
                pitch += mouseDelta.Y;
            }

            // Zoom in/out with mouse wheel
            if (input.IsZooming && !input.IsRotating)
            {
                position += Vector3LongFloat.Expand(forward * (Viewport.MouseWheelZoomSpeedFactor * input.MouseWheelDelta * 25.0f));
                if (input.IsAltDown)
                {
                    position += Vector3LongFloat.Expand(forward * (Viewport.MouseSpeed * 40 * Viewport.MousePositionDelta.ValuesSum));
                }
            }

            // Zoom in and out by changing FOV
            if (input.IsRotating && (input.ZoomInDown || input.ZoomOutDown))
            {
                float delta = (input.ZoomInDown ? -0.8f : 0.8f);
                AdditionalZoomFOV += delta;
            }
            else if (!input.IsRotating)
            {
                AdditionalZoomFOV = 0f;
            }

            // Move camera with the gizmo
            if (input.IsOrbiting && isUsingGizmo)
            {
                centerMouse = false;
                Viewport.ViewPosition += Vector3LongFloat.Expand(transformGizmo.LastDelta.Translation);
                return;
            }

            // Update view
            Viewport.Yaw = yaw;
            Viewport.Pitch = pitch;
            if (input.IsOrbiting)
            {
                LongFloat orbitRadius = Vector3LongFloat.Distance(position, TargetPoint);
                LongFloat minOrbitRadius = LongFloatMath.Expand(0.0001);

                if (orbitRadius < minOrbitRadius)
                    orbitRadius = minOrbitRadius;

                Vector3LongFloat localPosition =
                    Vector3LongFloat.Expand(-Viewport.ViewDirection) * orbitRadius;

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
