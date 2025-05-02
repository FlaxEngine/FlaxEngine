// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Actions
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> used to add/remove <see cref="Script"/> from the <see cref="Actor"/>.
    /// </summary>
    /// <seealso cref="IUndoAction" />
    [Serializable]
    sealed class AddRemoveScript : IUndoAction
    {
        [Serialize]
        private bool _isAdd;

        [Serialize]
        private Guid _scriptId;

        [Serialize]
        private Guid _prefabId;

        [Serialize]
        private Guid _prefabObjectId;

        [Serialize]
        private string _scriptTypeName;

        [Serialize]
        private string _scriptData;

        [Serialize]
        private Guid _parentId;

        [Serialize]
        private int _orderInParent;

        [Serialize]
        private bool _enabled;

        internal AddRemoveScript(bool isAdd, Script script)
        {
            _isAdd = isAdd;
            _scriptId = script.ID;
            _scriptTypeName = script.TypeName;
            _prefabId = script.PrefabID;
            _prefabObjectId = script.PrefabObjectID;
            try
            {
                _scriptData = FlaxEngine.Json.JsonSerializer.Serialize(script);
            }
            catch (Exception ex)
            {
                _scriptData = null;
                Debug.LogError("Failed to serialize script data for Undo due to exception");
                Debug.LogException(ex);
            }
            _parentId = script.Actor.ID;
            _orderInParent = script.OrderInParent;
            _enabled = script.Enabled;
        }

        internal AddRemoveScript(bool isAdd, Actor parentActor, ScriptType scriptType)
        {
            _isAdd = isAdd;
            _scriptId = Guid.NewGuid();
            _scriptTypeName = scriptType.TypeName;
            _scriptData = null;
            _parentId = parentActor.ID;
            _orderInParent = -1;
            _enabled = true;
        }

        public int GetOrderInParent()
        {
            return _orderInParent;
        }

        /// <summary>
        /// Creates a new added script undo action.
        /// </summary>
        /// <param name="script">The new script.</param>
        /// <returns>The action.</returns>
        public static AddRemoveScript Added(Script script)
        {
            if (script == null)
                throw new ArgumentNullException(nameof(script));
            return new AddRemoveScript(true, script);
        }

        /// <summary>
        /// Creates a new add script undo action.
        /// </summary>
        /// <param name="parentActor">The parent actor.</param>
        /// <param name="scriptType">The script type.</param>
        /// <returns>The action.</returns>
        public static AddRemoveScript Add(Actor parentActor, ScriptType scriptType)
        {
            if (parentActor == null)
                throw new ArgumentNullException(nameof(parentActor));
            if (!scriptType)
                throw new ArgumentNullException(nameof(scriptType));
            return new AddRemoveScript(true, parentActor, scriptType);
        }

        /// <summary>
        /// Creates a new remove script undo action.
        /// </summary>
        /// <param name="script">The script.</param>
        /// <returns>The action.</returns>
        public static AddRemoveScript Remove(Script script)
        {
            if (script == null)
                throw new ArgumentNullException(nameof(script));
            return new AddRemoveScript(false, script);
        }

        /// <inheritdoc />
        public string ActionString => _isAdd ? "Add script" : "Remove script";

        /// <inheritdoc />
        public void Do()
        {
            if (_isAdd)
                DoAdd();
            else
                DoRemove();
        }

        /// <inheritdoc />
        public void Undo()
        {
            if (_isAdd)
                DoRemove();
            else
                DoAdd();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _scriptTypeName = null;
            _scriptData = null;
        }

        private void DoRemove()
        {
            // Remove script (it could be removed by sth else, just check it)
            var script = Object.Find<Script>(ref _scriptId);
            if (!script)
            {
                Editor.LogWarning("Missing script.");
                return;
            }
            if (script.Actor)
                Editor.Instance.Scene.MarkSceneEdited(script.Scene);
            Object.Destroy(ref script);
        }

        private void DoAdd()
        {
            // Restore script
            var parentActor = Object.Find<Actor>(ref _parentId);
            if (parentActor == null)
            {
                Editor.LogWarning("Missing parent actor.");
                return;
            }
            var type = TypeUtils.GetType(_scriptTypeName);
            if (!type)
            {
                Editor.LogWarning("Cannot find script type " + _scriptTypeName);
                return;
            }
            var script = type.CreateInstance() as Script;
            if (script == null)
            {
                Editor.LogWarning("Cannot create script of type " + _scriptTypeName);
                return;
            }
            Object.Internal_ChangeID(Object.GetUnmanagedPtr(script), ref _scriptId);
            if (_scriptData != null)
                FlaxEngine.Json.JsonSerializer.Deserialize(script, _scriptData);
            script.Enabled = _enabled;
            script.Parent = parentActor;
            if (_orderInParent != -1)
                script.OrderInParent = _orderInParent;
            _orderInParent = script.OrderInParent; // Ensure order is correct for script that want to use it later
            if (_prefabObjectId != Guid.Empty)
                SceneObject.Internal_LinkPrefab(Object.GetUnmanagedPtr(script), ref _prefabId, ref _prefabObjectId);
            Editor.Instance.Scene.MarkSceneEdited(parentActor.Scene);
        }
    }
}
