// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Comparisons group.
    /// </summary>
    [HideInEditor]
    public static class Comparisons
    {
        private static NodeArchetype Op(ushort id, string title, string desc)
        {
            return new NodeArchetype
            {
                TypeID = id,
                Title = title,
                Description = desc,
                Flags = NodeFlags.AllGraphs,
                ConnectionsHints = ConnectionsHint.Value,
                Size = new Vector2(100, 40),
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

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            Op(1, "==", "Determines whether two values are equal"),
            Op(2, "!=", "Determines whether two values are not equal"),
            Op(3, ">", "Determines whether the first value is greater than the other"),
            Op(4, "<", "Determines whether the first value is less than the other"),
            Op(5, "<=", "Determines whether the first value is less or equal to the other"),
            Op(6, ">=", "Determines whether the first value is greater or equal to the other"),
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Switch On Bool",
                AlternativeTitles = new[] { "if", "switch" },
                Description = "Returns one of the input values based on the condition value",
                Flags = NodeFlags.AllGraphs,
                Size = new Vector2(160, 60),
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
        };
    }
}
