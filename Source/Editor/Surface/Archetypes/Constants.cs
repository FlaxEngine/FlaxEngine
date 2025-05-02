// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Constants group.
    /// </summary>
    [HideInEditor]
    public static class Constants
    {
        /// <summary>
        /// A special type of node that adds the functionality to convert nodes to parameters.
        /// </summary>
        internal class ConvertToParameterNode : SurfaceNode
        {
            private readonly ScriptType _type;
            private readonly Func<object[], object> _convertFunction;

            /// <inheritdoc />
            public ConvertToParameterNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch, ScriptType type, Func<object[], object> convertFunction = null)
            : base(id, context, nodeArch, groupArch)
            {
                _type = type;
                _convertFunction = convertFunction;
            }

            /// <inheritdoc />
            public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
            {
                base.OnShowSecondaryContextMenu(menu, location);

                menu.AddSeparator();
                var b = menu.AddButton("Convert to Parameter", OnConvertToParameter);
                b.Enabled = Surface.Owner is IVisjectSurfaceWindow window && Surface.Owner.SurfaceAsset && window.NewParameterTypes.Contains(_type);
            }

            private void OnConvertToParameter()
            {
                if (Surface.Owner is not IVisjectSurfaceWindow window)
                    throw new Exception("Surface owner is not a Visject Surface Window");

                Asset asset = Surface.Owner.SurfaceAsset;
                if (asset == null || !asset.IsLoaded)
                {
                    Editor.LogError("Asset is null or not loaded");
                    return;
                }

                // Add parameter to editor
                var paramIndex = Surface.Parameters.Count;
                var initValue = _convertFunction == null ? Values[0] : _convertFunction.Invoke(Values);
                var paramAction = new AddRemoveParamAction
                {
                    Window = window,
                    IsAdd = true,
                    Name = Utilities.Utils.IncrementNameNumber("New parameter", OnParameterRenameValidate),
                    Type = _type,
                    Index = paramIndex,
                    InitValue = initValue,
                };
                paramAction.Do();
                Surface.AddBatchedUndoAction(paramAction);

                // Spawn Get Parameter Node based on the added parameter
                Guid parameterGuid = Surface.Parameters[paramIndex].ID;
                bool undoEnabled = Surface.Undo.Enabled;
                Surface.Undo.Enabled = false;
                NodeArchetype arch = Surface.GetParameterGetterNodeArchetype(out var groupId);
                SurfaceNode node = Surface.Context.SpawnNode(groupId, arch.TypeID, Location, new object[] { parameterGuid });
                Surface.Undo.Enabled = undoEnabled;
                if (node is not Parameters.SurfaceNodeParamsGet getNode)
                    throw new Exception("Node is not a ParamsGet node!");
                Surface.AddBatchedUndoAction(new AddRemoveNodeAction(getNode, true));

                // Recreate connections of constant node
                // Constant nodes and parameter nodes should have the same ports, so we can just iterate through the connections
                var editConnectionsAction1 = new EditNodeConnections(Context, this);
                var editConnectionsAction2 = new EditNodeConnections(Context, node);
                var boxes = GetBoxes();
                for (int i = 0; i < boxes.Count; i++)
                {
                    Box box = boxes[i];
                    if (!box.HasAnyConnection)
                        continue;
                    if (!getNode.TryGetBox(box.ID, out Box paramBox))
                        continue;

                    // Iterating backwards, because the CreateConnection method deletes entries from the box connections when target box IsSingle is set to true
                    for (int k = box.Connections.Count - 1; k >= 0; k--)
                    {
                        Box connectedBox = box.Connections[k];
                        paramBox.CreateConnection(connectedBox);
                    }
                }
                editConnectionsAction1.End();
                editConnectionsAction2.End();
                Surface.AddBatchedUndoAction(editConnectionsAction1);
                Surface.AddBatchedUndoAction(editConnectionsAction2);

                // Add undo actions and remove constant node
                var removeConstantAction = new AddRemoveNodeAction(this, false);
                Surface.AddBatchedUndoAction(removeConstantAction);
                removeConstantAction.Do();
                Surface.MarkAsEdited();
            }

            private bool OnParameterRenameValidate(string value)
            {
                var window = (IVisjectSurfaceWindow)Surface.Owner;
                return !string.IsNullOrWhiteSpace(value) && window.VisjectSurface.Parameters.All(x => x.Name != value);
            }
        }

        private class EnumNode : SurfaceNode
        {
            private EnumComboBox _picker;

            public EnumNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                if (_picker != null)
                    _picker.EnumTypeValue = Values[0];
                var box = (OutputBox)GetBox(0);
                box.CurrentType = new ScriptType(Values[0].GetType());
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                var box = (OutputBox)GetBox(0);
                if (Values[0] == null)
                {
                    box.Enabled = false;
                    return;
                }
                var type = Values[0].GetType();
                if (!type.IsEnum)
                {
                    box.Enabled = false;
                    return;
                }
                _picker = new EnumComboBox(type)
                {
                    EnumTypeValue = Values[0],
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 160, 16),
                    Parent = this,
                };
                _picker.ValueChanged += () => SetValue(0, _picker.EnumTypeValue);
                Title = type.Name;
                ResizeAuto();
                _picker.Width = Width - 30;
                box.CurrentType = new ScriptType(type);
            }

            public override void OnDestroy()
            {
                _picker = null;

                base.OnDestroy();
            }
            
            internal static void GetInputOutputDescription(NodeArchetype nodeArch, out (string, ScriptType)[] inputs, out (string, ScriptType)[] outputs)
            {
                var type = new ScriptType(nodeArch.DefaultValues[0].GetType());
                inputs = null;
                outputs = [(type.Name, type)];
            }
        }

        private class ArrayNode : SurfaceNode
        {
            private OutputBox _output;
            private TypePickerControl _typePicker;
            private Button _addButton;
            private Button _removeButton;
            private bool _isUpdatingUI;

            public ArrayNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnValuesChanged()
            {
                UpdateUI();

                base.OnValuesChanged();
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                _output = (OutputBox)Elements[0];
                _typePicker = new TypePickerControl
                {
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 160, 16),
                    Parent = this,
                };
                _typePicker.ValueChanged += () => Set(3);
                _removeButton = new Button(0, _typePicker.Y + FlaxEditor.Surface.Constants.LayoutOffsetY, 20, 20)
                {
                    Text = "-",
                    TooltipText = "Remove the last item (smaller array)",
                    Parent = this
                };
                _removeButton.Clicked += () => Set(((Array)Values[0]).Length - 1);
                _addButton = new Button(_removeButton.Location, _removeButton.Size)
                {
                    Text = "+",
                    TooltipText = "Add new item to array (bigger array)",
                    Parent = this
                };
                _addButton.Clicked += () => Set(((Array)Values[0]).Length + 1);

                UpdateUI();
            }

            private void Set(int length)
            {
                if (_isUpdatingUI)
                    return;
                var prev = (Array)Values[0];
                var next = Array.CreateInstance(TypeUtils.GetType(_typePicker.Value), length);
                if (prev.GetType() == next.GetType())
                    Array.Copy(prev, next, Mathf.Min(prev.Length, next.Length));
                SetValue(0, next);
            }

            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _typePicker.Enabled = canEdit;
                _addButton.Enabled = canEdit;
                _removeButton.Enabled = canEdit;
            }

            public override void OnDestroy()
            {
                _output = null;
                _typePicker = null;
                _addButton = null;
                _removeButton = null;

                base.OnDestroy();
            }

            private void UpdateUI()
            {
                if (_isUpdatingUI)
                    return;
                var array = (Array)Values[0];
                var arrayType = array.GetType();
                var elementType = new ScriptType(arrayType.GetElementType());
                _isUpdatingUI = true;
                _typePicker.Value = elementType;
                _output.CurrentType = new ScriptType(arrayType);
                _isUpdatingUI = false;

                var count = array.Length;
                var countMin = 0;
                var countMax = 32;
                for (int i = 0; i < array.Length; i++)
                {
                    var box = (InputBox)AddBox(false, i + 1, i + 1, $"[{i}]", elementType, true);
                    box.UseCustomValueAccess(GetBoxValue, SetBoxValue);
                }
                for (int i = count; i <= countMax; i++)
                {
                    var box = GetBox(i + 1);
                    if (box == null)
                        break;
                    RemoveElement(box);
                }

                var canEdit = Surface.CanEdit;
                _typePicker.Enabled = canEdit;
                _addButton.Enabled = count < countMax && canEdit;
                _removeButton.Enabled = count > countMin && canEdit;

                Title = string.Format("{0} Array", Surface.GetTypeName(elementType));
                _typePicker.Width = 160.0f;
                _addButton.X = 0;
                _removeButton.X = 0;
                ResizeAuto();
                _addButton.X = Width - _addButton.Width - FlaxEditor.Surface.Constants.NodeMarginX;
                _removeButton.X = _addButton.X - _removeButton.Width - 4;
                _typePicker.Width = Width - 30;
            }

            private object GetBoxValue(InputBox box)
            {
                var array = (Array)Values[0];
                return array.GetValue(box.ID - 1);
            }

            private void SetBoxValue(InputBox box, object value)
            {
                if (_isDuringValuesEditing || !Surface.CanEdit)
                    return;
                var array = (Array)Values[0];
                array = (Array)array.Clone();
                array.SetValue(value, box.ID - 1);
                SetValue(0, array);
            }
            
            internal static void GetInputOutputDescription(NodeArchetype nodeArch, out (string, ScriptType)[] inputs, out (string, ScriptType)[] outputs)
            {
                inputs = null;
                outputs = [("", new ScriptType(typeof(Array)))];
            }
        }

        private class DictionaryNode : SurfaceNode
        {
            private OutputBox _output;
            private TypePickerControl _keyTypePicker;
            private TypePickerControl _valueTypePicker;
            private bool _isUpdatingUI;

            public DictionaryNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnValuesChanged()
            {
                UpdateUI();

                base.OnValuesChanged();
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                _output = (OutputBox)Elements[0];
                _keyTypePicker = new TypePickerControl
                {
                    Bounds = new Rectangle(FlaxEditor.Surface.Constants.NodeMarginX, FlaxEditor.Surface.Constants.NodeMarginY + FlaxEditor.Surface.Constants.NodeHeaderSize, 160, 16),
                    Parent = this,
                };
                _keyTypePicker.ValueChanged += OnKeyTypeChanged;
                _valueTypePicker = new TypePickerControl
                {
                    Bounds = new Rectangle(_keyTypePicker.X, _keyTypePicker.Y + FlaxEditor.Surface.Constants.LayoutOffsetY, _keyTypePicker.Width, _keyTypePicker.Height),
                    Parent = this,
                };
                _valueTypePicker.ValueChanged += OnValueTypeChanged;

                UpdateUI();
            }

            private void OnKeyTypeChanged()
            {
                if (_isUpdatingUI)
                    return;
                SetValue(0, _keyTypePicker.ValueTypeName);
            }

            private void OnValueTypeChanged()
            {
                if (_isUpdatingUI)
                    return;
                SetValue(1, _valueTypePicker.ValueTypeName);
            }

            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _keyTypePicker.Enabled = canEdit;
                _valueTypePicker.Enabled = canEdit;
            }

            public override void OnDestroy()
            {
                _output = null;
                _keyTypePicker = null;
                _valueTypePicker = null;

                base.OnDestroy();
            }

            private void UpdateUI()
            {
                if (_isUpdatingUI)
                    return;
                var keyTypeName = (string)Values[0];
                var valueTypeName = (string)Values[1];
                var keyType = TypeUtils.GetType(keyTypeName);
                var valueType = TypeUtils.GetType(valueTypeName);
                if (keyType == ScriptType.Null)
                {
                    Editor.LogError("Missing type " + keyTypeName);
                    keyType = ScriptType.Object;
                }
                if (valueType == ScriptType.Null)
                {
                    Editor.LogError("Missing type " + valueTypeName);
                    valueType = ScriptType.Object;
                }
                var dictionaryType = ScriptType.MakeDictionaryType(keyType, valueType);

                _isUpdatingUI = true;
                _keyTypePicker.Value = keyType;
                _valueTypePicker.Value = valueType;
                _output.CurrentType = dictionaryType;
                _isUpdatingUI = false;

                if (Surface == null)
                    return;
                var canEdit = Surface.CanEdit;
                _keyTypePicker.Enabled = canEdit;
                _valueTypePicker.Enabled = canEdit;

                Title = Surface.GetTypeName(dictionaryType);
                _keyTypePicker.Width = 160.0f;
                _valueTypePicker.Width = 160.0f;
                ResizeAuto();
                _keyTypePicker.Width = Width - 30;
                _valueTypePicker.Width = Width - 30;
            }

            private object GetBoxValue(InputBox box)
            {
                var array = (Array)Values[0];
                return array.GetValue(box.ID - 1);
            }

            private void SetBoxValue(InputBox box, object value)
            {
                if (_isDuringValuesEditing || !Surface.CanEdit)
                    return;
                var array = (Array)Values[0];
                array = (Array)array.Clone();
                array.SetValue(value, box.ID - 1);
                SetValue(0, array);
            }
            
            internal static void GetInputOutputDescription(NodeArchetype nodeArch, out (string, ScriptType)[] inputs, out (string, ScriptType)[] outputs)
            {
                inputs = null;
                outputs = [("", new ScriptType(typeof(System.Collections.Generic.Dictionary<int, string>)))];
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
                Title = "Bool",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(bool))),
                Description = "Constant boolean value",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(110, 20),
                DefaultValues = new object[]
                {
                    false
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(bool), 0),
                    NodeElementArchetype.Factory.Bool(0, 0, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (string.Equals(filterText, bool.TrueString, StringComparison.OrdinalIgnoreCase))
                    {
                        data = new object[] { true };
                        return true;
                    }
                    if (string.Equals(filterText, bool.FalseString, StringComparison.OrdinalIgnoreCase))
                    {
                        data = new object[] { false };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Title = "Integer",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(int))),
                Description = "Constant integer value",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(110, 20),
                DefaultValues = new object[]
                {
                    0
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(int), 0),
                    NodeElementArchetype.Factory.Integer(0, 0, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (int.TryParse(filterText, out var number))
                    {
                        data = new object[] { number };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "Float",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(float))),
                Description = "Constant floating point",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(110, 20),
                DefaultValues = new object[]
                {
                    0.0f
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(float), 0),
                    NodeElementArchetype.Factory.Float(0, 0, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (TryParseValues(filterText, out var values) && values.Length < 2)
                    {
                        data = new object[] { values[0] };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Float2",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Float2))),
                Description = "Constant Float2",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(130, 60),
                DefaultValues = new object[]
                {
                    Float2.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Float2), 0),
                    NodeElementArchetype.Factory.Output(1, "X", typeof(float), 1),
                    NodeElementArchetype.Factory.Output(2, "Y", typeof(float), 2),
                    NodeElementArchetype.Factory.Vector_X(0, 1 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, 2 * Surface.Constants.LayoutOffsetY, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (TryParseValues(filterText, out var values) && values.Length < 3)
                    {
                        data = new object[] { new Float2(ValuesToFloat4(values)) };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Float3",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Float3))),
                Description = "Constant Float3",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(130, 80),
                DefaultValues = new object[]
                {
                    Float3.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Float3), 0),
                    NodeElementArchetype.Factory.Output(1, "X", typeof(float), 1),
                    NodeElementArchetype.Factory.Output(2, "Y", typeof(float), 2),
                    NodeElementArchetype.Factory.Output(3, "Z", typeof(float), 3),
                    NodeElementArchetype.Factory.Vector_X(0, 1 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, 2 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Z(0, 3 * Surface.Constants.LayoutOffsetY, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (TryParseValues(filterText, out var values) && values.Length < 4)
                    {
                        data = new object[] { new Float3(ValuesToFloat4(values)) };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Float4",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Float4))),
                Description = "Constant Float4",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(130, 100),
                DefaultValues = new object[]
                {
                    Float4.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Float4), 0),
                    NodeElementArchetype.Factory.Output(1, "X", typeof(float), 1),
                    NodeElementArchetype.Factory.Output(2, "Y", typeof(float), 2),
                    NodeElementArchetype.Factory.Output(3, "Z", typeof(float), 3),
                    NodeElementArchetype.Factory.Output(4, "W", typeof(float), 4),
                    NodeElementArchetype.Factory.Vector_X(0, 1 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, 2 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Z(0, 3 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_W(0, 4 * Surface.Constants.LayoutOffsetY, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (TryParseValues(filterText, out var values))
                    {
                        data = new object[] { ValuesToFloat4(values) };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Color",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Color))),
                Description = "RGBA color",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(70, 100),
                DefaultValues = new object[]
                {
                    Color.White
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Float4), 0),
                    NodeElementArchetype.Factory.Output(1, "R", typeof(float), 1),
                    NodeElementArchetype.Factory.Output(2, "G", typeof(float), 2),
                    NodeElementArchetype.Factory.Output(3, "B", typeof(float), 3),
                    NodeElementArchetype.Factory.Output(4, "A", typeof(float), 4),
                    NodeElementArchetype.Factory.Color(0, 0, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    if (Color.TryParse(filterText, out var color))
                    {
                        // Color constant from hex
                        data = new object[] { color };
                        return true;
                    }
                    data = null;
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Rotation",
                Create = (id, context, arch, groupArch) =>
                new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Quaternion)), values => Quaternion.Euler((float)values[0], (float)values[1], (float)values[2])),
                Description = "Euler angle rotation",
                Flags = NodeFlags.AnimGraph | NodeFlags.VisualScriptGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Float2(110, 60),
                DefaultValues = new object[]
                {
                    0.0f,
                    0.0f,
                    0.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Quaternion), 0),
                    NodeElementArchetype.Factory.Float(32, 0, 0),
                    NodeElementArchetype.Factory.Float(32, Surface.Constants.LayoutOffsetY, 1),
                    NodeElementArchetype.Factory.Float(32, Surface.Constants.LayoutOffsetY * 2, 2),
                    NodeElementArchetype.Factory.Text(0, 0, "Pitch:"),
                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY, "Yaw:"),
                    NodeElementArchetype.Factory.Text(0, Surface.Constants.LayoutOffsetY * 2, "Roll:"),
                }
            },
            new NodeArchetype
            {
                TypeID = 9,
                Title = "String",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(string))),
                Description = "Text",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(200, 20),
                DefaultValues = new object[]
                {
                    string.Empty,
                },
                AlternativeTitles = new[]
                {
                    "text",
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(string), 0),
                    NodeElementArchetype.Factory.TextBox(0, 0, 180, TextBox.DefaultHeight, 0, false),
                }
            },
            new NodeArchetype
            {
                TypeID = 10,
                Title = "PI",
                Description = "A value specifying the approximation of π which is 180 degrees",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(50, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "π", typeof(float), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 11,
                Title = "Enum",
                Create = (id, context, arch, groupArch) => new EnumNode(id, context, arch, groupArch),
                Description = "Enum constant value.",
                GetInputOutputDescription = EnumNode.GetInputOutputDescription,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph | NodeFlags.NoSpawnViaGUI,
                Size = new Float2(180, 20),
                DefaultValues = new object[]
                {
                    null, // Value
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 12,
                Title = "Unsigned Integer",
                AlternativeTitles = new[] { "UInt", "U Int" },
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(uint))),
                Description = "Constant unsigned integer value",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(170, 20),
                DefaultValues = new object[]
                {
                    0u
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(uint), 0),
                    NodeElementArchetype.Factory.UnsignedInteger(0, 0, 0, -1, 0, int.MaxValue)
                }
            },
            new NodeArchetype
            {
                TypeID = 13,
                Title = "Array",
                Create = (id, context, arch, groupArch) => new ArrayNode(id, context, arch, groupArch),
                Description = "Constant array value.",
                GetInputOutputDescription = ArrayNode.GetInputOutputDescription,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 20),
                DefaultValues = new object[] { new int[] { 0, 1, 2 } },
                Elements = new[] { NodeElementArchetype.Factory.Output(0, string.Empty, null, 0) }
            },
            new NodeArchetype
            {
                TypeID = 14,
                Title = "Dictionary",
                Create = (id, context, arch, groupArch) => new DictionaryNode(id, context, arch, groupArch),
                Description = "Creates an empty dictionary.",
                GetInputOutputDescription = DictionaryNode.GetInputOutputDescription,
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 40),
                DefaultValues = new object[] { typeof(int).FullName, typeof(string).FullName },
                Elements = new[] { NodeElementArchetype.Factory.Output(0, string.Empty, null, 0) }
            },
            new NodeArchetype
            {
                TypeID = 15,
                Title = "Double",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(double))),
                Description = "Constant floating point",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(110, 20),
                DefaultValues = new object[]
                {
                    0.0d
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(double), 0),
                    NodeElementArchetype.Factory.Float(0, 0, 0)
                },
            },
            new NodeArchetype
            {
                TypeID = 16,
                Title = "Vector2",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Vector2))),
                Description = "Constant Vector2",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(130, 60),
                DefaultValues = new object[]
                {
                    Vector2.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Vector2), 0),
                    NodeElementArchetype.Factory.Output(1, "X", typeof(Real), 1),
                    NodeElementArchetype.Factory.Output(2, "Y", typeof(Real), 2),
                    NodeElementArchetype.Factory.Vector_X(0, 1 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, 2 * Surface.Constants.LayoutOffsetY, 0)
                }
            },
            new NodeArchetype
            {
                TypeID = 17,
                Title = "Vector3",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Vector3))),
                Description = "Constant Vector3",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(130, 80),
                DefaultValues = new object[]
                {
                    Vector3.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Vector3), 0),
                    NodeElementArchetype.Factory.Output(1, "X", typeof(Real), 1),
                    NodeElementArchetype.Factory.Output(2, "Y", typeof(Real), 2),
                    NodeElementArchetype.Factory.Output(3, "Z", typeof(Real), 3),
                    NodeElementArchetype.Factory.Vector_X(0, 1 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, 2 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Z(0, 3 * Surface.Constants.LayoutOffsetY, 0)
                }
            },
            new NodeArchetype
            {
                TypeID = 18,
                Title = "Vector4",
                Create = (id, context, arch, groupArch) => new ConvertToParameterNode(id, context, arch, groupArch, new ScriptType(typeof(Vector4))),
                Description = "Constant Vector4",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(130, 100),
                DefaultValues = new object[]
                {
                    Vector4.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Vector4), 0),
                    NodeElementArchetype.Factory.Output(1, "X", typeof(Real), 1),
                    NodeElementArchetype.Factory.Output(2, "Y", typeof(Real), 2),
                    NodeElementArchetype.Factory.Output(3, "Z", typeof(Real), 3),
                    NodeElementArchetype.Factory.Output(4, "W", typeof(Real), 4),
                    NodeElementArchetype.Factory.Vector_X(0, 1 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Y(0, 2 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_Z(0, 3 * Surface.Constants.LayoutOffsetY, 0),
                    NodeElementArchetype.Factory.Vector_W(0, 4 * Surface.Constants.LayoutOffsetY, 0)
                }
            },
        };

        /// <summary>
        /// Tries to parse a list of numbers separated by commas
        /// </summary>
        private static bool TryParseValues(string filterText, out float[] values)
        {
            float[] vec = new float[4];
            int count = 0;
            if (ExtractNumber(ref filterText, out vec[count]))
            {
                count++;
                while (count < 4)
                {
                    if (ExtractComma(ref filterText) && ExtractNumber(ref filterText, out vec[count]))
                    {
                        count++;
                    }
                    else
                    {
                        break;
                    }
                }

                // If the user inputted something like 3+2.2, it can't be turned into a single node
                if (filterText.TrimEnd().Length > 0)
                {
                    values = null;
                    return false;
                }

                // And return the values
                values = new float[count];
                for (int i = 0; i < values.Length; i++)
                {
                    values[i] = vec[i];
                }
                return true;
            }

            values = null;
            return false;
        }

        private static bool ExtractNumber(ref string filterText, out float number)
        {
            var numberMatcher = new System.Text.RegularExpressions.Regex(@"^([+-]?([0-9]+(\.[0-9]*)?)|(\.[0-9]*))");
            var match = numberMatcher.Match(filterText);
            if (match.Success && float.TryParse(match.Value, out number))
            {
                filterText = filterText.Substring(match.Length);
                return true;
            }
            number = 0;
            return false;
        }

        private static bool ExtractComma(ref string filterText)
        {
            var commaMatcher = new System.Text.RegularExpressions.Regex(@"^([ ]*,[ ]*)");
            var match = commaMatcher.Match(filterText);
            if (match.Success)
            {
                filterText = filterText.Substring(match.Length);
                return true;
            }

            return false;
        }

        private static Float4 ValuesToFloat4(float[] values)
        {
            if (values.Length > 4)
                throw new ArgumentException("Too many values");
            var vector = new Float4();
            for (int i = 0; i < values.Length; i++)
                vector[i] = values[i];
            return vector;
        }
    }
}
