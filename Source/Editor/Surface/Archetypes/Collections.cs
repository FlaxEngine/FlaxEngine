// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Collections group.
    /// </summary>
    [HideInEditor]
    public static class Collections
    {
        internal static ScriptType GetArrayItemType(Box box, ScriptType type)
        {
            if (type == ScriptType.Null)
                return box.DefaultType;
            return box.DefaultType != null ? new ScriptType(type.GetElementType()) : type;
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                TypeID = 1,
                Title = "Array Length",
                Description = "Gets the length of the arary (amount of the items).",
                AlternativeTitles = new[] { "Count" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 20),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(int), 1)
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Title = "Array Contains",
                Description = "Returns the true if arrayt contains a given item, otherwise false.",
                AlternativeTitles = new[] { "Contains" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 40),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Item", true, typeof(object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 3,
                Title = "Array Index Of",
                Description = "Returns the zero-based index of the item found in the array or -1 if nothing found.",
                AlternativeTitles = new[] { "IndexOf", "Find" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(170, 40),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Item", true, typeof(object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(int), 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 4,
                Title = "Array Last Index Of",
                Description = "Returns the zero-based index of the item found in the array or -1 if nothing found (searches from back to front).",
                AlternativeTitles = new[] { "LastIndexOf", "FindLast" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(170, 40),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Item", true, typeof(object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(int), 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 5,
                Title = "Array Clear",
                Description = "Clears array.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 20),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1)
                }
            },
            new NodeArchetype
            {
                TypeID = 6,
                Title = "Array Remove",
                Description = "Removes the given item from the array.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 40),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1, 2 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Item", true, typeof(object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 7,
                Title = "Array Remove At",
                Description = "Removes an item at the given index from the array.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(170, 40),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 2 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Index", true, typeof(int), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 8,
                Title = "Array Add",
                Description = "Adds the item to the array (to the end).",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 40),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1, 2 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Item", true, typeof(object), 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 9,
                Title = "Array Insert",
                Description = "Inserts the item to the array at the given index.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 60),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1, 3 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Item", true, typeof(object), 1),
                    NodeElementArchetype.Factory.Input(2, "Index", true, typeof(int), 2, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 10,
                Title = "Array Get",
                Description = "Gets the item from the array (at the given zero-based index).",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 40),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 2 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Index", true, typeof(int), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(object), 2)
                }
            },
            new NodeArchetype
            {
                TypeID = 11,
                Title = "Array Set",
                Description = "Sets the item in the array (at the given zero-based index).",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 60),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 2, 3 },
                DependentBoxFilter = GetArrayItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Index", true, typeof(int), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Item", true, typeof(object), 2),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 12,
                Title = "Array Sort",
                Description = "Sorts the items in the array (ascending).",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 20),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1)
                }
            },
            new NodeArchetype
            {
                TypeID = 13,
                Title = "Array Reverse",
                Description = "Reverses the order of the items in the array.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Vector2(150, 20),
                ConnectionsHints = ConnectionsHint.Array,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Array", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1)
                }
            },
            // first 100 IDs reserved for arrays
        };
    }
}
