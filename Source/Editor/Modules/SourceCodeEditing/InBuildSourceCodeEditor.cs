// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEditor.Modules.SourceCodeEditing
{
    /// <summary>
    /// In-build source code editor.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.SourceCodeEditing.ISourceCodeEditor" />
    internal class InBuildSourceCodeEditor : ISourceCodeEditor
    {
        /// <summary>
        /// The type of the editor.
        /// </summary>
        public readonly CodeEditorTypes Type;

        /// <summary>
        /// Initializes a new instance of the <see cref="InBuildSourceCodeEditor"/> class.
        /// </summary>
        /// <param name="type">The type.</param>
        public InBuildSourceCodeEditor(CodeEditorTypes type)
        {
            Type = type;
            Name = CodeEditingManager.GetName(type);
        }

        /// <inheritdoc />
        public string Name { get; set; }

        /// <inheritdoc />
        public string GenerateProjectCustomArgs
        {
            get
            {
                switch (Type)
                {
                case CodeEditorTypes.VSCodeInsiders:
                case CodeEditorTypes.VSCode: return "-vscode -vs2022";
                case CodeEditorTypes.Rider: return "-vs2022";
                default: return null;
                }
            }
        }

        /// <inheritdoc />
        public void OpenSolution()
        {
            CodeEditingManager.OpenSolution(Type);
        }

        /// <inheritdoc />
        public void OpenFile(string path, int line)
        {
            CodeEditingManager.OpenFile(Type, path, line);
        }

        /// <inheritdoc />
        public void OnFileAdded(string path)
        {
            switch (Type)
            {
            case CodeEditorTypes.VS2008:
            case CodeEditorTypes.VS2010:
            case CodeEditorTypes.VS2012:
            case CodeEditorTypes.VS2013:
            case CodeEditorTypes.VS2015:
            case CodeEditorTypes.VS2017:
            case CodeEditorTypes.VS2019:
            case CodeEditorTypes.VS2022:
            case CodeEditorTypes.VS2026:
                // TODO: finish dynamic files adding to the project
                //Editor.Instance.ProgressReporting.GenerateScriptsProjectFiles.RunAsync();
                break;
            default:
                CodeEditingManager.OnFileAdded(Type, path);
                break;
            }
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
        }

        /// <inheritdoc />
        public void OnRemoved(Editor editor)
        {
        }
    }
}
