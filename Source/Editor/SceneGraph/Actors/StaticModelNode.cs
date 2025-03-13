// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="StaticModel"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class StaticModelNode : ActorNode
    {
        private Dictionary<IntPtr, Float3[]> _vertices;
        private Vector3[] _selectionPoints;
        private Transform _selectionPointsTransform;
        private Model _selectionPointsModel;

        /// <inheritdoc />
        public StaticModelNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnDispose()
        {
            _vertices = null;
            _selectionPoints = null;
            _selectionPointsModel = null;

            base.OnDispose();
        }

        /// <inheritdoc />
        public override bool OnVertexSnap(ref Ray ray, Real hitDistance, out Vector3 result)
        {
            // Find the closest vertex to bounding box point (collision detection approximation)
            result = ray.GetPoint(hitDistance);
            var model = ((StaticModel)Actor).Model;
            if (model && !model.WaitForLoaded())
            {
                // TODO: move to C++ and use cached vertex buffer internally inside the Mesh
                if (_vertices == null)
                    _vertices = new();
                var pointLocal = (Float3)Actor.Transform.WorldToLocal(result);
                var minDistance = Real.MaxValue;
                var lodIndex = 0; // TODO: use LOD index based on the game view
                var lod = model.LODs[lodIndex];
                {
                    var hit = false;
                    foreach (var mesh in lod.Meshes)
                    {
                        var key = FlaxEngine.Object.GetUnmanagedPtr(mesh);
                        if (!_vertices.TryGetValue(key, out var verts))
                        {
                            var accessor = new MeshAccessor();
                            if (accessor.LoadMesh(mesh))
                                continue;
                            verts = accessor.Positions;
                            if (verts == null)
                                continue;
                            _vertices.Add(key, verts);
                        }
                        for (int i = 0; i < verts.Length; i++)
                        {
                            ref var v = ref verts[i];
                            var distance = Float3.DistanceSquared(ref pointLocal, ref v);
                            if (distance <= minDistance)
                            {
                                hit = true;
                                minDistance = distance;
                                result = v;
                            }
                        }
                    }
                    if (hit)
                    {
                        result = Actor.Transform.LocalToWorld(result);
                        return true;
                    }
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnContextMenu(ContextMenu contextMenu, EditorWindow window)
        {
            base.OnContextMenu(contextMenu, window);

            contextMenu.AddButton("Add collider", () => OnAddMeshCollider(window)).Enabled = ((StaticModel)Actor).Model != null;
        }

        /// <inheritdoc />
        public override Vector3[] GetActorSelectionPoints()
        {
            if (Actor is StaticModel sm && sm.Model)
            {
                // Try to use cache
                var model = sm.Model;
                var transform = Actor.Transform;
                if (_selectionPoints != null && 
                    _selectionPointsTransform == transform && 
                    _selectionPointsModel == model)
                    return _selectionPoints;
                Profiler.BeginEvent("GetActorSelectionPoints");

                // Check collision proxy points for more accurate selection
                var vecPoints = new List<Vector3>();
                var m = model.LODs[0];
                foreach (var mesh in m.Meshes)
                {
                    var points = mesh.GetCollisionProxyPoints();
                    vecPoints.EnsureCapacity(vecPoints.Count + points.Length);
                    for (int i = 0; i < points.Length; i++)
                    {
                        vecPoints.Add(transform.LocalToWorld(points[i]));
                    }
                }

                Profiler.EndEvent();
                if (vecPoints.Count != 0)
                {
                    _selectionPoints = vecPoints.ToArray();
                    _selectionPointsTransform = transform;
                    _selectionPointsModel = model;
                    return _selectionPoints;
                }
            }
            return base.GetActorSelectionPoints();
        }

        private void OnAddMeshCollider(EditorWindow window)
        {
            // Allow collider to be added to evey static model selection
            var selection = Array.Empty<SceneGraphNode>();
            if (window is SceneTreeWindow)
                selection = Editor.Instance.SceneEditing.Selection.ToArray();
            else if (window is PrefabWindow prefabWindow)
                selection = prefabWindow.Selection.ToArray();

            var createdNodes = new List<SceneGraphNode>();
            foreach (var node in selection)
            {
                if (node is not StaticModelNode staticModelNode)
                    continue;

                var model = ((StaticModel)staticModelNode.Actor).Model;
                if (!model)
                    continue;

                // Special case for in-built Editor models that can use analytical collision
                var modelPath = model.Path;
                if (modelPath.EndsWith("/Primitives/Cube.flax", StringComparison.Ordinal))
                {
                    var actor = new BoxCollider
                    {
                        StaticFlags = staticModelNode.Actor.StaticFlags,
                        Transform = staticModelNode.Actor.Transform,
                    };
                    staticModelNode.Root.Spawn(actor, staticModelNode.Actor);
                    createdNodes.Add(window is PrefabWindow pWindow ? pWindow.Graph.Root.Find(actor) : Editor.Instance.Scene.GetActorNode(actor));
                    continue;
                }
                if (modelPath.EndsWith("/Primitives/Sphere.flax", StringComparison.Ordinal))
                {
                    var actor = new SphereCollider
                    {
                        StaticFlags = staticModelNode.Actor.StaticFlags,
                        Transform = staticModelNode.Actor.Transform,
                    };
                    staticModelNode.Root.Spawn(actor, staticModelNode.Actor);
                    createdNodes.Add(window is PrefabWindow pWindow ? pWindow.Graph.Root.Find(actor) : Editor.Instance.Scene.GetActorNode(actor));
                    continue;
                }
                if (modelPath.EndsWith("/Primitives/Plane.flax", StringComparison.Ordinal))
                {
                    var actor = new BoxCollider
                    {
                        StaticFlags = staticModelNode.Actor.StaticFlags,
                        Transform = staticModelNode.Actor.Transform,
                        Size = new Float3(100.0f, 100.0f, 1.0f),
                    };
                    staticModelNode.Root.Spawn(actor, staticModelNode.Actor);
                    createdNodes.Add(window is PrefabWindow pWindow ? pWindow.Graph.Root.Find(actor) : Editor.Instance.Scene.GetActorNode(actor));
                    continue;
                }
                if (modelPath.EndsWith("/Primitives/Capsule.flax", StringComparison.Ordinal))
                {
                    var actor = new CapsuleCollider
                    {
                        StaticFlags = staticModelNode.Actor.StaticFlags,
                        Transform = staticModelNode.Actor.Transform,
                        Radius = 25.0f,
                        Height = 50.0f,
                    };
                    Editor.Instance.SceneEditing.Spawn(actor, staticModelNode.Actor);
                    actor.LocalPosition = new Vector3(0, 50.0f, 0);
                    actor.LocalOrientation = Quaternion.Euler(0, 0, 90.0f);
                    createdNodes.Add(window is PrefabWindow pWindow ? pWindow.Graph.Root.Find(actor) : Editor.Instance.Scene.GetActorNode(actor));
                    continue;
                }

                // Create collision data (or reuse) and add collision actor
                Action<CollisionData> created = collisionData =>
                {
                    var actor = new MeshCollider
                    {
                        StaticFlags = staticModelNode.Actor.StaticFlags,
                        Transform = staticModelNode.Actor.Transform,
                        CollisionData = collisionData,
                    };
                    staticModelNode.Root.Spawn(actor, staticModelNode.Actor);
                    createdNodes.Add(window is PrefabWindow pWindow ? pWindow.Graph.Root.Find(actor) : Editor.Instance.Scene.GetActorNode(actor));
                };
                var collisionDataProxy = (CollisionDataProxy)Editor.Instance.ContentDatabase.GetProxy<CollisionData>();
                collisionDataProxy.CreateCollisionDataFromModel(model, created, selection.Length == 1);
            }

            // Select all created nodes
            if (window is SceneTreeWindow)
            {
                Editor.Instance.SceneEditing.Select(createdNodes);
            }
            else if (window is PrefabWindow pWindow)
            {
                pWindow.Select(createdNodes);
            }
        }
    }
}
