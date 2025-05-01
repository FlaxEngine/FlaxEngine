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
    /// Scene Graph node type used for the collider shapes.
    /// </summary>
    /// <seealso cref="ActorNode" />
    /// <seealso cref="Collider" />
    [HideInEditor]
    public class ColliderNode : ActorNode
    {
        /// <inheritdoc />
        public ColliderNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool AffectsNavigation => true;

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            // Check if skip raycasts
            if (((ray.Flags & RayCastData.FlagTypes.SkipColliders) == RayCastData.FlagTypes.SkipColliders) ||
                (((ray.Flags & RayCastData.FlagTypes.SkipTriggers) == RayCastData.FlagTypes.SkipTriggers) && ((Collider)Actor).IsTrigger))
            {
                distance = 0;
                normal = Vector3.Up;
                return false;
            }

            return base.RayCastSelf(ref ray, out distance, out normal);
        }
    }
}
