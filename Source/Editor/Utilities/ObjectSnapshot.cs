// Copyright (c) Wojciech Figat. All rights reserved.

//#define DEBUG_OBJECT_SNAPSHOT_COMPARISION

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Helper class to gather object snapshots and compare them using reflection.
    /// </summary>
    public sealed class ObjectSnapshot
    {
        private readonly List<TypeEntry> _members;
        private readonly List<object> _values;

        /// <summary>
        /// The object type.
        /// </summary>
        public readonly ScriptType ObjectType;

        private ObjectSnapshot(ScriptType type, List<object> values, List<TypeEntry> members)
        {
            ObjectType = type;
            _values = values;
            _members = members;
        }

        private struct TypeEntry
        {
            public MemberInfoPath Path;
            public int SubEntriesCount;

            public TypeEntry(MemberInfoPath membersPath, int subEntriesCount)
            {
                Path = membersPath;
                SubEntriesCount = subEntriesCount;
            }
        }

        private static readonly Type[] AttributesIgnoreList =
        {
            typeof(NonSerializedAttribute),
            typeof(NoSerializeAttribute),
            typeof(NoUndoAttribute),
        };

        private static void GetEntries(MemberInfoPath.Entry member, Stack<MemberInfoPath.Entry> membersPath, List<TypeEntry> result, List<object> values, Stack<object> refStack, ScriptType memberType, object memberValue)
        {
            membersPath.Push(member);
            var path = new MemberInfoPath(membersPath);
            var beforeCount = result.Count;

            // Check if record object sub members (skip Flax objects)
            // It's used for ref types but not null types and with checking cyclic references
            if ((memberType.IsClass || memberType.IsArray || typeof(IList).IsAssignableFrom(memberType.Type) || typeof(IDictionary).IsAssignableFrom(memberType.Type))
                && memberValue != null
                && !refStack.Contains(memberValue))
            {
                if (memberType.IsArray && !ScriptType.FlaxObject.IsAssignableFrom(memberType.GetElementType()))
                {
                    // Array
                    var array = (Array)memberValue;
                    var elementType = memberType.GetElementType();
                    var length = array.Length;
                    refStack.Push(memberValue);
                    for (int i = 0; i < length; i++)
                    {
                        var value = array.GetValue(i);
                        GetEntries(new MemberInfoPath.Entry(member.Member, i), membersPath, result, values, refStack, elementType, value);
                    }
                    refStack.Pop();
                }
                else if (typeof(IList).IsAssignableFrom(memberType.Type) && !ScriptType.FlaxObject.IsAssignableFrom(memberType.GetElementType()))
                {
                    // List
                    var list = (IList)memberValue;
                    var genericArguments = memberType.GetGenericArguments();
                    var elementType = new ScriptType(genericArguments[0]);
                    var count = list.Count;
                    refStack.Push(memberValue);
                    for (int i = 0; i < count; i++)
                    {
                        var value = list[i];
                        GetEntries(new MemberInfoPath.Entry(member.Member, i), membersPath, result, values, refStack, elementType, value);
                    }
                    refStack.Pop();
                }
                else if (typeof(IDictionary).IsAssignableFrom(memberType.Type))
                {
                    // Dictionary
                    var dictionary = (IDictionary)memberValue;
                    var genericArguments = memberType.GetGenericArguments();
                    var valueType = new ScriptType(genericArguments[1]);
                    foreach (var key in dictionary.Keys)
                    {
                        var value = dictionary[key];
                        GetEntries(new MemberInfoPath.Entry(member.Member, key), membersPath, result, values, refStack, valueType, value);
                    }
                }
                else if (memberType.IsClass && !ScriptType.FlaxObject.IsAssignableFrom(memberType))
                {
                    // Object
                    refStack.Push(memberValue);
                    GetEntries(memberValue, membersPath, memberType, result, values, refStack);
                    refStack.Pop();
                }
            }

            var afterCount = result.Count;
            result.Add(new TypeEntry(path, afterCount - beforeCount));
            var memberValueType = TypeUtils.GetObjectType(memberValue);
            if (memberValue != null && memberValueType.IsStructure)
            {
                var json = JsonSerializer.Serialize(memberValue);
                var clone = JsonSerializer.Deserialize(json, memberValueType.Type);
                values.Add(clone);
            }
            else
            {
                values.Add(memberValue);
            }
            membersPath.Pop();
        }

        private static ScriptType GetMemberType(ScriptType memberType, object memberValue)
        {
            // Try using the type of the actual value (eg. Control property that has value set to Button - capture all button properties, not just control properties)
            if (memberValue != null)
            {
                return TypeUtils.GetObjectType(memberValue);
            }
            return memberType;
        }

        private static void GetEntries(object instance, Stack<MemberInfoPath.Entry> membersPath, ScriptType type, List<TypeEntry> result, List<object> values, Stack<object> refStack)
        {
            // Note: this should match Flax serialization rules and attributes (see ExtendedDefaultContractResolver)

            var fields = type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
            var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);

            for (int i = 0; i < fields.Length; i++)
            {
                var f = fields[i];
                var attributes = f.GetAttributes();

                // Serialize non-public fields only with a proper attribute
                if (!f.IsPublic && !attributes.Any(x => x is SerializeAttribute))
                    continue;

                // Check if has attribute to skip serialization
                bool noSerialize = false;
                foreach (var attribute in attributes)
                {
                    if (AttributesIgnoreList.Contains(attribute.GetType()))
                    {
                        noSerialize = true;
                        break;
                    }
                }
                if (noSerialize)
                    continue;

                var memberValue = f.GetValue(instance);
                GetEntries(new MemberInfoPath.Entry(f), membersPath, result, values, refStack, GetMemberType(f.ValueType, memberValue), memberValue);
            }

            for (int i = 0; i < properties.Length; i++)
            {
                var p = properties[i];

                // Serialize only properties with read/write
                if (!p.HasGet || !p.HasSet)
                    continue;

                var attributes = p.GetAttributes();

                // Serialize non-public properties only with a proper attribute
                if (!p.IsPublic && !attributes.Any(x => x is SerializeAttribute))
                    continue;

                // Check if has attribute to skip serialization
                bool noSerialize = false;
                foreach (var attribute in attributes)
                {
                    if (AttributesIgnoreList.Contains(attribute.GetType()))
                    {
                        noSerialize = true;
                        break;
                    }
                }
                if (noSerialize)
                    continue;

                var memberValue = p.GetValue(instance);
                GetEntries(new MemberInfoPath.Entry(p), membersPath, result, values, refStack, GetMemberType(p.ValueType, memberValue), memberValue);
            }
        }

        private static List<TypeEntry> GetMembers(object instance, ScriptType type, out List<object> values)
        {
            values = new List<object>();
            var result = new List<TypeEntry>();
            var membersPath = new Stack<MemberInfoPath.Entry>(8);
            var refsStack = new Stack<object>(8);

            refsStack.Push(instance);
            GetEntries(instance, membersPath, type, result, values, refsStack);
            return result;
        }

        /// <summary>
        /// Captures the snapshot of the object.
        /// </summary>
        /// <param name="obj">The object to capture.</param>
        /// <returns>The object snapshot.</returns>
        public static ObjectSnapshot CaptureSnapshot(object obj)
        {
            if (obj == null)
                throw new ArgumentNullException();

            var type = TypeUtils.GetObjectType(obj);
            var members = GetMembers(obj, type, out var values);
#if DEBUG_OBJECT_SNAPSHOT_COMPARISION
            Debug.Logger.LogHandler.LogWrite(LogType.Warning, "-------------- CaptureSnapshot:  " + type + "  --------------");
            for (int i = 0; i < values.Count; i++)
                Debug.Logger.LogHandler.LogWrite(LogType.Warning, members[i].Path.Path + " = " + (values[i] ?? "<null>"));
#endif
            return new ObjectSnapshot(type, values, members);
        }

        /// <summary>
        /// Gets a list of MemberComparison values that represent the fields and/or properties
        /// that differ between the given object and the captured state.
        /// </summary>
        /// <param name="obj">The object to compare.</param>
        /// <returns>The collection of modified properties.</returns>
        public List<MemberComparison> Compare(object obj)
        {
            if (obj == null)
                throw new ArgumentNullException();
            if (ObjectType != TypeUtils.GetObjectType(obj))
                throw new ArgumentException("Given object must be the same type as captured object.");

            var list = new List<MemberComparison>();
#if DEBUG_OBJECT_SNAPSHOT_COMPARISION
            Debug.Logger.LogHandler.LogWrite(LogType.Warning, "-------------- Comparison --------------");
#endif
            for (int i = _members.Count - 1; i >= 0; i--)
            {
                var m = _members[i];
                var xValue = _values[i];
                var yValue = m.Path.GetLastValue(obj);
#if DEBUG_OBJECT_SNAPSHOT_COMPARISION
                Debug.Logger.LogHandler.LogWrite(LogType.Warning, "Compare: " + (new MemberComparison(m.Path, xValue, yValue)));
#endif

                if (!JsonSerializer.ValueEquals(xValue, yValue))
                {
#if DEBUG_OBJECT_SNAPSHOT_COMPARISION
                    Debug.Logger.LogHandler.LogWrite(LogType.Warning, "Diff on: " + (new MemberComparison(m.Path, xValue, yValue)));
#endif

                    list.Add(new MemberComparison(m.Path, xValue, yValue));

                    // Value changed, skip sub entries compare
                    i -= m.SubEntriesCount;
                }
            }

#if DEBUG_OBJECT_SNAPSHOT_COMPARISION
            Debug.Logger.LogHandler.LogWrite(LogType.Warning, "-------------- End --------------");
#endif
            return list;
        }
    }
}
