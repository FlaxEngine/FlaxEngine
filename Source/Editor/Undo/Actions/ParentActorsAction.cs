// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Actions
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> used to change parent for <see cref="Actor"/> or <see cref="Script"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    class ParentActorsAction : IUndoAction
    {
        private struct Item
        {
            public Guid ID;
            public Guid Parent;
            public int OrderInParent;
            public Transform LocalTransform;
        }

        [Serialize]
        private bool _worldPositionsStays;

        [Serialize]
        private Guid _newParent;

        [Serialize]
        private int _newOrder;

        [Serialize]
        private Item[] _items;

        [Serialize]
        private Guid[] _idsForPrefab;

        [Serialize]
        private Guid[] _prefabIds;

        [Serialize]
        private Guid[] _prefabObjectIds;

        public ParentActorsAction(SceneObject[] objects, Actor newParent, int newOrder, bool worldPositionsStays = true)
        {
            // Sort source objects to provide deterministic behavior
            Array.Sort(objects, SortObjects);

            // Cache initial state for undo
            _worldPositionsStays = worldPositionsStays;
            _newParent = newParent.ID;
            _newOrder = newOrder;
            _items = new Item[objects.Length];
            for (int i = 0; i < objects.Length; i++)
            {
                var obj = objects[i];
                _items[i] = new Item
                {
                    ID = obj.ID,
                    Parent = obj.Parent?.ID ?? Guid.Empty,
                    OrderInParent = obj.OrderInParent,
                    LocalTransform = obj is Actor actor ? actor.LocalTransform : Transform.Identity,
                };
            }

            // Collect all objects that have prefab links so they can be restored on undo
            var prefabs = new List<SceneObject>();
            for (int i = 0; i < objects.Length; i++)
                GetAllPrefabs(prefabs, objects[i]);
            if (prefabs.Count != 0)
            {
                // Cache ids of all objects
                _idsForPrefab = new Guid[prefabs.Count];
                _prefabIds = new Guid[prefabs.Count];
                _prefabObjectIds = new Guid[prefabs.Count];
                for (int i = 0; i < prefabs.Count; i++)
                {
                    var obj = prefabs[i];
                    _idsForPrefab[i] = obj.ID;
                    _prefabIds[i] = obj.PrefabID;
                    _prefabObjectIds[i] = obj.PrefabObjectID;
                }
            }
        }

        private static int SortObjects(SceneObject a, SceneObject b)
        {
            // By parent
            var aParent = Object.GetUnmanagedPtr(a.Parent);
            var bParent = Object.GetUnmanagedPtr(b.Parent);
            if (aParent == bParent)
            {
                // By index in parent
                var aOrder = a.OrderInParent;
                var bOrder = b.OrderInParent;
                return aOrder.CompareTo(bOrder);
            }
            return aParent.CompareTo(bParent);
        }

        private static void GetAllPrefabs(List<SceneObject> result, SceneObject obj)
        {
            if (result.Contains(obj))
                return;
            if (obj.HasPrefabLink)
                result.Add(obj);
            if (obj is Actor actor)
            {
                for (int i = 0; i < actor.ScriptsCount; i++)
                    GetAllPrefabs(result, actor.GetScript(i));
                for (int i = 0; i < actor.ChildrenCount; i++)
                    GetAllPrefabs(result, actor.GetChild(i));
            }
        }

        public string ActionString => "Change parent";

        public void Do()
        {
            // Perform action
            var newParent = Object.Find<Actor>(ref _newParent);
            if (newParent == null)
            {
                Editor.LogError("Missing actor to change objects parent.");
                return;
            }
            var order = _newOrder;
            var scenes = new HashSet<Scene> { newParent.Scene };
            for (int i = 0; i < _items.Length; i++)
            {
                var item = _items[i];
                var obj = Object.Find<SceneObject>(ref item.ID);
                if (obj != null)
                {
                    scenes.Add(obj.Parent.Scene);
                    if (obj is Actor actor)
                        actor.SetParent(newParent, _worldPositionsStays, true);
                    else
                        obj.Parent = newParent;
                    if (order != -1)
                        obj.OrderInParent = order++;
                }
            }

            // Prefab links are broken by the C++ backend on actor reparenting

            // Mark scenes as edited
            foreach (var scene in scenes)
                Editor.Instance.Scene.MarkSceneEdited(scene);
        }

        public void Undo()
        {
            // Restore state
            for (int i = 0; i < _items.Length; i++)
            {
                var item = _items[i];
                var obj = Object.Find<SceneObject>(ref item.ID);
                if (obj != null)
                {
                    var parent = Object.Find<Actor>(ref item.Parent);
                    if (parent != null)
                        obj.Parent = parent;
                    if (obj is Actor actor)
                        actor.LocalTransform = item.LocalTransform;
                }
            }
            for (int j = 0; j < _items.Length; j++) // TODO: find a better way ensure the order is properly restored when moving back multiple objects
            for (int i = 0; i < _items.Length; i++)
            {
                var item = _items[i];
                var obj = Object.Find<SceneObject>(ref item.ID);
                if (obj != null)
                    obj.OrderInParent = item.OrderInParent;
            }

            // Restore prefab links (if any was in use)
            if (_idsForPrefab != null)
            {
                for (int i = 0; i < _idsForPrefab.Length; i++)
                {
                    var obj = Object.Find<SceneObject>(ref _idsForPrefab[i]);
                    if (obj != null && _prefabIds[i] != Guid.Empty)
                        SceneObject.Internal_LinkPrefab(Object.GetUnmanagedPtr(obj), ref _prefabIds[i], ref _prefabObjectIds[i]);
                }
            }
        }

        public void Dispose()
        {
            _items = null;
            _idsForPrefab = null;
            _prefabIds = null;
            _prefabObjectIds = null;
        }
    }
}
