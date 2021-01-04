// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
using FlaxEditor.Content.Settings;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Context proxy object for C++ script files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpScriptProxy" />
    public class CppScriptProxy : ScriptProxy
    {
        /// <inheritdoc />
        public override string Name => "C++ Script";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is CppScriptItem;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            // Load templates
            var headerTemplate = File.ReadAllText(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.h"));
            var sourceTemplate = File.ReadAllText(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.cpp"));

            // Find the module that this script is being added (based on the path)
            var module = string.Empty;
            var project = TryGetProjectAtFolder(outputPath, out var moduleName);
            if (project != null)
            {
                module = moduleName.ToUpperInvariant() + "_API ";
            }

            // Format
            var gameSettings = GameSettings.Load();
            var scriptName = ScriptItem.CreateScriptName(outputPath);
            var filename = Path.GetFileNameWithoutExtension(outputPath);
            var copyrightComment = string.IsNullOrEmpty(gameSettings.CopyrightNotice) ? string.Empty : string.Format("// {0}{1}{1}", gameSettings.CopyrightNotice, Environment.NewLine);
            headerTemplate = headerTemplate.Replace("%copyright%", copyrightComment);
            headerTemplate = headerTemplate.Replace("%class%", scriptName);
            headerTemplate = headerTemplate.Replace("%module%", module);
            sourceTemplate = sourceTemplate.Replace("%filename%", filename);
            sourceTemplate = sourceTemplate.Replace("%copyright%", copyrightComment);
            sourceTemplate = sourceTemplate.Replace("%class%", scriptName);
            sourceTemplate = sourceTemplate.Replace("%filename%", filename);

            // Save
            File.WriteAllText(Path.ChangeExtension(outputPath, ".h"), headerTemplate, Encoding.UTF8);
            File.WriteAllText(outputPath, sourceTemplate, Encoding.UTF8);
        }

        /// <inheritdoc />
        public override string FileExtension => "cpp";

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x9c1c9c);
    }
}
