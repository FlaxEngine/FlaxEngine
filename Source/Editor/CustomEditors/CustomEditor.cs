// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Newtonsoft.Json;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Custom editor layout style modes.
    /// </summary>
    [HideInEditor]
    public enum DisplayStyle
    {
        /// <summary>
        /// Creates a separate group for the editor (drop down element). This is a default value.
        /// </summary>
        Group,

        /// <summary>
        /// Inlines editor contents into the property area without creating a drop-down group.
        /// </summary>
        Inline,

        /// <summary>
        /// Inlines editor contents into the parent editor layout. Won;t use property with label name.
        /// </summary>
        InlineIntoParent,
    }

    /// <summary>
    /// Base class for all custom editors used to present object(s) properties. Allows to extend game objects editing with more logic and customization.
    /// </summary>
    [HideInEditor]
    public abstract class CustomEditor
    {
        private LayoutElementsContainer _layout;
        private CustomEditorPresenter _presenter;
        private CustomEditor _parent;
        private readonly List<CustomEditor> _children = new List<CustomEditor>();
        private ValueContainer _values;
        private bool _isSetBlocked;
        private bool _skipChildrenRefresh;
        private bool _hasValueDirty;
        private bool _rebuildOnRefresh;
        private object _valueToSet;

        /// <summary>
        /// Gets the custom editor style.
        /// </summary>
        public virtual DisplayStyle Style => DisplayStyle.Group;

        /// <summary>
        /// Gets a value indicating whether single object is selected.
        /// </summary>
        public bool IsSingleObject => _values.IsSingleObject;

        /// <summary>
        /// Gets a value indicating whether selected objects are different values.
        /// </summary>
        public bool HasDifferentValues => _values.HasDifferentValues;

        /// <summary>
        /// Gets a value indicating whether selected objects are different types.
        /// </summary>
        public bool HasDifferentTypes => _values.HasDifferentTypes;

        /// <summary>
        /// Gets the values types array (without duplicates).
        /// </summary>
        public ScriptType[] ValuesTypes => _values.ValuesTypes;

        /// <summary>
        /// Gets the values.
        /// </summary>
        public ValueContainer Values => _values;

        /// <summary>
        /// Gets the parent editor.
        /// </summary>
        public CustomEditor ParentEditor => _parent;

        /// <summary>
        /// Gets the children editors (readonly).
        /// </summary>
        public List<CustomEditor> ChildrenEditors => _children;

        /// <summary>
        /// Gets the presenter control. It's the top most part of the Custom Editors layout.
        /// </summary>
        public CustomEditorPresenter Presenter => _presenter;

        /// <summary>
        /// Gets a value indicating whether setting value is blocked (during refresh).
        /// </summary>
        protected bool IsSetBlocked => _isSetBlocked;

        /// <summary>
        /// Gets a value indicating whether this editor needs value propagation up (value synchronization when one of the child editors changes value, used by the struct types).
        /// </summary>
        protected virtual bool NeedsValuePropagationUp => Values.HasValueType;

        /// <summary>
        /// The linked label used to show this custom editor. Can be null if not used (eg. editor is inlined or is using a very customized UI layout).
        /// </summary>
        public PropertyNameLabel LinkedLabel;

        internal virtual void Initialize(CustomEditorPresenter presenter, LayoutElementsContainer layout, ValueContainer values)
        {
            _layout = layout;
            _presenter = presenter;
            _values = values;

            var prev = CurrentCustomEditor;
            CurrentCustomEditor = this;

            _isSetBlocked = true;
            Initialize(layout);
            Refresh();
            _isSetBlocked = false;

            CurrentCustomEditor = prev;
        }

        internal static CustomEditor CurrentCustomEditor;

        internal void OnChildCreated(CustomEditor child)
        {
            // Link
            _children.Add(child);
            child._parent = this;
        }

        /// <summary>
        /// Rebuilds the editor layout. Cleans the whole UI with child elements/editors and then builds new hierarchy. Call it only when necessary.
        /// </summary>
        public void RebuildLayout()
        {
            // Special case for root objects to run normal layout build
            if (_presenter.Selection == Values)
            {
                _presenter.BuildLayout();
                return;
            }

            var values = _values;
            var presenter = _presenter;
            var layout = _layout;
            var control = layout.ContainerControl;
            var parent = _parent;
            var parentScrollV = (_presenter.Panel.Parent as Panel)?.VScrollBar?.Value ?? -1;

            control.IsLayoutLocked = true;
            control.DisposeChildren();

            layout.ClearLayout();
            Cleanup();

            _parent = parent;
            Initialize(presenter, layout, values);

            control.IsLayoutLocked = false;
            control.PerformLayout();

            // Restore scroll value
            if (parentScrollV > -1 && _presenter != null && _presenter.Panel.Parent is Panel panel && panel.VScrollBar != null)
                panel.VScrollBar.Value = parentScrollV;
        }

        /// <summary>
        /// Rebuilds the editor layout on editor refresh. Postponed after dirty value gets synced. Call it after <see cref="SetValue"/> to update editor after actual value assign.
        /// </summary>
        public void RebuildLayoutOnRefresh()
        {
            _rebuildOnRefresh = true;
        }

        /// <summary>
        /// Sets the request to skip the custom editor children refresh during next update. Can be used when editor layout has to be rebuild during the update itself.
        /// </summary>
        public void SkipChildrenRefresh()
        {
            _skipChildrenRefresh = true;
        }

        /// <summary>
        /// Initializes this editor.
        /// </summary>
        /// <param name="layout">The layout builder.</param>
        public abstract void Initialize(LayoutElementsContainer layout);

        /// <summary>
        /// Deinitializes this editor (unbind events and cleanup).
        /// </summary>
        protected virtual void Deinitialize()
        {
        }

        /// <summary>
        /// Cleanups this editor resources and child editors.
        /// </summary>
        internal void Cleanup()
        {
            Deinitialize();

            for (int i = 0; i < _children.Count; i++)
            {
                _children[i].Cleanup();
            }

            _children.Clear();
            _presenter = null;
            _parent = null;
            _hasValueDirty = false;
            _valueToSet = null;
            _values = null;
            _isSetBlocked = false;
            _rebuildOnRefresh = false;
            LinkedLabel = null;
        }

        internal void RefreshRoot()
        {
            try
            {
                for (int i = 0; i < _children.Count; i++)
                    _children[i].RefreshRootChild();
            }
            catch (TargetException ex)
            {
                // This happens when something (from root editor) calls the error.
                // Just handle it and rebuild UI. Some parts of the pipeline can report data problems via that exception.
                Editor.LogWarning("Exception while updating the root editors");
                Editor.LogWarning(ex);
                Presenter.BuildLayoutOnUpdate();
            }
        }

        internal void RefreshRootChild()
        {
            Refresh();

            for (int i = 0; i < _children.Count; i++)
                _children[i].RefreshInternal();
        }

        internal virtual void RefreshInternal()
        {
            if (_values == null)
                return;

            // Check if need to update value
            if (_hasValueDirty)
            {
                // Cleanup (won't retry update in case of exception)
                object val = _valueToSet;
                _hasValueDirty = false;
                _valueToSet = null;

                // Assign value
                _values.Set(_parent.Values, val);

                // Propagate values up (eg. when member of structure gets modified, also structure should be updated as a part of the other object)
                var obj = _parent;
                while (obj._parent != null && !(obj._parent is SyncPointEditor)) // && obj.NeedsValuePropagationUp)
                {
                    obj.Values.Set(obj._parent.Values, obj.Values);
                    obj = obj._parent;
                }

                OnUnDirty();
            }
            else
            {
                // Update values
                _values.Refresh(_parent.Values);
            }

            // Update itself
            _isSetBlocked = true;
            Refresh();
            _isSetBlocked = false;

            // Update children
            try
            {
                var childrenCount = _skipChildrenRefresh ? 0 : _children.Count;
                for (int i = 0; i < childrenCount; i++)
                    _children[i].RefreshInternal();
                _skipChildrenRefresh = false;
            }
            catch (TargetException ex)
            {
                // This happens when one of the edited objects changes its type.
                // Eg. from Label to TextBox, while both types were assigned to the same field of type Control.
                // It's valid, just rebuild the child editors and log the warning to keep it tracking.
                Editor.LogWarning("Exception while updating the child editors");
                Editor.LogWarning(ex);
                RebuildLayoutOnRefresh();
            }

            // Rebuild if flag is set
            if (_rebuildOnRefresh)
            {
                _rebuildOnRefresh = false;
                RebuildLayout();
            }
        }

        /// <summary>
        /// Refreshes this editor.
        /// </summary>
        public virtual void Refresh()
        {
            if (LinkedLabel != null)
            {
                // Prefab value diff
                if (Values.HasReferenceValue)
                {
                    var style = FlaxEngine.GUI.Style.Current;
                    LinkedLabel.HighlightStripColor = CanRevertReferenceValue ? style.BackgroundSelected * 0.8f : Color.Transparent;
                }
                // Default value diff
                else if (Values.HasDefaultValue)
                {
                    LinkedLabel.HighlightStripColor = CanRevertDefaultValue ? Color.Yellow * 0.8f : Color.Transparent;
                }
            }
        }

        internal void LinkLabel(PropertyNameLabel label)
        {
            LinkedLabel = label;
        }

        private void RevertDiffToDefault(CustomEditor editor)
        {
            if (editor.ChildrenEditors.Count == 0)
            {
                // Skip if no change detected
                if (!editor.Values.IsDefaultValueModified)
                    return;

                editor.SetValueToDefault();
            }
            else
            {
                for (int i = 0; i < editor.ChildrenEditors.Count; i++)
                {
                    RevertDiffToDefault(editor.ChildrenEditors[i]);
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether this editor can revert the value to default value.
        /// </summary>
        public bool CanRevertDefaultValue
        {
            get
            {
                if (!Values.IsDefaultValueModified)
                    return false;

                // Skip array items (show diff only on a bottom level properties and fields)
                if (ParentEditor != null && ParentEditor.Values.Type != ScriptType.Null && ParentEditor.Values.Type.IsArray)
                    return false;

                return true;
            }
        }

        /// <summary>
        /// Reverts the property value to default value (if has). Handles undo.
        /// </summary>
        public void RevertToDefaultValue()
        {
            if (!Values.HasDefaultValue)
                return;

            Editor.Log("Reverting object changes to default");

            RevertDiffToDefault(this);
        }

        /// <summary>
        /// Updates the default value assigned to the editor's values container. Sends the event down the custom editors hierarchy to propagate the change.
        /// </summary>
        /// <remarks>
        /// Has no effect on editors that don't have default value assigned.
        /// </remarks>
        public void RefreshDefaultValue()
        {
            if (!Values.HasDefaultValue)
                return;

            if (ParentEditor?.Values?.HasDefaultValue ?? false)
            {
                Values.RefreshDefaultValue(ParentEditor.Values.DefaultValue);
            }

            for (int i = 0; i < ChildrenEditors.Count; i++)
            {
                ChildrenEditors[i].RefreshDefaultValue();
            }
        }

        /// <summary>
        /// Clears all the default value of the container in the whole custom editors tree (this container and all children).
        /// </summary>
        public void ClearDefaultValueAll()
        {
            Values.ClearDefaultValue();

            for (int i = 0; i < ChildrenEditors.Count; i++)
            {
                ChildrenEditors[i].ClearDefaultValueAll();
            }
        }

        private void RevertDiffToReference(CustomEditor editor)
        {
            if (editor.ChildrenEditors.Count == 0)
            {
                // Skip if no change detected
                if (!editor.Values.IsReferenceValueModified)
                    return;

                editor.SetValueToReference();
            }
            else
            {
                for (int i = 0; i < editor.ChildrenEditors.Count; i++)
                {
                    RevertDiffToReference(editor.ChildrenEditors[i]);
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether this editor can revert the value to reference value.
        /// </summary>
        public bool CanRevertReferenceValue
        {
            get
            {
                if (!Values.IsReferenceValueModified)
                    return false;

                // Skip array items (show diff only on a bottom level properties and fields)
                if (ParentEditor != null && ParentEditor.Values.Type != ScriptType.Null && ParentEditor.Values.Type.IsArray)
                    return false;

                return true;
            }
        }

        /// <summary>
        /// Reverts the property value to reference value (if has). Handles undo.
        /// </summary>
        public void RevertToReferenceValue()
        {
            if (!Values.HasReferenceValue)
                return;

            Editor.Log("Reverting object changes to prefab");

            RevertDiffToReference(this);
        }

        /// <summary>
        /// Updates the reference value assigned to the editor's values container. Sends the event down the custom editors hierarchy to propagate the change.
        /// </summary>
        /// <remarks>
        /// Has no effect on editors that don't have reference value assigned.
        /// </remarks>
        public void RefreshReferenceValue()
        {
            if (!Values.HasReferenceValue)
                return;

            if (ParentEditor?.Values?.HasReferenceValue ?? false)
            {
                Values.RefreshReferenceValue(ParentEditor.Values.ReferenceValue);
            }

            for (int i = 0; i < ChildrenEditors.Count; i++)
            {
                ChildrenEditors[i].RefreshReferenceValue();
            }
        }


        /// <summary>
        /// Clears all the reference value of the container in the whole custom editors tree (this container and all children).
        /// </summary>
        public void ClearReferenceValueAll()
        {
            Values.ClearReferenceValue();

            for (int i = 0; i < ChildrenEditors.Count; i++)
            {
                ChildrenEditors[i].ClearReferenceValueAll();
            }
        }

        /// <summary>
        /// Copies the value to the system clipboard.
        /// </summary>
        public void Copy()
        {
            Editor.Log("Copy custom editor value");

            try
            {
                string text;
                if (ParentEditor is Dedicated.ScriptsEditor)
                {
                    // Script
                    text = JsonSerializer.Serialize(Values[0]);

                    // Remove properties that should be ignored when copy/pasting data
                    if (text == null)
                        text = string.Empty;
                    int idx = text.IndexOf("\"Actor\":");
                    if (idx != -1)
                    {
                        int endIdx = text.IndexOf("\n", idx);
                        if (endIdx != -1)
                            text = text.Remove(idx, endIdx - idx);
                    }
                    idx = text.IndexOf("\"Parent\":");
                    if (idx != -1)
                    {
                        int endIdx = text.IndexOf("\n", idx);
                        if (endIdx != -1)
                            text = text.Remove(idx, endIdx - idx);
                    }
                    idx = text.IndexOf("\"OrderInParent\":");
                    if (idx != -1)
                    {
                        int endIdx = text.IndexOf("\n", idx);
                        if (endIdx != -1)
                            text = text.Remove(idx, endIdx - idx);
                    }
                }
                else if (new ScriptType(typeof(FlaxEngine.Object)).IsAssignableFrom(Values.Type))
                {
                    // Object reference
                    text = JsonSerializer.GetStringID(Values[0] as FlaxEngine.Object);
                }
                else
                {
                    // Default
                    text = JsonSerializer.Serialize(Values[0]);
                }
                Clipboard.Text = text;
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Editor.LogError("Cannot copy property. See log for more info.");
            }
        }

        private bool GetClipboardObject(out object result, bool deserialize)
        {
            result = null;
            var text = Clipboard.Text;
            if (string.IsNullOrEmpty(text))
                return false;

            object obj;
            if (ParentEditor is Dedicated.ScriptsEditor)
            {
                // Script
                obj = Values[0];
                if (deserialize)
                {
                    if (Presenter.Undo != null && Presenter.Undo.Enabled)
                    {
                        using (new UndoBlock(Presenter.Undo, obj, "Paste values"))
                            JsonSerializer.Deserialize(obj, text);
                    }
                    else
                    {
                        JsonSerializer.Deserialize(obj, text);
                    }
                }
#pragma warning disable 618
                else if (Newtonsoft.Json.Schema.JsonSchema.Parse(text) == null)
#pragma warning restore 618
                {
                    return false;
                }
            }
            else if (new ScriptType(typeof(FlaxEngine.Object)).IsAssignableFrom(Values.Type))
            {
                // Object reference
                if (text.Length != 32)
                    return false;
                JsonSerializer.ParseID(text, out var id);
                obj = FlaxEngine.Object.Find<FlaxEngine.Object>(ref id);
            }
            else
            {
                // Default
                obj = JsonConvert.DeserializeObject(text, TypeUtils.GetType(Values.Type), JsonSerializer.Settings);
            }

            if (obj == null || Values.Type.IsInstanceOfType(obj))
            {
                result = obj;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Gets a value indicating whether can paste value from the system clipboard to the property value container.
        /// </summary>
        public bool CanPaste
        {
            get
            {
                try
                {
                    return GetClipboardObject(out _, false);
                }
                catch
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// Sets the value from the system clipboard.
        /// </summary>
        public void Paste()
        {
            Editor.Log("Paste custom editor value");

            try
            {
                if (GetClipboardObject(out var obj, true))
                {
                    SetValue(obj);
                }
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Editor.LogError("Cannot paste property value. See log for more info.");
            }
        }

        private Actor FindPrefabRoot(CustomEditor editor)
        {
            if (editor.Values[0] is Actor actor)
                return FindPrefabRoot(actor);
            if (editor.ParentEditor != null)
                return FindPrefabRoot(editor.ParentEditor);
            return null;
        }

        private Actor FindPrefabRoot(Actor actor)
        {
            if (actor is Scene)
                return null;
            if (actor.IsPrefabRoot)
                return actor;
            return FindPrefabRoot(actor.Parent);
        }

        private SceneObject FindObjectWithPrefabObjectId(Actor actor, ref Guid prefabObjectId)
        {
            if (actor.PrefabObjectID == prefabObjectId)
                return actor;

            for (int i = 0; i < actor.ScriptsCount; i++)
            {
                if (actor.GetScript(i).PrefabObjectID == prefabObjectId)
                {
                    var a = actor.GetScript(i);
                    if (a != null)
                        return a;
                }
            }

            for (int i = 0; i < actor.ChildrenCount; i++)
            {
                if (actor.GetChild(i).PrefabObjectID == prefabObjectId)
                {
                    var a = actor.GetChild(i);
                    if (a != null)
                        return a;
                }
            }

            return null;
        }

        /// <summary>
        /// Sets the editor value to the default value (if assigned).
        /// </summary>
        public void SetValueToDefault()
        {
            SetValue(Values.DefaultValue);
        }

        /// <summary>
        /// Sets the editor value to the reference value (if assigned).
        /// </summary>
        public void SetValueToReference()
        {
            // Special case for object references
            // If prefab object has reference to other object in prefab needs to revert to matching prefab instance object not the reference prefab object value
            if (Values.ReferenceValue is SceneObject referenceSceneObject && referenceSceneObject.HasPrefabLink)
            {
                if (Values.Count > 1)
                {
                    Editor.LogError("Cannot revert to reference value for more than one object selected.");
                    return;
                }

                Actor prefabInstanceRoot = FindPrefabRoot(this);
                if (prefabInstanceRoot == null)
                {
                    Editor.LogError("Cannot revert to reference value. Missing prefab instance root actor.");
                    return;
                }

                var prefabObjectId = referenceSceneObject.PrefabObjectID;
                var prefabInstanceRef = FindObjectWithPrefabObjectId(prefabInstanceRoot, ref prefabObjectId);
                if (prefabInstanceRef == null)
                {
                    Editor.LogWarning("Missing prefab instance reference in the prefab instance. Cannot revert to it.");
                }

                SetValue(prefabInstanceRef);

                return;
            }

            SetValue(Values.ReferenceValue);
        }

        /// <summary>
        /// Sets the object value. Actual update is performed during editor refresh in sync.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="token">The source editor token used by the value setter to support batching Undo actions (eg. for sliders or color pickers).</param>
        protected void SetValue(object value, object token = null)
        {
            if (_isSetBlocked)
                return;

            if (OnDirty(this, value, token))
            {
                _hasValueDirty = true;
                _valueToSet = value;
            }
        }

        /// <summary>
        /// Called when custom editor gets dirty (UI value has been modified).
        /// Allows to filter the event, block it or handle in a custom way.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="value">The value.</param>
        /// <param name="token">The source editor token used by the value setter to support batching Undo actions (eg. for sliders or color pickers).</param>
        /// <returns>True if allow to handle this event, otherwise false.</returns>
        protected virtual bool OnDirty(CustomEditor editor, object value, object token = null)
        {
            ParentEditor.OnDirty(editor, value, token);
            return true;
        }

        /// <summary>
        /// Called when custom editor sets the value to the object and resets the dirty state. Can be sued to perform custom work after editing the target object.
        /// </summary>
        protected virtual void OnUnDirty()
        {
        }

        /// <summary>
        /// Clears the token assigned with <see cref="OnDirty"/> parameter. Called on merged undo action end (eg. users stops using slider).
        /// </summary>
        protected virtual void ClearToken()
        {
            ParentEditor.ClearToken();
        }
    }
}
