// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Settings;
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
        internal class SurfaceNodeMaterial : SurfaceNode
        {
            /// <summary>
            /// Material node input boxes (each enum item value maps to box ID).
            /// </summary>
            internal enum MaterialNodeBoxes
            {
                Layer = 0,
                Color = 1,
                Mask = 2,
                Emissive = 3,
                Metalness = 4,
                Specular = 5,
                Roughness = 6,
                AmbientOcclusion = 7,
                Normal = 8,
                Opacity = 9,
                Refraction = 10,
                PositionOffset = 11,
                TessellationMultiplier = 12,
                WorldDisplacement = 13,
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
                if (!(Surface?.Owner is MaterialWindow materialWindow) || materialWindow.Item == null)
                    return;

                // Layered material
                if (GetBox(MaterialNodeBoxes.Layer).HasAnyConnection)
                {
                    GetBox(MaterialNodeBoxes.Color).IsActive = false;
                    GetBox(MaterialNodeBoxes.Mask).IsActive = false;
                    GetBox(MaterialNodeBoxes.Emissive).IsActive = false;
                    GetBox(MaterialNodeBoxes.Metalness).IsActive = false;
                    GetBox(MaterialNodeBoxes.Specular).IsActive = false;
                    GetBox(MaterialNodeBoxes.Roughness).IsActive = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).IsActive = false;
                    GetBox(MaterialNodeBoxes.Normal).IsActive = false;
                    GetBox(MaterialNodeBoxes.Opacity).IsActive = false;
                    GetBox(MaterialNodeBoxes.Refraction).IsActive = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).IsActive = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).IsActive = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).IsActive = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).IsActive = false;
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
                    bool withTess = info.TessellationMode != TessellationMethod.None;

                    GetBox(MaterialNodeBoxes.Color).IsActive = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Mask).IsActive = true;
                    GetBox(MaterialNodeBoxes.Emissive).IsActive = true;
                    GetBox(MaterialNodeBoxes.Metalness).IsActive = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Specular).IsActive = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Roughness).IsActive = isNotUnlit;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).IsActive = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Normal).IsActive = isNotUnlit;
                    GetBox(MaterialNodeBoxes.Opacity).IsActive = info.ShadingModel == MaterialShadingModel.Subsurface || info.ShadingModel == MaterialShadingModel.Foliage || info.BlendMode != MaterialBlendMode.Opaque;
                    GetBox(MaterialNodeBoxes.Refraction).IsActive = info.BlendMode != MaterialBlendMode.Opaque;
                    GetBox(MaterialNodeBoxes.PositionOffset).IsActive = true;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).IsActive = withTess;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).IsActive = withTess;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).IsActive = info.ShadingModel == MaterialShadingModel.Subsurface || info.ShadingModel == MaterialShadingModel.Foliage;
                    break;
                }
                case MaterialDomain.PostProcess:
                {
                    GetBox(MaterialNodeBoxes.Color).IsActive = false;
                    GetBox(MaterialNodeBoxes.Mask).IsActive = false;
                    GetBox(MaterialNodeBoxes.Emissive).IsActive = true;
                    GetBox(MaterialNodeBoxes.Metalness).IsActive = false;
                    GetBox(MaterialNodeBoxes.Specular).IsActive = false;
                    GetBox(MaterialNodeBoxes.Roughness).IsActive = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).IsActive = false;
                    GetBox(MaterialNodeBoxes.Normal).IsActive = false;
                    GetBox(MaterialNodeBoxes.Opacity).IsActive = true;
                    GetBox(MaterialNodeBoxes.Refraction).IsActive = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).IsActive = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).IsActive = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).IsActive = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).IsActive = false;
                    break;
                }
                case MaterialDomain.Decal:
                {
                    var mode = info.DecalBlendingMode;

                    GetBox(MaterialNodeBoxes.Color).IsActive = mode == MaterialDecalBlendingMode.Translucent || mode == MaterialDecalBlendingMode.Stain;
                    GetBox(MaterialNodeBoxes.Mask).IsActive = true;
                    GetBox(MaterialNodeBoxes.Emissive).IsActive = mode == MaterialDecalBlendingMode.Translucent || mode == MaterialDecalBlendingMode.Emissive;
                    GetBox(MaterialNodeBoxes.Metalness).IsActive = mode == MaterialDecalBlendingMode.Translucent;
                    GetBox(MaterialNodeBoxes.Specular).IsActive = mode == MaterialDecalBlendingMode.Translucent;
                    GetBox(MaterialNodeBoxes.Roughness).IsActive = mode == MaterialDecalBlendingMode.Translucent;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).IsActive = false;
                    GetBox(MaterialNodeBoxes.Normal).IsActive = mode == MaterialDecalBlendingMode.Translucent || mode == MaterialDecalBlendingMode.Normal;
                    GetBox(MaterialNodeBoxes.Opacity).IsActive = true;
                    GetBox(MaterialNodeBoxes.Refraction).IsActive = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).IsActive = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).IsActive = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).IsActive = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).IsActive = false;
                    break;
                }
                case MaterialDomain.GUI:
                {
                    GetBox(MaterialNodeBoxes.Color).IsActive = false;
                    GetBox(MaterialNodeBoxes.Mask).IsActive = true;
                    GetBox(MaterialNodeBoxes.Emissive).IsActive = true;
                    GetBox(MaterialNodeBoxes.Metalness).IsActive = false;
                    GetBox(MaterialNodeBoxes.Specular).IsActive = false;
                    GetBox(MaterialNodeBoxes.Roughness).IsActive = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).IsActive = false;
                    GetBox(MaterialNodeBoxes.Normal).IsActive = false;
                    GetBox(MaterialNodeBoxes.Opacity).IsActive = true;
                    GetBox(MaterialNodeBoxes.Refraction).IsActive = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).IsActive = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).IsActive = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).IsActive = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).IsActive = false;
                    break;
                }
                case MaterialDomain.VolumeParticle:
                {
                    GetBox(MaterialNodeBoxes.Color).IsActive = true;
                    GetBox(MaterialNodeBoxes.Mask).IsActive = true;
                    GetBox(MaterialNodeBoxes.Emissive).IsActive = true;
                    GetBox(MaterialNodeBoxes.Metalness).IsActive = false;
                    GetBox(MaterialNodeBoxes.Specular).IsActive = false;
                    GetBox(MaterialNodeBoxes.Roughness).IsActive = false;
                    GetBox(MaterialNodeBoxes.AmbientOcclusion).IsActive = false;
                    GetBox(MaterialNodeBoxes.Normal).IsActive = false;
                    GetBox(MaterialNodeBoxes.Opacity).IsActive = true;
                    GetBox(MaterialNodeBoxes.Refraction).IsActive = false;
                    GetBox(MaterialNodeBoxes.PositionOffset).IsActive = false;
                    GetBox(MaterialNodeBoxes.TessellationMultiplier).IsActive = false;
                    GetBox(MaterialNodeBoxes.WorldDisplacement).IsActive = false;
                    GetBox(MaterialNodeBoxes.SubsurfaceColor).IsActive = false;
                    break;
                }
                default: throw new ArgumentOutOfRangeException();
                }
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                // Fix emissive box (it's a strange error)
                GetBox(3).CurrentType = new ScriptType(typeof(Float3));

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

        internal enum MaterialTemplateInputsMapping
        {
            /// <summary>
            /// Constant buffers.
            /// </summary>
            Constants = 1,

            /// <summary>
            /// Shader resources such as textures and buffers.
            /// </summary>
            ShaderResources = 2,

            /// <summary>
            /// Pre-processor definitions.
            /// </summary>
            Defines = 3,

            /// <summary>
            /// Included files.
            /// </summary>
            Includes = 7,

            /// <summary>
            /// Default location after all shader resources and methods but before actual material code.
            /// </summary>
            Utilities = 8,

            /// <summary>
            /// Shader functions location after all material shaders.
            /// </summary>
            Shaders = 9,
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
                Size = new Float2(150, 300),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "", true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Color", true, typeof(Float3), 1),
                    NodeElementArchetype.Factory.Input(2, "Mask", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Input(3, "Emissive", true, typeof(Float3), 3),
                    NodeElementArchetype.Factory.Input(4, "Metalness", true, typeof(float), 4),
                    NodeElementArchetype.Factory.Input(5, "Specular", true, typeof(float), 5),
                    NodeElementArchetype.Factory.Input(6, "Roughness", true, typeof(float), 6),
                    NodeElementArchetype.Factory.Input(7, "Ambient Occlusion", true, typeof(float), 7),
                    NodeElementArchetype.Factory.Input(8, "Normal", true, typeof(Float3), 8),
                    NodeElementArchetype.Factory.Input(9, "Opacity", true, typeof(float), 9),
                    NodeElementArchetype.Factory.Input(10, "Refraction", true, typeof(float), 10),
                    NodeElementArchetype.Factory.Input(11, "Position Offset", true, typeof(Float3), 11),
                    NodeElementArchetype.Factory.Input(12, "Tessellation Multiplier", true, typeof(float), 12),
                    NodeElementArchetype.Factory.Input(13, "World Displacement", true, typeof(Float3), 13),
                    NodeElementArchetype.Factory.Input(14, "Subsurface Color", true, typeof(Float3), 14),
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Title = "World Position",
                Description = "Absolute world space position",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "View",
                Description = "View properties",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 60),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Position", typeof(Float3), 0),
                    NodeElementArchetype.Factory.Output(1, "Direction", typeof(Float3), 1),
                    NodeElementArchetype.Factory.Output(2, "Far Plane", typeof(float), 2),
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Normal Vector",
                Description = "World space normal vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Normal", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Camera Vector",
                Description = "Calculates camera vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Vector", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Screen Position",
                Description = "Gathers screen position or texcoord",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 40),
                DefaultValues = new object[] { false },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Position", typeof(Float2), 0),
                    NodeElementArchetype.Factory.Output(1, "Texcoord", typeof(Float2), 1),
                    NodeElementArchetype.Factory.Bool(0, 0, 0),
                    NodeElementArchetype.Factory.Text(20, 0, "Main View"),
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Screen Size",
                Description = "Gathers screen size",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 40),
                DefaultValues = new object[] { false },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Size", typeof(Float2), 0),
                    NodeElementArchetype.Factory.Output(1, "Inv Size", typeof(Float2), 1),
                    NodeElementArchetype.Factory.Bool(0, 0, 0),
                    NodeElementArchetype.Factory.Text(20, 0, "Main View"),
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Custom Code",
                Description = "Custom HLSL shader code expression",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(300, 200),
                DefaultValues = new object[]
                {
                    "// Here you can add HLSL code\nOutput0 = Input0;"
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Input0", true, typeof(Float4), 0),
                    NodeElementArchetype.Factory.Input(1, "Input1", true, typeof(Float4), 1),
                    NodeElementArchetype.Factory.Input(2, "Input2", true, typeof(Float4), 2),
                    NodeElementArchetype.Factory.Input(3, "Input3", true, typeof(Float4), 3),
                    NodeElementArchetype.Factory.Input(4, "Input4", true, typeof(Float4), 4),
                    NodeElementArchetype.Factory.Input(5, "Input5", true, typeof(Float4), 5),
                    NodeElementArchetype.Factory.Input(6, "Input6", true, typeof(Float4), 6),
                    NodeElementArchetype.Factory.Input(7, "Input7", true, typeof(Float4), 7),

                    NodeElementArchetype.Factory.Output(0, "Output0", typeof(Float4), 8),
                    NodeElementArchetype.Factory.Output(1, "Output1", typeof(Float4), 9),
                    NodeElementArchetype.Factory.Output(2, "Output2", typeof(Float4), 10),
                    NodeElementArchetype.Factory.Output(3, "Output3", typeof(Float4), 11),

                    NodeElementArchetype.Factory.TextBox(60, 0, 175, 200, 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 9,
                Title = "Object Position",
                Description = "Absolute world space object position",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 10,
                Title = "Two Sided Sign",
                Description = "Scalar value with surface side sign. 1 for normal facing, -1 for inverted",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 30),
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
                Size = new Float2(200, 60),
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
                Size = new Float2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Color", typeof(Float4), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 13,
                Title = "Pre-skinned Local Position",
                Description = "Per vertex local position (before skinning)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(230, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 14,
                Title = "Pre-skinned Local Normal",
                Description = "Per vertex local normal (before skinning)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(230, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 15,
                Title = "Depth",
                Description = "Current pixel/vertex linear distance to the camera",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(100, 30),
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
                Size = new Float2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Tangent", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 17,
                Title = "Bitangent Vector",
                Description = "World space bitangent vector",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Bitangent", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 18,
                Title = "Camera Position",
                Description = "World space camera location",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 19,
                Title = "Per Instance Random",
                Description = "Per object instance random value (normalized to range 0-1)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 40),
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
                Size = new Float2(220, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Vertex Shader", true, typeof(Float4), 0),
                    NodeElementArchetype.Factory.Output(0, "Pixel Shader", typeof(Float4), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 21,
                Title = "Terrain Holes Mask",
                Description = "Scalar terrain visibility mask used mostly for creating holes in terrain",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 30),
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
                Size = new Float2(220, 30),
                DefaultValues = new object[]
                {
                    0,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, 0, 70.0f, 0, LayersAndTagsSettings.GetCurrentTerrainLayers()),
                    NodeElementArchetype.Factory.Output(0, "", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 23,
                Title = "Depth Fade",
                Description = "Creates a gradient of 0 near the scene depth geometry. Useful for preventing particles from clipping with geometry (use it for soft particles).",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(200, 40),
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
                Size = new Float2(220, 120),
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
                Size = new Float2(150, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "XYZ", typeof(Float3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 26,
                Title = "Blend Normals",
                Description = "Blend two normal maps to create a single normal map",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(170, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Base Normal", true, typeof(Float3), 0),
                    NodeElementArchetype.Factory.Input(1, "Additional Normal", true, typeof(Float3), 1),
                    NodeElementArchetype.Factory.Output(0, "Result", typeof(Float3), 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 27,
                Title = "Rotator",
                Description = "Rotates UV coordinates according to a scalar angle (in radians, 0-2PI)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 55),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Center", true, typeof(Float2), 1),
                    NodeElementArchetype.Factory.Input(2, "Rotation Angle", true, typeof(float), 2),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float2), 3),
                }
            },
            new NodeArchetype
            {
                TypeID = 28,
                Title = "Sphere Mask",
                Description = "Creates a sphere mask",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 100),
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
                Size = new Float2(175, 60),
                DefaultValues = new object[]
                {
                    Float2.One,
                    Float2.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Tiling", true, typeof(Float2), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Offset", true, typeof(Float2), 2, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float2), 3),
                }
            },
            new NodeArchetype
            {
                TypeID = 30,
                Title = "DDX",
                Description = "Returns the partial derivative of the specified value with respect to the screen-space x-coordinate",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(90, 25),
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
                Size = new Float2(90, 25),
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
                Size = new Float2(90, 25),
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
                Size = new Float2(90, 25),
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
                Size = new Float2(90, 25),
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
                Size = new Float2(120, 25),
                DefaultValues = new object[]
                {
                    0.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Temp", true, typeof(float), 0, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float3), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 36,
                Title = "HSVToRGB",
                Description = "Converts a HSV value to linear RGB [X = 0/360, Y = 0/1, Z = 0/1]",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 25),
                DefaultValues = new object[]
                {
                    new Float3(240, 1, 1),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "HSV", true, typeof(Float3), 0, 0),
                    NodeElementArchetype.Factory.Output(0, "RGB", typeof(Float3), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 37,
                Title = "RGBToHSV",
                Description = "Converts a linear RGB value to HSV [X = 0/360, Y = 0/1, Z = 0/1]",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(160, 25),
                DefaultValues = new object[]
                {
                    new Float3(0, 1, 0),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "RGB", true, typeof(Float3), 0, 0),
                    NodeElementArchetype.Factory.Output(0, "HSV", typeof(Float3), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 38,
                Title = "Custom Global Code",
                Description = "Custom global HLSL shader code expression (placed before material shader code). Can contain includes to shader utilities or declare functions to reuse later.",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(300, 240),
                DefaultValues = new object[]
                {
                    "// Here you can add HLSL code\nfloat4 GetCustomColor()\n{\n\treturn float4(1, 0, 0, 1);\n}",
                    true,
                    (int)MaterialTemplateInputsMapping.Utilities,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Bool(0, 0, 1),
                    NodeElementArchetype.Factory.Text(20, 0, "Enabled"),
                    NodeElementArchetype.Factory.Text(0, 20, "Location"),
                    NodeElementArchetype.Factory.Enum(50, 20, 120, 2, typeof(MaterialTemplateInputsMapping)),
                    NodeElementArchetype.Factory.TextBox(0, 40, 300, 200, 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 39,
                Title = "View Size",
                Description = "The size of the view. The draw rectangle size in GUI materials.",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Size", typeof(Float2), 0),
                    NodeElementArchetype.Factory.Output(1, "Inv Size", typeof(Float2), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 40,
                Title = "Rectangle Mask",
                Description = "Creates a rectangle mask",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues = new object[]
                {
                    new Float2(0.5f, 0.5f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Rectangle", true, typeof(Float2), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 2),
                }
            },
            new NodeArchetype
            {
                TypeID = 41,
                Title = "FWidth",
                Description = "Creates a partial derivative (fwidth)",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 20),
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
                TypeID = 42,
                Title = "AA Step",
                Description = "Smooth version of step function with less aliasing",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(150, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues = new object[]
                {
                    0.5f
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, typeof(float), 0),
                    NodeElementArchetype.Factory.Input(1, "Gradient", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 2),
                }
            },
            new NodeArchetype
            {
                TypeID = 43,
                Title = "Rotate UV",
                Description = "Rotates 2D vector by given angle around (0,0) origin",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(250, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues =
                [
                    0.0f,
                ],
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Angle", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float2), 2),
                ]
            },
            new NodeArchetype
            {
                TypeID = 44,
                Title = "Cone Gradient",
                Description = "Creates cone gradient around normalized UVs (range [-1; 1]), angle is in radians (range [0; TwoPi])",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(175, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues =
                [
                    0.0f,
                ],
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Angle", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 2),
                ]
            },
            new NodeArchetype
            {
                TypeID = 45,
                Title = "Cycle Gradient",
                Description = "Creates 2D sphere mask gradient around normalized UVs (range [-1; 1])",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(175, 20),
                ConnectionsHints = ConnectionsHint.Vector,
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 1),
                ]
            },
            new NodeArchetype
            {
                TypeID = 46,
                Title = "Falloff and Offset",
                Description = "",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(175, 60),
                ConnectionsHints = ConnectionsHint.Numeric,
                DefaultValues =
                [
                    0.0f,
                    0.9f
                ],
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "Value", true, typeof(float), 0),
                    NodeElementArchetype.Factory.Input(1, "Offset", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Falloff", true, typeof(float), 2, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 3),
                ]
            },
            new NodeArchetype
            {
                TypeID = 47,
                Title = "Linear Gradient",
                Description = "x = Gradient along X axis, y = Gradient along Y axis",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(175, 60),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues =
                [
                    0.0f,
                    false
                ],
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Angle", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Mirror", true, typeof(bool), 2, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float2), 3),
                ]
            },
            new NodeArchetype
            {
                TypeID = 48,
                Title = "Radial Gradient",
                Description = "",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(175, 40),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues =
                [
                    0.0f,
                ],
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "Angle", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 2),
                ]
            },
            new NodeArchetype
            {
                TypeID = 49,
                Title = "Ring Gradient",
                Description = "x = InnerMask,y = OuterMask,z = Mask",
                Flags = NodeFlags.MaterialGraph,
                Size = new Float2(175, 80),
                ConnectionsHints = ConnectionsHint.Vector,
                DefaultValues =
                [
                    1.0f,
                    0.8f,
                    0.05f,
                ],
                Elements =
                [
                    NodeElementArchetype.Factory.Input(0, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(1, "OuterBounds", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "InnerBounds", true, typeof(float), 2, 1),
                    NodeElementArchetype.Factory.Input(3, "Falloff", true, typeof(float), 3, 2),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float3), 4),
                ]
            },
        };
    }
}
