// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Particles group.
    /// </summary>
    [HideInEditor]
    public static class Particles
    {
        /// <summary>
        /// The particle value types.
        /// </summary>
        public enum ValueTypes
        {
            /// <summary>
            ///  <see cref="float"/>
            /// </summary>
            Float,

            /// <summary>
            /// <see cref="FlaxEngine.Vector2"/>
            /// </summary>
            Vector2,

            /// <summary>
            /// <see cref="FlaxEngine.Vector3"/>
            /// </summary>
            Vector3,

            /// <summary>
            /// <see cref="FlaxEngine.Vector4"/>
            /// </summary>
            Vector4,

            /// <summary>
            /// <see cref="int"/>
            /// </summary>
            Int,

            /// <summary>
            /// <see cref="uint"/>
            /// </summary>
            Uint,
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode"/> for main particle emitter node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class ParticleEmitterNode : SurfaceNode
        {
            /// <summary>
            /// The particle emitter modules set header (per context).
            /// </summary>
            /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
            public class ModulesHeader : ContainerControl
            {
                private static readonly string[] Names =
                {
                    "Spawn",
                    "Initialize",
                    "Update",
                    "Render",
                };

                /// <summary>
                /// The header height.
                /// </summary>
                public const float HeaderHeight = FlaxEditor.Surface.Constants.NodeHeaderSize;

                /// <summary>
                /// Gets the type of the module.
                /// </summary>
                public ParticleModules.ModuleType ModuleType { get; }

                /// <summary>
                /// The add module button.
                /// </summary>
                public readonly Button AddModuleButton;

                /// <summary>
                /// Initializes a new instance of the <see cref="ModulesHeader"/> class.
                /// </summary>
                /// <param name="parent">The parent emitter node.</param>
                /// <param name="type">The module type.</param>
                public ModulesHeader(ParticleEmitterNode parent, ParticleModules.ModuleType type)
                {
                    ModuleType = type;
                    Parent = parent;
                    AutoFocus = false;

                    float addButtonWidth = 80.0f;
                    float addButtonHeight = 16.0f;
                    AddModuleButton = new Button
                    {
                        Text = "Add Module",
                        TooltipText = "Add new particle modules to the emitter",
                        AnchorPreset = AnchorPresets.BottomCenter,
                        Parent = this,
                        Bounds = new Rectangle((Width - addButtonWidth) * 0.5f, Height - addButtonHeight - 2, addButtonWidth, addButtonHeight),
                    };
                    AddModuleButton.ButtonClicked += OnAddModuleButtonClicked;
                }

                private void OnAddModuleButtonClicked(Button button)
                {
                    var modules = ParticleModules.Nodes.Where(x => (ParticleModules.ModuleType)x.DefaultValues[1] == ModuleType);

                    // Show context menu with list of module types to add
                    var cm = new ItemsListContextMenu(180);
                    foreach (var module in modules)
                    {
                        cm.AddItem(new ItemsListContextMenu.Item(module.Title, module.TypeID)
                        {
                            TooltipText = module.Description,
                        });
                    }
                    cm.ItemClicked += item => AddModule((ushort)item.Tag);
                    cm.SortChildren();
                    cm.Show(this, button.BottomLeft);
                }

                private void AddModule(ushort typeId)
                {
                    var parent = (SurfaceNode)Parent;
                    parent.Surface.Context.SpawnNode(15, typeId, Vector2.Zero);
                }

                /// <inheritdoc />
                public override void Draw()
                {
                    var style = Style.Current;
                    var backgroundRect = new Rectangle(Vector2.Zero, Size);
                    var mousePosition = ((SurfaceNode)Parent).MousePosition;
                    mousePosition = PointFromParent(ref mousePosition);

                    // Background
                    Render2D.FillRectangle(backgroundRect, style.BackgroundNormal);

                    // Header
                    var idx = (int)ModuleType;
                    var headerRect = new Rectangle(0, 0, Width, HeaderHeight);
                    var headerColor = style.BackgroundHighlighted;
                    if (headerRect.Contains(mousePosition))
                        headerColor *= 1.07f;
                    Render2D.FillRectangle(headerRect, headerColor);
                    Render2D.DrawText(style.FontLarge, Names[idx], headerRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);

                    DrawChildren();
                }
            }

            /// <summary>
            /// Gets the particle emitter surface.
            /// </summary>
            public ParticleEmitterSurface ParticleSurface => (ParticleEmitterSurface)Surface;

            /// <summary>
            /// The particle modules sets headers (per context).
            /// </summary>
            public readonly ModulesHeader[] Headers = new ModulesHeader[4];

            /// <inheritdoc />
            public ParticleEmitterNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                Headers[0] = new ModulesHeader(this, ParticleModules.ModuleType.Spawn);
                Headers[1] = new ModulesHeader(this, ParticleModules.ModuleType.Initialize);
                Headers[2] = new ModulesHeader(this, ParticleModules.ModuleType.Update);
                Headers[3] = new ModulesHeader(this, ParticleModules.ModuleType.Render);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;
                var backgroundRect = new Rectangle(Vector2.Zero, Size);

                // Background
                Render2D.FillRectangle(backgroundRect, style.BackgroundNormal);

                // Header
                var headerColor = style.BackgroundHighlighted;
                if (_headerRect.Contains(ref _mousePosition))
                    headerColor *= 1.07f;
                Render2D.FillRectangle(_headerRect, headerColor);
                Render2D.DrawText(style.FontLarge, Title, _headerRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);

                DrawChildren();

                // Options border
                var optionsAreaStart = FlaxEditor.Surface.Constants.NodeHeaderSize + 3.0f;
                var optionsAreaHeight = 7 * FlaxEditor.Surface.Constants.LayoutOffsetY + 6.0f;
                Render2D.DrawRectangle(new Rectangle(1, optionsAreaStart, Width - 2, optionsAreaHeight), style.BackgroundSelected);

                // Selection outline
                if (_isSelected)
                {
                    backgroundRect.Expand(1.5f);
                    var colorTop = Color.Orange;
                    var colorBottom = Color.OrangeRed;
                    Render2D.DrawRectangle(backgroundRect, colorTop, colorTop, colorBottom, colorBottom);
                }
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                // Always keep root node in the back (modules with lay on top of it)
                IndexInParent = 0;

                // Link and update modules
                ParticleSurface._rootNode = this;
                ParticleSurface.ArrangeModulesNodes();
            }

            /// <inheritdoc />
            protected override void OnLocationChanged()
            {
                base.OnLocationChanged();

                if (Surface != null && ParticleSurface._rootNode == this)
                {
                    // Update modules to match root node location
                    ParticleSurface.ArrangeModulesNodes();
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                // Unlink
                ParticleSurface._rootNode = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode"/> for particle data access node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class ParticleAttributeNode : SurfaceNode
        {
            /// <inheritdoc />
            public ParticleAttributeNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                UpdateOutputBoxType();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateOutputBoxType();
            }

            private void UpdateOutputBoxType()
            {
                Type type;
                switch ((ValueTypes)Values[1])
                {
                case ValueTypes.Float:
                    type = typeof(float);
                    break;
                case ValueTypes.Vector2:
                    type = typeof(Vector2);
                    break;
                case ValueTypes.Vector3:
                    type = typeof(Vector3);
                    break;
                case ValueTypes.Vector4:
                    type = typeof(Vector4);
                    break;
                case ValueTypes.Int:
                    type = typeof(int);
                    break;
                case ValueTypes.Uint:
                    type = typeof(uint);
                    break;
                default: throw new ArgumentOutOfRangeException();
                }
                GetBox(0).CurrentType = new ScriptType(type);
            }
        }

        private sealed class ParticleEmitterFunctionNode : Function.FunctionNode
        {
            /// <inheritdoc />
            public ParticleEmitterFunctionNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            protected override Asset LoadSignature(Guid id, out string[] types, out string[] names)
            {
                types = null;
                names = null;
                var function = FlaxEngine.Content.Load<ParticleEmitterFunction>(id);
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
                Create = (id, context, arch, groupArch) => new ParticleEmitterNode(id, context, arch, groupArch),
                Title = "Particle Emitter",
                Description = "Main particle emitter node. Contains a set of modules per emitter context. Modules are executed in order from top to bottom of the stack.",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.NoRemove | NodeFlags.NoSpawnViaGUI | NodeFlags.NoSpawnViaPaste | NodeFlags.NoCloseButton,
                Size = new Vector2(300, 600),
                DefaultValues = new object[]
                {
                    1000, // Capacity
                    (int)ParticlesSimulationMode.Default, // Simulation Mode
                    (int)ParticlesSimulationSpace.Local, // Simulation Space
                    true, // Enable Pooling
                    new BoundingBox(new Vector3(-1000.0f), new Vector3(1000.0f)), // Custom Bounds
                    true, // Use Auto Bounds
                },
                Elements = new[]
                {
                    // Capacity
                    NodeElementArchetype.Factory.Text(0, 0, "Capacity", 100.0f, 16.0f, "The particle system capacity (the maximum amount of particles to simulate at once)."),
                    NodeElementArchetype.Factory.Integer(110, 0, 0, -1, 1),

                    // Simulation Mode
                    NodeElementArchetype.Factory.Text(0, 1 * Surface.Constants.LayoutOffsetY, "Simulation Mode", 100.0f, 16.0f, "The particles simulation execution mode."),
                    NodeElementArchetype.Factory.ComboBox(110, 1 * Surface.Constants.LayoutOffsetY, 80, 1, typeof(ParticlesSimulationMode)),

                    // Simulation Space
                    NodeElementArchetype.Factory.Text(0, 2 * Surface.Constants.LayoutOffsetY, "Simulation Space", 100.0f, 16.0f, "The particles simulation space."),
                    NodeElementArchetype.Factory.ComboBox(110, 2 * Surface.Constants.LayoutOffsetY, 80, 2, typeof(ParticlesSimulationSpace)),

                    // Enable Pooling
                    NodeElementArchetype.Factory.Text(0, 3 * Surface.Constants.LayoutOffsetY, "Enable Pooling", 100.0f, 16.0f, "True if enable pooling emitter instance data, otherwise immediately dispose. Pooling can improve performance and reduce memory usage."),
                    NodeElementArchetype.Factory.Bool(110, 3 * Surface.Constants.LayoutOffsetY, 3),

                    // Use Auto Bounds
                    NodeElementArchetype.Factory.Text(0, 4 * Surface.Constants.LayoutOffsetY, "Use Auto Bounds", 100.0f, 16.0f, "True if enable using automatic bounds (valid only for CPU particles)."),
                    NodeElementArchetype.Factory.Bool(110, 4 * Surface.Constants.LayoutOffsetY, 5),

                    // Custom Bounds
                    NodeElementArchetype.Factory.Text(0, 5 * Surface.Constants.LayoutOffsetY, "Custom Bounds", 100.0f, 16.0f, "The custom bounds to use for the particles. In local-space of the emitter."),
                    NodeElementArchetype.Factory.Box(110, 5 * Surface.Constants.LayoutOffsetY, 4),
                }
            },

            // Particle data access nodes
            new NodeArchetype
            {
                TypeID = 100,
                Create = (id, context, arch, groupArch) => new ParticleAttributeNode(id, context, arch, groupArch),
                Title = "Particle Attribute",
                Description = "Particle attribute data access node. Use it to read the particle data.",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 50),
                DefaultValues = new object[]
                {
                    "Color", // Name
                    (int)ValueTypes.Vector4, // ValueType
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.TextBox(0, 0, 120, TextBox.DefaultHeight, 0, false),
                    NodeElementArchetype.Factory.ComboBox(0, Surface.Constants.LayoutOffsetY, 120, 1, typeof(ValueTypes)),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 303,
                Create = (id, context, arch, groupArch) => new ParticleAttributeNode(id, context, arch, groupArch),
                Title = "Particle Attribute (by index)",
                Description = "Particle attribute data access node. Use it to read the other particle data.",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(260, 60),
                DefaultValues = new object[]
                {
                    "Color", // Name
                    (int)ValueTypes.Vector4, // ValueType
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.TextBox(0, 0, 120, TextBox.DefaultHeight, 0, false),
                    NodeElementArchetype.Factory.ComboBox(0, Surface.Constants.LayoutOffsetY, 120, 1, typeof(ValueTypes)),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 0),
                    NodeElementArchetype.Factory.Input(2, "Index", true, typeof(uint), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 101,
                Title = "Particle Position",
                Description = "Particle position (in simulation space for emitter graph, world space in material graph).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 102,
                Title = "Particle Lifetime",
                Description = "Particle lifetime (in seconds).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 103,
                Title = "Particle Age",
                Description = "Particle age (in seconds).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 104,
                Title = "Particle Color",
                Description = "Particle color (RGBA).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector4), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 105,
                Title = "Particle Velocity",
                Description = "Particle velocity (position delta per second).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 106,
                Title = "Particle Sprite Size",
                Description = "Particle size (width and height of the sprite).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector2), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 1027,
                Title = "Particle Mass",
                Description = "Particle mass (in kilograms).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 108,
                Title = "Particle Rotation",
                Description = "Particle rotation (in XYZ).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 109,
                Title = "Particle Angular Velocity",
                Description = "Particle velocity (rotation delta per second).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(230, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 110,
                Title = "Particle Normalized Age",
                Description = "Particle normalized age to range 0-1 (age divided by lifetime).",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(230, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 111,
                Title = "Particle Radius",
                Description = "Particle radius.",
                Flags = NodeFlags.MaterialGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(200, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },

            // Simulation data access nodes
            new NodeArchetype
            {
                TypeID = 200,
                Title = "Effect Position",
                Description = "Particle effect position (in world space).",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(230, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 201,
                Title = "Effect Rotation",
                Description = "Particle effect rotation (in world space).",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(230, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Quaternion), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 202,
                Title = "Effect Scale",
                Description = "Particle effect scale (in world space).",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(230, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 203,
                Title = "Simulation Mode",
                Description = "Particle emitter simulation execution mode.",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(230, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "CPU", typeof(bool), 0),
                    NodeElementArchetype.Factory.Output(1, "GPU", typeof(bool), 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 204,
                Title = "View Position",
                Description = "World-space camera location (of the main game view)",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 205,
                Title = "View Direction",
                Description = "Camera forward vector (of the main game view)",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 206,
                Title = "View Far Plane",
                Description = "Camera far plane distance (of the main game view)",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 207,
                Title = "Screen Size",
                Description = "Gets the screen size (of the main game view)",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Size", typeof(Vector2), 0),
                    NodeElementArchetype.Factory.Output(1, "Inv Size", typeof(Vector2), 1),
                }
            },

            // Random values generation
            new NodeArchetype
            {
                TypeID = 208,
                Title = "Random Float",
                Description = "Gets the random floating point value (normalized to 0-1 range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(160, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 209,
                Title = "Random Vector2",
                Description = "Gets the random Vector2 value (normalized to 0-1 range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(160, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector2), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 210,
                Title = "Random Vector3",
                Description = "Gets the random Vector3 value (normalized to 0-1 range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(160, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 211,
                Title = "Random Vector4",
                Description = "Gets the random Vector4 value (normalized to 0-1 range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(160, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector4), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 212,
                Title = "Particle Position (world space)",
                Description = "Particle position (in world space).",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(280, 30),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 213,
                Title = "Random Float Range",
                Description = "Gets the random floating point value from a given range",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(260, 40),
                DefaultValues = new object[]
                {
                    0.0f,
                    1.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(float), 0),

                    NodeElementArchetype.Factory.Text(0, 0, "Min", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Float(30, 0, 0),

                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY, "Max", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Float(30, Surface.Constants.LayoutOffsetY, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 214,
                Title = "Random Vector2 Range",
                Description = "Gets the random Vector2 value from a given range (per-component range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(260, 40),
                DefaultValues = new object[]
                {
                    new Vector2(0.0f),
                    new Vector2(1.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector2), 0),

                    NodeElementArchetype.Factory.Text(0, 0, "Min", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Vector_X(30, 0, 0),
                    NodeElementArchetype.Factory.Vector_Y(83, 0, 0),

                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY, "Max", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Vector_X(30, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Vector_Y(83, Surface.Constants.LayoutOffsetY, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 215,
                Title = "Random Vector3 Range",
                Description = "Gets the random Vector3 value from a given range (per-component range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(260, 40),
                DefaultValues = new object[]
                {
                    new Vector3(0.0f),
                    new Vector3(1.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector3), 0),

                    NodeElementArchetype.Factory.Text(0, 0, "Min", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Vector_X(30, 0, 0),
                    NodeElementArchetype.Factory.Vector_Y(83, 0, 0),
                    NodeElementArchetype.Factory.Vector_Z(136, 0, 0),

                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY, "Max", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Vector_X(30, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Vector_Y(83, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Vector_Z(136, Surface.Constants.LayoutOffsetY, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 216,
                Title = "Random Vector4 Range",
                Description = "Gets the random Vector4 value from a given range (per-component range)",
                Flags = NodeFlags.ParticleEmitterGraph | NodeFlags.AnimGraph,
                Size = new Vector2(260, 40),
                DefaultValues = new object[]
                {
                    new Vector4(0.0f),
                    new Vector4(1.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector4), 0),

                    NodeElementArchetype.Factory.Text(0, 0, "Min", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Vector_X(30, 0, 0),
                    NodeElementArchetype.Factory.Vector_Y(83, 0, 0),
                    NodeElementArchetype.Factory.Vector_Z(136, 0, 0),
                    NodeElementArchetype.Factory.Vector_W(189, 0, 0),

                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY, "Max", 30.0f, 18.0f),
                    NodeElementArchetype.Factory.Vector_X(30, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Vector_Y(83, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Vector_Z(136, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Vector_W(189, Surface.Constants.LayoutOffsetY, 1),
                }
            },

            // Utilities
            new NodeArchetype
            {
                TypeID = 300,
                Create = (id, context, arch, groupArch) => new ParticleEmitterFunctionNode(id, context, arch, groupArch),
                Title = "Particle Emitter Function",
                Description = "Calls particle emitter function",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(220, 120),
                DefaultValues = new object[]
                {
                    Guid.Empty,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Asset(0, 0, 0, typeof(ParticleEmitterFunction)),
                }
            },
            new NodeArchetype
            {
                TypeID = 301,
                Title = "Particle Index",
                Description = "Gets the zero-based index of the current particle",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(160, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(uint), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 302,
                Title = "Particles Count",
                Description = "Gets the amount of particles alive in the current emitter",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(160, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(uint), 0),
                }
            },
        };
    }
}
