// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
using FlaxEditor.Content.Settings;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Context proxy object for script files (represented by <see cref="ScriptItem"/>).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentProxy" />
    public abstract class ScriptProxy : ContentProxy
    {
        /// <summary>
        /// Tries the get project that is related to the given source file path. Works only for source files located under Source folder of the project.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="moduleName">The name of the module where the source file lays. Computed from path. Might be invalid..</param>
        /// <returns>The found project or null.</returns>
        protected ProjectInfo TryGetProjectAtFolder(string path, out string moduleName)
        {
            moduleName = string.Empty;
            var projects = Editor.Instance.GameProject.GetAllProjects();
            foreach (var project in projects)
            {
                var projectSourceFolderPath = StringUtils.CombinePaths(project.ProjectFolderPath, "Source");
                if (path == projectSourceFolderPath)
                    return project;
                if (path.StartsWith(projectSourceFolderPath))
                {
                    var localProjectPath = path.Substring(projectSourceFolderPath.Length + 1);
                    var split = localProjectPath.IndexOf('/');
                    if (split != -1)
                    {
                        moduleName = localProjectPath.Substring(0, split);
                    }
                    return project;
                }
            }
            return null;
        }

        /// <inheritdoc />
        public override string NewItemName => "MyScript";

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            if (targetLocation.CanHaveScripts)
            {
                var path = targetLocation.Path;
                var projects = Editor.Instance.GameProject.GetAllProjects();
                foreach (var project in projects)
                {
                    var projectSourceFolderPath = StringUtils.CombinePaths(project.ProjectFolderPath, "Source");
                    if (path == projectSourceFolderPath)
                        return false;
                    if (path.StartsWith(projectSourceFolderPath))
                        return true;
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool IsFileNameValid(string filename)
        {
            if (char.IsDigit(filename[0]))
                return false;
            if (filename.Equals("Script"))
                return false;
            return true;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            // Load template
            var templatePath = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.cs");
            var scriptTemplate = File.ReadAllText(templatePath);
            var scriptNamespace = Editor.Instance.GameProject.Name.Replace(" ", "") + ".Source";

            // Get directories
            var sourceDirectory = Globals.ProjectFolder.Replace('\\', '/') + "/Source/";
            var outputDirectory = new FileInfo(outputPath).DirectoryName.Replace('\\', '/');

            // Generate "sub" namespace from relative path between source root and output path
            // NOTE: Could probably use Replace instead substring, but this is faster :)
            var subNamespaceStr = outputDirectory.Substring(sourceDirectory.Length - 1).Replace(" ", "").Replace(".", "").Replace('/', '.');

            // Replace all namespace invalid characters
            // NOTE: Need to handle number sequence at the beginning since namespace which begin with numeric sequence are invalid
            string subNamespace = string.Empty;
            bool isStart = true;
            for (int pos = 0; pos < subNamespaceStr.Length; pos++)
            {
                var c = subNamespaceStr[pos];

                if (isStart)
                {
                    // Skip characters that cannot start the sub namespace
                    if (char.IsLetter(c))
                    {
                        isStart = false;
                        subNamespace += '.';
                        subNamespace += c;
                    }
                }
                else
                {
                    // Add only valid characters
                    if (char.IsLetterOrDigit(c) || c == '_')
                    {
                        subNamespace += c;
                    }
                    // Check for sub namespace start
                    else if (c == '.')
                    {
                        isStart = true;
                    }
                }
            }

            // Append if valid
            if (subNamespace.Length > 1)
                scriptNamespace += subNamespace;

            // Format
            var gameSettings = GameSettings.Load();
            var scriptName = ScriptItem.CreateScriptName(outputPath);
            var copyrightComment = string.IsNullOrEmpty(gameSettings.CopyrightNotice) ? string.Empty : string.Format("// {0}{1}{1}", gameSettings.CopyrightNotice, Environment.NewLine);
            scriptTemplate = scriptTemplate.Replace("%copyright%", copyrightComment);
            scriptTemplate = scriptTemplate.Replace("%class%", scriptName);
            scriptTemplate = scriptTemplate.Replace("%namespace%", scriptNamespace);

            // Save
            File.WriteAllText(outputPath, scriptTemplate, Encoding.UTF8);
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            Editor.Instance.CodeEditing.OpenFile(item.Path);
            return null;
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x1c9c2b);
    }
}
