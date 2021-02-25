// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content proxy for <see cref="PrefabItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetBaseProxy" />
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
        public override void Create(string outputPath, object arg)
        {
            if (!(arg is Actor actor))
            {
                // Create default prefab root object
                actor = new EmptyActor
                {
                    Name = "Root"
                };

                // Cleanup it after usage
                Object.Destroy(actor, 20.0f);
            }

            PrefabManager.CreatePrefab(actor, outputPath, true);
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new PrefabPreview(false)
                {
                    RenderOnlyWithWindow = false,
                    UseAutomaticTaskManagement = false,
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                };
                _preview.Task.Enabled = false;

                var eyeAdaptation = _preview.PostFxVolume.EyeAdaptation;
                eyeAdaptation.Mode = EyeAdaptationMode.None;
                eyeAdaptation.OverrideFlags |= EyeAdaptationSettingsOverride.Mode;
                _preview.PostFxVolume.EyeAdaptation = eyeAdaptation;
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
            _preview.Scale = Vector2.One;
            _preview.ShowDefaultSceneActors = true;
            _preview.SyncBackbufferSize();

            // Special case for UI prefabs
            if (_preview.Instance is UIControl uiControl && uiControl.HasControl)
            {
                // Ensure to place UI in a proper way
                uiControl.Control.Location = Vector2.Zero;
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
                Editor.GetActorEditorBox(_preview.Instance, out var bounds);
                float maxSize = Mathf.Max(0.001f, bounds.Size.MaxValue);
                _preview.Instance.Scale = new Vector3(targetSize / maxSize);
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
}
