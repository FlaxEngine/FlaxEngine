// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Threading.Tasks;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="CollisionData"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    class CollisionDataItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public CollisionDataItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Other)
        {
        }

        /// <inheritdoc />
        public override bool OnEditorDrag(object context)
        {
            return true;
        }

        /// <inheritdoc />
        public override Actor OnEditorDrop(object context)
        {
            return new MeshCollider { CollisionData = FlaxEngine.Content.LoadAsync<CollisionData>(ID) };
        }
    }

    /// <summary>
    /// A <see cref="CollisionData"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Physics/Collision Data")]
    class CollisionDataProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Collision Data";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new CollisionDataWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new CollisionDataItem(path, ref id, typeName, AssetType);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x2c3e50);

        /// <inheritdoc />
        public override Type AssetType => typeof(CollisionData);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("CollisionData", outputPath))
                throw new Exception("Failed to create new asset.");
        }

        /// <summary>
        /// Create collision data from model.
        /// </summary>
        /// <param name="model">The associated model.</param>
        /// <param name="created">The action to call once the collision data gets created (or reused from existing).</param>
        /// <param name="withRenaming">True if start initial item renaming by user, or tru to skip it.</param>
        public void CreateCollisionDataFromModel(Model model, Action<CollisionData> created = null, bool withRenaming = true)
        {
            // Check if there already is collision data for that model to reuse
            var modelItem = (AssetItem)Editor.Instance.ContentDatabase.Find(model.ID);
            if (modelItem?.ParentFolder != null)
            {
                foreach (var child in modelItem.ParentFolder.Children)
                {
                    // Check if there is collision that was made with this model
                    if (child is BinaryAssetItem b && b.IsOfType<CollisionData>())
                    {
                        var collisionData = FlaxEngine.Content.Load<CollisionData>(b.ID);
                        if (collisionData && collisionData.Options.Model == model.ID)
                        {
                            Editor.Instance.Windows.ContentWin.Select(b);
                            if (created != null)
                                FlaxEngine.Scripting.InvokeOnUpdate(() => created(collisionData));
                            return;
                        }
                    }

                    // Check if there is a auto-imported collision
                    if (child is ContentFolder childFolder && childFolder.ShortName == modelItem.ShortName)
                    {
                        foreach (var childFolderChild in childFolder.Children)
                        {
                            if (childFolderChild is BinaryAssetItem c && c.IsOfType<CollisionData>())
                            {
                                var collisionData = FlaxEngine.Content.Load<CollisionData>(c.ID);
                                if (collisionData && (collisionData.Options.Model == model.ID || collisionData.Options.Model == Guid.Empty))
                                {
                                    Editor.Instance.Windows.ContentWin.Select(c);
                                    if (created != null)
                                        FlaxEngine.Scripting.InvokeOnUpdate(() => created(collisionData));
                                    return;
                                }
                            }
                        }
                    }
                }
            }

            // Create new item so user can name it and then generate collision for it in async
            Action<ContentItem> create = contentItem =>
            {
                var assetItem = (AssetItem)contentItem;
                var collisionData = FlaxEngine.Content.LoadAsync<CollisionData>(assetItem.ID);
                if (collisionData == null || collisionData.WaitForLoaded())
                {
                    Editor.LogError("Failed to load created collision data.");
                    return;
                }
                Task.Run(() =>
                {
                    Editor.CookMeshCollision(assetItem.Path, CollisionDataType.ConvexMesh, model);
                    if (created != null)
                        FlaxEngine.Scripting.InvokeOnUpdate(() => created(collisionData));
                });
            };
            var initialName = (modelItem?.ShortName ?? Path.GetFileNameWithoutExtension(model.Path)) + " Collision";
            Editor.Instance.Windows.ContentWin.NewItem(this, null, create, initialName, withRenaming);
        }
    }
}
