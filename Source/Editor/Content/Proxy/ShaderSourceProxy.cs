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
    /// Base class for shader source files.
    /// </summary>
    public abstract class ShaderBaseProxy : ContentProxy
    {
        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            // Allow to create shaders only under '<root>/Source/Shaders' directory and it's sub-dirs
            var prevTargetLocation = targetLocation;
            while (targetLocation.ParentFolder?.ParentFolder != null)
            {
                prevTargetLocation = targetLocation;
                targetLocation = targetLocation.ParentFolder;
            }
            return targetLocation.ShortName == "Source" && prevTargetLocation.ShortName == "Shaders";
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            Editor.Instance.CodeEditing.OpenFile(item.Path);
            return null;
        }
    }

    /// <summary>
    /// Context proxy object for shader source files (represented by <see cref="ShaderSourceItem"/>).
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentProxy" />
    [ContentContextMenu("New/Shader Source (.shader)")]
    public class ShaderSourceProxy : ShaderBaseProxy
    {
        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            // Load template
            var shaderTemplate = File.ReadAllText(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Scripting/ShaderTemplate.shader"));

            // Format
            var gameSettings = GameSettings.Load();
            var copyrightComment = string.IsNullOrEmpty(gameSettings.CopyrightNotice) ? string.Empty : string.Format("// {0}{1}{1}", gameSettings.CopyrightNotice, Environment.NewLine);
            shaderTemplate = shaderTemplate.Replace("%copyright%", copyrightComment);

            // Save
            File.WriteAllText(outputPath, shaderTemplate, Encoding.UTF8);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x7542f5);

        /// <inheritdoc />
        public override string FileExtension => "shader";

        /// <inheritdoc />
        public override string Name => "Shader Source";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is ShaderSourceItem;
        }
    }

    /// <summary>
    /// Context proxy object for shader header files.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentProxy" />
    [ContentContextMenu("New/Shader Header (.hlsl)")]
    public class ShaderHeaderProxy : ShaderBaseProxy
    {
        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            File.WriteAllText(outputPath, "\n", Encoding.UTF8);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x2545a5);

        /// <inheritdoc />
        public override string FileExtension => "hlsl";

        /// <inheritdoc />
        public override string Name => "Shader Header";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return false;
        }
    }
}
