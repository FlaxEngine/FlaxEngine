// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using System;

namespace FlaxEditor
{
    /// <summary>
    /// Shared utilities for scene editing in main viewport and prefab windows.
    /// </summary>
    /// <seealso cref="ISceneEditingContext"/>
    internal static class SceneEditingTools
    {
        public static Actor SpawnActorMenu(ContextMenuButton button)
        {
            var type = (ScriptType)button.Tag;
            if (new ScriptType(typeof(FlaxEngine.GUI.Control)).IsAssignableFrom(type))
            {
                // UI Control
                var actor = new UIControl();
                actor.Name = type.Name;
                actor.Control = (FlaxEngine.GUI.Control)type.CreateInstance();
                return actor;
            }

            // Create actor
            {
                var actor = (Actor)FlaxEngine.Object.New(type.TypeName);
                actor.Name = type.Name;
                return actor;
            }
        }

        public static void AddActorContextMenu(ContextMenu contextMenu, Action<ContextMenuButton> spawn, bool isConvert = false)
        {
            // Go through each actor and add it to the context menu if it has the ActorContextMenu attribute
            foreach (var actorType in Editor.Instance.CodeEditing.Actors.Get())
            {
                if (actorType.IsAbstract || !actorType.HasAttribute(typeof(ActorContextMenuAttribute), false))
                    continue;

                ActorContextMenuAttribute attribute = null;
                foreach (var actorAttribute in actorType.GetAttributes(false))
                {
                    if (actorAttribute is ActorContextMenuAttribute actorContextMenuAttribute)
                    {
                        attribute = actorContextMenuAttribute;
                    }
                }
                var splitPath = attribute?.Path.Split('/');
                ContextMenuChildMenu childCM = null;
                bool mainCM = true;
                for (int i = 0; i < splitPath?.Length; i++)
                {
                    var name = splitPath[i].Trim();
                    if (i == splitPath.Length - 1)
                    {
                        // Hardcoded UI Controls toolbox
                        if (!isConvert && childCM != null && actorType.Type == typeof(UIControl))
                        {
                            var menu = childCM.ContextMenu.GetOrAddChildMenu(name);
                            menu.ButtonClicked += spawn;
                            menu.Tag = actorType;
                            menu.ContextMenu.AutoSort = true;
                            var b = menu.ContextMenu.AddButton("Empty", spawn);
                            b.Tag = actorType;
                            foreach (var controlType in Editor.Instance.CodeEditing.Controls.Get())
                            {
                                if (controlType.IsAbstract)
                                    continue;
                                name = Utilities.Utils.GetPropertyNameUI(controlType.Name);
                                b = menu.ContextMenu.AddButton(name, spawn);
                                b.Tag = controlType;
                            }
                            continue;
                        }

                        if (mainCM)
                        {
                            var b = contextMenu.AddButton(name, spawn);
                            b.Tag = actorType;
                            mainCM = false;
                        }
                        else if (childCM != null)
                        {
                            var b = childCM.ContextMenu.AddButton(name, spawn);
                            b.Tag = actorType;
                        }
                    }
                    else
                    {
                        // Remove new path for converting menu
                        if (isConvert && splitPath[i] == "New")
                            continue;

                        if (mainCM)
                        {
                            childCM = contextMenu.GetOrAddChildMenu(name);
                            childCM.ContextMenu.AutoSort = true;
                            mainCM = false;
                        }
                        else if (childCM != null)
                        {
                            childCM = childCM.ContextMenu.GetOrAddChildMenu(name);
                            childCM.ContextMenu.AutoSort = true;
                        }
                    }
                }
            }
        }

        public static void BuildTreePanel(ContainerControl parent, out SearchBox searchBox, out Panel treePanel)
        {
            var margin = Utilities.Constants.UIMargin;
            var headerPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = Style.Current.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, TextBoxBase.DefaultHeight + margin * 2),
                Parent = parent,
            };
            searchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = headerPanel,
                Bounds = new Rectangle(margin, margin, headerPanel.Width - margin * 2, TextBoxBase.DefaultHeight),
                TooltipText = "Search actors hierarchy.\n\nYou can prefix your search with different search operators:\ns: -> Actor with script of type\na: -> Actor type\nc: -> Control type",
            };
            treePanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, headerPanel.Bottom, 0),
                IsScrollable = true,
                ScrollBars = ScrollBars.Both,
                Parent = parent,
            };
        }

    }
}
