// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
                var countMax = 16;
                for (int i = 0; i < count; i++)
                {
                    var box = GetBox(i + 1);
                    box.Visible = true;
                }
                for (int i = count; i <= countMax; i++)
                {
                    var box = GetBox(i + 1);
                    box.RemoveConnections();
                    box.Visible = false;
                }

                _addButton.Enabled = count < countMax && Surface.CanEdit;
                _removeButton.Enabled = count > countMin && Surface.CanEdit;

                Resize(140, 40 + (count - 1) * 20);

                _addButton.Location = new Vector2(Width - _addButton.Width - FlaxEditor.Surface.Constants.NodeMarginX, Height - 20 - FlaxEditor.Surface.Constants.NodeMarginY - FlaxEditor.Surface.Constants.NodeFooterSize);
                _removeButton.Location = new Vector2(_addButton.X - _removeButton.Width - 4, _addButton.Y);
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
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(void), 1, true),
                    NodeElementArchetype.Factory.Output(1, string.Empty, typeof(void), 2, true),
                    NodeElementArchetype.Factory.Output(2, string.Empty, typeof(void), 3, true),
                    NodeElementArchetype.Factory.Output(3, string.Empty, typeof(void), 4, true),
                    NodeElementArchetype.Factory.Output(4, string.Empty, typeof(void), 5, true),
                    NodeElementArchetype.Factory.Output(5, string.Empty, typeof(void), 6, true),
                    NodeElementArchetype.Factory.Output(6, string.Empty, typeof(void), 7, true),
                    NodeElementArchetype.Factory.Output(7, string.Empty, typeof(void), 8, true),
                    NodeElementArchetype.Factory.Output(8, string.Empty, typeof(void), 9, true),
                    NodeElementArchetype.Factory.Output(9, string.Empty, typeof(void), 10, true),
                    NodeElementArchetype.Factory.Output(10, string.Empty, typeof(void), 11, true),
                    NodeElementArchetype.Factory.Output(11, string.Empty, typeof(void), 12, true),
                    NodeElementArchetype.Factory.Output(12, string.Empty, typeof(void), 13, true),
                    NodeElementArchetype.Factory.Output(13, string.Empty, typeof(void), 14, true),
                    NodeElementArchetype.Factory.Output(14, string.Empty, typeof(void), 15, true),
                    NodeElementArchetype.Factory.Output(15, string.Empty, typeof(void), 16, true),
                    NodeElementArchetype.Factory.Output(16, string.Empty, typeof(void), 17, true),
                }
            },
        };
    }
}
