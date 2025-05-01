// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Helper class for actors with icon drawn in editor (eg. lights, probes, etc.).
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public abstract class ActorNodeWithIcon : ActorNode
    {
        /// <inheritdoc />
        protected ActorNodeWithIcon(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            normal = Vector3.Up;

            // Check if skip raycasts
            if ((ray.Flags & RayCastData.FlagTypes.SkipEditorPrimitives) == RayCastData.FlagTypes.SkipEditorPrimitives)
            {
                distance = 0;
                return false;
            }

            BoundingSphere sphere = new BoundingSphere(Transform.Translation, 7.0f);
            return CollisionsHelper.RayIntersectsSphere(ref ray.Ray, ref sphere, out distance);
        }
    }
}
