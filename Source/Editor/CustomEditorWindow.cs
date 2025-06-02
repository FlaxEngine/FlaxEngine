// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.Docking;
using FlaxEditor.Windows;
using FlaxEngine.GUI;

namespace FlaxEditor
{
    /// <summary>
    /// Base class for custom editor window that can create custom GUI layout and expose various functionalities to the user.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    public abstract class CustomEditorWindow : CustomEditor
    {
        private class Win : EditorWindow
        {
            private readonly CustomEditorPresenter _presenter;
            private CustomEditorWindow _customEditor;

            public Win(CustomEditorWindow customEditor)
            : base(Editor.Instance, false, ScrollBars.Vertical)
            {
                Title = customEditor.GetType().Name;
                _customEditor = customEditor;

                _presenter = new CustomEditorPresenter(null);
                _presenter.Panel.Parent = this;

                Set(customEditor);
            }

            private void Set(CustomEditorWindow value)
            {
                _customEditor = value;
                _presenter.OverrideEditor = value;
                _presenter.Select(value);
            }

            /// <inheritdoc />
            protected override void OnShow()
            {
                base.OnShow();

                _presenter.BuildLayout();
            }

            /// <inheritdoc />
            protected override void OnClose()
            {
                Set(null);

                base.OnClose();
            }
        }

        private readonly Win _win;

        /// <summary>
        /// Gets the editor window.
        /// </summary>
        public EditorWindow Window => _win;

        /// <summary>
        /// Initializes a new instance of the <see cref="CustomEditorWindow"/> class.
        /// </summary>
        protected CustomEditorWindow()
        {
            _win = new Win(this);
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="CustomEditorWindow"/> class.
        /// </summary>
        ~CustomEditorWindow()
        {
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
        }

        private void OnScriptsReloadBegin()
        {
            // Skip if window type is not from game script assembly (eg. plugin code)
            var type = GetType();
            if (!FlaxEngine.Scripting.IsTypeFromGameScripts(type))
                return;

            if (!Window.IsHidden)
            {
                Editor.Instance.Windows.AddToRestore(this);
            }
            Window.Close();
            Window.Dispose();
        }

        /// <summary>
        /// Shows the window.
        /// </summary>
        /// <param name="state">Initial window state.</param>
        /// <param name="toDock">The panel to dock to, if any.</param>
        /// <param name="autoSelect">Only used if <paramref name="toDock"/> is set. If true the window will be selected after docking it.</param>
        /// <param name="splitterValue">The splitter value to use if toDock is not null. If not specified, a default value will be used.</param>
        public void Show(DockState state = DockState.Float, DockPanel toDock = null, bool autoSelect = true, float? splitterValue = null)
        {
            _win.Show(state, toDock, autoSelect, splitterValue);
        }
    }
}
