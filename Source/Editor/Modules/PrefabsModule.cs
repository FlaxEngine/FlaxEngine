// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Actions;
using FlaxEditor.Content;
using FlaxEditor.SceneGraph;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Prefabs management module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class PrefabsModule : EditorModule
    {
        /// <summary>
        /// Occurs before prefab asset creating. Argument is a target actor.
        /// </summary>
        public event Action<Actor> PrefabCreating;

        /// <summary>
        /// Occurs after prefab asset creating. Arguments is created prefab asset item.
        /// </summary>
        public event Action<PrefabItem> PrefabCreated;

        /// <summary>
        /// Occurs before applying changes to the prefab. Arguments are prefab and the target instance.
        /// </summary>
        public event Action<Prefab, Actor> PrefabApplying;

        /// <summary>
        /// Occurs after applying changes to the prefab. Arguments are prefab and the target instance.
        /// </summary>
        public event Action<Prefab, Actor> PrefabApplied;

        internal PrefabsModule(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Starts the creating prefab for the selected actor by showing the new item creation dialog in <see cref="ContentWindow"/>.
        /// </summary>
        /// <remarks>
        /// To create prefab manually (from code) use <see cref="PrefabManager.CreatePrefab"/> method.
        /// </remarks>
        public void CreatePrefab()
        {
            CreatePrefab(Editor.SceneEditing.Selection);
        }

        /// <summary>
        /// Starts the creating prefab for the selected actor by showing the new item creation dialog in <see cref="ContentWindow"/>.
        /// </summary>
        /// <remarks>
        /// To create prefab manually (from code) use <see cref="PrefabManager.CreatePrefab"/> method.
        /// </remarks>
        /// <param name="selection">The scene selection to use.</param>
        /// <param name="prefabWindow">The prefab window that creates it.</param>
        public void CreatePrefab(List<SceneGraphNode> selection, Windows.Assets.PrefabWindow prefabWindow = null)
        {
            if (selection == null)
                selection = Editor.SceneEditing.Selection;
            if (selection.Count == 1 && selection[0] is ActorNode actorNode && actorNode.CanCreatePrefab)
            {
                CreatePrefab(actorNode.Actor, true, prefabWindow);
            }
        }

        /// <summary>
        /// Starts the creating prefab for the given actor by showing the new item creation dialog in <see cref="ContentWindow"/>. User can specify the new asset name.
        /// </summary>
        /// <remarks>
        /// To create prefab manually (from code) use <see cref="PrefabManager.CreatePrefab"/> method.
        /// </remarks>
        /// <param name="actor">The root prefab actor.</param>
        public void CreatePrefab(Actor actor)
        {
            CreatePrefab(actor, true);
        }

        /// <summary>
        /// Starts the creating prefab for the given actor by showing the new item creation dialog in <see cref="ContentWindow"/>.
        /// </summary>
        /// <param name="actor">The root prefab actor.</param>
        /// <param name="rename">Allow renaming or not</param>
        /// <param name="prefabWindow">The prefab window that creates it.</param>
        public void CreatePrefab(Actor actor, bool rename, Windows.Assets.PrefabWindow prefabWindow = null)
        {
            // Skip in invalid states
            if (!Editor.StateMachine.CurrentState.CanEditContent)
                return;

            // Skip if cannot create assets in the given location
            if (!Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets)
                return;

            PrefabCreating?.Invoke(actor);

            var proxy = Editor.ContentDatabase.GetProxy<Prefab>();
            Editor.Windows.ContentWin.NewItem(proxy, actor, contentItem => OnPrefabCreated(contentItem, actor, prefabWindow), actor.Name, rename);
        }

        /// <summary>
        /// Opens a prefab editor window.
        /// </summary>
        public void OpenPrefab(ActorNode actorNode)
        {
            if (actorNode != null)
                OpenPrefab(actorNode.Actor.PrefabID);
        }

        /// <summary>
        /// Opens a prefab editor window.
        /// </summary>
        public void OpenPrefab(Guid prefabID = default)
        {
            if (prefabID == Guid.Empty)
            {
                var selection = Editor.SceneEditing.Selection.Where(x => x is ActorNode actorNode && actorNode.HasPrefabLink).ToList().BuildNodesParents();
                if (selection.Count == 0 || !((ActorNode)selection[0]).Actor.HasPrefabLink)
                    return;
                prefabID = ((ActorNode)selection[0]).Actor.PrefabID;
            }

            var item = Editor.ContentDatabase.Find(prefabID);
            if (item != null)
                Editor.ContentEditing.Open(item);
        }

        private void OnPrefabCreated(ContentItem contentItem, Actor actor, Windows.Assets.PrefabWindow prefabWindow)
        {
            if (contentItem is PrefabItem prefabItem)
            {
                PrefabCreated?.Invoke(prefabItem);
            }

            Undo undo = null;
            if (prefabWindow != null)
            {
                prefabWindow.MarkAsEdited();
                undo = prefabWindow.Undo;
            }
            else
            {
                // Skip in invalid states
                if (!Editor.StateMachine.CurrentState.CanEditScene)
                    return;
                undo = Editor.Undo;
                Editor.Scene.MarkSceneEdited(actor.Scene);
            }

            // Record undo for prefab creating (backend links the target instance with the prefab)
            if (undo.Enabled)
            {
                if (!actor)
                    return;
                var actorsList = new List<Actor>();
                Utilities.Utils.GetActorsTree(actorsList, actor);

                var actions = new IUndoAction[actorsList.Count];
                for (int i = 0; i < actorsList.Count; i++)
                    actions[i] = BreakPrefabLinkAction.Linked(actorsList[i]);
                undo.AddAction(new MultiUndoAction(actions));
            }

            Editor.Windows.PropertiesWin.Presenter.BuildLayout();
            if (prefabWindow != null)
                prefabWindow.Presenter.BuildLayout();
        }

        /// <summary>
        /// Breaks any prefab links for the selected objects. Supports undo/redo.
        /// </summary>
        public void BreakLinks()
        {
            // Skip in invalid states
            if (!Editor.StateMachine.CurrentState.CanEditScene)
                return;

            // Get valid objects (the top ones, C++ backend will process the child objects)
            var selection = Editor.SceneEditing.Selection.Where(x => x is ActorNode actorNode && actorNode.HasPrefabLink).ToList().BuildNodesParents();
            if (selection.Count == 0)
                return;

            // Perform action
            if (Editor.StateMachine.CurrentState.CanUseUndoRedo)
            {
                if (selection.Count == 1)
                {
                    var action = BreakPrefabLinkAction.Break(((ActorNode)selection[0]).Actor);
                    Undo.AddAction(action);
                    action.Do();
                }
                else
                {
                    var actions = new IUndoAction[selection.Count];
                    for (int i = 0; i < selection.Count; i++)
                    {
                        var action = BreakPrefabLinkAction.Break(((ActorNode)selection[i]).Actor);
                        actions[i] = action;
                        action.Do();
                    }
                    Undo.AddAction(new MultiUndoAction(actions));
                }
            }
            else
            {
                for (int i = 0; i < selection.Count; i++)
                {
                    ((ActorNode)selection[i]).Actor.BreakPrefabLink();
                }
            }
        }

        /// <summary>
        /// Selects in Content Window the prefab asset used by the selected objects.
        /// </summary>
        public void SelectPrefab()
        {
            // Get valid objects (the top ones, C++ backend will process the child objects)
            var selection = Editor.SceneEditing.Selection.Where(x => x is ActorNode actorNode && actorNode.HasPrefabLink).ToList().BuildNodesParents();
            if (selection.Count == 0)
                return;

            var prefabId = ((ActorNode)selection[0]).Actor.PrefabID;
            var prefab = FlaxEngine.Content.LoadAsync<Prefab>(prefabId);
            Editor.Windows.ContentWin.ClearItemsSearch();
            Editor.Windows.ContentWin.Select(prefab);
        }

        /// <summary>
        /// Applies the difference from the prefab object instance, saves the changes and synchronizes them with the active instances of the prefab asset.
        /// </summary>
        /// <remarks>
        /// Applies all the changes from not only the given actor instance but all actors created within that prefab instance.
        /// </remarks>
        /// <param name="instance">The modified instance.</param>
        public void ApplyAll(Actor instance)
        {
            // Validate input
            if (!instance)
                throw new ArgumentNullException(nameof(instance));
            if (!instance.HasPrefabLink || instance.PrefabID == Guid.Empty)
                throw new ArgumentException("The modified actor instance has missing prefab link.");

            var prefab = FlaxEngine.Content.LoadAsync<Prefab>(instance.PrefabID);
            if (prefab == null)
                throw new ArgumentException("Missing prefab to apply.");
            PrefabApplying?.Invoke(prefab, instance);

            // When applying changes to prefab from actor in level ignore it's root transformation (see ActorEditor.ProcessDiff)
            var originalTransform = instance.LocalTransform;
            if (instance.IsPrefabRoot && instance.HasScene)
                instance.LocalTransform = prefab.GetDefaultInstance().Transform;

            // Call backend
            var failed = PrefabManager.Internal_ApplyAll(FlaxEngine.Object.GetUnmanagedPtr(instance));
            instance.LocalTransform = originalTransform;
            if (failed)
                throw new Exception("Failed to apply the prefab. See log to learn more.");

            PrefabApplied?.Invoke(prefab, instance);
        }
    }
}
