// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Comparisons group.
    /// </summary>
    [HideInEditor]
    public static class Comparisons
    {
        private static NodeArchetype Op(ushort id, string title, string desc, string[] altTitles = null)
        {
            return new NodeArchetype
            {
                TypeID = id,
                Title = title,
                Description = desc,
                Flags = NodeFlags.AllGraphs,
                AlternativeTitles = altTitles,
                ConnectionsHints = ConnectionsHint.Value,
                Size = new Float2(100, 40),
                IndependentBoxes = new[]
                {
                    0,
                    1
                },
                DefaultValues = new object[]
                {
                    0.0f,
                    0.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, null, 0, 0),
                    NodeElementArchetype.Factory.Input(1, string.Empty, true, null, 1, 1),
                    NodeElementArchetype.Factory.Output(0, title, typeof(bool), 2)
                }
            };
        }

        private class SwitchOnEnumNode : SurfaceNode
        {
            public SwitchOnEnumNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                // Restore saved input boxes layout
                if (Values[0] is byte[] data)
                {
                    for (int i = 0; i < data.Length / 4; i++)
                        AddBox(false, i + 2, i + 1, string.Empty, new ScriptType(), true);
                }
            }

            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateBoxes();
                GetBox(0).CurrentTypeChanged += box => UpdateBoxes();
            }

            public override void ConnectionTick(Box box)
            {
                base.ConnectionTick(box);

                UpdateBoxes();
            }

            private unsafe void UpdateBoxes()
            {
                var valueBox = (InputBox)GetBox(0);
                const int firstBox = 2;
                const int maxBoxes = 40;
                bool isInvalid = false;
                var data = Utils.GetEmptyArray<byte>();

                if (valueBox.HasAnyConnection)
                {
                    var valueType = valueBox.CurrentType;
                    if (valueType.IsEnum)
                    {
                        // Get enum entries
                        var entries = new List<EnumComboBox.Entry>();
                        EnumComboBox.BuildEntriesDefault(valueType.Type, entries);

                        // Setup switch value inputs
                        int id = firstBox;
                        ScriptType type = new ScriptType();
                        for (; id < maxBoxes; id++)
                        {
                            var box = GetBox(id);
                            if (box == null)
                                break;
                            if (box.HasAnyConnection)
                            {
                                type = box.Connections[0].CurrentType;
                                break;
                            }
                        }
                        id = firstBox;
                        for (; id < entries.Count + firstBox; id++)
                        {
                            var e = entries[id - firstBox];
                            var box = AddBox(false, id, id - 1, e.Name, type, true);
                            if (!string.IsNullOrEmpty(e.Tooltip))
                                box.TooltipText = e.Tooltip;
                        }
                        for (; id < maxBoxes; id++)
                        {
                            var box = GetBox(id);
                            if (box == null)
                                break;
                            RemoveElement(box);
                        }

                        // Setup output
                        var outputBox = (OutputBox)GetBox(1);
                        outputBox.CurrentType = type;

                        // Build data about enum entries values for the runtime
                        if (Values[0] is byte[] dataPrev && dataPrev.Length == entries.Count * 4)
                            data = dataPrev;
                        else
                            data = new byte[entries.Count * 4];
                        fixed (byte* dataPtr = data)
                        {
                            int* dataValues = (int*)dataPtr;
                            for (int i = 0; i < entries.Count; i++)
                                dataValues[i] = (int)entries[i].Value;
                        }
                    }
                    else
                    {
                        // Not an enum
                        isInvalid = true;
                    }
                }

                if (isInvalid)
                {
                    // Remove input values boxes if switch value is unused
                    for (int id = firstBox; id < maxBoxes; id++)
                    {
                        var box = GetBox(id);
                        if (box == null)
                            break;
                        RemoveElement(box);
                    }
                }

                Values[0] = data;

                ResizeAuto();
            }
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            Op(1, "==", "Determines whether two values are equal", new[] { "equals" }),
            Op(2, "!=", "Determines whether two values are not equal", new[] { "not equals" }),
            Op(3, ">", "Determines whether the first value is greater than the other", new[] { "greater than", "larger than", "bigger than" }),
            Op(4, "<", "Determines whether the first value is less than the other", new[] { "less than", "smaller than" }),
            Op(5, "<=", "Determines whether the first value is less or equal to the other", new[] { "less equals than", "smaller equals than" }),
            Op(6, ">=", "Determines whether the first value is greater or equal to the other", new[] { "greater equals than", "larger equals than", "bigger equals than" }),
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Switch On Bool",
                AlternativeTitles = new[] { "if", "switch" },
                Description = "Returns one of the input values based on the condition value",
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(160, 60),
                ConnectionsHints = ConnectionsHint.Value,
                IndependentBoxes = new[]
                {
                    1,
                    2,
                },
                DependentBoxes = new[]
                {
                    3,
                },
                DefaultValues = new object[]
                {
                    0.0f,
                    0.0f,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Condition", true, typeof(bool), 0),
                    NodeElementArchetype.Factory.Input(1, "False", true, null, 1, 0),
                    NodeElementArchetype.Factory.Input(2, "True", true, null, 2, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Switch On Enum",
                Create = (id, context, arch, groupArch) => new SwitchOnEnumNode(id, context, arch, groupArch),
                Description = "Returns one of the input values based on the enum value",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(160, 60),
                DefaultValues = new object[] { Utils.GetEmptyArray<byte>() },
                ConnectionsHints = ConnectionsHint.Enum,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Value", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1),
                }
            },
        };
    }
}
