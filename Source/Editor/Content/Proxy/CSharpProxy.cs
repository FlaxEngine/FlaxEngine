// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
using FlaxEditor.Content.Settings;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Proxy object for C# files
    /// </summary>
    /// /// <seealso cref="FlaxEditor.Content.ScriptProxy" />
    public abstract class CSharpProxy : ScriptProxy
    {
        /// <summary>
        /// The script files extension filter.
        /// </summary>
        public static readonly string ExtensionFilter = "*.cs";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is CSharpScriptItem;
        }

        /// <summary>
        /// Gets the path for the C# template.
        /// </summary>
        /// <param name="path">The path to the template</param>
        protected abstract void GetTemplatePath(out string path);

        /// <inheritdoc />
        public override ContentItem ConstructItem(string path)
        {
            return new CSharpScriptItem(path);
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            // Load template
            GetTemplatePath(out var templatePath);
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
    
    /// <summary>
    /// Context proxy object for C# Script files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# Script")]
    public class CSharpScriptProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# Script";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ScriptTemplate.cs");
        }
    }
    
    /// <summary>
    /// Context proxy object for C# Actor files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# Actor")]
    public class CSharpActorProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# Actor";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ActorTemplate.cs");
        }
    }
    
    /// <summary>
    /// Context proxy object for C# GamePlugin files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# GamePlugin")]
    public class CSharpGamePluginProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# GamePlugin";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/GamePluginTemplate.cs");
        }
    }
    
    /// <summary>
    /// Context proxy object for empty C# files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# Empty File")]
    public class CSharpEmptyProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# Empty File";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/CSharpEmptyTemplate.cs");
        }
    }
    
    /// <summary>
    /// Context proxy object for empty C# class files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# Class")]
    public class CSharpEmptyClassProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# Class";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/EmptyClassTemplate.cs");
        }
    }
    
    /// <summary>
    /// Context proxy object for empty C# struct files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# Struct")]
    public class CSharpEmptyStructProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# Struct";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/EmptyStructTemplate.cs");
        }
    }
    
    /// <summary>
    /// Context proxy object for empty C# interface files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.CSharpProxy" />
    [ContentContextMenu("New/C#/C# Interface")]
    public class CSharpEmptyInterfaceProxy : CSharpProxy
    {
        /// <inheritdoc />
        public override string Name => "C# Interface";

        /// <inheritdoc />
        protected override void GetTemplatePath(out string path)
        {
            path = StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/EmptyInterfaceTemplate.cs");
        }
    }
}
