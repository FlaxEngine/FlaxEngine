// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Modules.SourceCodeEditing
{
    /// <summary>
    /// Default source code editor. Picks the best available editor on the current the platform.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.SourceCodeEditing.ISourceCodeEditor" />
    internal class DefaultSourceCodeEditor : ISourceCodeEditor
    {
        private ISourceCodeEditor _currentEditor;

        /// <summary>
        /// Initializes a new instance of the <see cref="DefaultSourceCodeEditor"/> class.
        /// </summary>
        public DefaultSourceCodeEditor()
        {
        }

        private void OnEditorAdded(ISourceCodeEditor editor)
        {
            if (editor == this)
                return;

            UpdateCurrentEditor();
        }

        private void OnEditorRemoved(ISourceCodeEditor editor)
        {
            if (editor != _currentEditor)
                return;

            UpdateCurrentEditor();
        }

        private void UpdateCurrentEditor()
        {
            var codeEditing = Editor.Instance.CodeEditing;
            var vsCode = codeEditing.GetInBuildEditor(CodeEditorTypes.VSCode);
            var rider = codeEditing.GetInBuildEditor(CodeEditorTypes.Rider);

#if PLATFORM_WINDOW
            // Favor the newest Visual Studio
            for (int i = (int)CodeEditorTypes.VS2019; i >= (int)CodeEditorTypes.VS2008; i--)
            {
                var visualStudio = codeEditing.GetInBuildEditor((CodeEditorTypes)i);
                if (visualStudio != null)
                {
                    _currentEditor = visualStudio;
                    return;
                }
            }
#elif PLATFORM_LINUX
            // Favor the VS Code
            if (vsCode != null)
            {
                _currentEditor = vsCode;
                return;
            }
#endif

            // Code editor fallback sequence
            if (vsCode != null)
                _currentEditor = vsCode;
            else if (rider != null)
                _currentEditor = rider;
            else
                _currentEditor = codeEditing.GetInBuildEditor(CodeEditorTypes.SystemDefault);
        }

        /// <inheritdoc />
        public string Name => "Default";

        /// <inheritdoc />
        public string GenerateProjectCustomArgs => null;

        /// <inheritdoc />
        public void OpenSolution()
        {
            _currentEditor?.OpenSolution();
        }

        /// <inheritdoc />
        public void OpenFile(string path, int line)
        {
            _currentEditor?.OpenFile(path, line);
        }

        /// <inheritdoc />
        public void OnFileAdded(string path)
        {
        }

        /// <inheritdoc />
        public void OnSelected(Editor editor)
        {
        }

        /// <inheritdoc />
        public void OnDeselected(Editor editor)
        {
        }

        /// <inheritdoc />
        public void OnAdded(Editor editor)
        {
            editor.CodeEditing.EditorAdded += OnEditorAdded;
            editor.CodeEditing.EditorRemoved += OnEditorRemoved;

            UpdateCurrentEditor();
        }

        /// <inheritdoc />
        public void OnRemoved(Editor editor)
        {
            _currentEditor = null;

            editor.CodeEditing.EditorAdded -= OnEditorAdded;
            editor.CodeEditing.EditorRemoved -= OnEditorRemoved;
        }
    }
}
