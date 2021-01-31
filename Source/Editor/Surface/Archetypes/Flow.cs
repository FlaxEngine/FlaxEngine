// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;

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

            public override void OnLoaded()
            {
                base.OnLoaded();

                // Restore saved output boxes layout
                var count = (int)Values[0];
                for (int i = 0; i < count; i++)
                    AddBox(true, i + 1, i, string.Empty, new ScriptType(typeof(void)), true);
            }

            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

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

                _addButton.Location = new Vector2(Width - _addButton.Width - FlaxEditor.Surface.Constants.NodeMarginX, Height - 20 - FlaxEditor.Surface.Constants.NodeMarginY - FlaxEditor.Surface.Constants.NodeFooterSize);
                _removeButton.Location = new Vector2(_addButton.X - _removeButton.Width - 4, _addButton.Y);
            }
        }

        private class SwitchIntNode : SurfaceNode
        {
            private enum SwitchNodeState
            {
                Default,
                AddCase,
                RemoveCase,
                UpdateMapData
            }

            private class CaseTextBox : IntValueBox
            {
                /// <summary>
                /// Gets called when the value gets updated
                /// </summary>
                public event EventHandler<int> CaseValueUpdated;

                public CaseTextBox(int caseValue, float x, float y, float width = 120) : base(caseValue, x, y, width)
                {
                }

                protected override void OnValueChanged()
                {
                    base.OnValueChanged();

                    CaseValueUpdated?.Invoke(this, this.Value);
                }
            }

            private class CaseTuple
            {
                public CaseTuple(CaseTextBox label, Box box)
                {
                    CaseBox = label;
                    Box = box;
                }

                public CaseTextBox CaseBox;
                public Box Box;
            }

            private Button _addButton;
            private Button _removeButton;
            private List<CaseTuple> _cases = new List<CaseTuple>();
            private bool _reinitialize = true;
            private Box _defaultBox = null;
            private SwitchNodeState _currentAction;

            public SwitchIntNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

                _removeButton = new Button(0, 0, 20, 20)
                {
                    Text = "-",
                    TooltipText = "Remove last sequence output",
                    Parent = this
                };
                _removeButton.Clicked += () =>
                {
                    _currentAction = SwitchNodeState.RemoveCase;
                    _reinitialize = true;
                    SetValue(0, (int)Values[0] - 1);
                };

                _addButton = new Button(0, 0, 20, 20)
                {
                    Text = "+",
                    TooltipText = "Add sequence output",
                    Parent = this
                };
                _addButton.Clicked += () =>
                {
                    _currentAction = SwitchNodeState.AddCase;
                    _reinitialize = true;
                    SetValue(0, (int)Values[0] + 1);
                };

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

                if (_currentAction != SwitchNodeState.UpdateMapData)
                    UpdateUI();
            }

            private void UpdateUI()
            {
                // Generic
                var outPinCount = (int)Values[0]; // Total pin count
                var outPinMin = 0;   // Min pin count
                var outPinMax = 101; // Max pin count

                if (_reinitialize)
                {
                    this.Initialize();
                    _reinitialize = false;
                }

                _addButton.Enabled = outPinCount < outPinMax && Surface.CanEdit;
                _removeButton.Enabled = outPinCount > outPinMin && Surface.CanEdit;

                if (!(outPinCount <= 1))
                {
                    Resize(170, 60 + (outPinCount - 1) * 20);
                }

                _addButton.Location = new Vector2(Width - _addButton.Width - FlaxEditor.Surface.Constants.NodeMarginX, Height - 20 - FlaxEditor.Surface.Constants.NodeMarginY - FlaxEditor.Surface.Constants.NodeFooterSize);
                _removeButton.Location = new Vector2(_addButton.X - _removeButton.Width - 4, _addButton.Y);

                // Re-position default case label
                ChangeBoxYLevel(_defaultBox, _cases.Count);
            }

            private void Initialize()
            {
                // Generic
                var outPinCount = (int)Values[0]; // Total pin count
                var outPinMin = 0;   // Min pin count
                var outPinMax = 101; // Max pin count

                // Default box
                Box targetBox = null;
                bool hasConnection = false;

                // Add default pin
                if (_defaultBox == null)
                    _defaultBox = AddBox(true, 102, outPinCount, "Default", new ScriptType(typeof(void)), true);

                if (_currentAction == SwitchNodeState.Default)
                {
                    for (int i = 0; i < outPinCount; i++)
                        CreateCase(i);

                    UpdateMapData();
                }
                else if (_currentAction == SwitchNodeState.AddCase)
                {
                    CreateCase(outPinCount - 1);
                    UpdateMapData();
                }
                else if (_currentAction == SwitchNodeState.RemoveCase)
                {
                    var lastCase = _cases[_cases.Count - 1];
                    RemoveCase(lastCase);
                    UpdateMapData();
                }

                _currentAction = SwitchNodeState.Default;

                void CreateCase(int caseIndex)
                {
                    // Output event pin
                    var box = AddBox(true, int.MaxValue - caseIndex, caseIndex, string.Empty, new ScriptType(typeof(void)), true);

                    // The case int
                    var caseBox = new CaseTextBox(caseIndex, box.Location.X - 50, box.Location.Y, 50)
                    {
                        TooltipText = "Add sequence output",
                        Parent = this
                    };

                    var caseTuple = new CaseTuple(caseBox, box);

                    caseBox.CaseValueUpdated += Case_ValueUpdated;

                    _cases.Add(new CaseTuple(caseBox, box));
                }
            }

            private void UpdateMapData()
            {
                _currentAction = SwitchNodeState.UpdateMapData;

                var tupleBuffer = new CaseTuple[_cases.Count];
                var processedValues = new HashSet<int>();
                var arrayIndex = 0;
                var caseCount = 0;

                using (var stream = new MemoryStream())
                using (var writer = new BinaryWriter(stream))
                {
                    writer.Write((byte)1); // Version

                    writer.Write(0); // Case count

                    for (int i = 0; i < _cases.Count; i++)
                    {
                        var caseTuple = _cases[i];
                        int caseValue = caseTuple.CaseBox.Value;

                        if (processedValues.Contains(caseValue))
                            continue;

                        CollectTuplesForCaseValue(caseValue);

                        if (arrayIndex == 0)
                            continue;

                        writer.Write(caseValue); // Case value
                        writer.Write(arrayIndex); // Case box id for the value

                        for (int j = 0; j < arrayIndex; j++)
                        {
                            var processedCaseTuple = tupleBuffer[j];

                            writer.Write(processedCaseTuple.Box.ID); // Box ID
                        }

                        caseCount++;
                        processedValues.Add(caseValue);
                    }

                    writer.BaseStream.Position = 1;
                    writer.Write(caseCount);

                    SetValue(2, stream.ToArray());
                }

                _currentAction = SwitchNodeState.Default;

                void CollectTuplesForCaseValue(int caseValue)
                {
                    arrayIndex = 0;

                    for (int i = 0; i < _cases.Count; i++)
                    {
                        var caseTuple = _cases[i];

                        if (caseTuple.CaseBox.Value == caseValue)
                        {
                            tupleBuffer[arrayIndex] = caseTuple;
                            arrayIndex++;
                        }
                    }
                }
            }

            private void Case_ValueUpdated(object sender, int newValue)
            {
                CaseTextBox caseBox = (CaseTextBox)sender;

                if (IsCaseValueExisting(caseBox, caseBox.Value))
                {
                    // TODO: Figure out what to do with this shit
                    Debug.Logger.Log("Case already exists!");
                }
            }

            /// <summary>
            /// Checks if a specific switch case value is already existing
            /// </summary>
            /// <param name="sender">case textbox sender</param>
            /// <param name="caseValue">value to be checked</param>
            /// <returns>whether the case value is already existing or not</returns>
            private bool IsCaseValueExisting(CaseTextBox sender, int caseValue)
            {
                for (int i = _cases.Count - 1; i >= 0; i--)
                {
                    var tuple = _cases[i];

                    if (tuple.CaseBox != sender && tuple.CaseBox.Value == caseValue)
                        return true;
                }

                return false;
            }

            /// <summary>
            /// Remove all old cases
            /// </summary>
            private void RemoveOldCases()
            {
                for (int i = _cases.Count - 1; i >= 0; i--)
                {
                    var tuple = _cases[i];
                    RemoveCase(tuple);
                }
            }

            /// <summary>
            /// Remove case
            /// </summary>
            /// <param name="caseTuple">case to remove</param>
            private void RemoveCase(CaseTuple caseTuple)
            {
                caseTuple.CaseBox.CaseValueUpdated -= Case_ValueUpdated;
                this.RemoveChild(caseTuple.CaseBox);
                this.RemoveElement(caseTuple.Box);
                _cases.Remove(caseTuple);
            }
        }

        private class BranchOnEnumNode : SurfaceNode
        {
            public BranchOnEnumNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public override void OnLoaded()
            {
                base.OnLoaded();

                // Restore saved output boxes layout
                if (Values[0] is byte[] data)
                {
                    for (int i = 0; i < data.Length / 4; i++)
                        AddBox(true, i + 2, i, string.Empty, new ScriptType(typeof(void)), true);
                }
            }

            public override void OnSurfaceLoaded()
            {
                base.OnSurfaceLoaded();

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
                                dataValues[i] = entries[i].Value;
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
                Size = new Vector2(160, 40),
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
                Size = new Vector2(160, 80),
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
                Size = new Vector2(160, 80),
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
                Size = new Vector2(140, 80),
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
                Size = new Vector2(160, 60),
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
                Title = "Switch on Int",
                Create = (id, context, arch, groupArch) => new SwitchIntNode(id, context, arch, groupArch),
                Description = "Executes the output that matches the input value, if not found, default is executed.",
                Flags = NodeFlags.VisualScriptGraph,
                Size = new Vector2(170, 80),
                // Output count, default value for the case value
                DefaultValues = new object[] { 2, 0, new byte[0] },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, false, typeof(void), 0),
                    // 102 is reserved for default, 1-101 are reserved for the output pins.
                    NodeElementArchetype.Factory.Input(1, "Int", true, typeof(int), 103, 1),
                }
            },
        };
    }
}
