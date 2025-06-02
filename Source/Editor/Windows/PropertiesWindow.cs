// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using FlaxEditor.CustomEditors;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Window used to present collection of selected object(s) properties in a grid. Supports Undo/Redo operations.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    /// <seealso cref="FlaxEditor.Windows.SceneEditorWindow" />
    public class PropertiesWindow : SceneEditorWindow, IPresenterOwner
    {
        private IEnumerable<object> undoRecordObjects;

        private readonly Dictionary<Guid, float> _actorScrollValues = new Dictionary<Guid, float>();
        private bool _lockObjects = false;

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <summary>
        /// The editor.
        /// </summary>
        public readonly CustomEditorPresenter Presenter;

        /// <summary>
        /// Indication of if the scale is locked.
        /// </summary>
        public bool ScaleLinked = false;

        /// <summary>
        /// Indication of if UI elements should size relative to the pivot point.
        /// </summary>
        public bool UIPivotRelative = true;

        /// <summary>
        /// Indication of if the properties window is locked on specific objects.
        /// </summary>
        public bool LockSelection
        {
            get => _lockObjects;
            set
            {
                if (value == _lockObjects)
                    return;
                _lockObjects = value;
                if (!value)
                    OnSelectionChanged();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertiesWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public PropertiesWindow(Editor editor)
        : base(editor, true, ScrollBars.Vertical)
        {
            Title = "Properties";
            Icon = editor.Icons.Build64;
            AutoFocus = true;

            Presenter = new CustomEditorPresenter(editor.Undo, null, this);
            Presenter.Panel.Parent = this;
            Presenter.GetUndoObjects += GetUndoObjects;
            Presenter.Features |= FeatureFlags.CacheExpandedGroups;

            VScrollBar.ValueChanged += OnScrollValueChanged;
            Editor.SceneEditing.SelectionChanged += OnSelectionChanged;
        }

        /// <inheritdoc />
        public override void OnSceneLoaded(Scene scene, Guid sceneId)
        {
            base.OnSceneLoaded(scene, sceneId);

            // Clear scroll values if new scene is loaded non additively
            if (Level.ScenesCount > 1)
                return;
            _actorScrollValues.Clear();
            if (LockSelection)
            {
                LockSelection = false;
                Presenter.Deselect();
            }
        }

        private void OnScrollValueChanged()
        {
            if (Editor.SceneEditing.SelectionCount > 1)
                return;

            // Clear first 10 scroll values to keep the memory down. Dont need to cache very single value in a scene. We could expose this as a editor setting in the future.
            if (_actorScrollValues.Count >= 20)
            {
                int i = 0;
                foreach (var e in _actorScrollValues)
                {
                    if (i >= 10)
                        break;
                    _actorScrollValues.Remove(e.Key);
                    i += 1;
                }
            }
            
            _actorScrollValues[Editor.SceneEditing.Selection[0].ID] = VScrollBar.TargetValue;
        }

        private IEnumerable<object> GetUndoObjects(CustomEditorPresenter customEditorPresenter)
        {
            return undoRecordObjects;
        }

        private void OnSelectionChanged()
        {
            if (LockSelection)
                return;

            // Update selected objects
            // TODO: use cached collection for less memory allocations
            undoRecordObjects = Editor.SceneEditing.Selection.ConvertAll(x => x.UndoRecordObject).Distinct();
            var objects = Editor.SceneEditing.Selection.ConvertAll(x => x.EditableObject).Distinct();
            Presenter.Select(objects);

            // Set scroll value of window if it exists
            if (Editor.SceneEditing.SelectionCount == 1)
                VScrollBar.TargetValue = _actorScrollValues.GetValueOrDefault(Editor.SceneEditing.Selection[0].ID, 0);
        }

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("ScaleLinked", ScaleLinked.ToString());
            writer.WriteAttributeString("UIPivotRelative", UIPivotRelative.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (bool.TryParse(node.GetAttribute("ScaleLinked"), out bool value1))
                ScaleLinked = value1;
            if (bool.TryParse(node.GetAttribute("UIPivotRelative"), out value1))
                UIPivotRelative = value1;
        }

        /// <inheritdoc />
        public EditorViewport PresenterViewport => Editor.Windows.EditWin.Viewport;

        /// <inheritdoc />
        public void Select(List<SceneGraphNode> nodes)
        {
            Editor.SceneEditing.Select(nodes);
        }
    }
}
