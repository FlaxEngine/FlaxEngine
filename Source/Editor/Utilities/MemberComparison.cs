// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Json;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// This structure represents the comparison of one member of an object to the corresponding member of another object.
    /// </summary>
    [Serializable, HideInEditor]
    public struct MemberComparison
    {
        /// <summary>
        /// Members path this Comparison compares.
        /// </summary>
        public MemberInfoPath MemberPath;

        /// <summary>
        /// The value of first object respective member
        /// </summary>
        public readonly object Value1;

        /// <summary>
        /// The value of second object respective member
        /// </summary>
        public readonly object Value2;

        /// <summary>
        /// Initializes a new instance of the <see cref="MemberComparison"/> struct.
        /// </summary>
        /// <param name="member">The member.</param>
        /// <param name="value1">The first value.</param>
        /// <param name="value2">The second value.</param>
        public MemberComparison(ScriptMemberInfo member, object value1, object value2)
        {
            MemberPath = new MemberInfoPath(member);
            Value1 = value1;
            Value2 = value2;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MemberComparison"/> struct.
        /// </summary>
        /// <param name="memberPath">The member path.</param>
        /// <param name="value1">The first value.</param>
        /// <param name="value2">The second value.</param>
        public MemberComparison(MemberInfoPath memberPath, object value1, object value2)
        {
            MemberPath = memberPath;
            Value1 = value1;
            Value2 = value2;
        }

        /// <summary>
        /// Sets the member value. Handles field or property type.
        /// </summary>
        /// <param name="instance">The instance.</param>
        /// <param name="value">The value.</param>
        public void SetMemberValue(object instance, object value)
        {
            var originalInstance = instance;
            var finalMember = MemberPath.GetLastMember(ref instance);

            var type = finalMember.Type;
            if (value != null && type != ScriptType.Null && type != value.GetType())
            {
                // Convert value to ensure it matches the member type (eg. undo that uses json serializer might return different value type for some cases)
                if (type.IsEnum)
                    value = Convert.ChangeType(value, Enum.GetUnderlyingType(type.Type));
                else if (type.Type == typeof(byte))
                    value = Convert.ToByte(value);
                else if (type.Type == typeof(sbyte))
                    value = Convert.ToSByte(value);
                else if (type.Type == typeof(short))
                    value = Convert.ToInt16(value);
                else if (type.Type == typeof(int))
                    value = Convert.ToInt32(value);
                else if (type.Type == typeof(long))
                    value = Convert.ToInt64(value);
                else if (type.Type == typeof(ushort))
                    value = Convert.ToUInt16(value);
                else if (type.Type == typeof(uint))
                    value = Convert.ToUInt32(value);
                else if (type.Type == typeof(ulong))
                    value = Convert.ToUInt64(value);
                else if (type.Type == typeof(float))
                    value = Convert.ToSingle(value);
                else if (type.Type == typeof(double))
                    value = Convert.ToDouble(value);
            }

            finalMember.SetValue(instance, value);

            if (instance != originalInstance && finalMember.Index != null)
            {
                // Set collection back to the parent object (in case of properties that always return a new object like 'Spline.SplineKeyframes')
                finalMember.Member.SetValue(originalInstance, instance);
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return MemberPath.Path + ": " + (Value1 ?? "<null>") + (JsonSerializer.ValueEquals(Value1, Value2) ? " == " : " != ") + (Value2 ?? "<null>");
        }
    }
}
