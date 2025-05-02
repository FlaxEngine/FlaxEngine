// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.Scripting;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Helper methods used by the editor.
    /// </summary>
    public static partial class Extensions
    {
        /// <summary>
        /// Gets a list of MemberComparison values that represent the fields and/or properties that differ between the two objects.
        /// </summary>
        /// <typeparam name="T">Type of object to compare.</typeparam>
        /// <param name="first">First object to compare.</param>
        /// <param name="second">Second object to compare.</param>
        /// <returns>Returns list of <see cref="MemberComparison" /> structs with all different fields and properties.</returns>
        public static List<MemberComparison> ReflectiveCompare<T>(this T first, T second)
        {
            if (first.GetType() != second.GetType())
                throw new ArgumentException("both first and second parameters has to be of the same type");

            var list = new List<MemberComparison>();
            var members = first.GetType().GetMembers(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public);
            for (int i = 0; i < members.Length; i++)
            {
                var m = members[i];
                if (m.MemberType == MemberTypes.Field)
                {
                    var f = (FieldInfo)m;
                    var xValue = f.GetValue(first);
                    var yValue = f.GetValue(second);
                    if (!Equals(xValue, yValue))
                    {
                        list.Add(new MemberComparison(new ScriptMemberInfo(f), xValue, yValue));
                    }
                }
                else if (m.MemberType == MemberTypes.Property)
                {
                    var p = (PropertyInfo)m;
                    if (p.CanRead && p.GetGetMethod().GetParameters().Length == 0)
                    {
                        var xValue = p.GetValue(first, null);
                        var yValue = p.GetValue(second, null);
                        if (!Equals(xValue, yValue))
                        {
                            list.Add(new MemberComparison(new ScriptMemberInfo(p), xValue, yValue));
                        }
                    }
                }
            }

            return list;
        }
    }
}
