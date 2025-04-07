// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Boolean group.
    /// </summary>
    [HideInEditor]
    public static class Boolean
    {
        private static NodeArchetype Op1(ushort id, string title, string desc, string[] altTitles = null)
        {
            return new NodeArchetype
            {
                TypeID = id,
                Title = title,
                Description = desc,
                AlternativeTitles = altTitles,
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(140, 20),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "A", true, typeof(bool), 0),
                    NodeElementArchetype.Factory.Output(0, "Result", typeof(bool), 1)
                }
            };
        }

        private static NodeArchetype Op2(ushort id, string title, string desc, string[] altTitles = null)
        {
            return new NodeArchetype
            {
                TypeID = id,
                Title = title,
                Description = desc,
                AlternativeTitles = altTitles,
                Flags = NodeFlags.AllGraphs,
                Size = new Float2(140, 40),
                DefaultValues = new object[]
                {
                    false,
                    false,
                },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "A", true, typeof(bool), 0, 0),
                    NodeElementArchetype.Factory.Input(1, "B", true, typeof(bool), 1, 1),
                    NodeElementArchetype.Factory.Output(0, "Result", typeof(bool), 2)
                }
            };
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            Op1(1, "Boolean NOT", "Negates the boolean value", new[] { "!", "~" }),
            Op2(2, "Boolean AND", "Performs a logical conjunction on two values. Returns true if both of its operands are true", new[] { "&&" }),
            Op2(3, "Boolean OR", "Performs a logical disjunction on two values. Returns true if either (or both) of its operands is true", new[] { "||" }),
            Op2(4, "Boolean XOR", "Performs a logical exclusive disjunction on two values. Returns true if both of its operands are different", new[] { "^" }),
            Op2(5, "Boolean NOR", "Performs a logical disjunction on two values and negates it. Returns true if both of its operands are false"),
            Op2(6, "Boolean NAND", "Performs a logical conjunction on two values and negates it. Returns true if either (or both) of its operands are false"),
        };
    }
}
