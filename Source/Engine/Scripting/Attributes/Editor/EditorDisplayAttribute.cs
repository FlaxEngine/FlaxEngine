// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Allows to change item display name or a group in the editor.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class EditorDisplayAttribute : Attribute
    {
        /// <summary>
        /// Special text sequence used for property names to override the default layout style and inline property into the parent layout.
        /// </summary>
        public const string InlineStyle = "__inline__";

        /// <summary>
        /// The group name. Default is null.
        /// </summary>
        public string Group;

        /// <summary>
        /// The overriden item display name. Default is null.
        /// </summary>
        public string Name;

        private EditorDisplayAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorDisplayAttribute"/> class.
        /// </summary>
        /// <param name="group">The group name.</param>
        /// <param name="name">The display name. Use special name `__inline__` (see <see cref="InlineStyle"/>) to inline property into the parent container.</param>
        public EditorDisplayAttribute(string group = null, string name = null)
        {
            Group = group;
            Name = name;
        }
    }
}
