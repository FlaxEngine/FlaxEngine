// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Main class for Custom Editors used to present selected objects properties and allow to modify them.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElementsContainer" />
    [HideInEditor]
    public class CustomEditorPresenter : LayoutElementsContainer
    {
        /// <summary>
        /// The panel control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.VerticalPanel" />
        public class PresenterPanel : VerticalPanel
        {
            private CustomEditorPresenter _presenter;

            /// <summary>
            /// Gets the presenter.
            /// </summary>
            public CustomEditorPresenter Presenter => _presenter;

            internal PresenterPanel(CustomEditorPresenter presenter)
            {
                _presenter = presenter;
                AnchorPreset = AnchorPresets.StretchAll;
                Offsets = Margin.Zero;
                IsScrollable = true;
            }

            /// <inheritdoc />
            public override void Update(float deltaTime)
            {
                // Update editors
                _presenter.Update();

                base.Update(deltaTime);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                base.OnDestroy();

                _presenter = null;
            }
        }

        /// <summary>
        /// The root editor. Mocks some custom editors events. Created a child editor for the selected objects.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.SyncPointEditor" />
        protected class RootEditor : SyncPointEditor
        {
            private readonly string _noSelectionText;
            private CustomEditor _overrideEditor;

            /// <summary>
            /// The selected objects editor.
            /// </summary>
            public CustomEditor Editor;

            /// <summary>
            /// Gets or sets the override custom editor used to edit selected objects.
            /// </summary>
            public CustomEditor OverrideEditor
            {
                get => _overrideEditor;
                set
                {
                    _overrideEditor = value;
                    RebuildLayout();
                }
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="RootEditor"/> class.
            /// </summary>
            /// <param name="noSelectionText">The text to show when no item is selected.</param>
            public RootEditor(string noSelectionText)
            {
                _noSelectionText = noSelectionText ?? "No selection";
            }

            /// <summary>
            /// Setups editor for selected objects.
            /// </summary>
            /// <param name="presenter">The presenter.</param>
            public void Setup(CustomEditorPresenter presenter)
            {
                Cleanup();
                Initialize(presenter, presenter, null);
            }

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                Presenter.BeforeLayout?.Invoke(layout);

                var selection = Presenter.Selection;
                selection.ClearReferenceValue();
                if (selection.Count > 0)
                {
                    if (_overrideEditor != null)
                    {
                        Editor = _overrideEditor;
                    }
                    else
                    {
                        var type = new ScriptType(typeof(object));
                        if (selection.HasDifferentTypes == false)
                            type = TypeUtils.GetObjectType(selection[0]);
                        Editor = CustomEditorsUtil.CreateEditor(type, false);
                    }

                    Editor.Initialize(Presenter, Presenter, selection);
                    OnChildCreated(Editor);
                }
                else
                {
                    var label = layout.Label(_noSelectionText, TextAlignment.Center);
                    label.Label.Height = 20.0f;
                }

                base.Initialize(layout);

                Presenter.AfterLayout?.Invoke(layout);
            }

            /// <inheritdoc />
            protected override void OnModified()
            {
                Presenter.OnModified();

                base.OnModified();
            }
        }

        /// <summary>
        /// The panel.
        /// </summary>
        public readonly PresenterPanel Panel;

        /// <summary>
        /// The selected objects editor (root, it generates actual editor for selection).
        /// </summary>
        protected readonly RootEditor Editor;

        /// <summary>
        /// The selected objects list (read-only).
        /// </summary>
        public readonly ValueContainer Selection = new ValueContainer(ScriptMemberInfo.Null);

        /// <summary>
        /// The undo object used by this editor.
        /// </summary>
        public readonly Undo Undo;

        /// <summary>
        /// Occurs when selection gets changed.
        /// </summary>
        public event Action SelectionChanged;

        /// <summary>
        /// Occurs when any property gets changed.
        /// </summary>
        public event Action Modified;

        /// <summary>
        /// Occurs when presenter wants to gather undo objects to record changes. Can be overriden to provide custom objects collection.
        /// </summary>
        public Func<CustomEditorPresenter, IEnumerable<object>> GetUndoObjects = presenter => presenter.Selection;

        /// <summary>
        /// Gets the amount of objects being selected.
        /// </summary>
        public int SelectionCount => Selection.Count;

        /// <summary>
        /// Gets or sets the override custom editor used to edit selected objects.
        /// </summary>
        public CustomEditor OverrideEditor
        {
            get => Editor.OverrideEditor;
            set => Editor.OverrideEditor = value;
        }

        /// <summary>
        /// Gets the root editor.
        /// </summary>
        public CustomEditor Root => Editor;

        /// <summary>
        /// Gets a value indicating whether build on update flag is set and layout will be updated during presenter update.
        /// </summary>
        public bool BuildOnUpdate => _buildOnUpdate;

        /// <summary>
        /// True if cache the expanded groups in this presenter, otherwise will disable this feature. Used to preserve the expanded groups using project cache.
        /// </summary>
        public bool CacheExpandedGroups;

        /// <summary>
        /// Occurs when before creating layout for the selected objects editor UI. Can be used to inject custom UI to the layout.
        /// </summary>
        public event Action<LayoutElementsContainer> BeforeLayout;

        /// <summary>
        /// Occurs when after creating layout for the selected objects editor UI. Can be used to inject custom UI to the layout.
        /// </summary>
        public event Action<LayoutElementsContainer> AfterLayout;

        private bool _buildOnUpdate;

        /// <summary>
        /// Initializes a new instance of the <see cref="CustomEditorPresenter"/> class.
        /// </summary>
        /// <param name="undo">The undo. It's optional.</param>
        /// <param name="noSelectionText">The custom text to display when no object is selected. Default is No selection.</param>
        public CustomEditorPresenter(Undo undo, string noSelectionText = null)
        {
            Undo = undo;
            Panel = new PresenterPanel(this);
            Editor = new RootEditor(noSelectionText);
            Editor.Initialize(this, this, null);
        }

        /// <summary>
        /// Selects the specified object.
        /// </summary>
        /// <param name="obj">The object.</param>
        public void Select(object obj)
        {
            if (obj == null)
            {
                Deselect();
                return;
            }
            if (Selection.Count == 1 && Selection[0] == obj)
                return;

            Selection.Clear();
            Selection.Add(obj);
            Selection.SetType(new ScriptType(obj.GetType()));

            OnSelectionChanged();
        }

        /// <summary>
        /// Selects the specified objects.
        /// </summary>
        /// <param name="objects">The objects.</param>
        public void Select(IEnumerable<object> objects)
        {
            if (objects == null)
            {
                Deselect();
                return;
            }
            var objectsArray = objects as object[] ?? objects.ToArray();
            if (Utils.ArraysEqual(objectsArray, Selection))
                return;

            Selection.Clear();
            Selection.AddRange(objectsArray);
            Selection.SetType(new ScriptType(objectsArray.GetType()));

            OnSelectionChanged();
        }

        /// <summary>
        /// Clears the selected objects.
        /// </summary>
        public void Deselect()
        {
            if (Selection.Count == 0)
                return;

            Selection.Clear();
            Selection.SetType(ScriptType.Null);

            OnSelectionChanged();
        }

        /// <summary>
        /// Builds the editors layout.
        /// </summary>
        public virtual void BuildLayout()
        {
            // Clear layout
            var panel = Panel.Parent as Panel;
            var parentScrollV = panel?.VScrollBar?.Value ?? -1;
            Panel.IsLayoutLocked = true;
            Panel.DisposeChildren();

            ClearLayout();
            Editor.Setup(this);

            Panel.IsLayoutLocked = false;
            Panel.PerformLayout();

            // Restore scroll value
            if (parentScrollV > -1)
                panel.VScrollBar.Value = parentScrollV;
        }

        /// <summary>
        /// Sets the request to build the editor layout on the next update.
        /// </summary>
        public void BuildLayoutOnUpdate()
        {
            _buildOnUpdate = true;
        }

        private void ExpandGroups(LayoutElementsContainer c, bool open)
        {
            if (c is Elements.GroupElement group)
            {
                if (open)
                    group.Panel.Open(false);
                else
                    group.Panel.Close(false);
            }

            foreach (var child in c.Children)
            {
                if (child is LayoutElementsContainer cc)
                    ExpandGroups(cc, open);
            }
        }

        /// <summary>
        /// Expands all the groups in this editor.
        /// </summary>
        public void OpenAllGroups()
        {
            ExpandGroups(this, true);
        }

        /// <summary>
        /// Closes all the groups in this editor.
        /// </summary>
        public void ClosesAllGroups()
        {
            ExpandGroups(this, false);
        }

        /// <summary>
        /// Invokes <see cref="Modified"/> event.
        /// </summary>
        public void OnModified()
        {
            Modified?.Invoke();
        }

        /// <summary>
        /// Called when selection gets changed.
        /// </summary>
        protected virtual void OnSelectionChanged()
        {
            BuildLayout();
            SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Updates custom editors. Refreshes UI values and applies changes to the selected objects.
        /// </summary>
        internal void Update()
        {
            if (_buildOnUpdate)
            {
                _buildOnUpdate = false;
                BuildLayout();
            }

            Editor?.RefreshInternal();
        }

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Panel;
    }
}
