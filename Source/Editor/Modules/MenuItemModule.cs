// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Manages menu items registered via <see cref="MenuItemAttribute"/>.
    /// Scans assemblies for static methods decorated with [MenuItem] and registers them to the editor main menu.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class MenuItemModule : EditorModule
    {
        /// <summary>
        /// Represents a menu item entry scanned from assemblies.
        /// </summary>
        private struct MenuItemEntry
        {
            /// <summary>
            /// The menu path (e.g., "Tools/My Tool").
            /// </summary>
            public string Path;

            /// <summary>
            /// The sorting priority. Lower values appear first.
            /// </summary>
            public int Priority;

            /// <summary>
            /// The shortcut text to display.
            /// </summary>
            public string Shortcut;

            /// <summary>
            /// The method to invoke when the menu item is clicked.
            /// </summary>
            public MethodInfo Method;
        }

        private readonly List<MenuItemEntry> _entries = new List<MenuItemEntry>();
        private readonly List<ContextMenuButton> _registeredButtons = new List<ContextMenuButton>();

        /// <summary>
        /// Initializes a new instance of the <see cref="MenuItemModule"/> class.
        /// </summary>
        /// <param name="editor">The editor instance.</param>
        internal MenuItemModule(Editor editor)
        : base(editor)
        {
            // Initialize after UIModule to ensure MainMenu exists
            InitOrder = -900;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
            ScanAndRegisterMenuItems();
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;
            ClearRegisteredItems();
        }

        /// <summary>
        /// Called when scripts reload ends. Re-scans and re-registers all menu items.
        /// </summary>
        private void OnScriptsReloadEnd()
        {
            ClearRegisteredItems();
            ScanAndRegisterMenuItems();
        }

        /// <summary>
        /// Clears all registered menu item buttons and entries.
        /// </summary>
        private void ClearRegisteredItems()
        {
            for (int i = 0; i < _registeredButtons.Count; i++)
            {
                _registeredButtons[i]?.Dispose();
            }
            _registeredButtons.Clear();
            _entries.Clear();
        }

        /// <summary>
        /// Scans all loaded assemblies for methods decorated with <see cref="MenuItemAttribute"/> and registers them to the main menu.
        /// </summary>
        private void ScanAndRegisterMenuItems()
        {
            // Scan all assemblies for [MenuItem] methods
            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                var assembly = assemblies[i];
                if (!ShouldScanAssembly(assembly))
                    continue;

                try
                {
                    ScanAssembly(assembly);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(string.Format("Failed to scan assembly '{0}' for menu items: {1}", assembly.FullName, ex.Message));
                }
            }

            // Sort by priority (lower values first)
            _entries.Sort((a, b) => a.Priority.CompareTo(b.Priority));

            // Register to main menu
            RegisterAllMenuItems();
        }

        /// <summary>
        /// Determines whether the specified assembly should be scanned for menu items.
        /// </summary>
        /// <param name="assembly">The assembly to check.</param>
        /// <returns><c>true</c> if the assembly should be scanned; otherwise, <c>false</c>.</returns>
        private bool ShouldScanAssembly(Assembly assembly)
        {
            var name = assembly.GetName().Name;

            // Skip system and engine core assemblies
            if (name.StartsWith("System") ||
                name.StartsWith("Microsoft") ||
                name.StartsWith("mscorlib") ||
                name == "FlaxEngine.CSharp" ||
                name == "Newtonsoft.Json")
                return false;

            // Only scan assemblies that reference FlaxEngine
            var references = assembly.GetReferencedAssemblies();
            for (int i = 0; i < references.Length; i++)
            {
                if (references[i].Name == "FlaxEngine.CSharp")
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Scans the specified assembly for methods decorated with <see cref="MenuItemAttribute"/>.
        /// </summary>
        /// <param name="assembly">The assembly to scan.</param>
        private void ScanAssembly(Assembly assembly)
        {
            var types = assembly.GetTypes();
            for (int i = 0; i < types.Length; i++)
            {
                var type = types[i];
                var methods = type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);

                for (int j = 0; j < methods.Length; j++)
                {
                    var method = methods[j];
                    var attr = method.GetCustomAttribute<MenuItemAttribute>();
                    if (attr == null)
                        continue;

                    // Validate method signature: must be parameterless
                    if (method.GetParameters().Length > 0)
                    {
                        Editor.LogWarning(string.Format("[MenuItem] method '{0}.{1}' must be parameterless", type.FullName, method.Name));
                        continue;
                    }

                    // Validate path: must not be empty
                    if (string.IsNullOrEmpty(attr.Path))
                    {
                        Editor.LogWarning(string.Format("[MenuItem] method '{0}.{1}' has empty path", type.FullName, method.Name));
                        continue;
                    }

                    _entries.Add(new MenuItemEntry
                    {
                        Path = attr.Path,
                        Priority = attr.Priority,
                        Shortcut = attr.Shortcut,
                        Method = method
                    });
                }
            }
        }

        /// <summary>
        /// Registers all scanned menu item entries to the editor main menu.
        /// </summary>
        private void RegisterAllMenuItems()
        {
            var mainMenu = Editor.UI.MainMenu;
            if (mainMenu == null)
                return;

            for (int i = 0; i < _entries.Count; i++)
            {
                try
                {
                    RegisterMenuItem(_entries[i]);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(string.Format("Failed to register menu item '{0}': {1}", _entries[i].Path, ex.Message));
                }
            }
        }

        /// <summary>
        /// Registers a single menu item entry to the editor main menu.
        /// </summary>
        /// <param name="entry">The menu item entry to register.</param>
        private void RegisterMenuItem(MenuItemEntry entry)
        {
            var parts = entry.Path.Split('/');
            if (parts.Length < 2)
            {
                Editor.LogWarning(string.Format("[MenuItem] path '{0}' is invalid, requires at least 'Menu/Item' format", entry.Path));
                return;
            }

            // Get or create top-level menu button
            var topMenuName = parts[0].Trim();
            var topMenu = Editor.UI.MainMenu.GetOrAddButton(topMenuName);
            var contextMenu = topMenu.ContextMenu;

            // Navigate or create submenu hierarchy
            ContextMenuChildMenu currentChildMenu = null;
            for (int i = 1; i < parts.Length - 1; i++)
            {
                var subMenuName = parts[i].Trim();
                if (currentChildMenu == null)
                    currentChildMenu = contextMenu.GetOrAddChildMenu(subMenuName);
                else
                    currentChildMenu = currentChildMenu.ContextMenu.GetOrAddChildMenu(subMenuName);
                
                currentChildMenu.ContextMenu.AutoSort = true;
            }

            // Add the menu item button to the target context menu
            var itemName = parts[parts.Length - 1].Trim();
            var method = entry.Method;
            var targetMenu = currentChildMenu?.ContextMenu ?? contextMenu;

            ContextMenuButton button;
            if (!string.IsNullOrEmpty(entry.Shortcut))
                button = targetMenu.AddButton(itemName, entry.Shortcut, () => InvokeMenuItem(method));
            else
                button = targetMenu.AddButton(itemName, () => InvokeMenuItem(method));

            _registeredButtons.Add(button);
        }

        /// <summary>
        /// Invokes the specified menu item method.
        /// </summary>
        /// <param name="method">The method to invoke.</param>
        private void InvokeMenuItem(MethodInfo method)
        {
            try
            {
                method.Invoke(null, null);
            }
            catch (Exception ex)
            {
                Editor.LogError(string.Format("Failed to invoke menu item '{0}.{1}': {2}", method.DeclaringType?.FullName, method.Name, ex));
            }
        }
    }
}
