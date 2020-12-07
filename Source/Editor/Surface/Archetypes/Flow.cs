// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Flow group.
    /// </summary>
    [HideInEditor]
    public static class Flow
    {
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
        };
    }
}
