// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
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
                contextMenu.AddButton(Editor.Windows.EditWin.IsPilotActorActive ? "Stop piloting actor" : "Pilot actor", Editor.UI.PilotActor);
            }

            contextMenu.AddSeparator();

            // Basic editing options

            b = contextMenu.AddButton("Rename", Rename);
            b.Enabled = isSingleActorSelected;

            b = contextMenu.AddButton("Duplicate", Editor.SceneEditing.Duplicate);
            b.Enabled = hasSthSelected;

            b = contextMenu.AddButton("Delete", Editor.SceneEditing.Delete);
            b.Enabled = hasSthSelected;

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Copy", Editor.SceneEditing.Copy);

            b.Enabled = hasSthSelected;
            contextMenu.AddButton("Paste", Editor.SceneEditing.Paste);

            b = contextMenu.AddButton("Cut", Editor.SceneEditing.Cut);
            b.Enabled = canEditScene;

            // Prefab options

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Create Prefab", Editor.Prefabs.CreatePrefab);
            b.Enabled = isSingleActorSelected &&
                        (Editor.SceneEditing.Selection[0] as ActorNode).CanCreatePrefab &&
                        Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;

            bool hasPrefabLink = canEditScene && isSingleActorSelected && (Editor.SceneEditing.Selection[0] as ActorNode).HasPrefabLink;

            b = contextMenu.AddButton("Select Prefab", Editor.Prefabs.SelectPrefab);
            b.Enabled = hasPrefabLink;

            b = contextMenu.AddButton("Break Prefab Link", Editor.Prefabs.BreakLinks);
            b.Enabled = hasPrefabLink;

            // Spawning actors options

            contextMenu.AddSeparator();

            var spawnMenu = contextMenu.AddChildMenu("New");
            var newActorCm = spawnMenu.ContextMenu;
            for (int i = 0; i < SpawnActorsGroups.Length; i++)
            {
                var group = SpawnActorsGroups[i];

                if (group.Types.Length == 1)
                {
                    var type = group.Types[0].Value;
                    newActorCm.AddButton(group.Types[0].Key, () => Spawn(type));
                }
                else
                {
                    var groupCm = newActorCm.AddChildMenu(group.Name).ContextMenu;
                    for (int j = 0; j < group.Types.Length; j++)
                    {
                        var type = group.Types[j].Value;
                        groupCm.AddButton(group.Types[j].Key, () => Spawn(type));
                    }
                }
            }

            // Custom options

            ContextMenuShow?.Invoke(contextMenu);

            return contextMenu;
        }

        /// <summary>
        /// Shows the context menu on a given location (in the given control coordinates).
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="location">The location (within a given control).</param>
        private void ShowContextMenu(Control parent, ref Vector2 location)
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
