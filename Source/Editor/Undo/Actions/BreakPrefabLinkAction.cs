// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Actions
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> used to break/restore <see cref="Prefab"/> connection for the collection of <see cref="Actor"/> and <see cref="Script"/> objects.
    /// </summary>
    /// <remarks>
    /// This action assumes that all objects in the given actor hierarchy are using the same prefab asset.
    /// </remarks>
    /// <seealso cref="IUndoAction" />
    [Serializable]
    sealed class BreakPrefabLinkAction : IUndoAction
    {
        private struct Item
        {
            public Guid ID;
            public Guid PrefabID;
            public Guid PrefabObjectID;

            public Item(SceneObject obj, List<Item> nestedPrefabLinks)
            {
                ID = obj.ID;
                PrefabID = obj.PrefabID;
                PrefabObjectID = obj.PrefabObjectID;
                if (nestedPrefabLinks != null)
                {
                    // Check if this object comes from another nested prefab (to break link only from the top-level prefab)
                    Item nested;
                    nested.ID = ID;
                    var prefab = FlaxEngine.Content.Load<Prefab>(PrefabID);
                    if (prefab != null && 
                        prefab.GetNestedObject(ref PrefabObjectID, out nested.PrefabID, out nested.PrefabObjectID) && 
                        nested.PrefabID != Guid.Empty && 
                        nested.PrefabObjectID != Guid.Empty)
                    {
                        nestedPrefabLinks.Add(nested);
                    }
                }
            }
        }

        [Serialize]
        private readonly bool _isBreak;

        [Serialize]
        private Guid _actorId;

        [Serialize]
        private List<Item> _items = new();

        private BreakPrefabLinkAction(bool isBreak, Guid actorId)
        {
            _isBreak = isBreak;
            _actorId = actorId;
        }

        private BreakPrefabLinkAction(bool isBreak, Actor actor)
        {
            _isBreak = isBreak;
            _actorId = actor.ID;
            CollectIds(actor);
        }

        /// <summary>
        /// Creates a new undo action that in state for breaking prefab connection.
        /// </summary>
        /// <param name="actor">The target actor.</param>
        /// <returns>The action.</returns>
        public static BreakPrefabLinkAction Break(Actor actor)
        {
            if (actor == null)
                throw new ArgumentNullException(nameof(actor));
            return new BreakPrefabLinkAction(true, actor.ID);
        }

        /// <summary>
        /// Creates a new undo action that in state for linked prefab connection. Action on perform will undo that.
        /// </summary>
        /// <param name="actor">The target actor.</param>
        /// <returns>The action.</returns>
        public static BreakPrefabLinkAction Linked(Actor actor)
        {
            if (actor == null)
                throw new ArgumentNullException(nameof(actor));
            if (!actor.HasPrefabLink)
                throw new Exception("Cannot register missing prefab link.");
            return new BreakPrefabLinkAction(false, actor);
        }

        /// <inheritdoc />
        public string ActionString => _isBreak ? "Break prefab link" : "Link prefab";

        /// <inheritdoc />
        public void Do()
        {
            if (_isBreak)
                DoBreak();
            else
                DoLink();
        }

        /// <inheritdoc />
        public void Undo()
        {
            if (_isBreak)
                DoLink();
            else
                DoBreak();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _items.Clear();
        }

        private void DoLink()
        {
            var actor = Object.Find<Actor>(ref _actorId);
            if (actor == null)
                throw new Exception("Cannot link prefab. Missing actor.");

            Link(_items);
            Refresh(actor);
        }

        private void Link(List<Item> items)
        {
            for (int i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var obj = Object.Find<Object>(ref item.ID);
                if (obj != null)
                    SceneObject.Internal_LinkPrefab(Object.GetUnmanagedPtr(obj), ref item.PrefabID, ref item.PrefabObjectID);
            }
        }

        private void CollectIds(Actor actor, List<Item> nestedPrefabLinks = null)
        {
            _items.Add(new Item(actor, nestedPrefabLinks));

            for (int i = 0; i < actor.ChildrenCount; i++)
                CollectIds(actor.GetChild(i), nestedPrefabLinks);

            for (int i = 0; i < actor.ScriptsCount; i++)
                _items.Add(new Item(actor.GetScript(i), nestedPrefabLinks));
        }

        private void Refresh(Actor actor)
        {
            Editor.Instance.Scene.MarkSceneEdited(actor.Scene);
            Editor.Instance.Windows.PropertiesWin.Presenter.BuildLayout();
        }

        private void DoBreak()
        {
            var actor = Object.Find<Actor>(ref _actorId);
            if (actor == null)
                throw new Exception("Cannot break prefab link. Missing actor.");
            if (!actor.HasPrefabLink)
                throw new Exception("Cannot break missing prefab link.");

            // Cache 'prev' state and extract any nested prefab instances to remain
            _items.Clear();
            var nestedPrefabLinks = new List<Item>();
            CollectIds(actor, nestedPrefabLinks);

            // Break prefab linkage
            actor.BreakPrefabLink();

            // Restore prefab link for nested instances
            Link(nestedPrefabLinks);

            Refresh(actor);
        }
    }
}
