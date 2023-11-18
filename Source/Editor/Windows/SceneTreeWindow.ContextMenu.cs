// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.ContextMenu.Utils;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    public partial class SceneTreeWindow
    {
        /// <summary>
        /// Occurs when scene tree window wants to show the context menu. Allows to add custom options.
        /// </summary>
        public event Action<ContextMenu> ContextMenuShow;

        /// <summary>
        /// Creates the context menu for the current objects selection and the current Editor state.
        /// </summary>
        /// <returns>The context menu.</returns>
        private ContextMenu CreateContextMenu()
        {
            // Prepare

            bool hasSthSelected = Editor.SceneEditing.HasSthSelected;
            bool isSingleActorSelected = Editor.SceneEditing.SelectionCount == 1 && Editor.SceneEditing.Selection[0] is ActorNode;
            bool canEditScene = Editor.StateMachine.CurrentState.CanEditScene && Level.IsAnySceneLoaded;
            var inputOptions = Editor.Options.Options.Input;

            // Create popup

            var contextMenu = new ContextMenu
            {
                MinimumWidth = 120
            };

            // Expand/collapse

            var b = contextMenu.AddButton("Expand All", OnExpandAllClicked);
            b.Enabled = hasSthSelected;

            b = contextMenu.AddButton("Collapse All", OnCollapseAllClicked);
            b.Enabled = hasSthSelected;

            if (hasSthSelected)
            {
                contextMenu.AddButton(Editor.Windows.EditWin.IsPilotActorActive ? "Stop piloting actor" : "Pilot actor", inputOptions.PilotActor, Editor.UI.PilotActor);
            }

            contextMenu.AddSeparator();

            // Basic editing options

            b = contextMenu.AddButton("Rename", inputOptions.Rename, Rename);
            b.Enabled = isSingleActorSelected;

            b = contextMenu.AddButton("Duplicate", inputOptions.Duplicate, Editor.SceneEditing.Duplicate);
            b.Enabled = hasSthSelected;

            if (isSingleActorSelected)
            {
                var convertMenu = contextMenu.AddChildMenu("Convert");
                convertMenu.ContextMenu.AutoSort = true;
                foreach (var actorType in Editor.CodeEditing.Actors.Get())
                {
                    if (actorType.IsAbstract)
                        continue;
                    ActorContextMenuAttribute attribute = null;
                    foreach (var e in actorType.GetAttributes(false))
                    {
                        if (e is ActorContextMenuAttribute actorContextMenuAttribute)
                        {
                            attribute = actorContextMenuAttribute;
                            break;
                        }
                    }
                    if (attribute == null)
                        continue;
                    var parts = attribute.Path.Split('/');
                    ContextMenuChildMenu childCM = convertMenu;
                    bool mainCM = true;
                    for (int i = 0; i < parts.Length; i++)
                    {
                        var part = parts[i].Trim();
                        if (i == parts.Length - 1)
                        {
                            if (mainCM)
                            {
                                convertMenu.ContextMenu.AddButton(part, () => Editor.SceneEditing.Convert(actorType.Type));
                                mainCM = false;
                            }
                            else
                            {
                                childCM.ContextMenu.AddButton(part, () => Editor.SceneEditing.Convert(actorType.Type));
                                childCM.ContextMenu.AutoSort = true;
                            }
                        }
                        else
                        {
                            // Remove new path for converting menu
                            if (parts[i] == "New")
                                continue;

                            if (mainCM)
                            {
                                childCM = convertMenu.ContextMenu.GetOrAddChildMenu(part);
                                childCM.ContextMenu.AutoSort = true;
                                mainCM = false;
                            }
                            else
                            {
                                childCM = childCM.ContextMenu.GetOrAddChildMenu(part);
                                childCM.ContextMenu.AutoSort = true;
                            }
                        }
                    }
                }
            }
            b = contextMenu.AddButton("Delete", inputOptions.Delete, Editor.SceneEditing.Delete);
            b.Enabled = hasSthSelected;

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Copy", inputOptions.Copy, Editor.SceneEditing.Copy);

            b.Enabled = hasSthSelected;
            contextMenu.AddButton("Paste", inputOptions.Paste, Editor.SceneEditing.Paste);

            b = contextMenu.AddButton("Cut", inputOptions.Cut, Editor.SceneEditing.Cut);
            b.Enabled = canEditScene;

            // Prefab options

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Create Prefab", Editor.Prefabs.CreatePrefab);
            b.Enabled = isSingleActorSelected &&
                        ((ActorNode)Editor.SceneEditing.Selection[0]).CanCreatePrefab &&
                        Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;

            bool hasPrefabLink = canEditScene && isSingleActorSelected && (Editor.SceneEditing.Selection[0] as ActorNode).HasPrefabLink;
            if (hasPrefabLink)
            {
                contextMenu.AddButton("Select Prefab", Editor.Prefabs.SelectPrefab);
                contextMenu.AddButton("Break Prefab Link", Editor.Prefabs.BreakLinks);
            }

            // Spawning actors options

            contextMenu.AddSeparator();

            // go through each actor and add it to the context menu if it has the ActorContextMenu attribute
            foreach (var actorType in Editor.CodeEditing.Actors.Get())
            {
                if (actorType.IsAbstract || !actorType.HasAttribute(typeof(ActorContextMenuAttribute), false))
                    continue;

                var attribute = GetContextMenu(actorType);
                var basePath = GetActorContextMenuPath(attribute.Path);
                var contextMenuPath = new List<string>(new string[] { "New" });
                contextMenuPath.AddRange(basePath);

                // create new actor cm
                ContextMenuUtils.CreateChildByPath(contextMenu, contextMenuPath, () => Spawn(actorType.Type, null));

                // create actor as child cm
                if (isSingleActorSelected)
                {
                    var newChildSplitPath = new List<string>(new string[] { "New Child" });
                    newChildSplitPath.AddRange(basePath);
                    ContextMenuUtils.CreateChildByPath(contextMenu, newChildSplitPath, () => Spawn(actorType.Type));
                }
            }

            // Custom options

            bool showCustomNodeOptions = Editor.SceneEditing.Selection.Count == 1;
            if (!showCustomNodeOptions && Editor.SceneEditing.Selection.Count != 0)
            {
                showCustomNodeOptions = true;
                for (int i = 1; i < Editor.SceneEditing.Selection.Count; i++)
                {
                    if (Editor.SceneEditing.Selection[0].GetType() != Editor.SceneEditing.Selection[i].GetType())
                    {
                        showCustomNodeOptions = false;
                        break;
                    }
                }
            }
            if (showCustomNodeOptions)
            {
                Editor.SceneEditing.Selection[0].OnContextMenu(contextMenu, this);
            }

            ContextMenuShow?.Invoke(contextMenu);

            return contextMenu;
        }

        private ActorContextMenuAttribute GetContextMenu(Scripting.ScriptType type)
        {
            foreach (var actorAttribute in type.GetAttributes(false))
            {
                if (actorAttribute is ActorContextMenuAttribute actorContextMenuAttribute)
                    return actorContextMenuAttribute;
            }

            return null;
        }

        private string[] GetActorContextMenuPath(string path)
        {
            var splitPath = path.Split('/');
            return splitPath;
        }

        /// <summary>
        /// Shows the context menu on a given location (in the given control coordinates).
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="location">The location (within a given control).</param>
        private void ShowContextMenu(Control parent, Float2 location)
        {
            var contextMenu = CreateContextMenu();

            contextMenu.Show(parent, location);
        }

        private void OnExpandAllClicked(ContextMenuButton button)
        {
            for (int i = 0; i < Editor.SceneEditing.SelectionCount; i++)
            {
                if (Editor.SceneEditing.Selection[i] is ActorNode node)
                    node.TreeNode.ExpandAll();
            }
        }

        private void OnCollapseAllClicked(ContextMenuButton button)
        {
            for (int i = 0; i < Editor.SceneEditing.SelectionCount; i++)
            {
                if (Editor.SceneEditing.Selection[i] is ActorNode node)
                    node.TreeNode.CollapseAll();
            }
        }
    }
}
