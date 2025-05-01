// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
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
        /// Focuses the viewport on the current selection of the gizmo.
        /// </summary>
        /// <param name="gizmos">The gizmo collection (from viewport).</param>
        /// <param name="orientation">The target view orientation.</param>
        public virtual void FocusSelection(GizmosCollection gizmos, ref Quaternion orientation)
        {
            var transformGizmo = gizmos.Get<TransformGizmo>();
            if (transformGizmo == null || transformGizmo.SelectedParents.Count == 0)
                return;
            if (gizmos.Active != null)
            {
                var gizmoBounds = gizmos.Active.FocusBounds;
                if (gizmoBounds != BoundingSphere.Empty)
                {
                    ShowSphere(ref gizmoBounds, ref orientation);
                    return;
                }
            }
            ShowActors(transformGizmo.SelectedParents, ref orientation);
        }

        /// <summary>
        /// Moves the viewport to visualize the actor.
        /// </summary>
        /// <param name="actor">The actor to preview.</param>
        public virtual void ShowActor(Actor actor)
        {
            Editor.GetActorEditorSphere(actor, out BoundingSphere sphere);
            ShowSphere(ref sphere);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        public void ShowActors(List<SceneGraphNode> selection)
        {
            var q = new Quaternion(0.424461186f, -0.0940724313f, 0.0443938486f, 0.899451137f);
            ShowActors(selection, ref q);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        /// <param name="orientation">The used orientation.</param>
        public virtual void ShowActors(List<SceneGraphNode> selection, ref Quaternion orientation)
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
        public virtual void ShowSphere(ref BoundingSphere sphere, ref Quaternion orientation)
        {
            SetArcBallView(orientation, sphere.Center, sphere.Radius);
        }

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
        public virtual void SetArcBallView(Quaternion orientation, Vector3 orbitCenter, Real orbitRadius)
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
        public abstract void UpdateView(float dt, ref Vector3 moveDelta, ref Float2 mouseDelta, out bool centerMouse);
    }
}
