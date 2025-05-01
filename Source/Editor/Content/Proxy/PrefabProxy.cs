// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Create;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content proxy for <see cref="PrefabItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetBaseProxy" />
    [ContentContextMenu("New/Prefab")]
    public sealed class PrefabProxy : JsonAssetBaseProxy
    {
        private PrefabPreview _preview;

        /// <summary>
        /// The prefab files extension.
        /// </summary>
        public static readonly string Extension = "prefab";

        /// <summary>
        /// The prefab asset data typename.
        /// </summary>
        public static readonly string AssetTypename = typeof(Prefab).FullName;

        /// <inheritdoc />
        public override string Name => "Prefab";

        /// <inheritdoc />
        public override string FileExtension => Extension;

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new PrefabWindow(editor, (AssetItem)item);
        }

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is PrefabItem;
        }

        /// <inheritdoc />
        public override bool IsProxyFor<T>()
        {
            return typeof(T) == typeof(Prefab);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x7eef21);

        /// <inheritdoc />
        public override string TypeName => AssetTypename;

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new PrefabItem(path, id);
        }

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override bool CanReimport(ContentItem item)
        {
            if (item is not PrefabItem prefabItem)
                return base.CanReimport(item);

            var prefab = FlaxEngine.Content.Load<Prefab>(prefabItem.ID);
            return prefab.GetDefaultInstance().GetScript<ModelPrefab>() != null;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            bool resetTransform = false;
            var transform = Transform.Identity;
            if (!(arg is Actor actor))
            {
                Editor.Instance.ContentImporting.Create(new PrefabCreateEntry(outputPath));
                return;
            }
            else if (actor.HasScene)
            {
                // Create prefab with identity transform so the actor instance on a level will have it customized
                resetTransform = true;
                transform = actor.LocalTransform;
                actor.LocalTransform = Transform.Identity;
            }

            PrefabManager.CreatePrefab(actor, outputPath, true);
            if (resetTransform)
                actor.LocalTransform = transform;
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new PrefabPreview(false);
                InitAssetPreview(_preview);
            }

            // TODO: disable streaming for asset during thumbnail rendering (and restore it after)
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            if (!_preview.HasLoadedAssets)
                return false;

            // Check if asset is streamed enough
            var asset = (Prefab)request.Asset;
            return asset.IsLoaded;
        }

        private void Prepare(Actor actor)
        {
            if (actor is TextRender textRender)
            {
                textRender.UpdateLayout();
            }

            for (int i = 0; i < actor.ChildrenCount; i++)
            {
                Prepare(actor.GetChild(i));
            }
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            _preview.Prefab = (Prefab)request.Asset;
            _preview.Parent = guiRoot;
            _preview.Scale = Float2.One;
            _preview.ShowDefaultSceneActors = true;
            _preview.SyncBackbufferSize();

            // Special case for UI prefabs
            if (_preview.Instance is UIControl uiControl && uiControl.HasControl)
            {
                // Ensure to place UI in a proper way
                uiControl.Control.Location = Float2.Zero;
                uiControl.Control.Scale *= PreviewsCache.AssetIconSize / uiControl.Control.Size.MaxValue;
                uiControl.Control.AnchorPreset = AnchorPresets.TopLeft;
                uiControl.Control.AnchorPreset = AnchorPresets.MiddleCenter;

                // Tweak preview
                _preview.ShowDefaultSceneActors = false;
            }
            else
            {
                // Update some actors data (some actor types update bounds/data later but its required to be done before rendering)
                Prepare(_preview.Instance);

                // Auto fit actor to camera
                float targetSize = 30.0f;
                var bounds = _preview.Instance.EditorBoxChildren;
                var maxSize = Math.Max(0.001f, (float)bounds.Size.MaxValue);
                _preview.Instance.Scale = new Float3(targetSize / maxSize);
                _preview.Instance.Position = Vector3.Zero;
            }

            _preview.Task.OnDraw();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            _preview.RemoveChildren();
            _preview.Prefab = null;
            _preview.Parent = null;
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            if (_preview != null)
            {
                _preview.Dispose();
                _preview = null;
            }

            base.Dispose();
        }
    }

    /// <summary>
    /// Content proxy for quick UI Control prefab creation as widget.
    /// </summary>
    [ContentContextMenu("New/Widget")]
    internal sealed class WidgetProxy : AssetProxy
    {
        /// <inheritdoc />
        public override string Name => "UI Widget";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return false;
        }

        /// <inheritdoc />
        public override string FileExtension => PrefabProxy.Extension;

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return null;
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.Transparent;

        /// <inheritdoc />
        public override string TypeName => PrefabProxy.AssetTypename;

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return null;
        }

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.Instance.ContentImporting.Create(new WidgetCreateEntry(outputPath));
            return;
        }
    }
}
