// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Thumbnails;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Base class for all asset proxy objects used to manage <see cref="AssetItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentProxy" />
    [HideInEditor]
    public abstract class AssetProxy : ContentProxy
    {
        /// <inheritdoc />
        public override bool IsAsset => true;

        /// <summary>
        /// Gets the full name of the asset type (stored data format).
        /// </summary>
        public abstract string TypeName { get; }

        /// <summary>
        /// Gets a value indicating whether this instance is virtual Proxy not linked to any asset.
        /// </summary>
        protected virtual bool IsVirtual { get; }

        /// <summary>
        /// Determines whether [is virtual proxy].
        /// </summary>
        /// <returns><c>true</c> if [is virtual proxy]; otherwise, <c>false</c>.</returns>
        public bool IsVirtualProxy()
        {
            return IsVirtual && CanExport == false;
        }

        /// <summary>
        /// Checks if this proxy supports the given asset type id at the given path.
        /// </summary>
        /// <param name="typeName">The asset type identifier.</param>
        /// <param name="path">The asset path.</param>
        /// <returns>True if proxy supports assets of the given type id and path.</returns>
        public virtual bool AcceptsAsset(string typeName, string path)
        {
            return typeName == TypeName && path.EndsWith(FileExtension, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Constructs the item for the asset.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="typeName">The asset type name identifier.</param>
        /// <param name="id">The asset identifier.</param>
        /// <returns>Created item.</returns>
        public abstract AssetItem ConstructItem(string path, string typeName, ref Guid id);

        /// <summary>
        /// Called when thumbnail request gets prepared for drawing.
        /// </summary>
        /// <param name="request">The request.</param>
        public virtual void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
        }

        /// <summary>
        /// Determines whether thumbnail can be drawn for the specified item.
        /// </summary>
        /// <param name="request">The request.</param>
        /// <returns><c>true</c> if this thumbnail can be drawn for the specified item; otherwise, <c>false</c>.</returns>
        public virtual bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return true;
        }

        /// <summary>
        /// Called when thumbnail drawing begins. Proxy should setup scene GUI for guiRoot.
        /// </summary>
        /// <param name="request">The request to render thumbnail.</param>
        /// <param name="guiRoot">The GUI root container control.</param>
        /// <param name="context">GPU context.</param>
        public virtual void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            guiRoot.AddChild(new Label
            {
                Text = Name,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Wrapping = TextWrapping.WrapWords
            });
        }

        /// <summary>
        /// Called when thumbnail drawing ends. Proxy should clear custom GUI from guiRoot from that should be not destroyed.
        /// </summary>
        /// <param name="request">The request to render thumbnail.</param>
        /// <param name="guiRoot">The GUI root container control.</param>
        public virtual void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
        }

        /// <summary>
        /// Called when thumbnail requests cleans data after drawing.
        /// </summary>
        /// <param name="request">The request.</param>
        public virtual void OnThumbnailDrawCleanup(ThumbnailRequest request)
        {
        }

        /// <summary>
        /// Initializes rendering settings for asset preview drawing for a thumbnail.
        /// </summary>
        /// <param name="preview">The asset preview.</param>
        protected void InitAssetPreview(Viewport.Previews.AssetPreview preview)
        {
            preview.RenderOnlyWithWindow = false;
            preview.UseAutomaticTaskManagement = false;
            preview.AnchorPreset = AnchorPresets.StretchAll;
            preview.Offsets = Margin.Zero;

            var task = preview.Task;
            task.Enabled = false;

            var view = task.View;
            view.IsSingleFrame = true; // Disable LOD transitions
            task.View = view;

            var eyeAdaptation = preview.PostFxVolume.EyeAdaptation;
            eyeAdaptation.Mode = EyeAdaptationMode.None;
            eyeAdaptation.OverrideFlags |= EyeAdaptationSettingsOverride.Mode;
            preview.PostFxVolume.EyeAdaptation = eyeAdaptation;
        }
    }
}
