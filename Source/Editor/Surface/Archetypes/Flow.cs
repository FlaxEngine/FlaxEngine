// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Flow group.
    /// </summary>
    [HideInEditor]
    public static class Flow
    {
        private class SequenceNode : SurfaceNode
        {
            private Button _addButton;
            private Button _removeButton;

            public SequenceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                // Restore saved output boxes layout
                var count = (int)Values[0];
                for (int i = 0; i < count; i++)
                    AddBox(true, i + 1, i, string.Empty, new ScriptType(typeof(void)), true);
            }

            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                _removeButton = new Button(0, 0, 20, 20)
                {
                    Text = "-",
                    TooltipText = "Remove last sequence output",
                    Parent = this
                };
                _removeButton.Clicked += () => SetValue(0, (int)Values[0] - 1);

                _addButton = new Button(0, 0, 20, 20)
                {
                    Text = "+",
                    TooltipText = "Add sequence output",
                    Parent = this
                };
                _addButton.Clicked += () => SetValue(0, (int)Values[0] + 1);

                UpdateUI();
            }

            public override void OnSurfaceCanEditChanged(bool canEdit)
            {
                base.OnSurfaceCanEditChanged(canEdit);

                _addButton.Enabled = canEdit;
                _removeButton.Enabled = canEdit;
                UpdateUI();
            }

            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                UpdateUI();
            }

            private void UpdateUI()
            {
                var count = (int)Values[0];
                var countMin = 0;
                var countMax = 20;
                for (int i = 0; i < count; i++)
                    AddBox(true, i + 1, i, string.Empty, new ScriptType(typeof(void)), true);
                for (int i = count; i <= countMax; i++)
                {
                    var box = GetBox(i + 1);
                    if (box == null)
                        break;
                    RemoveElement(box);
                }

                _addButton.Enabled = count < countMax && Surface.CanEdit;
                _removeButton.Enabled = count > countMin && Surface.CanEdit;

                Resize(140, 40 + (count - 1) * 20);

                _addButton.Location = new Float2(Width - _addButton.Width - FlaxEditor.Surface.Constants.NodeMarginX, Height - 20 - FlaxEditor.Surface.Constants.NodeMarginY - FlaxEditor.Surface.Constants.NodeFooterSize);
                _removeButton.Location = new Float2(_addButton.X - _removeButton.Width - 4, _addButton.Y);
            }
        }


        private class BranchOnEnumNode : SurfaceNode
        {
            public BranchOnEnumNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnLoaded(SurfaceNodeActions action)
            {
                base.OnLoaded(action);

                // Restore saved output boxes layout
                if (Values[0] is byte[] data)
                {
                    for (int i = 0; i < data.Length / 4; i++)
                        AddBox(true, i + 2, i, string.Empty, new ScriptType(typeof(void)), true);
                }
            }

            public override void OnSurfaceLoaded(SurfaceNodeActions action)
            {
                base.OnSurfaceLoaded(action);

                UpdateBoxes();
                GetBox(1).CurrentTypeChanged += box => UpdateBoxes();
            }

            public override void ConnectionTick(Box box)
            {
                base.ConnectionTick(box);

                UpdateBoxes();
            }

            private unsafe void UpdateBoxes()
            {
                var valueBox = (InputBox)GetBox(1);
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
                        for (; id < entries.Count + firstBox; id++)
                        {
                            var e = entries[id - firstBox];
                            var box = AddBox(true, id, id - firstBox, e.Name, new ScriptType(typeof(void)), true);
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
            new NodeArchetype
            {
                TypeID = 1,
                Title = "If",
                AlternativeTitles = new[] { "branch" },
                Description = "Branches the logic flow into two paths based on the conditional value.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(160, 40),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Condition", true, typeof(bool), 1),
                    NodeElementArchetype.Factory.Output(0, "True", typeof(void), 2, true),
                    NodeElementArchetype.Factory.Output(1, "False", typeof(void), 3, true),
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Title = "For Loop",
                AlternativeTitles = new[] { "loop" },
                Description = "Iterates over the range of indices, starting from first index and with given iterations count.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(160, 80),
                DefaultValues = new object[] { 0, 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Start", true, typeof(int), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Count", true, typeof(int), 2, 1),
                    NodeElementArchetype.Factory.Input(3, "Break", false, typeof(void), 3),
                    NodeElementArchetype.Factory.Output(0, "Loop", typeof(void), 4, true),
                    NodeElementArchetype.Factory.Output(1, "Index", typeof(int), 5),
                    NodeElementArchetype.Factory.Output(2, "Done", typeof(void), 6, true),
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "While Loop",
                Description = "Iterates in the loop until the given condition is True.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(160, 80),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Condition", true, typeof(bool), 1),
                    NodeElementArchetype.Factory.Input(2, "Break", false, typeof(void), 2),
                    NodeElementArchetype.Factory.Output(0, "Loop", typeof(void), 3, true),
                    NodeElementArchetype.Factory.Output(1, "Index", typeof(int), 4),
                    NodeElementArchetype.Factory.Output(2, "Done", typeof(void), 5, true),
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Sequence",
                Create = (id, context, arch, groupArch) => new SequenceNode(id, context, arch, groupArch),
                Description = "Performs a series of actions in a sequence (one after the another).",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(140, 80),
                DefaultValues = new object[] { 2 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Branch On Enum",
                Create = (id, context, arch, groupArch) => new BranchOnEnumNode(id, context, arch, groupArch),
                Description = "Performs the flow logic branch based on the enum value",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(160, 60),
                DefaultValues = new object[] { Utils.GetEmptyArray<byte>() },
                ConnectionsHints = ConnectionsHint.Enum,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Value", true, null, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Delay",
                Description = "Delays the graph execution. If delay is 0 then it will pass though.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(150, 40),
                DefaultValues = new object[] { 1.0f },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Duration", true, typeof(float), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(void), 2, true),
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Array For Each",
                AlternativeTitles = new[] { "foreach" },
                Description = "Iterates over the array items.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(160, 80),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 1 },
                DependentBoxes = new int[] { 4, },
                DependentBoxFilter = Collections.GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Array", true, null, 1),
                    NodeElementArchetype.Factory.Input(2, "Break", false, typeof(void), 2),
                    NodeElementArchetype.Factory.Output(0, "Loop", typeof(void), 3, true),
                    NodeElementArchetype.Factory.Output(1, "Item", typeof(object), 4),
                    NodeElementArchetype.Factory.Output(2, "Index", typeof(int), 5),
                    NodeElementArchetype.Factory.Output(3, "Done", typeof(void), 6, true),
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Dictionary For Each",
                AlternativeTitles = new[] { "foreach" },
                Description = "Iterates over the dictionary items.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Float2(180, 80),
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 4 },
                DependentBoxes = new int[] { 1, 2, },
                DependentBoxFilter = Collections.GetDictionaryItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    NodeElementArchetype.Factory.Input(1, "Dictionary", true, null, 4),
                    NodeElementArchetype.Factory.Input(2, "Break", false, typeof(void), 5),
                    NodeElementArchetype.Factory.Output(0, "Loop", typeof(void), 3, true),
                    NodeElementArchetype.Factory.Output(1, "Key", typeof(object), 1),
                    NodeElementArchetype.Factory.Output(2, "Value", typeof(object), 2),
                    NodeElementArchetype.Factory.Output(3, "Done", typeof(void), 6, true),
                }
            },
        };
    }
}
