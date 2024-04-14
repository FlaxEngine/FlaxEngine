// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using System.Xml;
using FlaxEditor.CustomEditors;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
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
        public bool LockObjects = false;

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
            if (LockObjects)
                return;

            // Update selected objects
            // TODO: use cached collection for less memory allocations
            undoRecordObjects = Editor.SceneEditing.Selection.ConvertAll(x => x.UndoRecordObject).Distinct();
            var objects = Editor.SceneEditing.Selection.ConvertAll(x => x.EditableObject).Distinct();
            Presenter.Select(objects);
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
