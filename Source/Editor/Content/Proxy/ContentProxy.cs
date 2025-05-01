// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Base class for assets proxy objects used to manage <see cref="ContentItem"/>.
    /// </summary>
    [HideInEditor]
    public abstract class ContentProxy
    {
        /// <summary>
        /// Gets the asset type name (used by GUI, should be localizable).
        /// </summary>
        public abstract string Name { get; }

        /// <summary>
        /// Gets the default name for the new items created by this proxy.
        /// </summary>
        public virtual string NewItemName => Name;

        /// <summary>
        /// Determines whether this proxy is for the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if is proxy for asset item; otherwise, <c>false</c>.</returns>
        public abstract bool IsProxyFor(ContentItem item);

        /// <summary>
        /// Determines whether this proxy is for the specified asset.
        /// </summary>
        /// <returns><c>true</c> if is proxy for asset item; otherwise, <c>false</c>.</returns>
        public virtual bool IsProxyFor<T>() where T : Asset
        {
            return false;
        }

        /// <summary>
        /// Constructs the item for the file.
        /// </summary>
        /// <param name="path">The file path.</param>
        /// <returns>Created item or null.</returns>
        public virtual ContentItem ConstructItem(string path)
        {
            return null;
        }

        /// <summary>
        /// Gets a value indicating whether this proxy if for assets.
        /// </summary>
        public virtual bool IsAsset => false;

        /// <summary>
        /// Gets the file extension used by the items managed by this proxy.
        /// ALL LOWERCASE! WITHOUT A DOT! example: for 'myFile.TxT' proper extension is 'txt'
        /// </summary>
        public abstract string FileExtension { get; }

        /// <summary>
        /// Opens the specified item.
        /// </summary>
        /// <param name="editor"></param>
        /// <param name="item">The item.</param>
        /// <returns>Opened window or null if cannot do it.</returns>
        public abstract EditorWindow Open(Editor editor, ContentItem item);

        /// <summary>
        /// Gets a value indicating whether content items used by this proxy can be exported.
        /// </summary>
        public virtual bool CanExport => false;

        /// <summary>
        /// Exports the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="outputPath">The output path.</param>
        public virtual void Export(ContentItem item, string outputPath)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Determines whether the specified filename is valid for this proxy.
        /// </summary>
        /// <param name="filename">The filename.</param>
        /// <returns><c>true</c> if the filename is valid, otherwise <c>false</c>.</returns>
        public virtual bool IsFileNameValid(string filename)
        {
            return true;
        }

        /// <summary>
        /// Determines whether this proxy can create items in the specified target location.
        /// </summary>
        /// <param name="targetLocation">The target location.</param>
        /// <returns><c>true</c> if this proxy can create items in the specified target location; otherwise, <c>false</c>.</returns>
        public virtual bool CanCreate(ContentFolder targetLocation)
        {
            return false;
        }

        /// <summary>
        /// Determines whether this proxy can reimport specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if this proxy can reimport given item; otherwise, <c>false</c>.</returns>
        public virtual bool CanReimport(ContentItem item)
        {
            return CanCreate(item.ParentFolder);
        }

        /// <summary>
        /// Creates new item at the specified output path.
        /// </summary>
        /// <param name="outputPath">The output path.</param>
        /// <param name="arg">The custom argument provided for the item creation. Can be used as a source of data or null.</param>
        public virtual void Create(string outputPath, object arg)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Called when content window wants to show the context menu. Allows to add custom functions for the given asset type.
        /// </summary>
        /// <param name="menu">The menu.</param>
        /// <param name="item">The item.</param>
        public virtual void OnContentWindowContextMenu(ContextMenu menu, ContentItem item)
        {
        }

        /// <summary>
        /// Gets the unique accent color for that asset type.
        /// </summary>
        public abstract Color AccentColor { get; }

        /// <summary>
        /// Releases resources and unregisters the proxy utilities. Called during editor closing. For custom proxies from game/plugin modules it should be called before scripting reload.
        /// </summary>
        public virtual void Dispose()
        {
        }
    }
}
