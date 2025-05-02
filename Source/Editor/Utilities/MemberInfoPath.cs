// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Helper class used to store path made of <see cref="MemberInfo"/>.
    /// </summary>
    [HideInEditor]
    public struct MemberInfoPath
    {
        /// <summary>
        /// The path entry.
        /// </summary>
        [HideInEditor]
        public struct Entry
        {
            /// <summary>
            /// The member.
            /// </summary>
            public readonly ScriptMemberInfo Member;

            /// <summary>
            /// The collection index or key.
            /// </summary>
            public readonly object Index;

            /// <summary>
            /// Gets the member type (field or property type).
            /// </summary>
            public ScriptType Type
            {
                get
                {
                    var result = Member.ValueType;

                    // Special case for collections
                    if (Index != null)
                        result = result.GetElementType();

                    return result;
                }
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="Entry"/> struct.
            /// </summary>
            /// <param name="member">The member.</param>
            /// <param name="index">The collection index or key.</param>
            public Entry(ScriptMemberInfo member, object index = null)
            {
                Member = member;
                Index = index;
            }

            /// <summary>
            /// Gets the value. Handles arrays.
            /// </summary>
            /// <param name="instance">The instance.</param>
            /// <returns>The result value.</returns>
            public object GetValue(object instance)
            {
                object value;

                // Special case for collections
                if (Index != null)
                {
                    if (instance is System.Collections.IList asList)
                    {
                        // Get value at index
                        value = asList[(int)Index];
                    }
                    else if (instance is System.Collections.IDictionary asDictionary)
                    {
                        // Get value at key
                        value = asDictionary[Index];
                    }
                    else
                    {
                        throw new NotSupportedException();
                    }
                }
                else
                {
                    // Get value
                    value = Member.GetValue(instance);
                }

                return value;
            }

            /// <summary>
            /// Sets the value.
            /// </summary>
            /// <param name="instance">The instance.</param>
            /// <param name="value">The value.</param>
            public void SetValue(object instance, object value)
            {
                // Special case for collections
                if (Index != null)
                {
                    if (instance is System.Collections.IList asList)
                    {
                        // Set value at index
                        asList[(int)Index] = value;
                    }
                    else if (instance is System.Collections.IDictionary asDictionary)
                    {
                        // Set value at key
                        asDictionary[Index] = value;
                    }
                    else
                    {
                        throw new NotSupportedException();
                    }
                }
                else
                {
                    // Set value
                    Member.SetValue(instance, value);
                }
            }

            /// <inheritdoc />
            public override bool Equals(object obj)
            {
                if (!(obj is Entry))
                {
                    return false;
                }

                var entry = (Entry)obj;
                return Member == entry.Member && Index == entry.Index;
            }

            /// <inheritdoc />
            public override int GetHashCode()
            {
                var hashCode = 2005110182;
                hashCode = hashCode * -1521134295 + Member.GetHashCode();
                if (Index != null)
                    hashCode = hashCode * -1521134295 + Index.GetHashCode();
                return hashCode;
            }

            /// <inheritdoc />
            public override string ToString()
            {
                if (Index != null)
                    return "[" + Index + "]";
                return Member.Name;
            }
        }

        private Entry[] _stack;

        /// <summary>
        /// Initializes a new instance of the <see cref="MemberInfoPath"/> class.
        /// </summary>
        /// <param name="member">The member.</param>
        /// <param name="index">The collection index or key.</param>
        public MemberInfoPath(ScriptMemberInfo member, object index = null)
        {
            if (member == null)
                throw new ArgumentNullException();
            _stack = new Entry[1];
            _stack[0] = new Entry(member, index);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MemberInfoPath"/> class.
        /// </summary>
        /// <param name="members">The members.</param>
        public MemberInfoPath(Stack<Entry> members)
        : this()
        {
            if (members == null || members.Count == 0)
                throw new ArgumentNullException();

            _stack = members.ToArray();
            Array.Reverse(_stack);
        }

        /// <summary>
        /// Gets the members path string.
        /// </summary>
        public string Path
        {
            get
            {
                string result = _stack[0].ToString();
                for (int i = 1; i < _stack.Length; i++)
                {
                    result += "." + _stack[i];
                }
                return result;
            }
        }

        /// <summary>
        /// Gets the last member (returns it) and the instance (by the ref parameter).
        /// </summary>
        /// <param name="instance">The instance. Also contains the result instance for the last member.</param>
        /// <returns>The last member info.</returns>
        public Entry GetLastMember(ref object instance)
        {
            Entry finalMember = _stack[0];
            for (int i = 1; i < _stack.Length; i++)
            {
                instance = finalMember.GetValue(instance);
                finalMember = _stack[i];
            }
            return finalMember;
        }

        /// <summary>
        /// Gets the last member value.
        /// </summary>
        /// <param name="instance">The top object instance.</param>
        /// <returns>The result value.</returns>
        public object GetLastValue(object instance)
        {
            var member = GetLastMember(ref instance);
            return member.GetValue(instance);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Path;
        }
    }
}
