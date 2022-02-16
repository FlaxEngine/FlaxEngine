// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Window used to present collection of selected object(s) properties in a grid. Supports Undo/Redo operations.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    /// <seealso cref="FlaxEditor.Windows.SceneEditorWindow" />
    public class PropertiesWindow : SceneEditorWindow
    {
        private IEnumerable<object> undoRecordObjects;

        /// <summary>
        /// The editor.
        /// </summary>
        public readonly CustomEditorPresenter Presenter;

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertiesWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public PropertiesWindow(Editor editor)
        : base(editor, true, ScrollBars.Vertical)
        {
            Title = "Properties";
            AutoFocus = true;

            Presenter = new CustomEditorPresenter(editor.Undo, null, this);
            Presenter.Panel.Parent = this;
            Presenter.GetUndoObjects += GetUndoObjects;
            Presenter.Features |= FeatureFlags.CacheExpandedGroups;

            Editor.SceneEditing.SelectionChanged += OnSelectionChanged;
        }

        private IEnumerable<object> GetUndoObjects(CustomEditorPresenter customEditorPresenter)
        {
            return undoRecordObjects;
        }

        private void OnSelectionChanged()
        {
            // Update selected objects
            // TODO: use cached collection for less memory allocations
            undoRecordObjects = Editor.SceneEditing.Selection.ConvertAll(x => x.UndoRecordObject).Distinct();
            var objects = Editor.SceneEditing.Selection.ConvertAll(x => x.EditableObject).Distinct();
            Presenter.Select(objects);
        }
    }
}
