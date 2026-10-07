// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Marks a static method to be registered as a menu item in the editor main menu.
    /// </summary>
    /// <remarks>
    /// The method must be static and parameterless. Use "/" to create submenus, e.g., "Tools/My Tool".
    /// </remarks>
    [Serializable]
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public sealed class MenuItemAttribute : Attribute
    {
        /// <summary>
        /// The menu item path. Use "/" to separate menu levels (e.g., "Tools/My Tool" or "Window/Custom/My Window").
        /// </summary>
        public string Path;

        /// <summary>
        /// The sorting priority. Lower values appear first. Default is 1000.
        /// </summary>
        public int Priority = 1000;

        /// <summary>
        /// The shortcut text to display (for display only, does not auto-bind input).
        /// </summary>
        public string Shortcut;

        /// <summary>
        /// Initializes a new instance of the <see cref="MenuItemAttribute"/> class.
        /// </summary>
        public MenuItemAttribute()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MenuItemAttribute"/> class.
        /// </summary>
        /// <param name="path">The menu item path (e.g., "Tools/My Tool").</param>
        public MenuItemAttribute(string path)
        {
            Path = path;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MenuItemAttribute"/> class.
        /// </summary>
        /// <param name="path">The menu item path (e.g., "Tools/My Tool").</param>
        /// <param name="priority">The sorting priority (lower values appear first).</param>
        public MenuItemAttribute(string path, int priority)
        {
            Path = path;
            Priority = priority;
        }
    }
}
