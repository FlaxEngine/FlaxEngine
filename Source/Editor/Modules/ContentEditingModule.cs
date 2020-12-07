// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.Windows;
using FlaxEngine;
using DockState = FlaxEditor.GUI.Docking.DockState;

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
                // Error
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
                // Check if there is a floating window that has the same size
                Vector2 defaultSize = window.DefaultSize;
                for (var i = 0; i < Editor.UI.MasterPanel.FloatingPanels.Count; i++)
                {
                    var win = Editor.UI.MasterPanel.FloatingPanels[i];

                    // Check if size is similar
                    if (Vector2.Abs(win.Size - defaultSize).LengthSquared < 100)
                    {
                        // Dock
                        window.Show(DockState.DockFill, win);
                        window.Focus();
                        return window;
                    }
                }

                // Show floating
                window.ShowFloating(defaultSize);
            }

            return window;
        }

        /// <summary>
        /// Determines whether specified new short name is valid name for the given content item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="shortName">The new short name.</param>
        /// <param name="hint">The hint text if name is invalid.</param>
        /// <returns>
        ///   <c>true</c> if name is valid; otherwise, <c>false</c>.
        /// </returns>
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

                // Find invalid characters
                if (Utilities.Utils.HasInvalidPathChar(shortName))
                {
                    hint = "Name contains invalid character.";
                    return false;
                }

                // Cache data
                string sourcePath = item.Path;
                string sourceFolder = System.IO.Path.GetDirectoryName(sourcePath);
                string extension = System.IO.Path.GetExtension(sourcePath);
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
