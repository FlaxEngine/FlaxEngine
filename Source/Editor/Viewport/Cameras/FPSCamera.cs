// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
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

        /// <summary>
        /// Moves the viewport to visualize the actor.
        /// </summary>
        /// <param name="actor">The actor to preview.</param>
        public void ShowActor(Actor actor)
        {
            Editor.GetActorEditorSphere(actor, out BoundingSphere sphere);
            ShowSphere(ref sphere);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="actor">The actors to show.</param>
        /// <param name="orientation">The used orientation.</param>
        public void ShowActor(Actor actor, ref Quaternion orientation)
        {
            Editor.GetActorEditorSphere(actor, out BoundingSphere sphere);
            ShowSphere(ref sphere, ref orientation);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        public void ShowActors(List<SceneGraphNode> selection)
        {
            if (selection.Count == 0)
                return;

            BoundingSphere mergesSphere = BoundingSphere.Empty;
            for (int i = 0; i < selection.Count; i++)
            {
                selection[i].GetEditorSphere(out var sphere);
                BoundingSphere.Merge(ref mergesSphere, ref sphere, out mergesSphere);
            }

            if (mergesSphere == BoundingSphere.Empty)
                return;
            ShowSphere(ref mergesSphere);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        /// <param name="orientation">The used orientation.</param>
        public void ShowActors(List<SceneGraphNode> selection, ref Quaternion orientation)
        {
            if (selection.Count == 0)
                return;

            BoundingSphere mergesSphere = BoundingSphere.Empty;
            for (int i = 0; i < selection.Count; i++)
            {
                selection[i].GetEditorSphere(out var sphere);
                BoundingSphere.Merge(ref mergesSphere, ref sphere, out mergesSphere);
            }

            if (mergesSphere == BoundingSphere.Empty)
                return;
            ShowSphere(ref mergesSphere, ref orientation);
        }

        /// <summary>
        /// Moves the camera to visualize given world area defined by the sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        public void ShowSphere(ref BoundingSphere sphere)
        {
            var q = new Quaternion(0.424461186f, -0.0940724313f, 0.0443938486f, 0.899451137f);
            ShowSphere(ref sphere, ref q);
        }

        /// <summary>
        /// Moves the camera to visualize given world area defined by the sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="orientation">The camera orientation.</param>
        public void ShowSphere(ref BoundingSphere sphere, ref Quaternion orientation)
        {
            Vector3 position;
            if (Viewport.UseOrthographicProjection)
            {
                position = sphere.Center + Vector3.Backward * orientation * (sphere.Radius * 5.0f);
                Viewport.OrthographicScale = Vector3.Distance(position, sphere.Center) / 1000;
            }
            else
            {
                position = sphere.Center - Vector3.Forward * orientation * (sphere.Radius * 2.5f);
            }
            TargetPoint = sphere.Center;
            MoveViewport(position, orientation);
        }

        /// <inheritdoc />
        public override void SetArcBallView(Quaternion orientation, Vector3 orbitCenter, float orbitRadius)
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
                float a = Mathf.Saturate(progress);
                a = a * a * a;
                Transform targetTransform = Transform.Lerp(_startMove, _endMove, a);
                targetTransform.Scale = Vector3.Zero;
                Viewport.ViewPosition = targetTransform.Translation;
                Viewport.ViewOrientation = targetTransform.Orientation;
            }
        }

        /// <inheritdoc />
        public override void UpdateView(float dt, ref Vector3 moveDelta, ref Vector2 mouseDelta, out bool centerMouse)
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
                var panningSpeed = 0.8f;
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
                float orbitRadius = Mathf.Max(Vector3.Distance(ref position, ref TargetPoint), 0.0001f);
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
