// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Windows;
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
        private Dictionary<IntPtr, Mesh.Vertex[]> _vertices;

        /// <inheritdoc />
        public StaticModelNode(Actor actor)
        : base(actor)
        {
        }
        

        /// <inheritdoc />
        public override bool OnVertexSnap(ref Ray ray, float hitDistance, out Vector3 result)
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
                var minDistance = float.MaxValue;
                foreach (var lod in model.LODs) //[ToDo] fix it [Nori_SC note] this is wrong it should get current lod level going it throw all lods will create ghost snaping points
                {
                    var hit = false;
                    foreach (var mesh in lod.Meshes)
                    {
                        var key = FlaxEngine.Object.GetUnmanagedPtr(mesh);
                        if (!_vertices.TryGetValue(key, out var verts))
                        {
                            verts = mesh.DownloadVertexBuffer();
                            if (verts == null)
                                continue;
                            _vertices.Add(key, verts);
                        }
                        for (int i = 0; i < verts.Length; i++)
                        {
                            var v = verts[i].Position;
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

            contextMenu.AddButton("Add collider", OnAddMeshCollider).Enabled = ((StaticModel)Actor).Model != null;
        }

        private void OnAddMeshCollider()
        {
            var model = ((StaticModel)Actor).Model;
            if (!model)
                return;

            // Special case for in-built Editor models that can use analytical collision
            var modelPath = model.Path;
            if (modelPath.EndsWith("/Primitives/Cube.flax", StringComparison.Ordinal))
            {
                var actor = new BoxCollider
                {
                    StaticFlags = Actor.StaticFlags,
                    Transform = Actor.Transform,
                };
                Root.Spawn(actor, Actor);
                return;
            }
            if (modelPath.EndsWith("/Primitives/Sphere.flax", StringComparison.Ordinal))
            {
                var actor = new SphereCollider
                {
                    StaticFlags = Actor.StaticFlags,
                    Transform = Actor.Transform,
                };
                Root.Spawn(actor, Actor);
                return;
            }
            if (modelPath.EndsWith("/Primitives/Plane.flax", StringComparison.Ordinal))
            {
                var actor = new BoxCollider
                {
                    StaticFlags = Actor.StaticFlags,
                    Transform = Actor.Transform,
                    Size = new Float3(100.0f, 100.0f, 1.0f),
                };
                Root.Spawn(actor, Actor);
                return;
            }
            if (modelPath.EndsWith("/Primitives/Capsule.flax", StringComparison.Ordinal))
            {
                var actor = new CapsuleCollider
                {
                    StaticFlags = Actor.StaticFlags,
                    Transform = Actor.Transform,
                    Radius = 25.0f,
                    Height = 50.0f,
                };
                Editor.Instance.SceneEditing.Spawn(actor, Actor);
                actor.LocalPosition = new Vector3(0, 50.0f, 0);
                actor.LocalOrientation = Quaternion.Euler(0, 0, 90.0f);
                return;
            }

            // Create collision data (or reuse) and add collision actor
            Action<CollisionData> created = collisionData =>
            {
                var actor = new MeshCollider
                {
                    StaticFlags = Actor.StaticFlags,
                    Transform = Actor.Transform,
                    CollisionData = collisionData,
                };
                Root.Spawn(actor, Actor);
            };
            var collisionDataProxy = (CollisionDataProxy)Editor.Instance.ContentDatabase.GetProxy<CollisionData>();
            collisionDataProxy.CreateCollisionDataFromModel(model, created);
        }
    }
}
