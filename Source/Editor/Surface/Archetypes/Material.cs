// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Material group.
    /// </summary>
    [HideInEditor]
    public static class Material
    {
        /// <summary>
        /// Customized <see cref="SurfaceNode"/> for main material node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class SurfaceNodeMaterial : SurfaceNode
        {
            /// <summary>
            /// Material node input boxes (each enum item value maps to box ID).
            /// </summary>
            public enum MaterialNodeBoxes
            {
                /// <summary>
                /// The layer input.
                /// </summary>
                Layer = 0,

                /// <summary>
                /// The color input.
                /// </summary>
                Color = 1,

                /// <summary>
                /// The mask input.
                /// </summary>
                Mask = 2,

                /// <summary>
                /// The emissive input.
                /// </summary>
                Emissive = 3,

                /// <summary>
                /// The metalness input.
                /// </summary>
                Metalness = 4,

                /// <summary>
                /// The specular input.
                /// </summary>
                Specular = 5,

                /// <summary>
                /// The roughness input.
                /// </summary>
                Roughness = 6,

                /// <summary>
                /// The ambient occlusion input.
                /// </summary>
                AmbientOcclusion = 7,

                /// <summary>
                /// The normal input.
                /// </summary>
                Normal = 8,

                /// <summary>
                /// The opacity input.
                /// </summary>
                Opacity = 9,

                /// <summary>
                /// The refraction input.
                /// </summary>
                Refraction = 10,

                /// <summary>
                /// The position offset input.
                /// </summary>
                PositionOffset = 11,

                /// <summary>
                /// The tessellation multiplier input.
                /// </summary>
                TessellationMultiplier = 12,

                /// <summary>
                /// The world displacement input.
                /// </summary>
                WorldDisplacement = 13,

                /// <summary>
                /// The subsurface color input.
                /// </summary>
                SubsurfaceColor = 14,
            };

            /// <inheritdoc />
            public SurfaceNodeMaterial(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <summary>
            /// Gets the material box.
            /// </summary>
            /// <param name="box">The input type.</param>
            /// <returns>The box</returns>
            public Box GetBox(MaterialNodeBoxes box)
            {
                return GetBox((int)box);
            }

            /// <summary>
            /// Update material node boxes
            /// </summary>
            public void UpdateBoxes()
            {
                // Try get parent material window
                // Maybe too hacky :D
                if (!(Surface.Owner is MaterialWindow materialWindow) || materialWindow.Item == null)
                    return;

                // Layered material
                if (GetBox(MaterialNodeBoxes.Layer).HasAnyConnection)
                {
                    GetBox(MaterialNodeBoxes.Color).Enabled = false;
                    GetBox(MaterialNodeBoxes.Mask).Enabled = false;
                    GetBox(MaterialNodeBoxes.Emissive).Enabled = false;
                    GetBox(MaterialNodeBoxes.Metalness).Enabled = false;
                    GetBox(MaterialNodeBoxes.Specular).Enabled = false;
                    GetBox(MaterialNodeBoxes.Roughness).Enabled = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).Enabled = false;
                    GetBox(MaterialNodeBoxes.Normal).Enabled = false;
                    GetBox(MaterialNodeBoxes.Opacity).Enabled = false;
                    GetBox(MaterialNodeBoxes.Refraction).Enabled = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).Enabled = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).Enabled = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).Enabled = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).Enabled = false;
                    return;
                }

                // Get material info
                materialWindow.FillMaterialInfo(out var info);

                // Update boxes
                switch (info.Domain)
                {
                case MaterialDomain.Surface:
                case MaterialDomain.Terrain:
                case MaterialDomain.Particle:
                case MaterialDomain.Deformable:
                {
                    bool isNotUnlit = info.ShadingModel != MaterialShadingModel.Unlit;
                    bool isTransparent = info.BlendMode == MaterialBlendMode.Transparent;
                    bool withTess = info.TessellationMode != TessellationMethod.None;

                    GetBox(MaterialNodeBoxes.Color).Enabled = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Mask).Enabled = true;
                    GetBox(MaterialNodeBoxes.Emissive).Enabled = true;
                    GetBox(MaterialNodeBoxes.Metalness).Enabled = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Specular).Enabled = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Roughness).Enabled = isNotUnlit;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).Enabled = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Normal).Enabled = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Opacity).Enabled = info.ShadingModel == MaterialShadingModel.Subsurface || info.ShadingModel == MaterialShadingModel.Foliage || info.BlendMode != MaterialBlendMode.Opaque;
                    GetBox(MaterialNodeBoxes.Refraction).Enabled = isTransparent;
                    GetBox(MaterialNodeBoxes.PositionOffset).Enabled = true;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).Enabled = withTess;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).Enabled = withTess;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).Enabled = info.ShadingModel == MaterialShadingModel.Subsurface || info.ShadingModel == MaterialShadingModel.Foliage;
                    break;
                }
                case MaterialDomain.PostProcess:
                {
                    GetBox(MaterialNodeBoxes.Color).Enabled = false;
                    GetBox(MaterialNodeBoxes.Mask).Enabled = false;
                    GetBox(MaterialNodeBoxes.Emissive).Enabled = true;
                    GetBox(MaterialNodeBoxes.Metalness).Enabled = false;
                    GetBox(MaterialNodeBoxes.Specular).Enabled = false;
                    GetBox(MaterialNodeBoxes.Roughness).Enabled = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).Enabled = false;
                    GetBox(MaterialNodeBoxes.Normal).Enabled = false;
                    GetBox(MaterialNodeBoxes.Opacity).Enabled = true;
                    GetBox(MaterialNodeBoxes.Refraction).Enabled = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).Enabled = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).Enabled = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).Enabled = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).Enabled = false;
                    break;
                }
                case MaterialDomain.Decal:
                {
                    var mode = info.DecalBlendingMode;

                    GetBox(MaterialNodeBoxes.Color).Enabled = mode == MaterialDecalBlendingMode.Translucent || mode == MaterialDecalBlendingMode.Stain;
                    GetBox(MaterialNodeBoxes.Mask).Enabled = true;
                    GetBox(MaterialNodeBoxes.Emissive).Enabled = mode == MaterialDecalBlendingMode.Translucent || mode == MaterialDecalBlendingMode.Emissive;
                    GetBox(MaterialNodeBoxes.Metalness).Enabled = mode == MaterialDecalBlendingMode.Translucent;
                    GetBox(MaterialNodeBoxes.Specular).Enabled = mode == MaterialDecalBlendingMode.Translucent;
                    GetBox(MaterialNodeBoxes.Roughness).Enabled = mode == MaterialDecalBlendingMode.Translucent;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).Enabled = false;
                    GetBox(MaterialNodeBoxes.Normal).Enabled = mode == MaterialDecalBlendingMode.Translucent || mode == MaterialDecalBlendingMode.Normal;
                    GetBox(MaterialNodeBoxes.Opacity).Enabled = true;
                    GetBox(MaterialNodeBoxes.Refraction).Enabled = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).Enabled = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).Enabled = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).Enabled = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).Enabled = false;
                    break;
                }
                case MaterialDomain.GUI:
                {
                    GetBox(MaterialNodeBoxes.Color).Enabled = false;
                    GetBox(MaterialNodeBoxes.Mask).Enabled = true;
                    GetBox(MaterialNodeBoxes.Emissive).Enabled = true;
                    GetBox(MaterialNodeBoxes.Metalness).Enabled = false;
                    GetBox(MaterialNodeBoxes.Specular).Enabled = false;
                    GetBox(MaterialNodeBoxes.Roughness).Enabled = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).Enabled = false;
                    GetBox(MaterialNodeBoxes.Normal).Enabled = false;
                    GetBox(MaterialNodeBoxes.Opacity).Enabled = true;
                    GetBox(MaterialNodeBoxes.Refraction).Enabled = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).Enabled = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).Enabled = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).Enabled = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).Enabled = false;
                    break;
                }
                case MaterialDomain.VolumeParticle:
                {
                    GetBox(MaterialNodeBoxes.Color).Enabled = true;
                    GetBox(MaterialNodeBoxes.Mask).Enabled = true;
                    GetBox(MaterialNodeBoxes.Emissive).Enabled = true;
                    GetBox(MaterialNodeBoxes.Metalness).Enabled = false;
                    GetBox(MaterialNodeBoxes.Specular).Enabled = false;
                    GetBox(MaterialNodeBoxes.Roughness).Enabled = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).Enabled = false;
                    GetBox(MaterialNodeBoxes.Normal).Enabled = false;
                    GetBox(MaterialNodeBoxes.Opacity).Enabled = true;
                    GetBox(MaterialNodeBoxes.Refraction).Enabled = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).Enabled = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).Enabled = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).Enabled = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).Enabled = false;
                    break;
                }
                    default: throw new ArgumentOutOfRangeException();
                }
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                // Fix emissive box (it's a strange error)
                GetBox(3).CurrentType = new ScriptType(typeof(Vector3));

                UpdateBoxes();
            }

            /// <inheritdoc />
            public override void ConnectionTick(Box box)
            {
                base.ConnectionTick(box);

                UpdateBoxes();
            }
        }

        private sealed class MaterialFunctionNode : Function.FunctionNode
        {
            /// <inheritdoc />
            public MaterialFunctionNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            protected override Asset LoadSignature(Guid id, out string[] types, out string[] names)
            {
                types = null;
                names = null;
                var function = FlaxEngine.Content.Load<MaterialFunction>(id);
                if (function)
                    function.GetSignature(out types, out names);
                return function;
            }
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                TypeID = 1,
                Create = (id, context, arch, groupArch) => new SurfaceNodeMaterial(id, context, arch, groupArch),
                Title = "Material",
                Description = "Main material node",
                Flags = NodeFlags.MaterialGraph | NodeFlags.NoRemove | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste | NodeFlags.NoCloseButton,
                Size = new Vector2(150, 300),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Color", true, typeof(Vector3), 1),
                    NodeElementArchetype.Factory.Input(2, "Mask", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Input(3, "Emissive", true, typeof(Vector3), 3),
                    NodeElementArchetype.Factory.Input(4, "Metalness", true, typeof(float), 4),
                    NodeElementArchetype.Factory.Input(5, "Specular", true, typeof(float), 5),
                    NodeElementArchetype.Factory.Input(6, "Roughness", true, typeof(float), 6),
                    NodeElementArchetype.Factory.Input(7, "Ambient Occlusion", true, typeof(float), 7),
                    NodeElementArchetype.Factory.Input(8, "Normal", true, typeof(Vector3), 8),
                    NodeElementArchetype.Factory.Input(9, "Opacity", true, typeof(float), 9),
                    NodeElementArchetype.Factory.Input(10, "Refraction", true, typeof(float), 10),
                    NodeElementArchetype.Factory.Input(11, "Position Offset", true, typeof(Vector3), 11),
                    NodeElementArchetype.Factory.Input(12, "Tessellation Multiplier", true, typeof(float), 12),
                    NodeElementArchetype.Factory.Input(13, "World Displacement", true, typeof(Vector3), 13),
                    NodeElementArchetype.Factory.Input(14, "Subsurface Color", true, typeof(Vector3), 14),
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Title = "World Position",
                Description = "Absolute world space position",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "View",
                Description = "View properties",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 60),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Position", typeof(Vector3), 0),
                    NodeElementArchetype.Factory.Output(1, "Direction", typeof(Vector3), 1),
                    NodeElementArchetype.Factory.Output(2, "Far Plane", typeof(float), 2),
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Normal Vector",
                Description = "World space normal vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Normal", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Camera Vector",
                Description = "Calculates camera vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Vector", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Screen Position",
                Description = "Gathers screen position or texcoord",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Position", typeof(Vector2), 0),
                    NodeElementArchetype.Factory.Output(1, "Texcoord", typeof(Vector2), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Screen Size",
                Description = "Gathers screen size",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Size", typeof(Vector2), 0),
                    NodeElementArchetype.Factory.Output(1, "Inv Size", typeof(Vector2), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Custom Code",
                Description = "Custom HLSL shader code expression",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(300, 200),
                DefaultValues = new object[]
                {
                    "// Here you can add HLSL code\nOutput0 = Input0;"
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Input0", true, typeof(Vector4), 0),
                    NodeElementArchetype.Factory.Input(1, "Input1", true, typeof(Vector4), 1),
                    NodeElementArchetype.Factory.Input(2, "Input2", true, typeof(Vector4), 2),
                    NodeElementArchetype.Factory.Input(3, "Input3", true, typeof(Vector4), 3),
                    NodeElementArchetype.Factory.Input(4, "Input4", true, typeof(Vector4), 4),
                    NodeElementArchetype.Factory.Input(5, "Input5", true, typeof(Vector4), 5),
                    NodeElementArchetype.Factory.Input(6, "Input6", true, typeof(Vector4), 6),
                    NodeElementArchetype.Factory.Input(7, "Input7", true, typeof(Vector4), 7),

                    NodeElementArchetype.Factory.Output(0, "Output0", typeof(Vector4), 8),
                    NodeElementArchetype.Factory.Output(1, "Output1", typeof(Vector4), 9),
                    NodeElementArchetype.Factory.Output(2, "Output2", typeof(Vector4), 10),
                    NodeElementArchetype.Factory.Output(3, "Output3", typeof(Vector4), 11),

                    NodeElementArchetype.Factory.TextBox(60, 0, 175, 200, 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 9,
                Title = "Object Position",
                Description = "Absolute world space object position",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 10,
                Title = "Two Sided Sign",
                Description = "Scalar value with surface side sign. 1 for normal facing, -1 for inverted",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 11,
                Title = "Camera Depth Fade",
                Description = "Creates a gradient of 0 near the camera to white at fade length. Useful for preventing particles from camera clipping.",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(200, 60),
                DefaultValues = new object[]
                {
                    200.0f,
                    24.0f
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Fade Length", true, typeof(float), 0, 0),
                    NodeElementArchetype.Factory.Input(1, "Fade Offset", true, typeof(float), 1, 1),
                    NodeElementArchetype.Factory.Output(0, "Result", typeof(float), 2),
                }
            },
            new NodeArchetype
            {
                TypeID = 12,
                Title = "Vertex Color",
                Description = "Per vertex color",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Color", typeof(Vector4), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 13,
                Title = "Pre-skinned Local Position",
                Description = "Per vertex local position (before skinning)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(230, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 14,
                Title = "Pre-skinned Local Normal",
                Description = "Per vertex local normal (before skinning)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(230, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 15,
                Title = "Depth",
                Description = "Current pixel/vertex linear distance to the camera",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(100, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 16,
                Title = "Tangent Vector",
                Description = "World space tangent vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Tangent", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 17,
                Title = "Bitangent Vector",
                Description = "World space bitangent vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Bitangent", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 18,
                Title = "Camera Position",
                Description = "World space camera location",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 19,
                Title = "Per Instance Random",
                Description = "Per object instance random value (normalized to range 0-1)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(200, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 20,
                Title = "Interpolate VS To PS",
                Description = "Helper node used to pass data from Vertex Shader to Pixel Shader",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(220, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Vertex Shader", true, typeof(Vector4), 0),
                    NodeElementArchetype.Factory.Output(0, "Pixel Shader", typeof(Vector4), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 21,
                Title = "Terrain Holes Mask",
                Description = "Scalar terrain visibility mask used mostly for creating holes in terrain",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 22,
                Title = "Terrain Layer Weight",
                Description = "Terrain layer weight mask used for blending terrain layers",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(220, 30),
                DefaultValues = new object[]
                {
                    0,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, 0, 70.0f, 0, FlaxEditor.Tools.Terrain.PaintTerrainGizmoMode.TerrainLayerNames),
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 23,
                Title = "Depth Fade",
                Description = "Creates a gradient of 0 near the scene depth geometry. Useful for preventing particles from clipping with geometry (use it for soft particles).",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(200, 40),
                DefaultValues = new object[]
                {
                    10.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Fade Distance", true, typeof(float), 0, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 24,
                Create = (id, context, arch, groupArch) => new MaterialFunctionNode(id, context, arch, groupArch),
                Title = "Material Function",
                Description = "Calls material function",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(220, 120),
                DefaultValues = new object[]
                {
                    Guid.Empty,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Asset(0, 0, 0, typeof(MaterialFunction)),
                }
            },
            new NodeArchetype
            {
                TypeID = 25,
                Title = "Object Size",
                Description = "Absolute world space object size",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 26,
                Title = "Blend Normals",
                Description = "Blend two normal maps to create a single normal map",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(170, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Base Normal", true, typeof(Vector3), 0),
                    NodeElementArchetype.Factory.Input(1, "Additional Normal", true, typeof(Vector3), 1),
                    NodeElementArchetype.Factory.Output(0, "Result", typeof(Vector3), 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 27,
                Title = "Rotator",
                Description = "Rotates UV coordinates according to a scalar angle (0-1)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 55),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Vector2), 0),
                    NodeElementArchetype.Factory.Input(1, "Center", true, typeof(Vector2), 1),
                    NodeElementArchetype.Factory.Input(2, "Rotation Angle", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector2), 3),
                }
            },
            new NodeArchetype
            {
                TypeID = 28,
                Title = "Sphere Mask",
                Description = "Creates a sphere mask",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(150, 100),
                ConnectionsHints = ConnectionsHint.Vector,
                IndependentBoxes = new[]
                {
                    0,
                    1
                },
                DefaultValues = new object[]
                {
                    0.3f,
                    0.5f,
                    false
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "A", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "B", true, null, 1),
                    NodeElementArchetype.Factory.Input(2, "Radius", true, typeof(float), 2, 0),
                    NodeElementArchetype.Factory.Input(3, "Hardness", true, typeof(float), 3, 1),
                    NodeElementArchetype.Factory.Input(4, "Invert", true, typeof(bool), 4, 2),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 5),
                }
            },
            new NodeArchetype
            {
                TypeID = 29,
                Title = "UV Tiling & Offset",
                Description = "Takes UVs and applies tiling and offset",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(175, 60),
                DefaultValues = new object[]
                {
                    Vector2.One,
                    Vector2.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Vector2), 0),
                    NodeElementArchetype.Factory.Input(1, "Tiling", true, typeof(Vector2), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Offset", true, typeof(Vector2), 2, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector2), 3),
                }
            },
            new NodeArchetype
            {
                TypeID = 30,
                Title = "DDX",
                Description = "Returns the partial derivative of the specified value with respect to the screen-space x-coordinate",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(90, 25),
                ConnectionsHints = ConnectionsHint.Numeric,
                IndependentBoxes = new[] { 0 },
                DependentBoxes = new[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 31,
                Title = "DDY",
                Description = "Returns the partial derivative of the specified value with respect to the screen-space y-coordinate",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(90, 25),
                ConnectionsHints = ConnectionsHint.Numeric,
                IndependentBoxes = new[] { 0 },
                DependentBoxes = new[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 32,
                Title = "Sign",
                Description = "Returns -1 if value is less than zero; 0 if value equals zero; and 1 if value is greater than zero",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(90, 25),
                ConnectionsHints = ConnectionsHint.Numeric,
                IndependentBoxes = new[] { 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 33,
                Title = "Any",
                Description = "True if any components of value are non-zero; otherwise, false",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(90, 25),
                ConnectionsHints = ConnectionsHint.Numeric,
                IndependentBoxes = new[] { 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 34,
                Title = "All",
                Description = "Determines if all components of the specified value are non-zero",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(90, 25),
                ConnectionsHints = ConnectionsHint.Numeric,
                IndependentBoxes = new[] { 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 35,
                Title = "Black Body",
                Description = "Simulates black body radiation via a given temperature in kelvin",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(120, 25),
                DefaultValues = new object[]
                {
                    0.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Temp", true, typeof(float), 0, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 36,
                Title = "HSVToRGB",
                Description = "Converts a HSV value to linear RGB [X = 0/360, Y = 0/1, Z = 0/1]",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(160, 25),
                DefaultValues = new object[]
                {
                    new Vector3(240, 1, 1),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "HSV", true, typeof(Vector3), 0, 0),
                    NodeElementArchetype.Factory.Output(0, "RGB", typeof(Vector3), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 37,
                Title = "RGBToHSV",
                Description = "Converts a linear RGB value to HSV [X = 0/360, Y = 0/1, Z = 0/1]",
                Flags = NodeFlags.MaterialGraph,
                Size = new Vector2(160, 25),
                DefaultValues = new object[]
                {
                    new Vector3(0, 1, 0),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "RGB", true, typeof(Vector3), 0, 0),
                    NodeElementArchetype.Factory.Output(0, "HSV", typeof(Vector3), 1),
                }
            }
        };
    }
}
