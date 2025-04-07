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
    /// Actor node for <see cref="BoxBrush"/>.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class BoxBrushNode : ActorNode
    {
        /// <summary>
        /// Sub actor node used to edit volume.
        /// </summary>
        /// <seealso cref="FlaxEditor.SceneGraph.ActorChildNode{T}" />
        public sealed class SideLinkNode : ActorChildNode<BoxBrushNode>
        {
            private sealed class BrushSurfaceProxy
            {
                [HideInEditor]
                public BoxBrush Brush;

                [HideInEditor]
                public int Index;

                [EditorOrder(10), EditorDisplay("Brush")]
                [Tooltip("The material used to render the brush surface.")]
                public MaterialBase Material
                {
                    get => Brush.Surfaces[Index].Material;
                    set
                    {
                        var surfaces = Brush.Surfaces;
                        surfaces[Index].Material = value;
                        Brush.Surfaces = surfaces;
                    }
                }

                [EditorOrder(30), EditorDisplay("Brush", "UV Scale"), Limit(-1000, 1000, 0.01f)]
                [Tooltip("The surface texture coordinates scale.")]
                public Float2 TexCoordScale
                {
                    get => Brush.Surfaces[Index].TexCoordScale;
                    set
                    {
                        var surfaces = Brush.Surfaces;
                        surfaces[Index].TexCoordScale = value;
                        Brush.Surfaces = surfaces;
                    }
                }

                [EditorOrder(40), EditorDisplay("Brush", "UV Offset"), Limit(-1000, 1000, 0.01f)]
                [Tooltip("The surface texture coordinates offset.")]
                public Float2 TexCoordOffset
                {
                    get => Brush.Surfaces[Index].TexCoordOffset;
                    set
                    {
                        var surfaces = Brush.Surfaces;
                        surfaces[Index].TexCoordOffset = value;
                        Brush.Surfaces = surfaces;
                    }
                }

                [EditorOrder(50), EditorDisplay("Brush", "UV Rotation")]
                [Tooltip("The surface texture coordinates rotation angle (in degrees).")]
                public float TexCoordRotation
                {
                    get => Brush.Surfaces[Index].TexCoordRotation;
                    set
                    {
                        var surfaces = Brush.Surfaces;
                        surfaces[Index].TexCoordRotation = value;
                        Brush.Surfaces = surfaces;
                    }
                }

                [EditorOrder(20), EditorDisplay("Brush", "Scale In Lightmap"), Limit(0, 10000, 0.1f)]
                [Tooltip("The scale in lightmap (per surface).")]
                public float ScaleInLightmap
                {
                    get => Brush.Surfaces[Index].ScaleInLightmap;
                    set
                    {
                        var surfaces = Brush.Surfaces;
                        surfaces[Index].ScaleInLightmap = value;
                        Brush.Surfaces = surfaces;
                    }
                }
            }

            private Vector3 _offset;

            /// <summary>
            /// Gets the brush actor.
            /// </summary>
            public BoxBrush Brush => (BoxBrush)((BoxBrushNode)ParentNode).Actor;

            /// <summary>
            /// Gets the brush surface.
            /// </summary>
            public BrushSurface Surface
            {
                get => Brush.Surfaces[Index];
                set
                {
                    var surfaces = Brush.Surfaces;
                    surfaces[Index] = value;
                    Brush.Surfaces = surfaces;
                }
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="SideLinkNode"/> class.
            /// </summary>
            /// <param name="actor">The parent node.</param>
            /// <param name="id">The identifier.</param>
            /// <param name="index">The index.</param>
            public SideLinkNode(BoxBrushNode actor, Guid id, int index)
            : base(actor, id, index)
            {
                switch (index)
                {
                case 0:
                    _offset = new Vector3(0.5f, 0, 0);
                    break;
                case 1:
                    _offset = new Vector3(-0.5f, 0, 0);
                    break;
                case 2:
                    _offset = new Vector3(0, 0.5f, 0);
                    break;
                case 3:
                    _offset = new Vector3(0, -0.5f, 0);
                    break;
                case 4:
                    _offset = new Vector3(0, 0, 0.5f);
                    break;
                case 5:
                    _offset = new Vector3(0, 0, -0.5f);
                    break;
                }
            }

            /// <inheritdoc />
            public override Transform Transform
            {
                get
                {
                    var actor = Brush;
                    var localOffset = _offset * actor.Size + actor.Center;
                    Transform localTrans = new Transform(localOffset);
                    return actor.Transform.LocalToWorld(localTrans);
                }
                set
                {
                    var actor = Brush;
                    Transform localTrans = actor.Transform.WorldToLocal(value);
                    var prevLocalOffset = _offset * actor.Size + actor.Center;
                    var localOffset = Vector3.Abs(_offset) * 2.0f * localTrans.Translation;
                    var localOffsetDelta = localOffset - prevLocalOffset;
                    float centerScale = Index % 2 == 0 ? 0.5f : -0.5f;
                    actor.Size += localOffsetDelta;
                    actor.Center += localOffsetDelta * centerScale;
                }
            }

            /// <inheritdoc />
            public override object EditableObject => new BrushSurfaceProxy
            {
                Brush = Brush,
                Index = Index,
            };

            /// <inheritdoc />
            public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
            {
                return Brush.Intersects(Index, ref ray.Ray, out distance, out normal);
            }

            /// <inheritdoc />
            public override void OnDebugDraw(ViewportDebugDrawData data)
            {
                ParentNode.OnDebugDraw(data);
                data.HighlightBrushSurface(Brush.Surfaces[Index]);
            }
        }

        /// <inheritdoc />
        public BoxBrushNode(Actor actor)
        : base(actor)
        {
            var id = ID;
            for (int i = 0; i < 6; i++)
                AddChildNode(new SideLinkNode(this, GetSubID(id, i), i));
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            if (((BoxBrush)_actor).OrientedBox.Intersects(ref ray.Ray))
            {
                for (int i = 0; i < ChildNodes.Count; i++)
                {
                    if (ChildNodes[i] is SideLinkNode node && node.RayCastSelf(ref ray, out distance, out normal))
                        return true;
                }
            }

            distance = 0;
            normal = Vector3.Up;
            return false;
        }
    }
}
