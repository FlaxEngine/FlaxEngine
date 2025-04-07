// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Delegate for node data parsing.
    /// </summary>
    /// <param name="filterText">The filter text.</param>
    /// <param name="data">The node data.</param>
    /// <returns>True if requests has been parsed, otherwise false.</returns>
    [HideInEditor]
    public delegate bool NodeArchetypeTryParseHandler(string filterText, out object[] data);

    /// <summary>
    /// Connections hint.
    /// </summary>
    [Flags]
    [HideInEditor]
    public enum ConnectionsHint
    {
        /// <summary>
        /// Nothing.
        /// </summary>
        None = 0,

        /// <summary>
        /// Allow any scalar value types connections (bool, int, float..).
        /// </summary>
        Scalar = 1,

        /// <summary>
        /// Allow any vector value types connections (vector2, vector3, color...).
        /// </summary>
        Vector = 2,

        /// <summary>
        /// Allow any type connections (scalars, vectors, objects, structures, collections, value types..).
        /// </summary>
        Anything = 4,

        /// <summary>
        /// Allow any value types connections except Void (scalar, vectors, structures, objects..).
        /// </summary>
        Value = 8,

        /// <summary>
        /// Allow any enum types connections.
        /// </summary>
        Enum = 16,

        /// <summary>
        /// Allow any array types connections.
        /// </summary>
        Array = 32,

        /// <summary>
        /// Allow any dictionary types connections.
        /// </summary>
        Dictionary = 64,

        /// <summary>
        /// Allow any scalar or vector numeric value types connections (bool, int, float, vector2, color..).
        /// </summary>
        Numeric = Scalar | Vector,

        /// <summary>
        /// All flags.
        /// </summary>
        All = Scalar | Vector | Enum | Anything | Value | Array | Dictionary,
    }

    /// <summary>
    /// Surface node archetype description.
    /// </summary>
    [HideInEditor]
    public sealed class NodeArchetype : ICloneable
    {
        /// <summary>
        /// Create custom node callback.
        /// </summary>
        /// <param name="id">The node identifier.</param>
        /// <param name="context">The context.</param>
        /// <param name="nodeArch">The node archetype.</param>
        /// <param name="groupArch">The node parent group archetype.</param>
        /// <returns>The created node object.</returns>
        public delegate SurfaceNode CreateCustomNodeFunc(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch);

        /// <summary>
        /// Checks if the given type is compatible with the given node archetype. Used for custom nodes.
        /// </summary>
        public delegate bool IsCompatible(NodeArchetype nodeArch, ScriptType portType, ConnectionsHint hint, VisjectSurfaceContext context);

        /// <summary>
        /// Gets description of inputs and outputs of the archetype. Used for special cases for the description panel.
        /// </summary>
        public delegate void GetElementsDescriptionFunc(NodeArchetype nodeArch, out (string Name, ScriptType Type)[] inputs, out (string Name, ScriptType Type)[] outputs);

        /// <summary>
        /// Unique node type ID within a single group.
        /// </summary>
        public ushort TypeID;

        /// <summary>
        /// Custom create function (may be null).
        /// </summary>
        public CreateCustomNodeFunc Create;

        /// <summary>
        /// Function for asynchronously loaded nodes to check if input ports are compatible, for filtering.
        /// </summary>
        public IsCompatible IsInputCompatible;

        /// <summary>
        /// Function for asynchronously loaded nodes to check if output ports are compatible, for filtering.
        /// </summary>
        public IsCompatible IsOutputCompatible;

        /// <summary>
        /// Default initial size of the node.
        /// </summary>
        public Float2 Size;

        /// <summary>
        /// Custom set of flags.
        /// </summary>
        public NodeFlags Flags;

        /// <summary>
        /// Title text.
        /// </summary>
        public string Title;

        /// <summary>
        /// The additional title text to display next to the node title in the context menu when spawning a node.
        /// </summary>
        public string SubTitle;

        /// <summary>
        /// Node signature for tooltip and description purposes
        /// </summary>
        public string Signature;

        /// <summary>
        /// Short node description.
        /// </summary>
        public string Description;

        /// <summary>
        /// Custom function to get descriptions of input and output elements. Used for description panel (optional).
        /// </summary>
        public GetElementsDescriptionFunc GetInputOutputDescription;

        /// <summary>
        /// Alternative node titles.
        /// </summary>
        public string[] AlternativeTitles;

        /// <summary>
        /// The custom tag.
        /// </summary>
        public object Tag;

        /// <summary>
        /// Custom score value to use when sorting node archetypes in Editor. If positive (eg. 1, 2) can be used to add more importance for a specific node type.
        /// </summary>
        public float SortScore;

        /// <summary>
        /// Default node values. This array supports types: bool, int, float, Vector2, Vector3, Vector4, Color, Rectangle, Guid, string, Matrix and byte[].
        /// </summary>
        /// <remarks>
        /// The limit for the node values array is 32 (must match GRAPH_NODE_MAX_VALUES in C++ engine core).
        /// </remarks>
        public object[] DefaultValues;

        /// <summary>
        /// Default connections type for dependant boxes.
        /// </summary>
        public ScriptType DefaultType;

        /// <summary>
        /// Connections hints.
        /// </summary>
        public ConnectionsHint ConnectionsHints;

        /// <summary>
        /// Array with independent boxes IDs.
        /// </summary>
        public int[] IndependentBoxes;

        /// <summary>
        /// Array with dependent boxes IDs.
        /// </summary>
        public int[] DependentBoxes;

        /// <summary>
        /// Custom function to convert type for dependant box (optional).
        /// </summary>
        public Func<Elements.Box, ScriptType, ScriptType> DependentBoxFilter;

        /// <summary>
        /// Array with default elements descriptions.
        /// </summary>
        public NodeElementArchetype[] Elements;

        /// <summary>
        /// Tries to parse some text and extract the data from it.
        /// </summary>
        public NodeArchetypeTryParseHandler TryParseText;

        /// <inheritdoc />
        public object Clone()
        {
            return new NodeArchetype
            {
                TypeID = TypeID,
                Create = Create,
                IsInputCompatible = IsInputCompatible,
                IsOutputCompatible = IsOutputCompatible,
                Size = Size,
                Flags = Flags,
                Title = Title,
                SubTitle = SubTitle,
                Signature = Signature,
                Description = Description,
                GetInputOutputDescription = GetInputOutputDescription,
                AlternativeTitles = (string[])AlternativeTitles?.Clone(),
                Tag = Tag,
                SortScore = SortScore,
                DefaultValues = (object[])DefaultValues?.Clone(),
                DefaultType = DefaultType,
                ConnectionsHints = ConnectionsHints,
                IndependentBoxes = (int[])IndependentBoxes?.Clone(),
                DependentBoxes = (int[])DependentBoxes?.Clone(),
                DependentBoxFilter = DependentBoxFilter,
                Elements = (NodeElementArchetype[])Elements?.Clone(),
                TryParseText = TryParseText,
            };
        }
    }
}
