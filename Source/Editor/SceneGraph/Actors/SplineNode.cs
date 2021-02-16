// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Modules;
using FlaxEngine;
using FlaxEngine.Json;
using Object = FlaxEngine.Object;

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
                var g = (JsonSerializer.GuidInterop*)&id;
                g->D++;
                AddChild(new SplinePointTangentNode(node, id, index, true));
                g->D++;
                AddChild(new SplinePointTangentNode(node, id, index, false));
            }

            public override bool CanBeSelectedDirectly => true;

            public override bool CanDuplicate => true;

            public override bool CanDelete => true;

            public override bool CanUseState => true;

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
                    OnSplineEdited(actor);
                }
            }

            struct Data
            {
                public Guid Spline;
                public int Index;
                public BezierCurve<Transform>.Keyframe Keyframe;
            }

            public override StateData State
            {
                get
                {
                    var actor = (Spline)_node.Actor;
                    return new StateData
                    {
                        TypeName = typeof(SplinePointNode).FullName,
                        CreateMethodName = nameof(Create),
                        State = JsonSerializer.Serialize(new Data
                        {
                            Spline = actor.ID,
                            Index = Index,
                            Keyframe = actor.GetSplineKeyframe(Index),
                        }),
                    };
                }
                set => throw new NotImplementedException();
            }

            public override void Delete()
            {
                var actor = (Spline)_node.Actor;
                actor.RemoveSplinePoint(Index);
            }

            class DuplicateUndoAction : IUndoAction, ISceneEditAction
            {
                public Guid SplineId;
                public int Index;
                public float Time;
                public Transform Value;

                public string ActionString => "Duplicate spline point";

                public void Dispose()
                {
                }

                public void Do()
                {
                    var spline = Object.Find<Spline>(ref SplineId);
                    spline.InsertSplineLocalPoint(Index, Time, Value);
                }

                public void Undo()
                {
                    var spline = Object.Find<Spline>(ref SplineId);
                    spline.RemoveSplinePoint(Index);
                }

                public void MarkSceneEdited(SceneModule sceneModule)
                {
                    var spline = Object.Find<Spline>(ref SplineId);
                    sceneModule.MarkSceneEdited(spline.Scene);
                    OnSplineEdited(spline);
                }
            }

            public override SceneGraphNode Duplicate(out IUndoAction undoAction)
            {
                var actor = (Spline)_node.Actor;
                int newIndex;
                float newTime;
                if (Index == actor.SplinePointsCount - 1)
                {
                    // Append to the end
                    newIndex = actor.SplinePointsCount;
                    newTime = actor.GetSplineTime(newIndex - 1) + 1.0f;
                }
                else
                {
                    // Insert between this and next point
                    newIndex = Index + 1;
                    newTime = (actor.GetSplineTime(Index) + actor.GetSplineTime(Index + 1)) * 0.5f;
                }
                var action = new DuplicateUndoAction
                {
                    SplineId = actor.ID,
                    Index = newIndex,
                    Time = newTime,
                    Value = actor.GetSplineLocalTransform(Index),
                };
                actor.InsertSplineLocalPoint(newIndex, newTime, action.Value);
                undoAction = action;
                var splineNode = (SplineNode)SceneGraphFactory.FindNode(action.SplineId);
                splineNode.OnUpdate();
                OnSplineEdited(actor);
                return splineNode.ActorChildNodes[newIndex];
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
                var tangentIn = actor.GetSplineTangent(Index, true).Translation;
                var tangentOut = actor.GetSplineTangent(Index, false).Translation;

                // Draw spline path
                ParentNode.OnDebugDraw(data);

                // Draw selected point highlight
                DebugDraw.DrawSphere(new BoundingSphere(pos, 5.0f), Color.Yellow, 0, false);

                // Draw tangent points
                if (tangentIn != pos)
                {
                    DebugDraw.DrawLine(pos, tangentIn, Color.White.AlphaMultiplied(0.6f), 0, false);
                    DebugDraw.DrawWireSphere(new BoundingSphere(tangentIn, 4.0f), Color.White, 0, false);
                }
                if (tangentIn != pos)
                {
                    DebugDraw.DrawLine(pos, tangentOut, Color.White.AlphaMultiplied(0.6f), 0, false);
                    DebugDraw.DrawWireSphere(new BoundingSphere(tangentOut, 4.0f), Color.White, 0, false);
                }
            }

            public override void OnContextMenu(ContextMenu contextMenu)
            {
                ParentNode.OnContextMenu(contextMenu);
            }

            public static SceneGraphNode Create(StateData state)
            {
                var data = JsonSerializer.Deserialize<Data>(state.State);
                var spline = Object.Find<Spline>(ref data.Spline);
                spline.InsertSplineLocalPoint(data.Index, data.Keyframe.Time, data.Keyframe.Value, false);
                spline.SetSplineKeyframe(data.Index, data.Keyframe);
                var splineNode = (SplineNode)SceneGraphFactory.FindNode(data.Spline);
                if (splineNode == null)
                    return null;
                splineNode.OnUpdate();
                OnSplineEdited(spline);
                return splineNode.ActorChildNodes[data.Index];
            }
        }

        private sealed class SplinePointTangentNode : ActorChildNode
        {
            private SplineNode _node;
            private int _index;
            private bool _isIn;

            public SplinePointTangentNode(SplineNode node, Guid id, int index, bool isIn)
            : base(id, isIn ? 0 : 1)
            {
                _node = node;
                _index = index;
                _isIn = isIn;
            }

            public override Transform Transform
            {
                get
                {
                    var actor = (Spline)_node.Actor;
                    return actor.GetSplineTangent(_index, _isIn);
                }
                set
                {
                    var actor = (Spline)_node.Actor;
                    actor.SetSplineTangent(_index, value, _isIn);
                }
            }

            public override bool RayCastSelf(ref RayCastData ray, out float distance, out Vector3 normal)
            {
                var actor = (Spline)_node.Actor;
                var pos = actor.GetSplineTangent(_index, _isIn).Translation;
                normal = -ray.Ray.Direction;
                return new BoundingSphere(pos, 7.0f).Intersects(ref ray.Ray, out distance);
            }

            public override void OnDebugDraw(ViewportDebugDrawData data)
            {
                // Draw spline and spline point
                ParentNode.OnDebugDraw(data);

                // Draw selected tangent highlight
                var actor = (Spline)_node.Actor;
                var pos = actor.GetSplineTangent(_index, _isIn).Translation;
                DebugDraw.DrawSphere(new BoundingSphere(pos, 5.0f), Color.YellowGreen, 0, false);
            }

            public override void OnContextMenu(ContextMenu contextMenu)
            {
                ParentNode.OnContextMenu(contextMenu);
            }

            public override void OnDispose()
            {
                _node = null;

                base.OnDispose();
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
                g->D += (uint)srcCount * 3;
                while (srcCount < dstCount)
                {
                    g->D += 3;
                    AddChildNode(new SplinePointNode(this, id, srcCount++));
                }
            }
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            // Setup for an initial spline
            var spline = (Spline)Actor;
            spline.AddSplineLocalPoint(Vector3.Zero, false);
            spline.AddSplineLocalPoint(new Vector3(0, 0, 100.0f));
        }

        /// <inheritdoc />
        public override void OnContextMenu(ContextMenu contextMenu)
        {
            base.OnContextMenu(contextMenu);

            contextMenu.AddButton("Add spline model", OnAddSplineModel);
            contextMenu.AddButton("Add spline collider", OnAddSplineCollider);
            contextMenu.AddButton("Add spline rope body", OnAddSplineRopeBody);
        }

        private void OnAddSplineModel()
        {
            var actor = new SplineModel
            {
                StaticFlags = Actor.StaticFlags,
                Transform = Actor.Transform,
            };
            Editor.Instance.SceneEditing.Spawn(actor, Actor);
        }

        private void OnAddSplineCollider()
        {
            var actor = new SplineCollider
            {
                StaticFlags = Actor.StaticFlags,
                Transform = Actor.Transform,
            };
            // TODO: auto pick the collision data if already using spline model
            Editor.Instance.SceneEditing.Spawn(actor, Actor);
        }

        private void OnAddSplineRopeBody()
        {
            var actor = new SplineRopeBody
            {
                StaticFlags = StaticFlags.None,
                Transform = Actor.Transform,
            };
            Editor.Instance.SceneEditing.Spawn(actor, Actor);
        }

        internal static void OnSplineEdited(Spline spline)
        {
            var collider = spline.GetChild<SplineCollider>();
            if (collider && collider.Scene && collider.IsActiveInHierarchy && collider.HasStaticFlag(StaticFlags.Navigation) && !Editor.IsPlayMode)
            {
                var options = Editor.Instance.Options.Options.General;
                if (options.AutoRebuildNavMesh)
                {
                    Navigation.BuildNavMesh(collider.Scene, collider.Box, options.AutoRebuildNavMeshTimeoutMs);
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
