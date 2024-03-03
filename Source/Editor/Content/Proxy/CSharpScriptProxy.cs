// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
using FlaxEditor.Content.Settings;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Context proxy object for C# script files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpScriptProxy" />
    [ContentContextMenu("New/C# Script")]
    public class CSharpScriptProxy : ScriptProxy
    {
        /// <summary>
        /// The script files extension filter.
        /// </summary>
        public static readonly string ExtensionFiler = "*.cs";

        /// <inheritdoc />
        public override string Name => "C# Script";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is CSharpScriptItem;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            // Load template
            var templatePath = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.cs");
            var scriptTemplate = File.ReadAllText(templatePath);

            // Find the module that this script is being added (based on the path)
            var scriptNamespace = Editor.Instance.GameProject.Name;
            var project = TryGetProjectAtFolder(outputPath, out var moduleName);
            if (project != null)
            {
                scriptNamespace = moduleName.Length != 0 ? moduleName : project.Name;
            }
            scriptNamespace = scriptNamespace.Replace(" ", "");

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
        public override string FileExtension => "cs";

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x1c9c2b);
    }
}
