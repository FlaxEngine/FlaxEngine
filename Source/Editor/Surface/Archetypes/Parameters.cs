// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Parameters group.
    /// </summary>
    [HideInEditor]
    public static class Parameters
    {
        /// <summary>
        /// Surface node type for parameters group Get node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class SurfaceNodeParamsGet : SurfaceNode, IParametersDependantNode
        {
            private ComboBoxElement _combobox;
            private readonly List<ISurfaceNodeElement> _dynamicChildren = new List<ISurfaceNodeElement>();
            private bool _isUpdateLocked;
            private ScriptType _layoutType;
            private NodeElementArchetype[] _layoutElements;

            /// <summary>
            /// The prototypes to use for this node to setup elements based on the parameter type.
            /// </summary>
            public Dictionary<Type, NodeElementArchetype[]> Prototypes = DefaultPrototypes;

            /// <summary>
            /// The default prototypes for the node elements to use for the given parameter type.
            /// </summary>
            public static readonly Dictionary<Type, NodeElementArchetype[]> DefaultPrototypes = new Dictionary<Type, NodeElementArchetype[]>
            {
                {
                    typeof(bool),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(bool), 0),
                    }
                },
                {
                    typeof(int),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(int), 0),
                    }
                },
                {
                    typeof(float),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(float), 0),
                    }
                },
                {
                    typeof(double),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(double), 0),
                    }
                },
                {
                    typeof(Float2),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Float2), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(float), 2),
                    }
                },
                {
                    typeof(Float3),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Float3), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(float), 3),
                    }
                },
                {
                    typeof(Float4),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "W", typeof(float), 4),
                    }
                },
                {
                    typeof(Double2),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Double2), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(double), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(double), 2),
                    }
                },
                {
                    typeof(Double3),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Double3), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(double), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(double), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(double), 3),
                    }
                },
                {
                    typeof(Double4),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Double4), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(double), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(double), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(double), 3),
                        NodeElementArchetype.Factory.Output(5, "W", typeof(double), 4),
                    }
                },
                {
                    typeof(Vector2),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Vector2), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(Real), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(Real), 2),
                    }
                },
                {
                    typeof(Vector3),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Vector3), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(Real), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(Real), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(Real), 3),
                    }
                },
                {
                    typeof(Vector4),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Vector4), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(Real), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(Real), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(Real), 3),
                        NodeElementArchetype.Factory.Output(5, "W", typeof(Real), 4),
                    }
                },
                {
                    typeof(Color),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Color", typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(2, "R", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "G", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "B", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "A", typeof(float), 4),
                    }
                },
                {
                    typeof(Texture),
                    new[]
                    {
                        NodeElementArchetype.Factory.Input(1, "UVs", true, typeof(Float2), 0, -1),
                        NodeElementArchetype.Factory.Output(1, "", typeof(FlaxEngine.Object), 6),
                        NodeElementArchetype.Factory.Output(2, "Color", typeof(Float4), 1),
                        NodeElementArchetype.Factory.Output(3, "R", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "G", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "B", typeof(float), 4),
                        NodeElementArchetype.Factory.Output(6, "A", typeof(float), 5),
                    }
                },
                {
                    typeof(CubeTexture),
                    new[]
                    {
                        NodeElementArchetype.Factory.Input(1, "UVs", true, typeof(Float3), 0, -1),
                        NodeElementArchetype.Factory.Output(1, "", typeof(FlaxEngine.Object), 6),
                        NodeElementArchetype.Factory.Output(2, "Color", typeof(Float4), 1),
                        NodeElementArchetype.Factory.Output(3, "R", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "G", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "B", typeof(float), 4),
                        NodeElementArchetype.Factory.Output(6, "A", typeof(float), 5),
                    }
                },
                {
                    typeof(GPUTexture),
                    new[]
                    {
                        NodeElementArchetype.Factory.Input(1, "UVs", true, typeof(Float2), 0, -1),
                        NodeElementArchetype.Factory.Output(1, "", typeof(FlaxEngine.Object), 6),
                        NodeElementArchetype.Factory.Output(2, "Color", typeof(Float4), 1),
                        NodeElementArchetype.Factory.Output(3, "R", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "G", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "B", typeof(float), 4),
                        NodeElementArchetype.Factory.Output(6, "A", typeof(float), 5),
                    }
                },
                {
                    typeof(Matrix),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Row 0", typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(2, "Row 1", typeof(Float4), 1),
                        NodeElementArchetype.Factory.Output(3, "Row 2", typeof(Float4), 2),
                        NodeElementArchetype.Factory.Output(4, "Row 3", typeof(Float4), 3),
                    }
                },
                {
                    typeof(ChannelMask),
                    new[]
                    {
                        NodeElementArchetype.Factory.Input(1, "Vector", true, typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(1, "Component", typeof(float), 1),
                    }
                },
            };

            private readonly NodeElementArchetype[] _normalMapParameterElements =
            {
                NodeElementArchetype.Factory.Input(1, "UVs", true, typeof(Float2), 0, -1),
                NodeElementArchetype.Factory.Output(1, "", typeof(FlaxEngine.Object), 6),
                NodeElementArchetype.Factory.Output(2, "Vector", typeof(Float3), 1),
                NodeElementArchetype.Factory.Output(3, "X", typeof(float), 2),
                NodeElementArchetype.Factory.Output(4, "Y", typeof(float), 3),
                NodeElementArchetype.Factory.Output(5, "Z", typeof(float), 4),
            };

            /// <summary>
            /// True if use special node layout for normal maps.
            /// </summary>
            protected virtual bool UseNormalMaps => true;

            /// <inheritdoc />
            public SurfaceNodeParamsGet(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private NodeElementArchetype[] GetElementArchetypes(SurfaceParameter selected)
            {
                if (selected != null && selected.Type != ScriptType.Null)
                {
                    if (selected.Type.Type != null && Prototypes != null && Prototypes.TryGetValue(selected.Type.Type, out var elements))
                    {
                        // Special case for Normal Maps
                        if (selected.Type.Type == typeof(Texture) && UseNormalMaps && selected.Value != null)
                        {
                            var asset = selected.Value as Texture;
                            if (asset == null && selected.Value is Guid id)
                                asset = FlaxEngine.Content.Load<Texture>(id);
                            if (asset != null && !asset.WaitForLoaded() && asset.IsNormalMap)
                                elements = _normalMapParameterElements;
                        }
                        return elements;
                    }

                    return new[]
                    {
                        NodeElementArchetype.Factory.Output(0, string.Empty, selected.Type, 0),
                    };
                }
                return null;
            }

            private void UpdateLayout()
            {
                // Add elements and calculate node size if type changes
                var selected = GetSelected();
                var type = selected?.Type ?? ScriptType.Null;
                if (type != _layoutType || type.Type == typeof(Texture))
                {
                    var elements = GetElementArchetypes(selected);
                    if (elements != _layoutElements)
                    {
                        ClearDynamicElements();
                        if (elements != null)
                        {
                            for (var i = 0; i < elements.Length; i++)
                            {
                                var element = AddElement(elements[i]);
                                _dynamicChildren.Add(element);
                            }
                        }
                        _layoutElements = elements;
                    }
                    _layoutType = type;
                }

                UpdateTitle();
            }

            private void UpdateCombo()
            {
                if (_isUpdateLocked)
                    return;
                _isUpdateLocked = true;
                if (_combobox == null)
                {
                    _combobox = (ComboBoxElement)_children[0];
                    _combobox.SelectedIndexChanged += OnSelectedChanged;
                }
                int toSelect = -1;
                Guid loadedSelected = (Guid)Values[0];
                _combobox.ClearItems();
                for (int i = 0; i < Surface.Parameters.Count; i++)
                {
                    var param = Surface.Parameters[i];
                    _combobox.AddItem(param.Name);
                    if (param.ID == loadedSelected)
                        toSelect = i;
                }
                _combobox.SelectedIndex = toSelect;
                _isUpdateLocked = false;
            }

            private void OnSelectedChanged(ComboBox cb)
            {
                if (_isUpdateLocked)
                    return;
                var selected = GetSelected();
                var selectedID = selected?.ID ?? Guid.Empty;
                SetValue(0, selectedID);
            }

            private SurfaceParameter GetSelected()
            {
                if (Surface != null)
                {
                    var selectedIndex = _combobox.SelectedIndex;
                    return selectedIndex >= 0 && selectedIndex < Surface.Parameters.Count ? Surface.Parameters[selectedIndex] : null;
                }
                return Context.GetParameter((Guid)Values[0]);
            }

            private void ClearDynamicElements()
            {
                for (int i = 0; i < _dynamicChildren.Count; i++)
                    RemoveElement(_dynamicChildren[i]);
                _dynamicChildren.Clear();
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                if (GetSelected() == null)
                    return;
                menu.AddSeparator();
                menu.AddButton("Find references...", OnFindReferences);
            }

            private void OnFindReferences()
            {
                var selected = GetSelected();
                var query = ContentSearchText ?? FlaxEngine.Json.JsonSerializer.GetStringID(selected.ID);
                Editor.Instance.ContentFinding.ShowSearch(Surface, '\"' + query + '\"');
            }

            /// <inheritdoc />
            public override string ContentSearchText
            {
                get
                {
                    if (Surface is VisualScriptSurface visualScriptSurface)
                    {
                        // Override with parameter typename
                        var selected = GetSelected();
                        return visualScriptSurface.Script.ScriptTypeName + '.' + selected.Name;
                    }
                    if (Context.Context.SurfaceAsset is VisualScript visualScript)
                    {
                        // Override with parameter typename
                        var selected = Context.GetParameter((Guid)Values[0]);
                        if (selected != null)
                            return visualScript.ScriptTypeName + '.' + selected.Name;
                    }
                    return null;
                }
            }

            /// <inheritdoc />
            public void OnParamCreated(SurfaceParameter param)
            {
                UpdateCombo();
            }

            /// <inheritdoc />
            public void OnParamRenamed(SurfaceParameter param)
            {
                UpdateCombo();
                UpdateTitle();
            }

            /// <inheritdoc />
            public void OnParamEdited(SurfaceParameter param)
            {
                UpdateLayout();
            }

            /// <inheritdoc />
            public void OnParamDeleted(SurfaceParameter param)
            {
                // Deselect if that parameter is selected
                if ((Guid)Values[0] == param.ID)
                    _combobox.SelectedIndex = -1;

                UpdateCombo();
            }

            /// <inheritdoc />
            public bool IsParamUsed(SurfaceParameter param)
            {
                return (Guid)Values[0] == param.ID;
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                if (Surface != null)
                {
                    UpdateCombo();
                    UpdateLayout();
                }
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateTitle();
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                if (_combobox == null)
                {
                    UpdateCombo();
                }
                else if (!_isUpdateLocked)
                {
                    _isUpdateLocked = true;
                    int toSelect = -1;
                    Guid loadedSelected = (Guid)Values[0];
                    for (int i = 0; i < Surface.Parameters.Count; i++)
                    {
                        var param = Surface.Parameters[i];
                        if (param.ID == loadedSelected)
                            toSelect = i;
                    }
                    _combobox.SelectedIndex = toSelect;
                    _isUpdateLocked = false;
                }
                UpdateLayout();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _layoutElements = null;
                _layoutType = ScriptType.Null;

                base.OnDestroy();
            }

            private void UpdateTitle()
            {
                var selected = GetSelected();
                Title = selected != null ? "Get " + selected.Name : "Get Parameter";
                if (_combobox != null)
                {
                    ResizeAuto();
                    _combobox.Width = Width - 30;
                }
            }

            internal static bool IsOutputCompatible(NodeArchetype nodeArch, ScriptType inputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                if (inputType == ScriptType.Object)
                    return true;

                SurfaceParameter parameter = context.GetParameter((Guid)nodeArch.DefaultValues[0]);
                ScriptType type = parameter?.Type ?? ScriptType.Null;

                if (parameter == null || type == ScriptType.Null || parameter.Type.Type == null)
                    return false;
                if (DefaultPrototypes == null || !DefaultPrototypes.TryGetValue(parameter.Type.Type, out var elements))
                {
                    return VisjectSurface.FullCastCheck(inputType, type, hint);
                }
                if (elements == null)
                    return false;

                for (var i = 0; i < elements.Length; i++)
                {
                    if (elements[i].Type != NodeElementType.Output)
                        continue;

                    if (VisjectSurface.FullCastCheck(elements[i].ConnectionsType, inputType, hint))
                        return true;
                }
                return false;
            }

            internal static bool IsInputCompatible(NodeArchetype nodeArch, ScriptType outputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                SurfaceParameter parameter = context.GetParameter((Guid)nodeArch.DefaultValues[0]);
                ScriptType type = parameter?.Type ?? ScriptType.Null;

                if (parameter == null || type == ScriptType.Null)
                    return false;
                if (parameter.Type.Type == null || DefaultPrototypes == null || !DefaultPrototypes.TryGetValue(parameter.Type.Type, out var elements))
                    return false;
                if (elements == null)
                    return false;

                for (var i = 0; i < elements.Length; i++)
                {
                    if (elements[i].Type != NodeElementType.Input)
                        continue;
                    if (VisjectSurface.FullCastCheck(elements[i].ConnectionsType, outputType, hint))
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Surface node type for parameters group Get node for Particle Emitter graph.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class SurfaceNodeParamsGetParticleEmitter : SurfaceNodeParamsGet
        {
            /// <summary>
            /// The <see cref="SurfaceNodeParamsGet.DefaultPrototypes"/> implementation for Particle Emitter graph.
            /// </summary>
            public static readonly Dictionary<Type, NodeElementArchetype[]> DefaultPrototypesParticleEmitter = new Dictionary<Type, NodeElementArchetype[]>
            {
                {
                    typeof(bool),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(bool), 0),
                    }
                },
                {
                    typeof(int),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(int), 0),
                    }
                },
                {
                    typeof(float),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(float), 0),
                    }
                },
                {
                    typeof(double),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(double), 0),
                    }
                },
                {
                    typeof(Float2),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Float2), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(float), 2),
                    }
                },
                {
                    typeof(Float3),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Float3), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(float), 3),
                    }
                },
                {
                    typeof(Float4),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "W", typeof(float), 4),
                    }
                },
                {
                    typeof(Double2),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Double2), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(double), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(double), 2),
                    }
                },
                {
                    typeof(Double3),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Double3), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(double), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(double), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(double), 3),
                    }
                },
                {
                    typeof(Double4),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Double4), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(double), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(double), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(double), 3),
                        NodeElementArchetype.Factory.Output(5, "W", typeof(double), 4),
                    }
                },
                {
                    typeof(Vector2),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Vector2), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(Real), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(Real), 2),
                    }
                },
                {
                    typeof(Vector3),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Vector3), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(Real), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(Real), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(Real), 3),
                    }
                },
                {
                    typeof(Vector4),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Value", typeof(Vector4), 0),
                        NodeElementArchetype.Factory.Output(2, "X", typeof(Real), 1),
                        NodeElementArchetype.Factory.Output(3, "Y", typeof(Real), 2),
                        NodeElementArchetype.Factory.Output(4, "Z", typeof(Real), 3),
                        NodeElementArchetype.Factory.Output(5, "W", typeof(Real), 4),
                    }
                },
                {
                    typeof(Color),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Color", typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(2, "R", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(3, "G", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(4, "B", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(5, "A", typeof(float), 4),
                    }
                },
                {
                    typeof(Texture),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, string.Empty, typeof(FlaxEngine.Object), 0),
                    }
                },
                {
                    typeof(GPUTexture),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, string.Empty, typeof(FlaxEngine.Object), 0),
                    }
                },
                {
                    typeof(Matrix),
                    new[]
                    {
                        NodeElementArchetype.Factory.Output(1, "Row 0", typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(2, "Row 1", typeof(Float4), 1),
                        NodeElementArchetype.Factory.Output(3, "Row 2", typeof(Float4), 2),
                        NodeElementArchetype.Factory.Output(4, "Row 3", typeof(Float4), 3),
                    }
                },
                {
                    typeof(ChannelMask),
                    new[]
                    {
                        NodeElementArchetype.Factory.Input(1, "Vector", true, typeof(Float4), 0),
                        NodeElementArchetype.Factory.Output(1, "Component", typeof(float), 1),
                    }
                },
            };

            /// <inheritdoc />
            public SurfaceNodeParamsGetParticleEmitter(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                Prototypes = DefaultPrototypesParticleEmitter;
            }

            /// <inheritdoc />
            protected override bool UseNormalMaps => false;

            internal new static bool IsOutputCompatible(NodeArchetype nodeArch, ScriptType inputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                if (inputType == ScriptType.Object)
                    return true;

                SurfaceParameter parameter = context.GetParameter((Guid)nodeArch.DefaultValues[0]);
                ScriptType type = parameter?.Type ?? ScriptType.Null;

                if (parameter == null || type == ScriptType.Null)
                    return false;
                if (parameter.Type.Type == null || DefaultPrototypesParticleEmitter == null || !DefaultPrototypesParticleEmitter.TryGetValue(parameter.Type.Type, out var elements))
                    return false;
                if (elements == null)
                    return false;

                for (var i = 0; i < elements.Length; i++)
                {
                    if (elements[i].Type != NodeElementType.Output)
                        continue;
                    if (VisjectSurface.FullCastCheck(elements[i].ConnectionsType, inputType, hint))
                        return true;
                }
                return false;
            }

            internal new static bool IsInputCompatible(NodeArchetype nodeArch, ScriptType outputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                SurfaceParameter parameter = context.GetParameter((Guid)nodeArch.DefaultValues[0]);
                ScriptType type = parameter?.Type ?? ScriptType.Null;

                if (parameter == null || type == ScriptType.Null)
                    return false;
                if (parameter.Type.Type == null || DefaultPrototypesParticleEmitter == null || !DefaultPrototypesParticleEmitter.TryGetValue(parameter.Type.Type, out var elements))
                    return false;
                if (elements == null)
                    return false;

                for (var i = 0; i < elements.Length; i++)
                {
                    if (elements[i].Type != NodeElementType.Input)
                        continue;
                    if (VisjectSurface.FullCastCheck(elements[i].ConnectionsType, outputType, hint))
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Surface node type for parameters group Get node for Visual Script graph.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class SurfaceNodeParamsGetVisualScript : SurfaceNodeParamsGet
        {
            /// <inheritdoc />
            public SurfaceNodeParamsGetVisualScript(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
                Prototypes = null;
            }

            /// <inheritdoc />
            protected override bool UseNormalMaps => false;

            internal new static bool IsOutputCompatible(NodeArchetype nodeArch, ScriptType inputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                if (inputType == ScriptType.Object)
                    return true;

                SurfaceParameter parameter = context.GetParameter((Guid)nodeArch.DefaultValues[0]);
                ScriptType type = parameter?.Type ?? ScriptType.Null;

                return VisjectSurface.FullCastCheck(inputType, type, hint);
            }

            internal new static bool IsInputCompatible(NodeArchetype nodeArch, ScriptType outputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                return false;
            }
        }

        /// <summary>
        /// Surface node type for parameters group Set node.
        /// </summary>
        /// <seealso cref="FlaxEditor.Surface.SurfaceNode" />
        public class SurfaceNodeParamsSet : SurfaceNode, IParametersDependantNode
        {
            private ComboBoxElement _combobox;
            private bool _isUpdateLocked;

            /// <inheritdoc />
            public SurfaceNodeParamsSet(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            private void UpdateCombo()
            {
                if (_isUpdateLocked)
                    return;
                _isUpdateLocked = true;
                if (_combobox == null)
                {
                    _combobox = GetChild<ComboBoxElement>();
                    _combobox.SelectedIndexChanged += OnSelectedChanged;
                }
                int toSelect = -1;
                Guid loadedSelected = (Guid)Values[0];
                _combobox.ClearItems();
                for (int i = 0; i < Surface.Parameters.Count; i++)
                {
                    var param = Surface.Parameters[i];
                    _combobox.AddItem(param.Name);
                    if (param.ID == loadedSelected)
                        toSelect = i;
                }
                _combobox.SelectedIndex = toSelect;
                _isUpdateLocked = false;
            }

            private void OnSelectedChanged(ComboBox cb)
            {
                if (_isUpdateLocked)
                    return;
                var selected = GetSelected();
                var selectedID = selected?.ID ?? Guid.Empty;
                if (selectedID != (Guid)Values[0])
                {
                    SetValues(new[]
                    {
                        selectedID,
                        selected != null ? TypeUtils.GetDefaultValue(selected.Type) : null,
                    });
                    UpdateUI();
                }
            }

            private SurfaceParameter GetSelected()
            {
                if (Surface != null)
                {
                    var selectedIndex = _combobox.SelectedIndex;
                    return selectedIndex >= 0 && selectedIndex < Surface.Parameters.Count ? Surface.Parameters[selectedIndex] : null;
                }
                return Context.GetParameter((Guid)Values[0]);
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                if (GetSelected() == null)
                    return;
                menu.AddSeparator();
                menu.AddButton("Find references...", OnFindReferences);
            }

            private void OnFindReferences()
            {
                var selected = GetSelected();
                var query = ContentSearchText ?? FlaxEngine.Json.JsonSerializer.GetStringID(selected.ID);
                Editor.Instance.ContentFinding.ShowSearch(Surface, '\"' + query + '\"');
            }

            /// <inheritdoc />
            public override string ContentSearchText
            {
                get
                {
                    if (Surface is VisualScriptSurface visualScriptSurface)
                    {
                        // Override with parameter typename
                        var selected = GetSelected();
                        return visualScriptSurface.Script.ScriptTypeName + '.' + selected.Name;
                    }
                    if (Context.Context.SurfaceAsset is VisualScript visualScript)
                    {
                        // Override with parameter typename
                        var selected = Context.GetParameter((Guid)Values[0]);
                        if (selected != null)
                            return visualScript.ScriptTypeName + '.' + selected.Name;
                    }
                    return null;
                }
            }

            /// <inheritdoc />
            public void OnParamCreated(SurfaceParameter param)
            {
                UpdateCombo();
            }

            /// <inheritdoc />
            public void OnParamRenamed(SurfaceParameter param)
            {
                UpdateCombo();
                UpdateUI();
            }

            /// <inheritdoc />
            public void OnParamEdited(SurfaceParameter param)
            {
                UpdateUI();
            }

            /// <inheritdoc />
            public void OnParamDeleted(SurfaceParameter param)
            {
                // Deselect if that parameter is selected
                if ((Guid)Values[0] == param.ID)
                    _combobox.SelectedIndex = -1;

                UpdateCombo();
            }

            /// <inheritdoc />
            public bool IsParamUsed(SurfaceParameter param)
            {
                return (Guid)Values[0] == param.ID;
            }

            /// <inheritdoc />
            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                if (Surface != null)
                    UpdateCombo();
                UpdateUI();
            }

            /// <inheritdoc />
            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                if (Surface != null)
                {
                    UpdateUI();
                }
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateCombo();
                UpdateUI();
            }

            private void UpdateUI()
            {
                var box = GetBox(1);
                var selected = GetSelected();
                box.Enabled = selected != null;
                box.CurrentType = selected?.Type ?? ScriptType.Null;
                Title = selected != null ? "Set " + selected.Name : "Set Parameter";
                if (_combobox != null)
                {
                    ResizeAuto();
                    _combobox.Width = Width - 50;
                }
            }

            internal static bool IsOutputCompatible(NodeArchetype nodeArch, ScriptType inputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                return inputType == ScriptType.Void;
            }

            internal static bool IsInputCompatible(NodeArchetype nodeArch, ScriptType outputType, ConnectionsHint hint, VisjectSurfaceContext context)
            {
                if (outputType == ScriptType.Void)
                    return true;

                SurfaceParameter parameter = context.GetParameter((Guid)nodeArch.DefaultValues[0]);
                ScriptType type = parameter?.Type ?? ScriptType.Null;

                return VisjectSurface.FullCastCheck(outputType, type, hint);
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
                Create = (id, context, arch, groupArch) => new SurfaceNodeParamsGet(id, context, arch, groupArch),
                IsInputCompatible = SurfaceNodeParamsGet.IsInputCompatible,
                IsOutputCompatible = SurfaceNodeParamsGet.IsOutputCompatible,
                Title = "Get Parameter",
                Description = "Parameter value getter",
                Flags = NodeFlags.MaterialGraph | NodeFlags.AnimGraph,
                Size = new Float2(140, 60),
                DefaultValues = new object[]
                {
                    Guid.Empty
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(2, 0, 116)
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Create = (id, context, arch, groupArch) => new SurfaceNodeParamsGetParticleEmitter(id, context, arch, groupArch),
                IsInputCompatible = SurfaceNodeParamsGetParticleEmitter.IsInputCompatible,
                IsOutputCompatible = SurfaceNodeParamsGetParticleEmitter.IsOutputCompatible,
                Title = "Get Parameter",
                Description = "Parameter value getter",
                Flags = NodeFlags.ParticleEmitterGraph,
                Size = new Float2(140, 60),
                DefaultValues = new object[]
                {
                    Guid.Empty
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(2, 0, 116)
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Create = (id, context, arch, groupArch) => new SurfaceNodeParamsGetVisualScript(id, context, arch, groupArch),
                IsInputCompatible = SurfaceNodeParamsGetVisualScript.IsInputCompatible,
                IsOutputCompatible = SurfaceNodeParamsGetVisualScript.IsOutputCompatible,
                Title = "Get Parameter",
                Description = "Parameter value getter",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(140, 20),
                DefaultValues = new object[]
                {
                    Guid.Empty
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.ComboBox(2, 0, 116)
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Create = (id, context, arch, groupArch) => new SurfaceNodeParamsSet(id, context, arch, groupArch),
                IsInputCompatible = SurfaceNodeParamsSet.IsInputCompatible,
                IsOutputCompatible = SurfaceNodeParamsSet.IsOutputCompatible,
                Title = "Set Parameter",
                Description = "Parameter value setter",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(140, 40),
                DefaultValues = new object[]
                {
                    Guid.Empty,
                    null
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, string.Empty, true, ScriptType.Null, 1, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(void), 2, true),
                    NodeElementArchetype.Factory.ComboBox(2 + 20, 0, 116)
                }
            },
        };
    }
}
