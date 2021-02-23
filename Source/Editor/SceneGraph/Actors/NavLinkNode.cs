// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Actor node for <see cref="NavLink"/>.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class NavLinkNode : ActorNode
    {
        /// <summary>
        /// Sub actor node used to edit link start and end points.
        /// </summary>
        /// <seealso cref="FlaxEditor.SceneGraph.ActorChildNode{T}" />
        public sealed class LinkNode : ActorChildNode<NavLinkNode>
        {
            private readonly bool _isStart;

            /// <summary>
            /// Initializes a new instance of the <see cref="LinkNode"/> class.
            /// </summary>
            /// <param name="actor">The parent node.</param>
            /// <param name="id">The identifier.</param>
            /// <param name="isStart">The start node or end node.</param>
            public LinkNode(NavLinkNode actor, Guid id, bool isStart)
            : base(actor, id, isStart ? 0 : 1)
            {
                _isStart = isStart;
            }

            /// <inheritdoc />
            public override Transform Transform
            {
                get
                {
                    var actor = (NavLink)_node.Actor;
                    Transform localTrans = new Transform(_isStart ? actor.Start : actor.End);
                    return actor.Transform.LocalToWorld(localTrans);
                }
                set
                {
                    var actor = (NavLink)_node.Actor;
                    Transform localTrans = actor.Transform.WorldToLocal(value);
                    if (_isStart)
                        actor.Start = localTrans.Translation;
                    else
                        actor.End = localTrans.Translation;
                }
            }

            /// <inheritdoc />
            public override bool RayCastSelf(ref RayCastData ray, out float distance, out Vector3 normal)
            {
                normal = Vector3.Up;
                var sphere = new BoundingSphere(Transform.Translation, 10.0f);
                return sphere.Intersects(ref ray.Ray, out distance);
            }

            /// <inheritdoc />
            public override void OnDebugDraw(ViewportDebugDrawData data)
            {
                ParentNode.OnDebugDraw(data);
            }
        }

        /// <inheritdoc />
        public NavLinkNode(Actor actor)
        : base(actor)
        {
            var bytes = ID.ToByteArray();
            bytes[0] += 1;
            AddChildNode(new LinkNode(this, new Guid(bytes), true));
            bytes[0] += 1;
            AddChildNode(new LinkNode(this, new Guid(bytes), false));
        }

        /// <inheritdoc />
        public override bool AffectsNavigation => true;

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out float distance, out Vector3 normal)
        {
            normal = Vector3.Up;
            distance = float.MaxValue;
            return false;
        }
    }
}
