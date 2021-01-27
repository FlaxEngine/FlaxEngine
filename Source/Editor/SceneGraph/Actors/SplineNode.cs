// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.Json;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Spline"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class SplineNode : ActorNode
    {
        private sealed class SplinePointNode : ActorChildNode<SplineNode>
        {
            public unsafe SplinePointNode(SplineNode node, Guid id, int index)
            : base(node, id, index)
            {
            }

            public override bool CanBeSelectedDirectly => true;

            public override Transform Transform
            {
                get
                {
                    var actor = (Spline)_node.Actor;
                    return actor.GetSplineTransform(Index);
                }
                set
                {
                    var actor = (Spline)_node.Actor;
                    actor.SetSplineTransform(Index, value);
                }
            }

            public override bool RayCastSelf(ref RayCastData ray, out float distance, out Vector3 normal)
            {
                var actor = (Spline)_node.Actor;
                var pos = actor.GetSplinePoint(Index);
                normal = -ray.Ray.Direction;
                return new BoundingSphere(pos, 7.0f).Intersects(ref ray.Ray, out distance);
            }

            public override void OnDebugDraw(ViewportDebugDrawData data)
            {
                var actor = (Spline)_node.Actor;
                var pos = actor.GetSplinePoint(Index);

                // Draw spline path
                ParentNode.OnDebugDraw(data);

                // Draw selected point highlight
                DebugDraw.DrawSphere(new BoundingSphere(pos, 5.0f), Color.Yellow, 0, false);
            }
        }

        /// <inheritdoc />
        public SplineNode(Actor actor)
        : base(actor)
        {
            OnUpdate();
            FlaxEngine.Scripting.Update += OnUpdate;
        }

        private unsafe void OnUpdate()
        {
            // Sync spline points with gizmo handles
            var actor = (Spline)Actor;
            var dstCount = actor.SplinePointsCount;
            if (dstCount > 1 && actor.IsLoop)
                dstCount--; // The last point is the same as the first one for loop mode
            var srcCount = ActorChildNodes?.Count ?? 0;
            if (dstCount != srcCount)
            {
                // Remove unused points
                while (srcCount > dstCount)
                    ActorChildNodes[srcCount-- - 1].Dispose();

                // Add new points
                var id = ID;
                var g = (JsonSerializer.GuidInterop*)&id;
                g->D += (uint)srcCount;
                while (srcCount < dstCount)
                {
                    g->D += 1;
                    AddChildNode(new SplinePointNode(this, id, srcCount++));
                }
            }
        }

        /// <inheritdoc />
        public override void OnDispose()
        {
            FlaxEngine.Scripting.Update -= OnUpdate;

            base.OnDispose();
        }
    }
}
