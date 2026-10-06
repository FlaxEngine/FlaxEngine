// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using System;

namespace FlaxEditor
{
    /// <summary>
    /// Shared utilities for scene editing in main viewport and prefab windows.
    /// </summary>
    /// <seealso cref="ISceneEditingContext"/>
    internal static class SceneEditingTools
    {
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
                    if (i == splitPath.Length - 1)
                    {
                        if (mainCM)
                        {
                            var b = contextMenu.AddButton(splitPath[i].Trim(), spawn);
                            b.Tag = actorType;
                            mainCM = false;
                        }
                        else if (childCM != null)
                        {
                            var b = childCM.ContextMenu.AddButton(splitPath[i].Trim(), spawn);
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
                            childCM = contextMenu.GetOrAddChildMenu(splitPath[i].Trim());
                            childCM.ContextMenu.AutoSort = true;
                            mainCM = false;
                        }
                        else if (childCM != null)
                        {
                            childCM = childCM.ContextMenu.GetOrAddChildMenu(splitPath[i].Trim());
                            childCM.ContextMenu.AutoSort = true;
                        }
                    }
                }
            }
        }
    }
}
