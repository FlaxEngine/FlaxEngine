// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI.Docking;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Opening/editing asset windows module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class ContentEditingModule : EditorModule
    {
        internal ContentEditingModule(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Opens the specified asset in dedicated editor window.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <param name="disableAutoShow">True if disable automatic window showing. Used during workspace layout loading to deserialize it faster.</param>
        /// <returns>Opened window or null if cannot open item.</returns>
        public EditorWindow Open(Asset asset, bool disableAutoShow = false)
        {
            if (asset == null)
                throw new ArgumentNullException();
            var item = Editor.ContentDatabase.FindAsset(asset.ID);
            return item != null ? Open(item) : null;
        }

        /// <summary>
        /// Opens the specified item in dedicated editor window.
        /// </summary>
        /// <param name="item">The content item.</param>
        /// <param name="disableAutoShow">True if disable automatic window showing. Used during workspace layout loading to deserialize it faster.</param>
        /// <returns>Opened window or null if cannot open item.</returns>
        public EditorWindow Open(ContentItem item, bool disableAutoShow = false)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Check if any window is already editing this item
            var window = Editor.Windows.FindEditor(item);
            if (window != null)
            {
                window.Focus();
                return window;
            }

            // Find proxy object
            var proxy = Editor.ContentDatabase.GetProxy(item);
            if (proxy == null)
            {
                Editor.Log("Missing content proxy object for " + item);
                return null;
            }

            // Open
            try
            {
                window = proxy.Open(Editor, item);
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
            }
            if (window != null && !disableAutoShow)
            {
                Editor.Windows.Open(window);
            }

            return window;
        }

        /// <summary>
        /// Determines whether specified new short name is valid name for the given content item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="shortName">The new short name.</param>
        /// <param name="hint">The hint text if name is invalid.</param>
        /// <returns><c>true</c> if name is valid; otherwise, <c>false</c>.</returns>
        public bool IsValidAssetName(ContentItem item, string shortName, out string hint)
        {
            // Check if name is the same except has some chars in upper case and some in lower case
            if (shortName.Equals(item.ShortName, StringComparison.OrdinalIgnoreCase))
            {
                // The same file names but some chars have different case
            }
            else
            {
                // Validate length
                if (shortName.Length == 0)
                {
                    hint = "Name cannot be empty.";
                    return false;
                }
                if (shortName.Length > 60)
                {
                    hint = "Too long name.";
                    return false;
                }

                if (item.IsFolder && shortName.EndsWith("."))
                {
                    hint = "Name cannot end with '.'";
                    return false;
                }

                // Find invalid characters
                if (Utilities.Utils.HasInvalidPathChar(shortName))
                {
                    hint = "Name contains invalid character.";
                    return false;
                }

                // Check proxy name restrictions
                if (item is NewItem ni)
                {
                    if (!ni.Proxy.IsFileNameValid(shortName))
                    {
                        hint = "Name does not follow " + ni.Proxy.Name + " name restrictions !";
                        return false;
                    }
                }
                else
                {
                    var proxy = Editor.ContentDatabase.GetProxy(item);
                    if (proxy != null && !proxy.IsFileNameValid(shortName))
                    {
                        hint = "Name does not follow " + proxy.Name + " name restrictions !";
                        return false;
                    }
                }

                // Cache data
                string sourcePath = item.Path;
                string sourceFolder = System.IO.Path.GetDirectoryName(sourcePath);
                string extension = item.IsFolder ? "" : System.IO.Path.GetExtension(sourcePath);
                string destinationPath = StringUtils.CombinePaths(sourceFolder, shortName + extension);

                if (item.IsFolder)
                {
                    // Check if directory is unique
                    if (System.IO.Directory.Exists(destinationPath))
                    {
                        hint = "Already exists.";
                        return false;
                    }
                }
                else
                {
                    // Check if file is unique
                    if (System.IO.File.Exists(destinationPath))
                    {
                        hint = "Already exists.";
                        return false;
                    }
                }
            }

            hint = string.Empty;
            return true;
        }

        /// <summary>
        /// Clones the asset to the temporary folder.
        /// </summary>
        /// <param name="srcPath">The path of the source asset to clone.</param>
        /// <param name="resultPath">The result path.</param>
        /// <returns>True if failed, otherwise false.</returns>
        public bool FastTempAssetClone(string srcPath, out string resultPath)
        {
            var extension = System.IO.Path.GetExtension(srcPath);
            var id = Guid.NewGuid();
            resultPath = StringUtils.CombinePaths(Globals.TemporaryFolder, id.ToString("N")) + extension;

            if (CloneAssetFile(srcPath, resultPath, id))
                return true;

            return false;
        }

        /// <summary>
        /// Duplicates the asset file and changes it's ID.
        /// </summary>
        /// <param name="srcPath">The source file path.</param>
        /// <param name="dstPath">The destination file path.</param>
        /// <param name="dstId">The destination asset identifier.</param>
        /// <returns>True if cannot perform that operation, otherwise false.</returns>
        public bool CloneAssetFile(string srcPath, string dstPath, Guid dstId)
        {
            // Use internal call
            return Editor.Internal_CloneAssetFile(dstPath, srcPath, ref dstId);
        }
    }
}
