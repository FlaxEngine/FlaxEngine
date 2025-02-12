// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.Modules;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Content.Thumbnails
{
    /// <summary>
    /// Manages asset thumbnails rendering and presentation.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class ThumbnailsModule : EditorModule, IContentItemOwner
    {
        /// <summary>
        /// The minimum required quality (in range [0;1]) for content streaming resources to be loaded in order to generate thumbnail for them.
        /// </summary>
        public const float MinimumRequiredResourcesQuality = 0.8f;

        private readonly List<PreviewsCache> _cache = new List<PreviewsCache>(4);
        private readonly string _cacheFolder;
        private readonly List<ThumbnailRequest> _requests = new List<ThumbnailRequest>(128);
        private readonly PreviewRoot _guiRoot = new PreviewRoot();
        private DateTime _lastFlushTime;
        private RenderTask _task;
        private GPUTexture _output;

        internal ThumbnailsModule(Editor editor)
        : base(editor)
        {
            _cacheFolder = StringUtils.CombinePaths(Globals.ProjectCacheFolder, "Thumbnails");
            _lastFlushTime = DateTime.UtcNow;
        }

        /// <summary>
        /// Requests the item preview.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <exception cref="System.ArgumentNullException"></exception>
        public void RequestPreview(ContentItem item)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Check if use default icon
            var defaultThumbnail = item.DefaultThumbnail;
            if (defaultThumbnail.IsValid)
            {
                item.Thumbnail = defaultThumbnail;
                return;
            }

            // We cache previews only for items with 'ID', for now we support only AssetItems
            var assetItem = item as AssetItem;
            if (assetItem == null)
                return;

            // Ensure that there is valid proxy for that item
            var proxy = Editor.ContentDatabase.GetProxy(item) as AssetProxy;
            if (proxy == null)
            {
                Editor.LogWarning($"Cannot generate preview for item {item.Path}. Cannot find proxy for it.");
                return;
            }

            lock (_requests)
            {
                // Check if element hasn't been already processed for generating preview
                if (FindRequest(assetItem) == null)
                {
                    // Check each cache atlas
                    for (int i = 0; i < _cache.Count; i++)
                    {
                        var sprite = _cache[i].FindSlot(assetItem.ID);
                        if (sprite.IsValid)
                        {
                            // Found!
                            item.Thumbnail = sprite;
                            return;
                        }
                    }

                    AddRequest(assetItem, proxy);
                }
            }
        }

        /// <summary>
        /// Deletes the item preview from the cache.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <exception cref="System.ArgumentNullException"></exception>
        public void DeletePreview(ContentItem item)
        {
            if (item == null)
                throw new ArgumentNullException();

            // We cache previews only for items with 'ID', for now we support only AssetItems
            var assetItem = item as AssetItem;
            if (assetItem == null)
                return;

            lock (_requests)
            {
                // Cancel loading
                RemoveRequest(assetItem);

                // Find atlas with preview and remove it
                for (int i = 0; i < _cache.Count; i++)
                {
                    if (_cache[i].ReleaseSlot(assetItem.ID))
                        break;
                }
            }
        }

        internal static bool HasMinimumQuality(TextureBase asset)
        {
            if (asset.HasStreamingError)
                return true; // Don't block thumbnails queue when texture fails to stream in (eg. unsupported format)
            var mipLevels = asset.MipLevels;
            var minMipLevels = Mathf.Min(mipLevels, 7);
            return asset.IsLoaded && asset.ResidentMipLevels >= Mathf.Max(minMipLevels, (int)(mipLevels * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(Model asset)
        {
            if (!asset.IsLoaded)
                return false;
            var lods = asset.LODs.Length;
            var slots = asset.MaterialSlots;
            foreach (var slot in slots)
            {
                if (slot.Material && !HasMinimumQuality(slot.Material))
                    return false;
            }
            return asset.LoadedLODs >= Mathf.Max(1, (int)(lods * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(SkinnedModel asset)
        {
            var lods = asset.LODs.Length;
            if (asset.IsLoaded && lods == 0)
                return true; // Skeleton-only model
            var slots = asset.MaterialSlots;
            foreach (var slot in slots)
            {
                if (slot.Material && !HasMinimumQuality(slot.Material))
                    return false;
            }
            return asset.LoadedLODs >= Mathf.Max(1, (int)(lods * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(MaterialBase asset)
        {
            if (asset is MaterialInstance asInstance)
                return HasMinimumQuality(asInstance);
            return HasMinimumQualityInternal(asset);
        }

        internal static bool HasMinimumQuality(Material asset)
        {
            return HasMinimumQualityInternal(asset);
        }

        internal static bool HasMinimumQuality(MaterialInstance asset)
        {
            if (!HasMinimumQualityInternal(asset))
                return false;
            var baseMaterial = asset.BaseMaterial;
            return baseMaterial == null || HasMinimumQualityInternal(baseMaterial);
        }

        private static bool HasMinimumQualityInternal(MaterialBase asset)
        {
            if (!asset.IsLoaded)
                return false;
            var parameters = asset.Parameters;
            foreach (var parameter in parameters)
            {
                if (parameter.Value is TextureBase asTexture && !HasMinimumQuality(asTexture))
                    return false;
            }
            return true;
        }

        #region IContentItemOwner

        /// <inheritdoc />
        void IContentItemOwner.OnItemDeleted(ContentItem item)
        {
            DeletePreview(item);
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemDispose(ContentItem item)
        {
            if (item is AssetItem assetItem)
            {
                lock (_requests)
                {
                    RemoveRequest(assetItem);
                }
            }
        }

        #endregion

        /// <inheritdoc />
        public override void OnInit()
        {
            // Create cache folder
            if (!Directory.Exists(_cacheFolder))
            {
                Directory.CreateDirectory(_cacheFolder);
            }

            // Find atlases in a Editor cache directory
            var files = Directory.GetFiles(_cacheFolder, "cache_*.flax", SearchOption.TopDirectoryOnly);
            int atlases = 0;
            for (int i = 0; i < files.Length; i++)
            {
                // Load asset
                var asset = FlaxEngine.Content.LoadAsync(files[i]);
                if (asset == null)
                    continue;

                // Validate type
                if (asset is PreviewsCache atlas)
                {
                    // Cache atlas
                    atlases++;
                    _cache.Add(atlas);
                }
                else
                {
                    // Skip asset
                    Editor.LogWarning(string.Format("Asset \'{0}\' is inside Editor\'s private directory for Assets Thumbnails Cache. Please move it.", asset.Path));
                }
            }
            Editor.Log(string.Format("Previews cache count: {0} (capacity for {1} icons)", atlases, atlases * PreviewsCache.AssetIconsPerAtlas));

            // Prepare at least one atlas
            if (_cache.Count == 0)
            {
                GetValidAtlas();
            }

            // Create render task but disabled for now
            _output = GPUDevice.Instance.CreateTexture("ThumbnailsOutput");
            var desc = GPUTextureDescription.New2D(PreviewsCache.AssetIconSize, PreviewsCache.AssetIconSize, PreviewsCache.AssetIconsAtlasFormat);
            _output.Init(ref desc);
            _task = Object.New<RenderTask>();
            _task.Order = 50; // Render this task later
            _task.Enabled = false;
            _task.Render += OnRender;
        }

        private void OnRender(RenderTask task, GPUContext context)
        {
            lock (_requests)
            {
                // Check if there is ready next asset to render thumbnail for it
                // But don't check whole queue, only a few items
                var request = GetReadyRequest(10);
                if (request == null)
                {
                    // Disable task
                    _task.Enabled = false;
                    return;
                }

                // Find atlas with an free slot
                var atlas = GetValidAtlas();
                if (atlas == null)
                {
                    // Error
                    _task.Enabled = false;
                    _requests.Clear();
                    Editor.LogError("Failed to get atlas.");
                    return;
                }

                // Wait for atlas being loaded
                if (!atlas.IsReady)
                    return;

                try
                {
                    // Setup
                    _guiRoot.RemoveChildren();
                    _guiRoot.AccentColor = request.Proxy.AccentColor;

                    // Call proxy to prepare for thumbnail rendering
                    request.Proxy.OnThumbnailDrawBegin(request, _guiRoot, context);
                    _guiRoot.UnlockChildrenRecursive();

                    // Draw preview
                    context.Clear(_output.View(), Color.Black);
                    Render2D.CallDrawing(_guiRoot, context, _output);

                    // Call proxy and cleanup UI (delete create controls, shared controls should be unlinked during OnThumbnailDrawEnd event)
                    request.Proxy.OnThumbnailDrawEnd(request, _guiRoot);
                }
                catch (Exception ex)
                {
                    // Handle internal errors gracefully (eg. when asset is corrupted and proxy fails)
                    Editor.LogError("Failed to render thumbnail icon for asset: " + request.Item);
                    Editor.LogWarning(ex);
                    request.FinishRender(ref SpriteHandle.Invalid);
                    RemoveRequest(request);
                    return;
                }
                finally
                {
                    _guiRoot.DisposeChildren();
                }

                // Copy backbuffer with rendered preview into atlas
                SpriteHandle icon = atlas.OccupySlot(_output, request.Item.ID);
                if (!icon.IsValid)
                {
                    // Error
                    _task.Enabled = false;
                    _requests.Clear();
                    Editor.LogError("Failed to occupy previews cache atlas slot.");
                    return;
                }

                // End
                request.FinishRender(ref icon);
                RemoveRequest(request);
            }
        }

        private void StartPreviewsQueue()
        {
            // Ensure to have valid atlas
            GetValidAtlas();

            // Enable task
            _task.Enabled = true;
        }

        #region Requests Management

        private ThumbnailRequest FindRequest(AssetItem item)
        {
            for (int i = 0; i < _requests.Count; i++)
            {
                if (_requests[i].Item == item)
                    return _requests[i];
            }
            return null;
        }

        private void AddRequest(AssetItem item, AssetProxy proxy)
        {
            var request = new ThumbnailRequest(item, proxy);
            _requests.Add(request);
            item.AddReference(this);
        }

        private void RemoveRequest(ThumbnailRequest request)
        {
            request.Dispose();
            _requests.Remove(request);
            request.Item.RemoveReference(this);
        }

        private void RemoveRequest(AssetItem item)
        {
            var request = FindRequest(item);
            if (request != null)
                RemoveRequest(request);
        }

        private ThumbnailRequest GetReadyRequest(int maxChecks)
        {
            maxChecks = Mathf.Min(maxChecks, _requests.Count);
            for (int i = 0; i < maxChecks; i++)
            {
                var request = _requests[i];
                try
                {
                    if (request.IsReady)
                        return request;
                }
                catch (Exception ex)
                {
                    Editor.LogWarning($"Failed to prepare thumbnail rendering for {request.Item.ShortName}.");
                    Editor.LogWarning(ex);
                    _requests.RemoveAt(i--);
                }
            }

            return null;
        }

        #endregion

        #region Atlas Management

        private PreviewsCache CreateAtlas()
        {
            // Create atlas path
            var path = StringUtils.CombinePaths(_cacheFolder, string.Format("cache_{0:N}.flax", Guid.NewGuid()));

            // Create atlas
            if (PreviewsCache.Create(path))
            {
                Editor.LogError("Failed to create thumbnails atlas.");
                return null;
            }

            // Load atlas
            var atlas = FlaxEngine.Content.LoadAsync<PreviewsCache>(path);
            if (atlas == null)
            {
                Editor.LogError("Failed to load thumbnails atlas.");
                return null;
            }

            // Register new atlas
            _cache.Add(atlas);

            return atlas;
        }

        private void Flush()
        {
            for (int i = 0; i < _cache.Count; i++)
            {
                _cache[i].Flush();
            }
        }

        private bool HasAllAtlasesLoaded()
        {
            for (int i = 0; i < _cache.Count; i++)
            {
                if (!_cache[i].IsReady)
                {
                    return false;
                }
            }
            return true;
        }

        private PreviewsCache GetValidAtlas()
        {
            // Check if has no free slots
            for (int i = 0; i < _cache.Count; i++)
            {
                if (_cache[i].HasFreeSlot)
                {
                    return _cache[i];
                }
            }

            // Create new atlas
            return CreateAtlas();
        }

        #endregion

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Wait some frames before start generating previews (late init feature)
            if (Time.TimeSinceStartup < 1.0f || HasAllAtlasesLoaded() == false)
                return;

            lock (_requests)
            {
                var now = DateTime.UtcNow;

                // Check if has any request pending
                int count = _requests.Count;
                if (count > 0)
                {
                    // Prepare requests
                    bool isAnyReady = false;
                    int checks = Mathf.Min(10, _requests.Count);
                    for (int i = 0; i < checks && i < _requests.Count; i++)
                    {
                        var request = _requests[i];
                        try
                        {
                            request.Update();
                            if (request.IsReady)
                            {
                                isAnyReady = true;
                            }
                            else if (request.State == ThumbnailRequest.States.Created)
                            {
                                request.Prepare();
                            }
                            else if (request.State == ThumbnailRequest.States.Failed)
                            {
                                _requests.RemoveAt(i--);
                            }
                        }
                        catch (Exception ex)
                        {
                            Editor.LogWarning($"Failed to prepare thumbnail rendering for {request.Item.ShortName}.");
                            Editor.LogWarning(ex);
                            _requests.RemoveAt(i--);
                        }
                    }

                    // Check if has no rendering task enabled but should be
                    if (isAnyReady && _task.Enabled == false)
                    {
                        // Start generating preview
                        StartPreviewsQueue();
                    }
                }
                // Don't flush every frame
                else if (now - _lastFlushTime >= TimeSpan.FromSeconds(1))
                {
                    // Flush data
                    _lastFlushTime = now;
                    Flush();
                }
            }
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            if (_task)
                _task.Enabled = false;

            lock (_requests)
            {
                // Clear data
                while (_requests.Count > 0)
                    RemoveRequest(_requests[0]);
                _cache.Clear();
            }

            _guiRoot.Dispose();
            Object.Destroy(ref _task);
            Object.Destroy(ref _output);
        }

        /// <summary>
        /// Thumbnails GUI root control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        private class PreviewRoot : ContainerControl
        {
            /// <summary>
            /// The item accent color to draw.
            /// </summary>
            public Color AccentColor;

            /// <inheritdoc />
            public PreviewRoot()
            : base(0, 0, PreviewsCache.AssetIconSize, PreviewsCache.AssetIconSize)
            {
                AutoFocus = false;
                AccentColor = Color.Pink;
                IsLayoutLocked = false;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw accent
                const float accentHeight = 2;
                Render2D.FillRectangle(new Rectangle(0, Height - accentHeight, Width, accentHeight), AccentColor);
            }
        }
    }
}
