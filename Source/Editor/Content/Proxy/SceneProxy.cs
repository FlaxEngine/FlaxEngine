// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content proxy for <see cref="SceneItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetBaseProxy" />
    [ContentContextMenu("New/Scene")]
    public sealed class SceneProxy : JsonAssetBaseProxy
    {
        /// <summary>
        /// The scene files extension.
        /// </summary>
        public static readonly string Extension = "scene";

        /// <inheritdoc />
        public override string Name => "Scene";

        /// <inheritdoc />
        public override string FileExtension => Extension;

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is SceneItem;
        }

        /// <inheritdoc />
        public override bool AcceptsAsset(string typeName, string path)
        {
            return (typeName == Scene.AssetTypename || typeName == Scene.EditorPickerTypename) && path.EndsWith(FileExtension, StringComparison.OrdinalIgnoreCase);
        }

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.Instance.Scene.CreateSceneFile(outputPath);
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            // Load scene
            Editor.Instance.Scene.OpenScene(((SceneItem)item).ID);

            return null;
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xbb37ef);

        /// <inheritdoc />
        public override string TypeName => Scene.AssetTypename;

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new SceneItem(path, id);
        }

        /// <inheritdoc />
        public override void OnContentWindowContextMenu(ContextMenu menu, ContentItem item)
        {
            var id = ((SceneItem)item).ID;
            if (Level.FindScene(id) == null)
            {
                menu.AddButton("Open (additive)", () => { Editor.Instance.Scene.OpenScene(id, true); });
            }
        }
    }
}
