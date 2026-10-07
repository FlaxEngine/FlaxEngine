// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
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
            bool hasSthSelected = Editor.SceneEditing.HasSthSelected;
            bool isSingleActorSelected = Editor.SceneEditing.SelectionCount == 1 && Editor.SceneEditing.Selection[0] is ActorNode;
            bool canEditScene = Editor.StateMachine.CurrentState.CanEditScene && Level.IsAnySceneLoaded;
            var inputOptions = Editor.Options.Options.Input;
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
            var firstSelection = hasSthSelected ? Editor.SceneEditing.Selection[0] as ActorNode : null;
            b = contextMenu.AddButton("Rename", inputOptions.Rename, RenameSelection);
            b.Enabled = hasSthSelected;
            b = contextMenu.AddButton("Duplicate", inputOptions.Duplicate, Editor.SceneEditing.Duplicate);
            b.Enabled = hasSthSelected && (firstSelection != null ? firstSelection.CanDuplicate : true);

            if (isSingleActorSelected && firstSelection?.Actor is not Scene)
            {
                // Convert actor type
                var convertMenu = contextMenu.AddChildMenu("Convert");
                convertMenu.ContextMenu.AutoSort = true;
                SceneEditingTools.AddActorContextMenu(convertMenu.ContextMenu, b => Editor.SceneEditing.Convert(((ScriptType)b.Tag).Type), true);
            }
            b = contextMenu.AddButton("Delete", inputOptions.Delete, Editor.SceneEditing.Delete);
            b.Enabled = hasSthSelected && (firstSelection != null ? firstSelection.CanDelete : true);

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Copy", inputOptions.Copy, Editor.SceneEditing.Copy);
            b.Enabled = hasSthSelected && (firstSelection != null ? firstSelection.CanCopyPaste : true);

            contextMenu.AddButton("Paste", inputOptions.Paste, Editor.SceneEditing.Paste);

            b = contextMenu.AddButton("Cut", inputOptions.Cut, Editor.SceneEditing.Cut);
            b.Enabled = canEditScene && hasSthSelected && (firstSelection != null ? firstSelection.CanCopyPaste : true);

            // Create option

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Parent to new Actor", inputOptions.GroupSelectedActors, Editor.SceneEditing.CreateParentForSelectedActors);
            b.Enabled = canEditScene && hasSthSelected && firstSelection?.Actor is not Scene;

            b = contextMenu.AddButton("Create Prefab", Editor.Prefabs.CreatePrefab);
            // Skip if we cannot create assets in the given location
            // Or DefaultPrefabFolder setting is invalid
            b.Enabled = isSingleActorSelected &&
                        (firstSelection != null ? firstSelection.CanCreatePrefab : false) &&
                        (Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets
                            || Editor.Prefabs.GetDefaultPrefabFolder() != null);

            bool hasPrefabLink = canEditScene && isSingleActorSelected && (firstSelection != null ? firstSelection.HasPrefabLink : false);
            if (hasPrefabLink)
            {
                contextMenu.AddButton("Open Prefab", () => Editor.Prefabs.OpenPrefab(firstSelection));
                contextMenu.AddButton("Select Prefab", Editor.Prefabs.SelectPrefab);
                contextMenu.AddButton("Break Prefab Link", Editor.Prefabs.BreakLinks);
            }

            // Load additional scenes
            if (!hasSthSelected)
            {
                var allScenes = FlaxEngine.Content.GetAllAssetsByType(typeof(SceneAsset));
                var loadedSceneIds = Editor.Instance.Scene.Root.ChildNodes.Select(node => node.ID).ToList();
                var unloadedScenes = allScenes.Where(sceneId => !loadedSceneIds.Contains(sceneId)).ToList();
                if (unloadedScenes.Count > 0)
                {
                    contextMenu.AddSeparator();
                    var childCM = contextMenu.GetOrAddChildMenu("Open Scene");
                    foreach (var sceneGuid in unloadedScenes)
                    {
                        if (FlaxEngine.Content.GetAssetInfo(sceneGuid, out var unloadedScene))
                        {
                            var splitPath = unloadedScene.Path.Split('/');
                            var sceneName = splitPath[^1];
                            if (splitPath[^1].EndsWith(".scene"))
                                sceneName = sceneName[..^6];
                            childCM.ContextMenu.AddButton(sceneName, () => { Editor.Instance.Scene.OpenScene(sceneGuid, true); });
                        }
                    }
                }
            }

            // Spawning actors
            contextMenu.AddSeparator();
            SceneEditingTools.AddActorContextMenu(contextMenu, b => Spawn(SceneEditingTools.SpawnActorMenu(b)));

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

        /// <summary>
        /// Shows the context menu on a given location (in the given control coordinates).
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="location">The location (within a given control).</param>
        internal void ShowContextMenu(Control parent, Float2 location)
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
