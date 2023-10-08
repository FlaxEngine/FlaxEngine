// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        [Serialize]
        private readonly bool _isBreak;

        [Serialize]
        private Guid _actorId;

        [Serialize]
        private Guid _prefabId;

        [Serialize]
        private Dictionary<Guid, Guid> _prefabObjectIds;

        private BreakPrefabLinkAction(bool isBreak, Guid actorId, Guid prefabId)
        {
            _isBreak = isBreak;
            _actorId = actorId;
            _prefabId = prefabId;
        }

        private BreakPrefabLinkAction(bool isBreak, Actor actor)
        {
            _isBreak = isBreak;
            _actorId = actor.ID;
            _prefabId = actor.PrefabID;

            _prefabObjectIds = new Dictionary<Guid, Guid>(1024);
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
            return new BreakPrefabLinkAction(true, actor.ID, Guid.Empty);
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
            _prefabObjectIds.Clear();
        }

        private void DoLink()
        {
            if (_prefabObjectIds == null)
                throw new Exception("Cannot link prefab. Missing objects Ids mapping.");

            var actor = Object.Find<Actor>(in _actorId);
            if (actor == null)
                throw new Exception("Cannot link prefab. Missing actor.");

            // Restore cached links
            foreach (var e in _prefabObjectIds)
            {
                var objId = e.Key;
                var prefabObjId = e.Value;

                var obj = Object.Find<Object>(in objId);
                if (obj is Actor)
                {
                    Actor.Internal_LinkPrefab(Object.GetUnmanagedPtr(obj), ref _prefabId, ref prefabObjId);
                }
                else if (obj is Script)
                {
                    Script.Internal_LinkPrefab(Object.GetUnmanagedPtr(obj), ref _prefabId, ref prefabObjId);
                }
            }

            Editor.Instance.Scene.MarkSceneEdited(actor.Scene);
            Editor.Instance.Windows.PropertiesWin.Presenter.BuildLayout();
        }

        private void CollectIds(Actor actor)
        {
            _prefabObjectIds.Add(actor.ID, actor.PrefabObjectID);

            for (int i = 0; i < actor.ChildrenCount; i++)
            {
                CollectIds(actor.GetChild(i));
            }

            for (int i = 0; i < actor.ScriptsCount; i++)
            {
                var script = actor.GetScript(i);
                _prefabObjectIds.Add(script.ID, script.PrefabObjectID);
            }
        }

        private void DoBreak()
        {
            var actor = Object.Find<Actor>(in _actorId);
            if (actor == null)
                throw new Exception("Cannot break prefab link. Missing actor.");
            if (!actor.HasPrefabLink)
                throw new Exception("Cannot break missing prefab link.");

            if (_prefabObjectIds == null)
                _prefabObjectIds = new Dictionary<Guid, Guid>(1024);
            else
                _prefabObjectIds.Clear();
            CollectIds(actor);

            _prefabId = actor.PrefabID;

            actor.BreakPrefabLink();

            Editor.Instance.Scene.MarkSceneEdited(actor.Scene);
            Editor.Instance.Windows.PropertiesWin.Presenter.BuildLayout();
        }
    }
}
