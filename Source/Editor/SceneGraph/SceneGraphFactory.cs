// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Factory service for Scene Graph nodes creating.
    /// </summary>
    [HideInEditor]
    public static class SceneGraphFactory
    {
        private static readonly object[] _sharedArgsContainer = new object[1];

        /// <summary>
        /// The custom nodes types. Key = object type, Value = custom graph node type
        /// </summary>
        public static readonly Dictionary<Type, Type> CustomNodesTypes = new Dictionary<Type, Type>();

        /// <summary>
        /// The nodes collection where key is ID.
        /// </summary>
        public static readonly Dictionary<Guid, SceneGraphNode> Nodes = new Dictionary<Guid, SceneGraphNode>();

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneGraphFactory"/> class.
        /// </summary>
        static SceneGraphFactory()
        {
            // Init default nodes types
            CustomNodesTypes.Add(typeof(Scene), typeof(SceneNode));
            CustomNodesTypes.Add(typeof(Camera), typeof(CameraNode));
            CustomNodesTypes.Add(typeof(EnvironmentProbe), typeof(EnvironmentProbeNode));
            CustomNodesTypes.Add(typeof(PointLight), typeof(PointLightNode));
            CustomNodesTypes.Add(typeof(DirectionalLight), typeof(DirectionalLightNode));
            CustomNodesTypes.Add(typeof(SpotLight), typeof(SpotLightNode));
            CustomNodesTypes.Add(typeof(Skybox), typeof(SkyboxNode));
            CustomNodesTypes.Add(typeof(Sky), typeof(SkyNode));
            CustomNodesTypes.Add(typeof(ExponentialHeightFog), typeof(ExponentialHeightFogNode));
            CustomNodesTypes.Add(typeof(SkyLight), typeof(SkyLightNode));
            CustomNodesTypes.Add(typeof(PostFxVolume), typeof(PostFxVolumeNode));
            CustomNodesTypes.Add(typeof(StaticModel), typeof(StaticModelNode));
            CustomNodesTypes.Add(typeof(BoxBrush), typeof(BoxBrushNode));
            CustomNodesTypes.Add(typeof(TextRender), typeof(TextRenderNode));
            CustomNodesTypes.Add(typeof(AudioListener), typeof(AudioListenerNode));
            CustomNodesTypes.Add(typeof(AudioSource), typeof(AudioSourceNode));
            CustomNodesTypes.Add(typeof(BoneSocket), typeof(BoneSocketNode));
            CustomNodesTypes.Add(typeof(Decal), typeof(DecalNode));
            CustomNodesTypes.Add(typeof(BoxCollider), typeof(BoxColliderNode));
            CustomNodesTypes.Add(typeof(SphereCollider), typeof(ColliderNode));
            CustomNodesTypes.Add(typeof(CapsuleCollider), typeof(ColliderNode));
            CustomNodesTypes.Add(typeof(MeshCollider), typeof(ColliderNode));
            CustomNodesTypes.Add(typeof(CharacterController), typeof(ColliderNode));
            CustomNodesTypes.Add(typeof(UICanvas), typeof(UICanvasNode));
            CustomNodesTypes.Add(typeof(UIControl), typeof(UIControlNode));
            CustomNodesTypes.Add(typeof(Terrain), typeof(TerrainNode));
            CustomNodesTypes.Add(typeof(Foliage), typeof(FoliageNode));
            CustomNodesTypes.Add(typeof(NavMeshBoundsVolume), typeof(NavMeshBoundsVolumeNode));
            CustomNodesTypes.Add(typeof(BoxVolume), typeof(BoxVolumeNode));
            CustomNodesTypes.Add(typeof(NavLink), typeof(NavLinkNode));
            CustomNodesTypes.Add(typeof(ParticleEffect), typeof(ParticleEffectNode));
            CustomNodesTypes.Add(typeof(SceneAnimationPlayer), typeof(SceneAnimationPlayerNode));
        }

        /// <summary>
        /// Tries to find the node by the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        public static SceneGraphNode FindNode(Guid id)
        {
            if (id == Guid.Empty)
                return null;

            Nodes.TryGetValue(id, out SceneGraphNode result);
            return result;
        }

        /// <summary>
        /// Gets the node for the given object ID or creates a new node for actors (with automatic linkage to the parent).
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>The result node.</returns>
        public static SceneGraphNode GetNode(Guid id)
        {
            Nodes.TryGetValue(id, out SceneGraphNode result);
            if (result == null)
            {
                var actor = Object.TryFind<Actor>(ref id);
                if (actor != null)
                {
                    result = BuildActorNode(actor);
                    if (result != null && actor.HasParent)
                    {
                        result.ParentNode = FindNode(actor.Parent.ID);
                    }
                }
            }
            return result;
        }

        /// <summary>
        /// Builds the scene tree.
        /// </summary>
        /// <param name="scene">The scene.</param>
        /// <returns>The root scene node.</returns>
        public static SceneNode BuildSceneTree(Scene scene)
        {
            // TODO: make it faster by calling engine internally only once to gather optimized scene tree
            return (SceneNode)BuildActorNode(scene);
        }

        /// <summary>
        /// Builds the actor node. Warning! Don't create duplicated nodes.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Created node or null if failed.</returns>
        public static ActorNode BuildActorNode(Actor actor)
        {
            ActorNode result = null;
            Type customType;

            try
            {
                // Try to pick custom node type for that actor object
                if (CustomNodesTypes.TryGetValue(actor.GetType(), out customType))
                {
                    // Use custom type
                    _sharedArgsContainer[0] = actor;
                    result = (ActorNode)Activator.CreateInstance(customType, _sharedArgsContainer);
                }
                else
                {
                    // Use default type for actors
                    result = new ActorNode(actor);
                }

                // Build children
                var childrenCount = actor.ChildrenCount;
                for (int i = 0; i < childrenCount; i++)
                {
                    var child = actor.GetChild(i);
                    if (child == null)
                        continue;

                    var childNode = BuildActorNode(child);
                    if (childNode != null)
                        childNode.ParentNode = result;
                }
            }
            catch (Exception ex)
            {
                // Error
                Editor.LogWarning($"Failed to create scene graph node for actor {actor.Name} (type: {actor.GetType()}).");
                if (CustomNodesTypes.TryGetValue(actor.GetType(), out customType))
                {
                    Editor.LogWarning($"Custom node type: {customType}");
                }
                Editor.LogWarning(ex);
            }

            return result;
        }
    }
}
