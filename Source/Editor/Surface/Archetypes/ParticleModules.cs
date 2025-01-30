// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Particle Modules group.
    /// </summary>
    [HideInEditor]
    public static class ParticleModules
    {
        /// <summary>
        /// The particle emitter module types.
        /// </summary>
        public enum ModuleType
        {
            /// <summary>
            /// The spawn module.
            /// </summary>
            Spawn,

            /// <summary>
            /// The init module.
            /// </summary>
            Initialize,

            /// <summary>
            /// The update module.
            /// </summary>
            Update,

            /// <summary>
            /// The render module.
            /// </summary>
            Render,
        }

        /// <summary>
        /// Customized <see cref="SurfaceNode"/> for particle emitter module node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class ParticleModuleNode : SurfaceNode
        {
            private static readonly Color[] Colors =
            {
                Color.ForestGreen,
                Color.GreenYellow,
                Color.Violet,
                Color.Firebrick,
            };

            private CheckBox _enabled;
            private Rectangle _arrangeButtonRect;
            private bool _arrangeButtonInUse;

            /// <summary>
            /// Gets the particle emitter surface.
            /// </summary>
            public ParticleEmitterSurface ParticleSurface => (ParticleEmitterSurface)Surface;

            /// <summary>
            /// Gets or sets a value indicating whether the module is enabled.
            /// </summary>
            public bool ModuleEnabled
            {
                get => (bool)Values[0];
                set
                {
                    if (value != (bool)Values[0])
                    {
                        SetValue(0, value);
                    }
                }
            }

            /// <summary>
            /// Gets the type of the module.
            /// </summary>
            public ModuleType ModuleType { get; }

            /// <inheritdoc />
            public ParticleModuleNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                _enabled = new CheckBox(0, 0, true, FlaxEditor.Surface.Constants.NodeCloseButtonSize)
                {
                    TooltipText = "Enabled/disables this module",
                    Parent = this,
                };

                ModuleType = (ModuleType)nodeArch.DefaultValues[1];

                Size = CalculateNodeSize(nodeArch.Size.X, nodeArch.Size.Y);
            }

            private void OnEnabledStateChanged(CheckBox control)
            {
                ModuleEnabled = control.State == CheckBoxState.Checked;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;

                // Header
                var idx = (int)ModuleType;
                var headerRect = new Rectangle(0, 0, Width, 16.0f);
                //Render2D.FillRectangle(headerRect, Color.Red);
                Render2D.DrawText(style.FontMedium, Title, headerRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);

                DrawChildren();

                // Border
                Render2D.DrawRectangle(new Rectangle(1, 0, Width - 2, Height - 1), Colors[idx]);

                // Close button
                Render2D.DrawSprite(style.Cross, _closeButtonRect, _closeButtonRect.Contains(_mousePosition) ? style.Foreground : style.ForegroundGrey);

                // Arrange button
                var dragBarColor = _arrangeButtonRect.Contains(_mousePosition) ? style.Foreground : style.ForegroundGrey;
                Render2D.DrawSprite(Editor.Instance.Icons.DragBar12, _arrangeButtonRect, _arrangeButtonInUse ? Color.Orange : dragBarColor);
                if (_arrangeButtonInUse && ArrangeAreaCheck(out _, out var arrangeTargetRect))
                {
                    Render2D.FillRectangle(arrangeTargetRect, style.Selection);
                }

                // Disabled overlay
                if (!ModuleEnabled)
                {
                    Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), new Color(0, 0, 0, 0.4f));
                }
            }

            private bool ArrangeAreaCheck(out int index, out Rectangle rect)
            {
                var barSidesExtend = 20.0f;
                var barHeight = 10.0f;
                var barCheckAreaHeight = 40.0f;

                var pos = PointToParent(ref _mousePosition).Y + barCheckAreaHeight * 0.5f;
                var modules = Surface.Nodes.OfType<ParticleModuleNode>().Where(x => x.ModuleType == ModuleType).ToList();
                for (var i = 0; i < modules.Count; i++)
                {
                    var module = modules[i];
                    if (Mathf.IsInRange(pos, module.Top, module.Top + barCheckAreaHeight) || (i == 0 && pos < module.Top))
                    {
                        index = i;
                        var p1 = module.UpperLeft;
                        rect = new Rectangle(PointFromParent(ref p1) - new Float2(barSidesExtend * 0.5f, barHeight * 0.5f), Width + barSidesExtend, barHeight);
                        return true;
                    }
                }

                var p2 = modules[modules.Count - 1].BottomLeft;
                if (pos > p2.Y)
                {
                    index = modules.Count;
                    rect = new Rectangle(PointFromParent(ref p2) - new Float2(barSidesExtend * 0.5f, barHeight * 0.5f), Width + barSidesExtend, barHeight);
                    return true;
                }

                index = -1;
                rect = Rectangle.Empty;
                return false;
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                base.OnEndMouseCapture();

                _arrangeButtonInUse = false;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _arrangeButtonRect.Contains(ref location))
                {
                    _arrangeButtonInUse = true;
                    Focus();
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _arrangeButtonInUse)
                {
                    _arrangeButtonInUse = false;
                    EndMouseCapture();

                    if (ArrangeAreaCheck(out var index, out _))
                    {
                        var modules = Surface.Nodes.OfType<ParticleModuleNode>().Where(x => x.ModuleType == ModuleType).ToList();

                        foreach (var module in modules)
                        {
                            Surface.Nodes.Remove(module);
                        }

                        int oldIndex = modules.IndexOf(this);
                        modules.RemoveAt(oldIndex);
                        if (index < 0 || index >= modules.Count)
                            modules.Add(this);
                        else
                            modules.Insert(index, this);

                        foreach (var module in modules)
                        {
                            Surface.Nodes.Add(module);
                        }

                        foreach (var module in modules)
                        {
                            module.IndexInParent = int.MaxValue;
                        }

                        ParticleSurface.ArrangeModulesNodes();
                        Surface.MarkAsEdited();
                    }
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                if (_arrangeButtonInUse)
                {
                    // TODO: add support for undo the particle modules reorder
                    _arrangeButtonInUse = false;
                    EndMouseCapture();
                }

                base.OnLostFocus();
            }

            /// <inheritdoc />
            protected sealed override Float2 CalculateNodeSize(float width, float height)
            {
                return new Float2(width + FlaxEditor.Surface.Constants.NodeMarginX * 2, height + FlaxEditor.Surface.Constants.NodeMarginY * 2 + 16.0f);
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                const float headerSize = 16.0f;
                const float closeButtonMargin = FlaxEditor.Surface.Constants.NodeCloseButtonMargin;
                const float closeButtonSize = FlaxEditor.Surface.Constants.NodeCloseButtonSize;
                _headerRect = new Rectangle(0, 0, Width, headerSize);
                _closeButtonRect = new Rectangle(Width - closeButtonSize - closeButtonMargin, closeButtonMargin, closeButtonSize, closeButtonSize);
                _footerRect = Rectangle.Empty;
                _enabled.Location = new Float2(_closeButtonRect.X - _enabled.Width - 2, _closeButtonRect.Y);
                _arrangeButtonRect = new Rectangle(_enabled.X - closeButtonSize - closeButtonMargin, closeButtonMargin, closeButtonSize, closeButtonSize);
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                _enabled.Checked = ModuleEnabled;
                _enabled.StateChanged += OnEnabledStateChanged;

                base.OnSurfaceLoaded(action);

                ParticleSurface?.ArrangeModulesNodes();
            }

            /// <inheritdoc />
            public override void OnSpawned(SurfaceNodeActions action)
            {
                base.OnSpawned(action);

                ParticleSurface.ArrangeModulesNodes();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                _enabled.State = ModuleEnabled ? CheckBoxState.Checked : CheckBoxState.Default;
            }

            /// <inheritdoc />
            public override void OnDeleted(SurfaceNodeActions action)
            {
                ParticleSurface.ArrangeModulesNodes();

                base.OnDeleted(action);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _enabled = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// The particle emitter module that can write to the particle attribute.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.Archetypes.ParticleModules.ParticleModuleNode" />
        public class SetParticleAttributeModuleNode : ParticleModuleNode
        {
            /// <inheritdoc />
            public SetParticleAttributeModuleNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

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
                switch ((Particles.ValueTypes)Values[3])
                {
                case Particles.ValueTypes.Float:
                    type = typeof(float);
                    break;
                case Particles.ValueTypes.Float2:
                    type = typeof(Float2);
                    break;
                case Particles.ValueTypes.Float3:
                    type = typeof(Float3);
                    break;
                case Particles.ValueTypes.Float4:
                    type = typeof(Float4);
                    break;
                case Particles.ValueTypes.Int:
                    type = typeof(int);
                    break;
                case Particles.ValueTypes.Uint:
                    type = typeof(uint);
                    break;
                default: throw new ArgumentOutOfRangeException();
                }
                GetBox(0).CurrentType = new ScriptType(type);
            }
        }

        /// <summary>
        /// The particle emitter module applies the sprite orientation.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.Archetypes.ParticleModules.ParticleModuleNode" />
        public class OrientSpriteNode : ParticleModuleNode
        {
            /// <inheritdoc />
            public OrientSpriteNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateInputBox();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateInputBox();
            }

            private void UpdateInputBox()
            {
                var facingMode = (ParticleSpriteFacingMode)Values[2];
                GetBox(0).IsActive = facingMode == ParticleSpriteFacingMode.CustomFacingVector || facingMode == ParticleSpriteFacingMode.FixedAxis;
            }
        }

        /// <summary>
        /// The particle emitter module that can sort particles.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.Archetypes.ParticleModules.ParticleModuleNode" />
        public class SortModuleNode : ParticleModuleNode
        {
            /// <inheritdoc />
            public SortModuleNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateTextBox();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateTextBox();
            }

            private void UpdateTextBox()
            {
                var mode = (ParticleSortMode)Values[2];
                ((TextBoxView)Elements[2]).Visible = mode == ParticleSortMode.CustomAscending || mode == ParticleSortMode.CustomDescending;
            }
        }

        private static SurfaceNode CreateParticleModuleNode(uint id, VisjectSurfaceContext context, NodeArchetype arch, GroupArchetype groupArch)
        {
            return new ParticleModuleNode(id, context, arch, groupArch);
        }

        private static SurfaceNode CreateSetParticleAttributeModuleNode(uint id, VisjectSurfaceContext context, NodeArchetype arch, GroupArchetype groupArch)
        {
            return new SetParticleAttributeModuleNode(id, context, arch, groupArch);
        }

        private static SurfaceNode CreateOrientSpriteNode(uint id, VisjectSurfaceContext context, NodeArchetype arch, GroupArchetype groupArch)
        {
            return new OrientSpriteNode(id, context, arch, groupArch);
        }

        private static SurfaceNode CreateSortNode(uint id, VisjectSurfaceContext context, NodeArchetype arch, GroupArchetype groupArch)
        {
            return new SortModuleNode(id, context, arch, groupArch);
        }

        /// <summary>
        /// The particle module node elements offset applied to controls to reduce default surface node header thickness.
        /// </summary>
        private const float NodeElementsOffset = 16.0f - Surface.Constants.NodeHeaderSize;

        private const NodeFlags DefaultModuleFlags = NodeFlags.ParticleEmitterGraph | NodeFlags.NoSpawnViaGUI | NodeFlags.NoMove;

        private static NodeArchetype GetParticleAttribute(ModuleType moduleType, ushort typeId, string title, string description, Type type, object defaultValue)
        {
            return new NodeArchetype
            {
                TypeID = typeId,
                Create = CreateParticleModuleNode,
                Title = title,
                Description = description,
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 1 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new[]
                {
                    true,
                    (int)moduleType,
                    defaultValue,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, string.Empty, true, type, 0, 2),
                },
            };
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            // Spawn Modules
            new NodeArchetype
            {
                TypeID = 100,
                Create = CreateParticleModuleNode,
                Title = "Constant Spawn Rate",
                Description = "Emits constant amount of particles per second, depending of the rate property",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Spawn,
                    10.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Rate", true, typeof(float), 0, 2),
                },
            },
            new NodeArchetype
            {
                TypeID = 101,
                Create = CreateParticleModuleNode,
                Title = "Single Burst",
                Description = "Emits a given amount of particles on start",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Spawn,
                    10.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Count", true, typeof(float), 0, 2),
                },
            },
            new NodeArchetype
            {
                TypeID = 102,
                Create = CreateParticleModuleNode,
                Title = "Periodic Burst",
                Description = "Emits particles in periods of time",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, Surface.Constants.LayoutOffsetY * 2),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Spawn,
                    10.0f,
                    2.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Count", true, typeof(float), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Delay", true, typeof(float), 1, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 103,
                Create = CreateParticleModuleNode,
                Title = "Periodic Burst (range)",
                Description = "Emits random amount of particles in random periods of time (customizable ranges)",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, Surface.Constants.LayoutOffsetY * 2),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Spawn,
                    new Float2(5.0f, 10.0f),
                    new Float2(1.0f, 2.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Count (min, max)", true, typeof(Float2), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Delay (min, max)", true, typeof(Float2), 1, 3),
                },
            },

            // Initialize
            new NodeArchetype
            {
                TypeID = 200,
                Create = CreateSetParticleAttributeModuleNode,
                Title = "Set Attribute",
                Description = "Sets the particle attribute value",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    "Color",
                    (int)Particles.ValueTypes.Float4,
                    Color.White,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, -10.0f, 80, 3, typeof(Particles.ValueTypes)),
                    NodeElementArchetype.Factory.TextBox(90, -10.0f, 120, TextBox.DefaultHeight, 2, false),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, string.Empty, true, null, 0, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 201,
                Create = CreateOrientSpriteNode,
                Title = "Orient Sprite",
                Description = "Orientates the sprite particles in the space",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    (int)ParticleSpriteFacingMode.FaceCameraPosition,
                    Float3.Forward,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, -10.0f, 160, 2, typeof(ParticleSpriteFacingMode)),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Custom Vector", true, typeof(Float3), 0, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 202,
                Create = CreateParticleModuleNode,
                Title = "Position (sphere surface)",
                Description = "Places the particles on surface of the sphere",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    1000.0f,
                    360.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Arc", true, typeof(float), 2, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 203,
                Create = CreateParticleModuleNode,
                Title = "Position (plane)",
                Description = "Places the particles on plane",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    new Float2(1000.0f, 1000.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Size", true, typeof(Float2), 1, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 204,
                Create = CreateParticleModuleNode,
                Title = "Position (circle)",
                Description = "Places the particles on arc of the circle",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    1000.0f,
                    360.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Arc", true, typeof(float), 2, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 205,
                Create = CreateParticleModuleNode,
                Title = "Position (disc)",
                Description = "Places the particles on surface of the disc",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    1000.0f,
                    360.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Arc", true, typeof(float), 2, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 206,
                Create = CreateParticleModuleNode,
                Title = "Position (box surface)",
                Description = "Places the particles on top of the axis-aligned box surface",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    new Float3(1000.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Size", true, typeof(Float3), 1, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 207,
                Create = CreateParticleModuleNode,
                Title = "Position (box volume)",
                Description = "Places the particles inside the axis-aligned box volume",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    new Float3(1000.0f),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Size", true, typeof(Float3), 1, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 208,
                Create = CreateParticleModuleNode,
                Title = "Position (cylinder)",
                Description = "Places the particles on the cylinder sides",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 4 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    500.0f,
                    2000.0f,
                    360.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Height", true, typeof(float), 2, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3.0f, "Arc", true, typeof(float), 3, 5),
                },
            },
            new NodeArchetype
            {
                TypeID = 209,
                Create = CreateParticleModuleNode,
                Title = "Position (line)",
                Description = "Places the particles on the line",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    new Float3(-500.0f, 0, 0),
                    new Float3(500.0f, 0, 0),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Start", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "End", true, typeof(Float3), 1, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 210,
                Create = CreateParticleModuleNode,
                Title = "Position (torus)",
                Description = "Places the particles on a surface of the torus",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 4 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    500.0f,
                    200.0f,
                    360.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Thickness", true, typeof(float), 2, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3.0f, "Arc", true, typeof(float), 3, 5),
                },
            },
            new NodeArchetype
            {
                TypeID = 211,
                Create = CreateParticleModuleNode,
                Title = "Position (sphere volume)",
                Description = "Places the particles inside of the sphere",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    1000.0f,
                    360.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Arc", true, typeof(float), 2, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 212,
                Create = CreateParticleModuleNode,
                Title = "Position (depth)",
                Description = "Places the particles on top of the scene objects using depth buffer",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    new Float2(0.0f, 1.0f),
                    10.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "UV", true, typeof(Float2), 0),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Depth Cull Range", true, typeof(Float2), 1, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Depth Offset", true, typeof(float), 2, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 213,
                Create = CreateParticleModuleNode,
                Title = "Orient Model",
                Description = "Orientates the model particles in the space",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 1 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    (int)ParticleModelFacingMode.FaceCameraPosition,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, -10.0f, 160, 2, typeof(ParticleModelFacingMode)),
                },
            },
            new NodeArchetype
            {
                TypeID = 214,
                Create = CreateParticleModuleNode,
                Title = "Position (spiral)",
                Description = "Places the particles on rotating spiral",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Float3.Zero,
                    360.0f, // Rotation Speed
                    200.0f, // Velocity Scale
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Rotation Speed", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Velocity Scale", true, typeof(float), 2, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 215,
                Create = CreateParticleModuleNode,
                Title = "Position (Global SDF)",
                Description = "Places the particles on Global SDF surface (uses current particle position to snap it to SDF)",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 0 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                },
            },
            new NodeArchetype
            {
                TypeID = 216,
                Create = CreateParticleModuleNode,
                Title = "Rotate Position Shape",
                Description = "Rotate the shape.",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 1 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Initialize,
                    Quaternion.Identity,
                },
                Elements = new []
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Rotation", true, typeof(Quaternion), 0, 2),
                }
            },
            GetParticleAttribute(ModuleType.Initialize, 250, "Set Position", "Sets the particle position", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Initialize, 251, "Set Lifetime", "Sets the particle lifetime (in seconds)", typeof(float), 10.0f),
            GetParticleAttribute(ModuleType.Initialize, 252, "Set Age", "Sets the particle age (in seconds)", typeof(float), 0.0f),
            GetParticleAttribute(ModuleType.Initialize, 253, "Set Color", "Sets the particle color (RGBA)", typeof(Float4), Color.White),
            GetParticleAttribute(ModuleType.Initialize, 254, "Set Velocity", "Sets the particle velocity (position delta per second)", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Initialize, 255, "Set Sprite Size", "Sets the particle size (width and height of the sprite)", typeof(Float2), new Float2(10.0f, 10.0f)),
            GetParticleAttribute(ModuleType.Initialize, 256, "Set Mass", "Sets the particle mass (in kilograms)", typeof(float), 1.0f),
            GetParticleAttribute(ModuleType.Initialize, 257, "Set Rotation", "Sets the particle rotation (in XYZ as euler angle in degrees)", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Initialize, 258, "Set Angular Velocity", "Sets the angular particle velocity (rotation delta per second)", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Initialize, 259, "Set Scale", "Sets the particle scale (used by model particles)", typeof(Float3), Float3.One),
            GetParticleAttribute(ModuleType.Initialize, 260, "Set Ribbon Width", "Sets the ribbon width", typeof(float), 20.0f),
            GetParticleAttribute(ModuleType.Initialize, 261, "Set Ribbon Twist", "Sets the ribbon twist angle (in degrees)", typeof(float), 0.0f),
            GetParticleAttribute(ModuleType.Initialize, 262, "Set Ribbon Facing Vector", "Sets the ribbon particles facing vector", typeof(Float3), Float3.Up),
            GetParticleAttribute(ModuleType.Initialize, 263, "Set Radius", "Sets the particle radius", typeof(float), 100.0f),

            // Update Modules
            new NodeArchetype
            {
                TypeID = 300,
                Create = CreateParticleModuleNode,
                Title = "Update Age",
                Description = "Increases particle age every frame, based on delta time",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 0),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                },
            },
            new NodeArchetype
            {
                TypeID = 301,
                Create = CreateParticleModuleNode,
                Title = "Gravity",
                Description = "Applies the gravity force to particle velocity",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    new Float3(0, -981.0f, 0),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Force", true, typeof(Float3), 0, 2),
                },
            },
            new NodeArchetype
            {
                TypeID = 302,
                Create = CreateSetParticleAttributeModuleNode,
                Title = "Set Attribute",
                Description = "Sets the particle attribute value",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    "Color",
                    (int)Particles.ValueTypes.Float4,
                    Color.White,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, -10.0f, 80, 3, typeof(Particles.ValueTypes)),
                    NodeElementArchetype.Factory.TextBox(90, -10.0f, 120, TextBox.DefaultHeight, 2, false),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, string.Empty, true, null, 0, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 303,
                Create = CreateOrientSpriteNode,
                Title = "Orient Sprite",
                Description = "Orientates the sprite particles in the space",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    (int)ParticleSpriteFacingMode.FaceCameraPosition,
                    Float3.Forward,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, -10.0f, 160, 2, typeof(ParticleSpriteFacingMode)),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Custom Vector", true, typeof(Float3), 0, 3),
                },
            },
            new NodeArchetype
            {
                TypeID = 304,
                Create = CreateParticleModuleNode,
                Title = "Force",
                Description = "Applies the force vector to particle velocity",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    new Float3(100.0f, 0, 0),
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Force", true, typeof(Float3), 0, 2),
                },
            },
            new NodeArchetype
            {
                TypeID = 305,
                Create = CreateParticleModuleNode,
                Title = "Conform to Sphere",
                Description = "Applies the force vector to particles to conform around sphere",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 6 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    Float3.Zero,
                    100.0f,
                    5.0f,
                    2000.0f,
                    1.0f,
                    5000.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Sphere Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Sphere Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Attraction Speed", true, typeof(float), 2, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3.0f, "Attraction Force", true, typeof(float), 3, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4.0f, "Stick Distance", true, typeof(float), 4, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5.0f, "Stick Force", true, typeof(float), 5, 7),
                },
            },
            new NodeArchetype
            {
                TypeID = 306,
                Create = CreateParticleModuleNode,
                Title = "Kill (sphere)",
                Description = "Kills particles that enter/leave the sphere",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    Float3.Zero,
                    100.0f,
                    false,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Sphere Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Sphere Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Text(20.0f, (-0.5f + 2.0f) * Surface.Constants.LayoutOffsetY, "Invert"),
                    NodeElementArchetype.Factory.Bool(0, (-0.5f + 2.0f) * Surface.Constants.LayoutOffsetY, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 307,
                Create = CreateParticleModuleNode,
                Title = "Kill (box)",
                Description = "Kills particles that enter/leave the axis-aligned box",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    Float3.Zero,
                    new Float3(100.0f),
                    false,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Box Center", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Box Size", true, typeof(Float3), 1, 3),
                    NodeElementArchetype.Factory.Text(20.0f, (-0.5f + 2.0f) * Surface.Constants.LayoutOffsetY, "Invert"),
                    NodeElementArchetype.Factory.Bool(0, (-0.5f + 2.0f) * Surface.Constants.LayoutOffsetY, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 308,
                Create = CreateParticleModuleNode,
                Title = "Kill (custom)",
                Description = "Kills particles based on a custom boolean rule",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 1 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Kill", true, typeof(bool), 0),
                },
            },
            new NodeArchetype
            {
                TypeID = 309,
                Create = CreateParticleModuleNode,
                Title = "Orient Model",
                Description = "Orientates the model particles in the space",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 1 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    (int)ParticleModelFacingMode.FaceCameraPosition,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(0, -10.0f, 160, 2, typeof(ParticleModelFacingMode)),
                },
            },
            new NodeArchetype
            {
                TypeID = 330,
                Create = CreateParticleModuleNode,
                Title = "Collision (plane)",
                Description = "Collides particles with the plane",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 8 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    false, // Invert
                    0.0f, // Radius
                    0.0f, // Roughness
                    0.1f, // Elasticity
                    0.0f, // Friction
                    0.0f, // Lifetime Loss
                    Float3.Zero, // Plane Position
                    Float3.Up, // Plane Normal
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Text(20.0f, -0.5f * Surface.Constants.LayoutOffsetY, "Invert"),
                    NodeElementArchetype.Factory.Bool(0, -0.5f * Surface.Constants.LayoutOffsetY, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Radius", true, typeof(float), 0, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Roughness", true, typeof(float), 1, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Elasticity", true, typeof(float), 2, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Friction", true, typeof(float), 3, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5, "Lifetime Loss", true, typeof(float), 4, 7),

                    NodeElementArchetype.Factory.Input(-0.5f + 6, "Plane Position", true, typeof(Float3), 5, 8),
                    NodeElementArchetype.Factory.Input(-0.5f + 7, "Plane Normal", true, typeof(Float3), 6, 9),
                },
            },
            new NodeArchetype
            {
                TypeID = 310,
                Create = CreateParticleModuleNode,
                Title = "Linear Drag",
                Description = "Applies the linear drag force to particle velocity",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    0.2f, // Drag
                    false, // Use Sprite Size
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Drag", true, typeof(float), 0, 2),
                    NodeElementArchetype.Factory.Bool(0.0f, 0.5f * Surface.Constants.LayoutOffsetY, 3),
                    NodeElementArchetype.Factory.Text(20.0f, 0.5f * Surface.Constants.LayoutOffsetY, "Use Sprite Size"),
                },
            },
            new NodeArchetype
            {
                TypeID = 311,
                Create = CreateParticleModuleNode,
                Title = "Turbulence",
                Description = "Applies the noise-based turbulence force field to particle velocity",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 7 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    Float3.Zero, // Force Field Position
                    Float3.Zero, // Force Field Rotation
                    Float3.One, // Force Field Scale
                    0.4f, // Roughness
                    10.0f, // Intensity
                    3, // Octaves Count
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f + 0, "Force Field Position", true, typeof(Float3), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Force Field Rotation", true, typeof(Float3), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Force Field Scale", true, typeof(Float3), 2, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Roughness", true, typeof(float), 3, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Intensity", true, typeof(float), 4, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5, "Octaves Count", true, typeof(int), 5, 7),
                },
            },
            new NodeArchetype
            {
                TypeID = 331,
                Create = CreateParticleModuleNode,
                Title = "Collision (sphere)",
                Description = "Collides particles with the sphere",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 8 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    false, // Invert
                    0.0f, // Radius
                    0.0f, // Roughness
                    0.1f, // Elasticity
                    0.0f, // Friction
                    0.0f, // Lifetime Loss
                    Float3.Zero, // Sphere Position
                    100.0f, // Sphere Radius
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Text(20.0f, -0.5f * Surface.Constants.LayoutOffsetY, "Invert"),
                    NodeElementArchetype.Factory.Bool(0, -0.5f * Surface.Constants.LayoutOffsetY, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Radius", true, typeof(float), 0, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Roughness", true, typeof(float), 1, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Elasticity", true, typeof(float), 2, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Friction", true, typeof(float), 3, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5, "Lifetime Loss", true, typeof(float), 4, 7),

                    NodeElementArchetype.Factory.Input(-0.5f + 6, "Sphere Position", true, typeof(Float3), 5, 8),
                    NodeElementArchetype.Factory.Input(-0.5f + 7, "Sphere Radius", true, typeof(float), 6, 9),
                },
            },
            new NodeArchetype
            {
                TypeID = 332,
                Create = CreateParticleModuleNode,
                Title = "Collision (box)",
                Description = "Collides particles with the box",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 8 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    false, // Invert
                    0.0f, // Radius
                    0.0f, // Roughness
                    0.1f, // Elasticity
                    0.0f, // Friction
                    0.0f, // Lifetime Loss
                    Float3.Zero, // Box Position
                    new Float3(100.0f), // Box Size
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Text(20.0f, -0.5f * Surface.Constants.LayoutOffsetY, "Invert"),
                    NodeElementArchetype.Factory.Bool(0, -0.5f * Surface.Constants.LayoutOffsetY, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Radius", true, typeof(float), 0, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Roughness", true, typeof(float), 1, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Elasticity", true, typeof(float), 2, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Friction", true, typeof(float), 3, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5, "Lifetime Loss", true, typeof(float), 4, 7),

                    NodeElementArchetype.Factory.Input(-0.5f + 6, "Box Position", true, typeof(Float3), 5, 8),
                    NodeElementArchetype.Factory.Input(-0.5f + 7, "Box Size", true, typeof(Float3), 6, 9),
                },
            },
            new NodeArchetype
            {
                TypeID = 333,
                Create = CreateParticleModuleNode,
                Title = "Collision (cylinder)",
                Description = "Collides particles with the cylinder",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 9 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    false, // Invert
                    0.0f, // Radius
                    0.0f, // Roughness
                    0.1f, // Elasticity
                    0.0f, // Friction
                    0.0f, // Lifetime Loss
                    Float3.Zero, // Cylinder Position
                    100.0f, // Cylinder Height
                    50.0f, // Cylinder Radius
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Text(20.0f, -0.5f * Surface.Constants.LayoutOffsetY, "Invert"),
                    NodeElementArchetype.Factory.Bool(0, -0.5f * Surface.Constants.LayoutOffsetY, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Radius", true, typeof(float), 0, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Roughness", true, typeof(float), 1, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Elasticity", true, typeof(float), 2, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Friction", true, typeof(float), 3, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5, "Lifetime Loss", true, typeof(float), 4, 7),

                    NodeElementArchetype.Factory.Input(-0.5f + 6, "Cylinder Position", true, typeof(Float3), 5, 8),
                    NodeElementArchetype.Factory.Input(-0.5f + 7, "Cylinder Height", true, typeof(float), 6, 9),
                    NodeElementArchetype.Factory.Input(-0.5f + 8, "Cylinder Radius", true, typeof(float), 7, 10),
                },
            },
            new NodeArchetype
            {
                TypeID = 334,
                Create = CreateParticleModuleNode,
                Title = "Collision (depth)",
                Description = "Collides particles with the scene objects using depth buffer",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 7 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    false, // Invert
                    5.0f, // Radius
                    0.0f, // Roughness
                    0.1f, // Elasticity
                    0.0f, // Friction
                    0.0f, // Lifetime Loss
                    10.0f, // Surface Thickness
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Radius", true, typeof(float), 0, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Roughness", true, typeof(float), 1, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Elasticity", true, typeof(float), 2, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Friction", true, typeof(float), 3, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 5, "Lifetime Loss", true, typeof(float), 4, 7),

                    NodeElementArchetype.Factory.Input(-0.5f + 0, "Surface Thickness", true, typeof(float), 5, 8),
                },
            },
            new NodeArchetype
            {
                TypeID = 335,
                Create = CreateParticleModuleNode,
                Title = "Conform to Global SDF",
                Description = "Applies the force vector to particles to conform around Global SDF",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 4 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    5.0f,
                    2000.0f,
                    1.0f,
                    5000.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f, "Attraction Speed", true, typeof(float), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1.0f, "Attraction Force", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2.0f, "Stick Distance", true, typeof(float), 2, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 3.0f, "Stick Force", true, typeof(float), 3, 5),
                },
            },
            new NodeArchetype
            {
                TypeID = 336,
                Create = CreateParticleModuleNode,
                Title = "Collision (Global SDF)",
                Description = "Collides particles with the scene Global SDF",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 5 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Update,
                    false, // Invert
                    5.0f, // Radius
                    0.4f, // Roughness
                    0.1f, // Elasticity
                    0.0f, // Friction
                    0.0f, // Lifetime Loss
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f + 0, "Radius", true, typeof(float), 0, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Roughness", true, typeof(float), 1, 4),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Elasticity", true, typeof(float), 2, 5),
                    NodeElementArchetype.Factory.Input(-0.5f + 3, "Friction", true, typeof(float), 3, 6),
                    NodeElementArchetype.Factory.Input(-0.5f + 4, "Lifetime Loss", true, typeof(float), 4, 7),
                },
            },
            GetParticleAttribute(ModuleType.Update, 350, "Set Position", "Sets the particle position", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Update, 351, "Set Lifetime", "Sets the particle lifetime (in seconds)", typeof(float), 10.0f),
            GetParticleAttribute(ModuleType.Update, 352, "Set Age", "Sets the particle age (in seconds)", typeof(float), 0.0f),
            GetParticleAttribute(ModuleType.Update, 353, "Set Color", "Sets the particle color (RGBA)", typeof(Float4), Color.White),
            GetParticleAttribute(ModuleType.Update, 354, "Set Velocity", "Sets the particle velocity (position delta per second)", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Update, 355, "Set Sprite Size", "Sets the particle size (width and height of the sprite)", typeof(Float2), new Float2(10.0f, 10.0f)),
            GetParticleAttribute(ModuleType.Update, 356, "Set Mass", "Sets the particle mass (in kilograms)", typeof(float), 1.0f),
            GetParticleAttribute(ModuleType.Update, 357, "Set Rotation", "Sets the particle rotation (in XYZ as euler angle in degrees)", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Update, 358, "Set Angular Velocity", "Sets the angular particle velocity (rotation delta per second)", typeof(Float3), Float3.Zero),
            GetParticleAttribute(ModuleType.Update, 359, "Set Scale", "Sets the particle scale (used by model particles)", typeof(Float3), Float3.One),
            GetParticleAttribute(ModuleType.Update, 360, "Set Ribbon Width", "Sets the ribbon width", typeof(float), 20.0f),
            GetParticleAttribute(ModuleType.Update, 361, "Set Ribbon Twist", "Sets the ribbon twist angle (in degrees)", typeof(float), 0.0f),
            GetParticleAttribute(ModuleType.Update, 362, "Set Ribbon Facing Vector", "Sets the ribbon particles facing vector", typeof(Float3), Float3.Up),
            GetParticleAttribute(ModuleType.Update, 363, "Set Radius", "Sets the particle radius", typeof(float), 100.0f),

            // Render Modules
            new NodeArchetype
            {
                TypeID = 400,
                Create = CreateParticleModuleNode,
                Title = "Sprite Rendering",
                Description = "Draws quad-shaped sprite for every particle",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 100),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Render,
                    Guid.Empty, // Material
                    (int)(DrawPass.Forward | DrawPass.Distortion), // Draw Modes
                },
                Elements = new[]
                {
                    // Material
                    NodeElementArchetype.Factory.Text(0, -10, "Material", 80.0f, 16.0f, "The material used for sprites rendering (quads). It must have Domain set to Particle."),
                    NodeElementArchetype.Factory.Asset(100.0f, -10, 2, typeof(MaterialBase)),

                    // Draw Modes
                    NodeElementArchetype.Factory.Text(0, -10 + 70, "Draw Modes:", 40),
                    NodeElementArchetype.Factory.Enum(100.0f, -10 + 70, 140, 3, typeof(DrawPass)),
                },
            },
            new NodeArchetype
            {
                TypeID = 401,
                Create = CreateParticleModuleNode,
                Title = "Light Rendering",
                Description = "Renders the lights at the particles positions",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 3 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Render,
                    Color.White, // Color
                    1000.0f, // Brightness
                    8.0f, // Fall Off Exponent
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(-0.5f + 0, "Color", true, typeof(Float4), 0, 2),
                    NodeElementArchetype.Factory.Input(-0.5f + 1, "Radius", true, typeof(float), 1, 3),
                    NodeElementArchetype.Factory.Input(-0.5f + 2, "Fall Off Exponent", true, typeof(float), 2, 4),
                },
            },
            new NodeArchetype
            {
                TypeID = 402,
                Create = CreateSortNode,
                Title = "Sort",
                Description = "Sorts the particles for the rendering",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 2 * Surface.Constants.LayoutOffsetY),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Render,
                    (int)ParticleSortMode.ViewDistance, // Sort Mode
                    "Age", // Custom Attribute
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Text(0, -10.0f, "Mode:", 40),
                    NodeElementArchetype.Factory.ComboBox(40, -10.0f, 140, 2, typeof(ParticleSortMode)),
                    NodeElementArchetype.Factory.TextBox(185, -10.0f, 100, TextBox.DefaultHeight, 3, false),
                },
            },
            new NodeArchetype
            {
                TypeID = 403,
                Create = CreateParticleModuleNode,
                Title = "Model Rendering",
                Description = "Draws model for every particle",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 180),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Render,
                    Guid.Empty, // Model
                    Guid.Empty, // Material
                    (int)(DrawPass.Forward | DrawPass.Distortion), // Draw Modes
                },
                Elements = new[]
                {
                    // Model
                    NodeElementArchetype.Factory.Text(0, -10, "Model", 80.0f, 16.0f, "model material used for rendering."),
                    NodeElementArchetype.Factory.Asset(100.0f, -10, 2, typeof(Model)),

                    // Material
                    NodeElementArchetype.Factory.Text(0, -10 + 70, "Material", 80.0f, 16.0f, "The material used for models rendering. It must have Domain set to Particle."),
                    NodeElementArchetype.Factory.Asset(100.0f, -10 + 70, 3, typeof(MaterialBase)),

                    // Draw Modes
                    NodeElementArchetype.Factory.Text(0, -10 + 140, "Draw Modes:", 40),
                    NodeElementArchetype.Factory.Enum(100.0f, -10 + 140, 140, 4, typeof(DrawPass)),
                },
            },
            new NodeArchetype
            {
                TypeID = 404,
                Create = CreateParticleModuleNode,
                Title = "Ribbon Rendering",
                Description = "Draws a ribbon connecting all particles in order by particle age.",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 170),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Render,
                    Guid.Empty, // Material
                    0.0f, // UV Tiling Distance
                    Float2.One, // UV Scale
                    Float2.Zero, // UV Offset
                    (int)(DrawPass.Forward | DrawPass.Distortion), // Draw Modes
                },
                Elements = new[]
                {
                    // Material
                    NodeElementArchetype.Factory.Text(0, -10, "Material", 80.0f, 16.0f, "The material used for ribbons rendering. It must have Domain set to Particle."),
                    NodeElementArchetype.Factory.Asset(80, -10, 2, typeof(MaterialBase)),

                    // UV options
                    NodeElementArchetype.Factory.Text(0.0f, 3.0f * Surface.Constants.LayoutOffsetY, "UV Tiling Distance"),
                    NodeElementArchetype.Factory.Float(100.0f, 3.0f * Surface.Constants.LayoutOffsetY, 3, -1, 0.0f),
                    NodeElementArchetype.Factory.Text(0.0f, 4.0f * Surface.Constants.LayoutOffsetY, "UV Scale"),
                    NodeElementArchetype.Factory.Vector_X(100.0f, 4.0f * Surface.Constants.LayoutOffsetY, 4),
                    NodeElementArchetype.Factory.Vector_Y(155.0f, 4.0f * Surface.Constants.LayoutOffsetY, 4),
                    NodeElementArchetype.Factory.Text(0.0f, 5.0f * Surface.Constants.LayoutOffsetY, "UV Offset"),
                    NodeElementArchetype.Factory.Vector_X(100.0f, 5.0f * Surface.Constants.LayoutOffsetY, 5),
                    NodeElementArchetype.Factory.Vector_Y(155.0f, 5.0f * Surface.Constants.LayoutOffsetY, 5),

                    // Draw Modes
                    NodeElementArchetype.Factory.Text(0, 6.0f * Surface.Constants.LayoutOffsetY, "Draw Modes:", 40),
                    NodeElementArchetype.Factory.Enum(100.0f, 6.0f * Surface.Constants.LayoutOffsetY, 140, 6, typeof(DrawPass)),
                },
            },
            new NodeArchetype
            {
                TypeID = 405,
                Create = CreateParticleModuleNode,
                Title = "Volumetric Fog Rendering",
                Description = "Draws the particle into the volumetric fog (material color, opacity and emission are used for local fog properties).",
                Flags = DefaultModuleFlags,
                Size = new Float2(200, 70),
                DefaultValues = new object[]
                {
                    true,
                    (int)ModuleType.Render,
                    Guid.Empty, // Material
                },
                Elements = new[]
                {
                    // Material
                    NodeElementArchetype.Factory.Text(0, -10, "Material", 80.0f, 16.0f, "The material used for volumetric fog rendering. It must have Domain set to Particle."),
                    NodeElementArchetype.Factory.Asset(80, -10, 2, typeof(MaterialBase)),
                },
            },
        };
    }
}
