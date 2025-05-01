// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Layers group.
    /// </summary>
    [HideInEditor]
    public static class Layers
    {
        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                TypeID = 1,
                Title = "Sample Layer",
                Description = "Sample material or material instance",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 100),
                DefaultValues = new object[]
                {
                    Guid.Empty
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UVs", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Output(0, "", typeof(void), 1),
                    NodeElementArchetype.Factory.Asset(0, Surface.Constants.LayoutOffsetY, 0, typeof(MaterialBase))
                }
            },
            new NodeArchetype
            {
                // [Deprecated]
                TypeID = 2,
                Title = "Linear Layer Blend",
                Description = "Create blended layer using linear math",
                Flags = NodeFlags.MaterialGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste,
                Size = new Float2(200, 60),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Bottom", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Top", true, typeof(void), 1),
                    NodeElementArchetype.Factory.Input(2, "Alpha", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Output(0, "", typeof(void), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "Pack Material Layer",
                Description = "Pack material properties",
                Flags = NodeFlags.MaterialGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste,
                Size = new Float2(200, 240),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "", typeof(void), 0),
                    NodeElementArchetype.Factory.Input(0, "Color", true, typeof(Float3), 1),
                    NodeElementArchetype.Factory.Input(1, "Mask", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Input(2, "Emissive", true, typeof(Float3), 3),
                    NodeElementArchetype.Factory.Input(3, "Metalness", true, typeof(float), 4),
                    NodeElementArchetype.Factory.Input(4, "Specular", true, typeof(float), 5),
                    NodeElementArchetype.Factory.Input(5, "Roughness", true, typeof(float), 6),
                    NodeElementArchetype.Factory.Input(6, "Ambient Occlusion", true, typeof(float), 7),
                    NodeElementArchetype.Factory.Input(7, "Normal", true, typeof(Float3), 8),
                    NodeElementArchetype.Factory.Input(8, "Opacity", true, typeof(float), 9),
                    NodeElementArchetype.Factory.Input(9, "Refraction", true, typeof(float), 10),
                    NodeElementArchetype.Factory.Input(10, "Position Offset", true, typeof(Float3), 11),
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Unpack Material Layer",
                Description = "Unpack material properties",
                Flags = NodeFlags.MaterialGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste,
                Size = new Float2(210, 240),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Output(0, "Color", typeof(Float3), 1),
                    NodeElementArchetype.Factory.Output(1, "Mask", typeof(float), 2),
                    NodeElementArchetype.Factory.Output(2, "Emissive", typeof(Float3), 3),
                    NodeElementArchetype.Factory.Output(3, "Metalness", typeof(float), 4),
                    NodeElementArchetype.Factory.Output(4, "Specular", typeof(float), 5),
                    NodeElementArchetype.Factory.Output(5, "Roughness", typeof(float), 6),
                    NodeElementArchetype.Factory.Output(6, "Ambient Occlusion", typeof(float), 7),
                    NodeElementArchetype.Factory.Output(7, "Normal", typeof(Float3), 8),
                    NodeElementArchetype.Factory.Output(8, "Opacity", typeof(float), 9),
                    NodeElementArchetype.Factory.Output(9, "Refraction", typeof(float), 10),
                    NodeElementArchetype.Factory.Output(10, "Position Offset", typeof(Float3), 11),
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Linear Layer Blend",
                Description = "Create blended layer using linear math",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 60),
                DefaultValues = new object[]
                {
                    0.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Bottom", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Top", true, typeof(void), 1),
                    NodeElementArchetype.Factory.Input(2, "Alpha", true, typeof(float), 2, 0),
                    NodeElementArchetype.Factory.Output(0, "", typeof(void), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Pack Material Layer",
                Description = "Pack material properties",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 280),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "", typeof(void), 0),
                    NodeElementArchetype.Factory.Input(0, "Color", true, typeof(Float3), 1),
                    NodeElementArchetype.Factory.Input(1, "Mask", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Input(2, "Emissive", true, typeof(Float3), 3),
                    NodeElementArchetype.Factory.Input(3, "Metalness", true, typeof(float), 4),
                    NodeElementArchetype.Factory.Input(4, "Specular", true, typeof(float), 5),
                    NodeElementArchetype.Factory.Input(5, "Roughness", true, typeof(float), 6),
                    NodeElementArchetype.Factory.Input(6, "Ambient Occlusion", true, typeof(float), 7),
                    NodeElementArchetype.Factory.Input(7, "Normal", true, typeof(Float3), 8),
                    NodeElementArchetype.Factory.Input(8, "Opacity", true, typeof(float), 9),
                    NodeElementArchetype.Factory.Input(9, "Refraction", true, typeof(float), 10),
                    NodeElementArchetype.Factory.Input(10, "Position Offset", true, typeof(Float3), 11),
                    NodeElementArchetype.Factory.Input(11, "Tessellation Multiplier", true, typeof(float), 12),
                    NodeElementArchetype.Factory.Input(12, "World Displacement", true, typeof(Float3), 13),
                    NodeElementArchetype.Factory.Input(13, "Subsurface Color", true, typeof(Float3), 14),
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Unpack Material Layer",
                Description = "Unpack material properties",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(210, 280),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Output(0, "Color", typeof(Float3), 1),
                    NodeElementArchetype.Factory.Output(1, "Mask", typeof(float), 2),
                    NodeElementArchetype.Factory.Output(2, "Emissive", typeof(Float3), 3),
                    NodeElementArchetype.Factory.Output(3, "Metalness", typeof(float), 4),
                    NodeElementArchetype.Factory.Output(4, "Specular", typeof(float), 5),
                    NodeElementArchetype.Factory.Output(5, "Roughness", typeof(float), 6),
                    NodeElementArchetype.Factory.Output(6, "Ambient Occlusion", typeof(float), 7),
                    NodeElementArchetype.Factory.Output(7, "Normal", typeof(Float3), 8),
                    NodeElementArchetype.Factory.Output(8, "Opacity", typeof(float), 9),
                    NodeElementArchetype.Factory.Output(9, "Refraction", typeof(float), 10),
                    NodeElementArchetype.Factory.Output(10, "Position Offset", typeof(Float3), 11),
                    NodeElementArchetype.Factory.Output(11, "Tessellation Multiplier", typeof(float), 12),
                    NodeElementArchetype.Factory.Output(12, "World Displacement", typeof(Float3), 13),
                    NodeElementArchetype.Factory.Output(13, "Subsurface Color", typeof(Float3), 14),
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Height Layer Blend",
                Description = "Create blended layer using height-based blending",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 100),
                DefaultValues = new object[]
                {
                    0.5f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Bottom", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Bottom Height", true, typeof(float), 4),
                    NodeElementArchetype.Factory.Input(2, "Top", true, typeof(void), 1),
                    NodeElementArchetype.Factory.Input(3, "Top Height", true, typeof(float), 5),
                    NodeElementArchetype.Factory.Input(4, "Alpha", true, typeof(float), 2, 0),
                    NodeElementArchetype.Factory.Output(0, "", typeof(void), 3)
                }
            },
        };
    }
}
