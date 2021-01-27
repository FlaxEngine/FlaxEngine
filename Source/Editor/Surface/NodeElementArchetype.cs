// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.CustomEditors;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Surface node element archetype description.
    /// </summary>
    [HideInEditor]
    public sealed class NodeElementArchetype
    {
        /// <summary>
        /// The element type.
        /// </summary>
        public NodeElementType Type;

        /// <summary>
        /// Default element position in node that has default size.
        /// </summary>
        public Vector2 Position;

        /// <summary>
        /// The element size for some types.
        /// </summary>
        public Vector2 Size;

        /// <summary>
        /// Custom text value.
        /// </summary>
        public string Text;

        /// <summary>
        /// Control tooltip text.
        /// </summary>
        public string Tooltip;

        /// <summary>
        /// True if use single connections (for Input element).
        /// </summary>
        public bool Single;

        /// <summary>
        /// Index of the node value that is connected with that element.
        /// </summary>
        public int ValueIndex;

        /// <summary>
        /// The minimum value.
        /// </summary>
        public float ValueMin;

        /// <summary>
        /// The maximum value.
        /// </summary>
        public float ValueMax;

        /// <summary>
        /// Unique ID of the box in the graph data to link it to this element (Output/Input elements).
        /// </summary>
        public int BoxID;

        /// <summary>
        /// Default connections type for that element (can be set of types).
        /// </summary>
        public ScriptType ConnectionsType;

        /// <summary>
        /// Gets the actual element position on the x axis.
        /// </summary>
        public float ActualPositionX => Position.X + Constants.NodeMarginX;

        /// <summary>
        /// Gets the actual element position on the y axis.
        /// </summary>
        public float ActualPositionY => Position.Y + Constants.NodeMarginY + Constants.NodeHeaderSize;

        /// <summary>
        /// Gets the actual element position.
        /// </summary>
        public Vector2 ActualPosition => new Vector2(Position.X + Constants.NodeMarginX, Position.Y + Constants.NodeMarginY + Constants.NodeHeaderSize);

        /// <summary>
        /// Node element archetypes factory object. Helps to build surface nodes archetypes.
        /// </summary>
        public static class Factory
        {
            /// <summary>
            /// Creates new Input box element description.
            /// </summary>
            /// <param name="yLevel">The y level.</param>
            /// <param name="text">The text.</param>
            /// <param name="single">If true then box can have only one connection, otherwise multiple connections are allowed.</param>
            /// <param name="type">The type.</param>
            /// <param name="id">The unique box identifier.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Input(float yLevel, string text, bool single, ScriptType type, int id, int valueIndex = -1)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Input,
                    Position = new Vector2(
                                           Constants.NodeMarginX - Constants.BoxOffsetX,
                                           Constants.NodeMarginY + Constants.NodeHeaderSize + yLevel * Constants.LayoutOffsetY),
                    Text = text,
                    Single = single,
                    ValueIndex = valueIndex,
                    BoxID = id,
                    ConnectionsType = type
                };
            }

            /// <summary>
            /// Creates new Input box element description.
            /// </summary>
            /// <param name="yLevel">The y level.</param>
            /// <param name="text">The text.</param>
            /// <param name="single">If true then box can have only one connection, otherwise multiple connections are allowed.</param>
            /// <param name="type">The type.</param>
            /// <param name="id">The unique box identifier.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Input(float yLevel, string text, bool single, Type type, int id, int valueIndex = -1)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Input,
                    Position = new Vector2(
                                           Constants.NodeMarginX - Constants.BoxOffsetX,
                                           Constants.NodeMarginY + Constants.NodeHeaderSize + yLevel * Constants.LayoutOffsetY),
                    Text = text,
                    Single = single,
                    ValueIndex = valueIndex,
                    BoxID = id,
                    ConnectionsType = new ScriptType(type)
                };
            }

            /// <summary>
            /// Creates new Output box element description.
            /// </summary>
            /// <param name="yLevel">The y level.</param>
            /// <param name="text">The text.</param>
            /// <param name="type">The type.</param>
            /// <param name="id">The unique box identifier.</param>
            /// <param name="single">If true then box can have only one connection, otherwise multiple connections are allowed.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Output(float yLevel, string text, ScriptType type, int id, bool single = false)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Output,
                    Position = new Vector2(
                                           Constants.NodeMarginX - Constants.BoxSize + Constants.BoxOffsetX,
                                           Constants.NodeMarginY + Constants.NodeHeaderSize + yLevel * Constants.LayoutOffsetY),
                    Text = text,
                    Single = single,
                    ValueIndex = -1,
                    BoxID = id,
                    ConnectionsType = type
                };
            }

            /// <summary>
            /// Creates new Output box element description.
            /// </summary>
            /// <param name="yLevel">The y level.</param>
            /// <param name="text">The text.</param>
            /// <param name="type">The type.</param>
            /// <param name="id">The unique box identifier.</param>
            /// <param name="single">If true then box can have only one connection, otherwise multiple connections are allowed.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Output(float yLevel, string text, Type type, int id, bool single = false)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Output,
                    Position = new Vector2(
                                           Constants.NodeMarginX - Constants.BoxSize + Constants.BoxOffsetX,
                                           Constants.NodeMarginY + Constants.NodeHeaderSize + yLevel * Constants.LayoutOffsetY),
                    Text = text,
                    Single = single,
                    ValueIndex = -1,
                    BoxID = id,
                    ConnectionsType = new ScriptType(type)
                };
            }

            /// <summary>
            /// Creates new Bool value element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Bool(float x, float y, int valueIndex = -1)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.BoolValue,
                    Position = new Vector2(x, y),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    BoxID = -1,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Integer value element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="component">The index of the component to edit. For vectors this can be set to modify only single component of it. Eg. for vec2 value component set to 1 will edit only Y component. Default value -1 will be used to edit whole value.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Integer(float x, float y, int valueIndex = -1, int component = -1, int valueMin = -1000000, int valueMax = 1000000)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.IntegerValue,
                    Position = new Vector2(Constants.NodeMarginX + x, Constants.NodeMarginY + Constants.NodeHeaderSize + y),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    ValueMin = valueMin,
                    ValueMax = valueMax,
                    BoxID = -1,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Unsigned Integer value element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="component">The index of the component to edit. For vectors this can be set to modify only single component of it. Eg. for vec2 value component set to 1 will edit only Y component. Default value -1 will be used to edit whole value.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype UnsignedInteger(float x, float y, int valueIndex = -1, int component = -1, uint valueMin = 0, uint valueMax = 1000000)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.UnsignedIntegerValue,
                    Position = new Vector2(Constants.NodeMarginX + x, Constants.NodeMarginY + Constants.NodeHeaderSize + y),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    ValueMin = valueMin,
                    ValueMax = valueMax,
                    BoxID = -1,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Float value element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="component">The index of the component to edit. For vectors this can be set to modify only single component of it. Eg. for vec2 value component set to 1 will edit only Y component. Default value -1 will be used to edit whole value.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Float(float x, float y, int valueIndex = -1, int component = -1, float valueMin = -1000000, float valueMax = 1000000)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.FloatValue,
                    Position = new Vector2(Constants.NodeMarginX + x, Constants.NodeMarginY + Constants.NodeHeaderSize + y),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    ValueMin = valueMin,
                    ValueMax = valueMax,
                    BoxID = component,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Vector2 value element description to edit X component.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Vector_X(float x, float y, int valueIndex = -1, float valueMin = -1000000, float valueMax = 1000000)
            {
                return Float(x, y, valueIndex, 0, valueMin, valueMax);
            }

            /// <summary>
            /// Creates new Vector value element description to edit Y component.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space). The actual position is offset by 1 times <see cref="Constants.LayoutOffsetY"/> to make it easier to arrange.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Vector_Y(float x, float y, int valueIndex = -1, float valueMin = -1000000, float valueMax = 1000000)
            {
                return Float(x, y, valueIndex, 1, valueMin, valueMax);
            }

            /// <summary>
            /// Creates new Vector value element description to edit Z component.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space). The actual position is offset by 2 times <see cref="Constants.LayoutOffsetY"/> to make it easier to arrange.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Vector_Z(float x, float y, int valueIndex = -1, float valueMin = -1000000, float valueMax = 1000000)
            {
                return Float(x, y, valueIndex, 2, valueMin, valueMax);
            }

            /// <summary>
            /// Creates new Vector value element description to edit W component.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space). The actual position is offset by 3 times <see cref="Constants.LayoutOffsetY"/> to make it easier to arrange.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="valueMin">The minimum value range.</param>
            /// <param name="valueMax">The maximum value range.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Vector_W(float x, float y, int valueIndex = -1, float valueMin = -1000000, float valueMax = 1000000)
            {
                return Float(x, y, valueIndex, 3, valueMin, valueMax);
            }

            /// <summary>
            /// Creates new Color value element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Color(float x, float y, int valueIndex = -1)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.ColorValue,
                    Position = new Vector2(Constants.NodeMarginX + x, Constants.NodeMarginY + Constants.NodeHeaderSize + y),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    BoxID = -1,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Asset picker element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="type">The allowed assets type to use (including inherited types).</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Asset(float x, float y, int valueIndex, Type type)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Asset,
                    Position = new Vector2(x, y),
                    Text = type.FullName,
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Actor picker element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="type">The allowed actor type to use (including inherited types).</param>
            /// <param name="width">The element width.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Actor(float x, float y, int valueIndex, Type type, float width = 70)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Actor,
                    Position = new Vector2(x, y),
                    Size = new Vector2(width, 16),
                    Text = type.FullName,
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Combo Box element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="width">The width of the element.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="values">The set of combo box items to present. May be nul if provided at runtime.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype ComboBox(float x, float y, float width, int valueIndex = -1, string[] values = null)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.ComboBox,
                    Position = new Vector2(x, y),
                    Size = new Vector2(width, 0),
                    Text = values != null ? string.Join("\n", values) : null, // Pack all values to string separated with new line characters
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Combo Box element description for enum editing.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="width">The width of the element.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="enumType">The enum type to present all it's values. Important: first value should be 0 and so on.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype ComboBox(float x, float y, int width, int valueIndex, Type enumType)
            {
                if (enumType == null || !enumType.IsEnum)
                    throw new ArgumentException();

                FieldInfo[] fields = enumType.GetFields();
                List<string> values = new List<string>(fields.Length);
                for (int i = 0; i < fields.Length; i++)
                {
                    var field = fields[i];
                    if (field.Name.Equals("value__"))
                        continue;

                    var name = CustomEditorsUtil.GetPropertyNameUI(field.Name);
                    values.Add(name);
                }
                return ComboBox(x, y, width, valueIndex, values.ToArray());
            }

            /// <summary>
            /// Creates new Combo Box element description for enum editing.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="width">The width of the element.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="enumType">The enum type to present all it's values.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Enum(float x, float y, int width, int valueIndex, Type enumType)
            {
                if (enumType == null || !enumType.IsEnum)
                    throw new ArgumentException();

                return new NodeElementArchetype
                {
                    Type = NodeElementType.EnumValue,
                    Position = new Vector2(x, y),
                    Size = new Vector2(width, 0),
                    Text = enumType.FullName,
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Text element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="text">The text to show.</param>
            /// <param name="width">The control width.</param>
            /// <param name="height">The control height.</param>
            /// <param name="tooltip">The control tooltip text.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Text(float x, float y, string text, float width = 100.0f, float height = 16.0f, string tooltip = null)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.Text,
                    Position = new Vector2(x, y),
                    Size = new Vector2(width, height),
                    Text = text,
                    Tooltip = tooltip,
                    Single = false,
                    ValueIndex = -1,
                    BoxID = 0,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new TextBox element description.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="width">The width.</param>
            /// <param name="height">The height.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <param name="isMultiline">Enable/disable multiline text input support</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype TextBox(float x, float y, float width, float height, int valueIndex, bool isMultiline = true)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.TextBox,
                    Position = new Vector2(Constants.NodeMarginX + x, Constants.NodeMarginY + Constants.NodeHeaderSize + y),
                    Size = new Vector2(width, height),
                    Single = false,
                    ValueIndex = valueIndex,
                    BoxID = isMultiline ? 1 : 0,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Skeleton Bone Index Select element description for enum editing.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="width">The width of the element.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype SkeletonBoneIndexSelect(float x, float y, int width, int valueIndex)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.SkeletonBoneIndexSelect,
                    Position = new Vector2(x, y),
                    Size = new Vector2(width, 0),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new Skeleton Node Name Select element description for enum editing.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="width">The width of the element.</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype SkeletonNodeNameSelect(float x, float y, int width, int valueIndex)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.SkeletonNodeNameSelect,
                    Position = new Vector2(x, y),
                    Size = new Vector2(width, 0),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }

            /// <summary>
            /// Creates new element description for Bounding Box editing.
            /// </summary>
            /// <param name="x">The x location (in node area space).</param>
            /// <param name="y">The y location (in node area space).</param>
            /// <param name="valueIndex">The index of the node variable linked as the input. Useful to make a physical connection between input box and default value for it.</param>
            /// <returns>The archetype.</returns>
            public static NodeElementArchetype Box(float x, float y, int valueIndex)
            {
                return new NodeElementArchetype
                {
                    Type = NodeElementType.BoxValue,
                    Position = new Vector2(Constants.NodeMarginX + x, Constants.NodeMarginY + Constants.NodeHeaderSize + y),
                    Text = null,
                    Single = false,
                    ValueIndex = valueIndex,
                    ConnectionsType = ScriptType.Null
                };
            }
        }
    }
}
