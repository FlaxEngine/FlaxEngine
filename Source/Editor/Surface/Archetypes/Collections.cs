// Copyright (c) Wojciech Figat. All rights reserved.

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
            return box.DefaultType != null ? type.GetElementType() : type;
        }

        internal static ScriptType GetDictionaryItemType(Box box, ScriptType type)
        {
            if (type == ScriptType.Null)
                return box.DefaultType;
            // BoxID = 1 is Key
            // BoxID = 2 is Value
            return box.DefaultType != null ? new ScriptType(type.GetGenericArguments()[box.ID == 1 ? 0 : 1]) : type;
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
                Description = "Gets the length of the array (amount of the items).",
                AlternativeTitles = new[] { "Count" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 20),
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
                Description = "Returns the true if array contains a given item, otherwise false.",
                AlternativeTitles = new[] { "Contains" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(150, 40),
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
                Size = new Float2(170, 40),
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
                Size = new Float2(170, 40),
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
                Size = new Float2(150, 20),
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
                Size = new Float2(150, 40),
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
                Size = new Float2(170, 40),
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
                Size = new Float2(150, 40),
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
                Size = new Float2(150, 60),
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
                Size = new Float2(150, 40),
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
                Size = new Float2(150, 60),
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
                Size = new Float2(150, 20),
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
                Size = new Float2(150, 20),
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
                TypeID = 14,
                Title = "Array Add Unique",
                Description = "Adds the unique item to the array (to the end). Does nothing it specified item was already added.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(170, 40),
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
            // first 100 IDs reserved for arrays

            new NodeArchetype
            {
                TypeID = 101,
                Title = "Dictionary Count",
                Description = "Gets the size of the dictionary (amount of the items).",
                AlternativeTitles = new[] { "size" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(180, 20),
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(int), 1)
                }
            },
            new NodeArchetype
            {
                TypeID = 102,
                Title = "Dictionary Contains Key",
                Description = "Returns the true if dictionary contains a given key, otherwise false.",
                AlternativeTitles = new[] { "Contains" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(240, 40),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                DependentBoxFilter = GetDictionaryItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Key", true, typeof(object), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 103,
                Title = "Dictionary Contains Value",
                Description = "Returns the true if dictionary contains a given value, otherwise false.",
                AlternativeTitles = new[] { "Contains" },
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(240, 40),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 2 },
                DependentBoxFilter = GetDictionaryItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Value", true, typeof(object), 2, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(bool), 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 104,
                Title = "Dictionary Clear",
                Description = "Clears dictionary.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(180, 20),
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1 },
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 1)
                }
            },
            new NodeArchetype
            {
                TypeID = 105,
                Title = "Dictionary Remove",
                Description = "Removes the given item from the dictionary (by key).",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(180, 40),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1, 3 },
                DependentBoxFilter = GetDictionaryItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Key", true, typeof(object), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 106,
                Title = "Dictionary Set",
                Description = "Set the item in the dictionary (a pair of key and value). Adds or updates the pair.",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(180, 60),
                DefaultValues = new object[] { 0, 0 },
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1, 2, 3 },
                DependentBoxFilter = GetDictionaryItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Key", true, typeof(object), 1, 0),
                    NodeElementArchetype.Factory.Input(2, "Value", true, typeof(object), 2, 1),
                    NodeElementArchetype.Factory.Output(0, string.Empty, null, 3)
                }
            },
            new NodeArchetype
            {
                TypeID = 107,
                Title = "Dictionary Get",
                Description = "Gets the item from the dictionary (a pair of key and value).",
                Flags = NodeFlags.VisualScriptGraph | NodeFlags.AnimGraph,
                Size = new Float2(180, 40),
                DefaultValues = new object[] { 0 },
                ConnectionsHints = ConnectionsHint.Dictionary,
                IndependentBoxes = new int[] { 0 },
                DependentBoxes = new int[] { 1, 3 },
                DependentBoxFilter = GetDictionaryItemType,
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, "Dictionary", true, null, 0),
                    NodeElementArchetype.Factory.Input(1, "Key", true, typeof(object), 1, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, typeof(object), 3)
                }
            },
            // second 100 IDs reserved for dictionaries
        };
    }
}
