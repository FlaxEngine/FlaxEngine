// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="UICanvas"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class UICanvasNode : ActorNode
    {
        /// <inheritdoc />
        public UICanvasNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            if (Actor.HasPrefabLink)
            {
                return;
            }

            // Rotate to match the space (GUI uses upper left corner as a root)
            Actor.LocalOrientation = Quaternion.Euler(0, -180, -180);
            bool canSpawn = true;
            foreach (var uiControl in Actor.GetChildren<UIControl>())
            {
                if (uiControl.Get<CanvasScaler>() == null)
                    continue;
                canSpawn = false;
                break;
            }

            if (canSpawn)
            {
                var uiControl = new UIControl
                {
                    Name = "Canvas Scalar",
                    Transform = Actor.Transform,
                    Control = new CanvasScaler()
                };
                Root.Spawn(uiControl, Actor);
            }
            _treeNode.Expand();
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            normal = Vector3.Up;

            if (Actor is UICanvas uiCanvas && uiCanvas.Is3D)
                return uiCanvas.Bounds.Intersects(ref ray.Ray, out distance);

            distance = 0;
            return false;
        }

        /// <inheritdoc />
        public override void OnDebugDraw(ViewportDebugDrawData data)
        {
            base.OnDebugDraw(data);

            if (Actor is UICanvas uiCanvas && uiCanvas.Is3D)
                DebugDraw.DrawWireBox(uiCanvas.Bounds, Color.BlueViolet);
        }

        /// <inheritdoc />
        public override bool CanSelectInViewport => base.CanSelectInViewport && Actor is UICanvas uiCanvas && uiCanvas.Is3D;
    }
}
