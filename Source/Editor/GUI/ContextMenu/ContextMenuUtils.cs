// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEditor.GUI.ContextMenu.Utils;

/// <summary>
/// Utility class to use with ContextMenus.
/// </summary>
public static class ContextMenuUtils
{
    /// <summary>
    /// Create a context menu child of a context menu using a path
    /// </summary>
    /// <param name="contextMenu">The owner context menu</param>
    /// <param name="path">The path to create context menu, exemple: Actor - Lights - PointLight to create a context menu for light actor.</param>
    /// <param name="OnPressChild">What happen on press on the child item?</param>
    /// <returns>A child context menu</returns>
    public static ContextMenuChildMenu CreateChildByPath(ContextMenu contextMenu, List<string> path, Action OnPressChild)
    {
        if (path == null || contextMenu == null)
            return null;

        ContextMenuChildMenu childCM = null;
        var isMainCM = true;
        var pathCount = path.Count;

        for (int i = 0; i < pathCount; i++)
        {
            var part = path[i].Trim();

            if (i == pathCount - 1)
            {
                if (isMainCM)
                {
                    contextMenu.AddButton(part, OnPressChild);
                    isMainCM = false;
                }
                else
                {
                    childCM?.ContextMenu.AddButton(part, OnPressChild);
                    childCM.ContextMenu.AutoSort = true;
                }
            }
            else
            {
                if (isMainCM)
                {
                    childCM = contextMenu.GetOrAddChildMenu(part);
                    isMainCM = false;
                }
                else
                {
                    childCM = childCM?.ContextMenu.GetOrAddChildMenu(part);
                }
                childCM.ContextMenu.AutoSort = true;
            }
        }
        return childCM;
    }
}
