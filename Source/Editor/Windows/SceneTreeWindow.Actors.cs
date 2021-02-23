// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.Windows
{
    public partial class SceneTreeWindow
    {
        /// <summary>
        /// The spawnable actors group.
        /// </summary>
        public struct ActorsGroup
        {
            /// <summary>
            /// The group name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The types to spawn (name and type).
            /// </summary>
            public KeyValuePair<string, Type>[] Types;
        }

        /// <summary>
        /// The Spawnable actors (groups with single entry are inlined without a child menu)
        /// </summary>
        public static readonly ActorsGroup[] SpawnActorsGroups =
        {
            new ActorsGroup
            {
                Types = new[] { new KeyValuePair<string, Type>("Actor", typeof(EmptyActor)) }
            },
            new ActorsGroup
            {
                Types = new[] { new KeyValuePair<string, Type>("Model", typeof(StaticModel)) }
            },
            new ActorsGroup
            {
                Types = new[] { new KeyValuePair<string, Type>("Camera", typeof(Camera)) }
            },
            new ActorsGroup
            {
                Name = "Lights",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Directional Light", typeof(DirectionalLight)),
                    new KeyValuePair<string, Type>("Point Light", typeof(PointLight)),
                    new KeyValuePair<string, Type>("Spot Light", typeof(SpotLight)),
                    new KeyValuePair<string, Type>("Sky Light", typeof(SkyLight)),
                }
            },
            new ActorsGroup
            {
                Name = "Visuals",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Environment Probe", typeof(EnvironmentProbe)),
                    new KeyValuePair<string, Type>("Sky", typeof(Sky)),
                    new KeyValuePair<string, Type>("Skybox", typeof(Skybox)),
                    new KeyValuePair<string, Type>("Exponential Height Fog", typeof(ExponentialHeightFog)),
                    new KeyValuePair<string, Type>("PostFx Volume", typeof(PostFxVolume)),
                    new KeyValuePair<string, Type>("Decal", typeof(Decal)),
                    new KeyValuePair<string, Type>("Particle Effect", typeof(ParticleEffect)),
                }
            },
            new ActorsGroup
            {
                Name = "Physics",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Rigid Body", typeof(RigidBody)),
                    new KeyValuePair<string, Type>("Character Controller", typeof(CharacterController)),
                    new KeyValuePair<string, Type>("Box Collider", typeof(BoxCollider)),
                    new KeyValuePair<string, Type>("Sphere Collider", typeof(SphereCollider)),
                    new KeyValuePair<string, Type>("Capsule Collider", typeof(CapsuleCollider)),
                    new KeyValuePair<string, Type>("Mesh Collider", typeof(MeshCollider)),
                    new KeyValuePair<string, Type>("Fixed Joint", typeof(FixedJoint)),
                    new KeyValuePair<string, Type>("Distance Joint", typeof(DistanceJoint)),
                    new KeyValuePair<string, Type>("Slider Joint", typeof(SliderJoint)),
                    new KeyValuePair<string, Type>("Spherical Joint", typeof(SphericalJoint)),
                    new KeyValuePair<string, Type>("Hinge Joint", typeof(HingeJoint)),
                    new KeyValuePair<string, Type>("D6 Joint", typeof(D6Joint)),
                }
            },
            new ActorsGroup
            {
                Name = "Other",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("Animated Model", typeof(AnimatedModel)),
                    new KeyValuePair<string, Type>("Bone Socket", typeof(BoneSocket)),
                    new KeyValuePair<string, Type>("CSG Box Brush", typeof(BoxBrush)),
                    new KeyValuePair<string, Type>("Audio Source", typeof(AudioSource)),
                    new KeyValuePair<string, Type>("Audio Listener", typeof(AudioListener)),
                    new KeyValuePair<string, Type>("Scene Animation", typeof(SceneAnimationPlayer)),
                    new KeyValuePair<string, Type>("Nav Mesh Bounds Volume", typeof(NavMeshBoundsVolume)),
                    new KeyValuePair<string, Type>("Nav Link", typeof(NavLink)),
                    new KeyValuePair<string, Type>("Nav Modifier Volume", typeof(NavModifierVolume)),
                    new KeyValuePair<string, Type>("Spline", typeof(Spline)),
                }
            },
            new ActorsGroup
            {
                Name = "GUI",
                Types = new[]
                {
                    new KeyValuePair<string, Type>("UI Control", typeof(UIControl)),
                    new KeyValuePair<string, Type>("UI Canvas", typeof(UICanvas)),
                    new KeyValuePair<string, Type>("Text Render", typeof(TextRender)),
                    new KeyValuePair<string, Type>("Sprite Render", typeof(SpriteRender)),
                }
            },
        };
    }
}
