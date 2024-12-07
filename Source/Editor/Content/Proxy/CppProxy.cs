// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
using FlaxEditor.Content.Settings;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Context proxy object for C++ files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ScriptProxy" />
    public abstract class CppProxy : ScriptProxy
    {
        /// <summary>
        /// Gets the paths for header and source files to format.
        /// </summary>
        /// <param name="headerTemplate">The header template path.</param>
        /// <param name="sourceTemplate">The source template path.</param>
        protected abstract void GetTemplatePaths(out string headerTemplate, out string sourceTemplate);

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return false;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            // Find the module that this script is being added (based on the path)
            var module = string.Empty;
            var project = TryGetProjectAtFolder(outputPath, out var moduleName);
            if (project != null)
            {
                module = moduleName.ToUpperInvariant() + "_API ";
            }

            var gameSettings = GameSettings.Load();
            var scriptName = ScriptItem.CreateScriptName(outputPath);
            var filename = Path.GetFileNameWithoutExtension(outputPath);
            var copyrightComment = string.IsNullOrEmpty(gameSettings.CopyrightNotice) ? string.Empty : string.Format("// {0}{1}{1}", gameSettings.CopyrightNotice, Environment.NewLine);

            GetTemplatePaths(out var headerTemplatePath, out var sourceTemplatePath);
            if (headerTemplatePath != null)
            {
                var headerTemplate = File.ReadAllText(headerTemplatePath);
                headerTemplate = headerTemplate.Replace("%copyright%", copyrightComment);
                headerTemplate = headerTemplate.Replace("%class%", scriptName);
                headerTemplate = headerTemplate.Replace("%module%", module);
                headerTemplate = headerTemplate.Replace("%filename%", filename);
                File.WriteAllText(Path.ChangeExtension(outputPath, ".h"), headerTemplate, Encoding.UTF8);
            }
            if (sourceTemplatePath != null)
            {
                var sourceTemplate = File.ReadAllText(sourceTemplatePath);
                sourceTemplate = sourceTemplate.Replace("%copyright%", copyrightComment);
                sourceTemplate = sourceTemplate.Replace("%class%", scriptName);
                sourceTemplate = sourceTemplate.Replace("%module%", module);
                sourceTemplate = sourceTemplate.Replace("%filename%", filename);
                File.WriteAllText(outputPath, sourceTemplate, Encoding.UTF8);
            }
        }

        /// <inheritdoc />
        public override string FileExtension => "cpp";

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x9c1c9c);
    }

    /// <summary>
    /// Context proxy object for C++ script files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CppProxy" />
    [ContentContextMenu("New/C++/C++ Script")]
    public class CppScriptProxy : CppProxy
    {
        /// <inheritdoc />
        public override string Name => "C++ Script";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is CppScriptItem;
        }

        /// <inheritdoc />
        public override ContentItem ConstructItem(string path)
        {
            return new CppScriptItem(path);
        }

        /// <inheritdoc />
        protected override void GetTemplatePaths(out string headerTemplate, out string sourceTemplate)
        {
            headerTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.h");
            sourceTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.cpp");
        }
    }
    
    /// <summary>
    /// Context proxy object for C++ Actor files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CppProxy" />
    [ContentContextMenu("New/C++/C++ Actor")]
    public class CppActorProxy : CppProxy
    {
        /// <inheritdoc />
        public override string Name => "C++ Actor";

        /// <inheritdoc />
        protected override void GetTemplatePaths(out string headerTemplate, out string sourceTemplate)
        {
            headerTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ActorTemplate.h");
            sourceTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ActorTemplate.cpp");
        }
    }

    /// <summary>
    /// Context proxy object for C++ Json Asset files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CppProxy" />
    [ContentContextMenu("New/C++/C++ Function Library")]
    public class CppStaticClassProxy : CppProxy
    {
        /// <inheritdoc />
        public override string Name => "C++ Function Library";

        /// <inheritdoc />
        protected override void GetTemplatePaths(out string headerTemplate, out string sourceTemplate)
        {
            headerTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/CppStaticClassTemplate.h");
            sourceTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/CppStaticClassTemplate.cpp");
        }
    }

    /// <summary>
    /// Context proxy object for C++ Json Asset files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CppProxy" />
    [ContentContextMenu("New/C++/C++ Json Asset")]
    public class CppAssetProxy : CppProxy
    {
        /// <inheritdoc />
        public override string Name => "C++ Json Asset";

        /// <inheritdoc />
        protected override void GetTemplatePaths(out string headerTemplate, out string sourceTemplate)
        {
            headerTemplate = null;
            sourceTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/CppAssetTemplate.h");
            //sourceTemplate = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/CppAssetTemplate.cpp");
        }

        /// <inheritdoc />
        public override string FileExtension => "h";
    }
}
