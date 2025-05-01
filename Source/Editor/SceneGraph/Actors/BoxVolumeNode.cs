// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Actor node for <see cref="BoxVolume"/>.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public class BoxVolumeNode : ActorNode
    {
        private static readonly Int3[] BoxTrianglesIndicesCache =
        {
            new Int3(3, 1, 2),
            new Int3(3, 0, 1),
            new Int3(7, 0, 3),
            new Int3(7, 4, 0),
            new Int3(7, 6, 5),
            new Int3(7, 5, 4),
            new Int3(6, 2, 1),
            new Int3(6, 1, 5),
            new Int3(1, 0, 4),
            new Int3(1, 4, 5),
            new Int3(7, 2, 6),
            new Int3(7, 3, 2),
        };

        /// <summary>
        /// Sub actor node used to edit volume.
        /// </summary>
        /// <seealso cref="FlaxEditor.SceneGraph.ActorChildNode{T}" />
        public sealed class SideLinkNode : ActorChildNode<BoxVolumeNode>
        {
            private readonly Vector3 _offset;

            /// <summary>
            /// Initializes a new instance of the <see cref="SideLinkNode"/> class.
            /// </summary>
            /// <param name="actor">The parent node.</param>
            /// <param name="id">The identifier.</param>
            /// <param name="index">The index.</param>
            public SideLinkNode(BoxVolumeNode actor, Guid id, int index)
            : base(actor, id, index)
            {
                switch (index)
                {
                case 0:
                    // Front
                    _offset = new Vector3(0.5f, 0, 0);
                    break;
                case 1:
                    // Back
                    _offset = new Vector3(-0.5f, 0, 0);
                    break;
                case 2:
                    // Up
                    _offset = new Vector3(0, 0.5f, 0);
                    break;
                case 3:
                    _offset = new Vector3(0, -0.5f, 0);
                    break;
                case 4:
                    // Left
                    _offset = new Vector3(0, 0, 0.5f);
                    break;
                case 5:
                    // Right
                    _offset = new Vector3(0, 0, -0.5f);
                    break;
                }
            }

            /// <inheritdoc />
            public override Transform Transform
            {
                get
                {
                    var actor = (BoxVolume)_node.Actor;
                    var localOffset = _offset * actor.Size;
                    Transform localTrans = new Transform(localOffset);
                    return actor.Transform.LocalToWorld(localTrans);
                }
                set
                {
                    var actor = (BoxVolume)_node.Actor;
                    Transform localTrans = actor.Transform.WorldToLocal(value);
                    var prevLocalOffset = _offset * actor.Size;
                    var localOffset = Vector3.Abs(_offset) * 2.0f * localTrans.Translation;
                    var localOffsetDelta = localOffset - prevLocalOffset;
                    float centerScale = Index % 2 == 0 ? 1.0f : -1.0f;
                    actor.Size += localOffsetDelta * centerScale;
                    actor.Position += localOffsetDelta * 0.5f;
                }
            }

            /// <inheritdoc />
            public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
            {
                normal = Vector3.Up;
                var sphere = new BoundingSphere(Transform.Translation, 10.0f);
                return sphere.Intersects(ref ray.Ray, out distance);
            }

            /// <inheritdoc />
            public override unsafe void OnDebugDraw(ViewportDebugDrawData data)
            {
                // Draw box volume debug shapes
                ParentNode.OnDebugDraw(data);

                // Draw plane of the selected side
                int trangle0, trangle1;
                switch (Index)
                {
                case 0:
                    // Front
                    trangle0 = 8;
                    trangle1 = 9;
                    break;
                case 1:
                    // Back
                    trangle0 = 10;
                    trangle1 = 11;
                    break;
                case 2:
                    // Up
                    trangle0 = 0;
                    trangle1 = 1;
                    break;
                case 3:
                    // Down
                    trangle0 = 4;
                    trangle1 = 5;
                    break;
                case 4:
                    // Left
                    trangle0 = 2;
                    trangle1 = 3;
                    break;
                case 5:
                    // Right
                    trangle0 = 6;
                    trangle1 = 7;
                    break;
                default: return;
                }
                var actor = (BoxVolume)((BoxVolumeNode)ParentNode).Actor;
                var box = actor.OrientedBox;
                var corners = stackalloc Vector3[8];
                box.GetCorners(corners);
                var t0 = BoxTrianglesIndicesCache[trangle0];
                var t1 = BoxTrianglesIndicesCache[trangle1];
                var color = Color.CornflowerBlue.AlphaMultiplied(0.3f);
                DebugDraw.DrawTriangle(corners[t0.X], corners[t0.Y], corners[t0.Z], color);
                DebugDraw.DrawTriangle(corners[t1.X], corners[t1.Y], corners[t1.Z], color);
            }
        }

        /// <inheritdoc />
        public BoxVolumeNode(Actor actor)
        : base(actor)
        {
            var id = ID;
            for (int i = 0; i < 6; i++)
                AddChildNode(new SideLinkNode(this, GetSubID(id, i), i));
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

            // Skip itself if any link gets hit
            if (RayCastChildren(ref ray, out distance, out normal) != null)
                return false;

            // Check itself
            var actor = (BoxVolume)_actor;
            var box = actor.OrientedBox;
            return Utilities.Utils.RayCastWire(ref box, ref ray.Ray, out distance, ref ray.View.Position);
        }
    }
}
