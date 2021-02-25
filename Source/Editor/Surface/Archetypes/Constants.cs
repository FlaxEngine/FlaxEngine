// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Constants group.
    /// </summary>
    [HideInEditor]
    public static class Constants
    {
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

            public override void OnLoaded()
            {
                base.OnLoaded();

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
                Description = "Constant boolean value",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(110, 20),
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
                    if (filterText == "true")
                    {
                        data = new object[] { true };
                        return true;
                    }
                    if (filterText == "false")
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
                Description = "Constant integer value",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(110, 20),
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
                Description = "Constant floating point",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(110, 20),
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
                Title = "Vector2",
                Description = "Constant Vector2",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(130, 60),
                DefaultValues = new object[]
                {
                    Vector2.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Vector2), 0),
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
                        data = new object[] { new Vector2(ValuesToVector4(values)) };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Vector3",
                Description = "Constant Vector3",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(130, 80),
                DefaultValues = new object[]
                {
                    Vector3.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Vector3), 0),
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
                        data = new object[] { new Vector3(ValuesToVector4(values)) };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Vector4",
                Description = "Constant Vector4",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(130, 100),
                DefaultValues = new object[]
                {
                    Vector4.Zero
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, "Value", typeof(Vector4), 0),
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
                        data = new object[] { ValuesToVector4(values) };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Color",
                Description = "RGBA color",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(70, 100),
                DefaultValues = new object[]
                {
                    Color.White
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(Vector4), 0),
                    NodeElementArchetype.Factory.Output(1, "R", typeof(float), 1),
                    NodeElementArchetype.Factory.Output(2, "G", typeof(float), 2),
                    NodeElementArchetype.Factory.Output(3, "B", typeof(float), 3),
                    NodeElementArchetype.Factory.Output(4, "A", typeof(float), 4),
                    NodeElementArchetype.Factory.Color(0, 0, 0)
                },
                TryParseText = (string filterText, out object[] data) =>
                {
                    data = null;
                    if (!filterText.StartsWith("#"))
                        return false;
                    if (Color.TryParseHex(filterText, out var color))
                    {
                        data = new object[] { color };
                        return true;
                    }
                    return false;
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Rotation",
                Description = "Euler angle rotation",
                Flags = NodeFlags.AnimGraph | NodeFlags.VisualScriptGraph | NodeFlags.ParticleEmitterGraph,
                Size = new Vector2(110, 60),
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
                Description = "Text",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(200, 20),
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
                Size = new Vector2(50, 20),
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
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.NoSpawnViaGUI,
                Size = new Vector2(180, 20),
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
                Description = "Constant unsigned integer value",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(170, 20),
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

        private static Vector4 ValuesToVector4(float[] values)
        {
            if (values.Length > 4)
            {
                throw new ArgumentException("Too many values");
            }
            Vector4 vector = new Vector4();
            for (int i = 0; i < values.Length; i++)
            {
                vector[i] = values[i];
            }

            return vector;
        }
    }
}
