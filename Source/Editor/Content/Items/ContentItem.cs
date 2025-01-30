// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using FlaxEditor.Content.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item types.
    /// </summary>
    [HideInEditor]
    public enum ContentItemType
    {
        /// <summary>
        /// The binary or text asset.
        /// </summary>
        Asset,

        /// <summary>
        /// The directory.
        /// </summary>
        Folder,

        /// <summary>
        /// The script file.
        /// </summary>
        Script,

        /// <summary>
        /// The scene file.
        /// </summary>
        Scene,

        /// <summary>
        /// The other type.
        /// </summary>
        Other,
    }

    /// <summary>
    /// Content item filter types used for searching.
    /// </summary>
    [HideInEditor]
    public enum ContentItemSearchFilter
    {
        /// <summary>
        /// The model.
        /// </summary>
        Model,

        /// <summary>
        /// The skinned model.
        /// </summary>
        SkinnedModel,

        /// <summary>
        /// The material.
        /// </summary>
        Material,

        /// <summary>
        /// The texture.
        /// </summary>
        Texture,

        /// <summary>
        /// The scene.
        /// </summary>
        Scene,

        /// <summary>
        /// The prefab.
        /// </summary>
        Prefab,

        /// <summary>
        /// The script.
        /// </summary>
        Script,

        /// <summary>
        /// The audio.
        /// </summary>
        Audio,

        /// <summary>
        /// The animation.
        /// </summary>
        Animation,

        /// <summary>
        /// The json.
        /// </summary>
        Json,

        /// <summary>
        /// The particles.
        /// </summary>
        Particles,

        /// <summary>
        /// The shader source files.
        /// </summary>
        Shader,

        /// <summary>
        /// The other.
        /// </summary>
        Other,
    }

    /// <summary>
    /// Interface for objects that can reference the content items in order to receive events from them.
    /// </summary>
    [HideInEditor]
    public interface IContentItemOwner
    {
        /// <summary>
        /// Called when referenced item gets deleted (asset unloaded, file deleted, etc.).
        /// Item should not be used after that.
        /// </summary>
        /// <param name="item">The item.</param>
        void OnItemDeleted(ContentItem item);

        /// <summary>
        /// Called when referenced item gets renamed (filename change, path change, etc.)
        /// </summary>
        /// <param name="item">The item.</param>
        void OnItemRenamed(ContentItem item);

        /// <summary>
        /// Called when item gets reimported or reloaded.
        /// </summary>
        /// <param name="item">The item.</param>
        void OnItemReimported(ContentItem item);

        /// <summary>
        /// Called when referenced item gets disposed (editor closing, database internal changes, etc.).
        /// Item should not be used after that.
        /// </summary>
        /// <param name="item">The item.</param>
        void OnItemDispose(ContentItem item);
    }

    /// <summary>
    /// Base class for all content items.
    /// Item parent GUI control is always <see cref="ContentView"/> or null if not in a view.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public abstract class ContentItem : Control
    {
        /// <summary>
        /// The default margin size.
        /// </summary>
        public const int DefaultMarginSize = 4;

        /// <summary>
        /// The default text height.
        /// </summary>
        public const int DefaultTextHeight = 42;

        /// <summary>
        /// The default thumbnail size.
        /// </summary>
        public const int DefaultThumbnailSize = PreviewsCache.AssetIconSize;

        /// <summary>
        /// The default width.
        /// </summary>
        public const int DefaultWidth = (DefaultThumbnailSize + 2 * DefaultMarginSize);

        /// <summary>
        /// The default height.
        /// </summary>
        public const int DefaultHeight = (DefaultThumbnailSize + 2 * DefaultMarginSize + DefaultTextHeight);

        /// <summary>
        /// Whether the item is being but.
        /// </summary>
        public bool IsBeingCut;

        private ContentFolder _parentFolder;

        private bool _isMouseDown;
        private Float2 _mouseDownStartPos;
        private readonly List<IContentItemOwner> _references = new List<IContentItemOwner>(4);

        private SpriteHandle _thumbnail;
        private SpriteHandle _shadowIcon;

        /// <summary>
        /// Gets the type of the item.
        /// </summary>
        public abstract ContentItemType ItemType { get; }

        /// <summary>
        /// Gets the type of the item searching filter to use.
        /// </summary>
        public abstract ContentItemSearchFilter SearchFilter { get; }

        /// <summary>
        /// Gets a value indicating whether this instance is asset.
        /// </summary>
        public bool IsAsset => ItemType == ContentItemType.Asset;

        /// <summary>
        /// Gets a value indicating whether this instance is folder.
        /// </summary>
        public bool IsFolder => ItemType == ContentItemType.Folder;

        /// <summary>
        /// Gets a value indicating whether this instance can have children.
        /// </summary>
        public bool CanHaveChildren => ItemType == ContentItemType.Folder;

        /// <summary>
        /// Determines whether this item can be renamed.
        /// </summary>
        public virtual bool CanRename => true;

        /// <summary>
        /// Gets a value indicating whether this item can be dragged and dropped.
        /// </summary>
        public virtual bool CanDrag => Root != null;

        /// <summary>
        /// Gets a value indicating whether this <see cref="ContentItem"/> exists on drive.
        /// </summary>
        public virtual bool Exists => System.IO.File.Exists(Path);

        /// <summary>
        /// Gets the parent folder.
        /// </summary>
        public ContentFolder ParentFolder
        {
            get => _parentFolder;
            set
            {
                if (_parentFolder == value)
                    return;

                // Remove from old
                _parentFolder?.Children.Remove(this);

                // Link
                _parentFolder = value;

                // Add to new
                _parentFolder?.Children.Add(this);

                OnParentFolderChanged();
            }
        }

        /// <summary>
        /// Gets the path to the item.
        /// </summary>
        public string Path { get; private set; }

        /// <summary>
        /// Gets the item file name (filename with extension).
        /// </summary>
        public string FileName { get; internal set; }

        /// <summary>
        /// Gets the item short name (filename without extension).
        /// </summary>
        public string ShortName { get; internal set; }

        /// <summary>
        /// Gets the asset name relative to the project root folder (without asset file extension)
        /// </summary>
        public string NamePath => FlaxEditor.Utilities.Utils.GetAssetNamePath(Path);

        /// <summary>
        /// Gets the content item type description (for UI).
        /// </summary>
        public abstract string TypeDescription { get; }

        /// <summary>
        /// Gets the default name of the content item thumbnail. Returns null if not used.
        /// </summary>
        public virtual SpriteHandle DefaultThumbnail => SpriteHandle.Invalid;

        /// <summary>
        /// Gets a value indicating whether this item has default thumbnail.
        /// </summary>
        public bool HasDefaultThumbnail => DefaultThumbnail.IsValid;

        /// <summary>
        /// Gets or sets the item thumbnail. Warning, thumbnail may not be available if item has no references (<see cref="ReferencesCount"/>).
        /// </summary>
        public SpriteHandle Thumbnail
        {
            get => _thumbnail;
            set => _thumbnail = value;
        }

        /// <summary>
        /// True if force show file extension.
        /// </summary>
        public bool ShowFileExtension;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentItem"/> class.
        /// </summary>
        /// <param name="path">The path to the item.</param>
        protected ContentItem(string path)
        : base(0, 0, DefaultWidth, DefaultHeight)
        {
            // Set path
            Path = path;
            FileName = System.IO.Path.GetFileName(path);
            ShortName = System.IO.Path.GetFileNameWithoutExtension(path);
        }

        /// <summary>
        /// Updates the item path. Use with caution or even don't use it. It's dangerous.
        /// </summary>
        /// <param name="value">The new path.</param>
        internal virtual void UpdatePath(string value)
        {
            // Set path
            Path = StringUtils.NormalizePath(value);
            FileName = System.IO.Path.GetFileName(value);
            ShortName = System.IO.Path.GetFileNameWithoutExtension(value);

            // Fire event
            OnPathChanged();
            for (int i = 0; i < _references.Count; i++)
            {
                _references[i].OnItemRenamed(this);
            }
        }

        /// <summary>
        /// Refreshes the item thumbnail.
        /// </summary>
        public virtual void RefreshThumbnail()
        {
            // Skip if item has default thumbnail
            if (HasDefaultThumbnail)
                return;

            var thumbnails = Editor.Instance.Thumbnails;

            // Delete old thumbnail and remove it from the cache
            thumbnails.DeletePreview(this);

            // Request new one (if need to)
            if (_references.Count > 0)
            {
                thumbnails.RequestPreview(this);
            }
        }

        /// <summary>
        /// Updates the tooltip text text.
        /// </summary>
        public virtual void UpdateTooltipText()
        {
            var sb = new StringBuilder();
            OnBuildTooltipText(sb);
            if (sb.Length != 0 && sb[sb.Length - 1] == '\n')
            {
                // Remove new-line from end
                int sub = 1;
                if (sb.Length != 1 && sb[sb.Length - 2] == '\r')
                    sub = 2;
                sb.Length -= sub;
            }
            TooltipText = sb.ToString();
        }

        /// <summary>
        /// Called when building tooltip text.
        /// </summary>
        /// <param name="sb">The output string builder.</param>
        protected virtual void OnBuildTooltipText(StringBuilder sb)
        {
            sb.Append("Type: ").Append(TypeDescription).AppendLine();
            if (File.Exists(Path))
                sb.Append("Size: ").Append(Utilities.Utils.FormatBytesCount((ulong)new FileInfo(Path).Length)).AppendLine();
            sb.Append("Path: ").Append(Utilities.Utils.GetAssetNamePathWithExt(Path)).AppendLine();
        }

        /// <summary>
        /// Tries to find the item at the specified path.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>Found item or null if missing.</returns>
        public virtual ContentItem Find(string path)
        {
            return Path == path ? this : null;
        }

        /// <summary>
        /// Tries to find a specified item in the assets tree.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>True if has been found, otherwise false.</returns>
        public virtual bool Find(ContentItem item)
        {
            return this == item;
        }

        /// <summary>
        /// Tries to find the item with the specified id.
        /// </summary>
        /// <param name="id">The id.</param>
        /// <returns>Found item or null if missing.</returns>
        public virtual ContentItem Find(Guid id)
        {
            return null;
        }

        /// <summary>
        /// Tries to find script with the given name.
        /// </summary>
        /// <param name="scriptName">Name of the script.</param>
        /// <returns>Found script or null if missing.</returns>
        public virtual ScriptItem FindScriptWitScriptName(string scriptName)
        {
            return null;
        }

        /// <summary>
        /// Gets a value indicating whether draw item shadow.
        /// </summary>
        protected virtual bool DrawShadow => false;

        /// <summary>
        /// Gets the local space rectangle for element name text area.
        /// </summary>
        public Rectangle TextRectangle
        {
            get
            {
                // Skip when hidden
                if (!Visible)
                    return Rectangle.Empty;
                var view = Parent as ContentView;
                var size = Size;
                switch (view?.ViewType ?? ContentViewType.Tiles)
                {
                case ContentViewType.Tiles:
                {
                    var textHeight = DefaultTextHeight * size.X / DefaultWidth;
                    return new Rectangle(0, size.Y - textHeight, size.X, textHeight);
                }
                case ContentViewType.List:
                {
                    var thumbnailSize = size.Y - 2 * DefaultMarginSize;
                    var textHeight = Mathf.Min(size.Y, 24.0f);
                    return new Rectangle(thumbnailSize + DefaultMarginSize * 2, (size.Y - textHeight) * 0.5f, size.X - textHeight - DefaultMarginSize * 3.0f, textHeight);
                }
                default: throw new ArgumentOutOfRangeException();
                }
            }
        }

        /// <summary>
        /// Draws the item thumbnail.
        /// </summary>
        /// <param name="rectangle">The thumbnail rectangle.</param>
        public void DrawThumbnail(ref Rectangle rectangle)
        {
            // Draw shadow
            if (DrawShadow)
            {
                const float thumbnailInShadowSize = 50.0f;
                var shadowRect = rectangle.MakeExpanded((DefaultThumbnailSize - thumbnailInShadowSize) * rectangle.Width / DefaultThumbnailSize * 1.3f);
                if (!_shadowIcon.IsValid)
                    _shadowIcon = Editor.Instance.Icons.AssetShadow128;
                Render2D.DrawSprite(_shadowIcon, shadowRect);
            }

            // Draw thumbnail
            if (_thumbnail.IsValid)
                Render2D.DrawSprite(_thumbnail, rectangle);
            else
                Render2D.FillRectangle(rectangle, Color.Black);
        }

        /// <summary>
        /// Draws the item thumbnail.
        /// </summary>
        /// <param name="rectangle">The thumbnail rectangle.</param>
        /// /// <param name="shadow">Whether or not to draw the shadow. Overrides DrawShadow.</param>
        public void DrawThumbnail(ref Rectangle rectangle, bool shadow)
        {
            // Draw shadow
            if (shadow)
            {
                const float thumbnailInShadowSize = 50.0f;
                var shadowRect = rectangle.MakeExpanded((DefaultThumbnailSize - thumbnailInShadowSize) * rectangle.Width / DefaultThumbnailSize * 1.3f);
                if (!_shadowIcon.IsValid)
                    _shadowIcon = Editor.Instance.Icons.AssetShadow128;
                Render2D.DrawSprite(_shadowIcon, shadowRect);
            }

            // Draw thumbnail
            if (_thumbnail.IsValid)
                Render2D.DrawSprite(_thumbnail, rectangle);
            else
                Render2D.FillRectangle(rectangle, Color.Black);
        }

        /// <summary>
        /// Gets the amount of references to that item.
        /// </summary>
        public int ReferencesCount => _references.Count;

        /// <summary>
        /// Adds the reference to the item.
        /// </summary>
        /// <param name="obj">The object.</param>
        public void AddReference(IContentItemOwner obj)
        {
            Assert.IsNotNull(obj);
            Assert.IsFalse(_references.Contains(obj));

            _references.Add(obj);

            // Check if need to generate preview
            if (_references.Count == 1 && !_thumbnail.IsValid)
            {
                RequestThumbnail();
            }
        }

        /// <summary>
        /// Removes the reference from the item.
        /// </summary>
        /// <param name="obj">The object.</param>
        public void RemoveReference(IContentItemOwner obj)
        {
            if (_references.Remove(obj))
            {
                // Check if need to release the preview
                if (_references.Count == 0 && _thumbnail.IsValid)
                {
                    ReleaseThumbnail();
                }
            }
        }

        /// <summary>
        /// Called when context menu is being prepared to show. Can be used to add custom options.
        /// </summary>
        /// <param name="menu">The menu.</param>
        public virtual void OnContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu)
        {
        }

        /// <summary>
        /// Called when item gets renamed or location gets changed (path modification).
        /// </summary>
        public virtual void OnPathChanged()
        {
        }

        /// <summary>
        /// Called when content item gets removed (by the user or externally).
        /// </summary>
        public virtual void OnDelete()
        {
            // Fire event
            while (_references.Count > 0)
            {
                var reference = _references[0];
                reference.OnItemDeleted(this);
                RemoveReference(reference);
            }

            // Release thumbnail
            if (_thumbnail.IsValid)
            {
                ReleaseThumbnail();
            }
        }

        /// <summary>
        /// Called when item parent folder gets changed.
        /// </summary>
        protected virtual void OnParentFolderChanged()
        {
        }

        /// <summary>
        /// Requests the thumbnail.
        /// </summary>
        protected void RequestThumbnail()
        {
            Editor.Instance.Thumbnails.RequestPreview(this);
        }

        /// <summary>
        /// Releases the thumbnail.
        /// </summary>
        protected void ReleaseThumbnail()
        {
            // Simply unlink sprite
            _thumbnail = SpriteHandle.Invalid;
        }

        /// <summary>
        /// Called when item gets reimported or reloaded.
        /// </summary>
        protected virtual void OnReimport()
        {
            for (int i = 0; i < _references.Count; i++)
                _references[i].OnItemReimported(this);
            RefreshThumbnail();
        }

        /// <summary>
        /// Does the drag and drop operation with this asset.
        /// </summary>
        protected virtual void DoDrag()
        {
            if (!CanDrag)
                return;

            DragData data;

            // Check if is selected
            if (Parent is ContentView view && view.IsSelected(this))
            {
                // Drag selected item
                data = DragItems.GetDragData(view.Selection);
            }
            else
            {
                // Drag single item
                data = DragItems.GetDragData(this);
            }

            // Start drag operation
            DoDragDrop(data);
        }

        /// <inheritdoc />
        protected override bool ShowTooltip => true;

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            UpdateTooltipText();
            var result = base.OnShowTooltip(out text, out _, out area);
            location = Size * new Float2(0.9f, 0.5f);
            return result;
        }

        /// <inheritdoc />
        public override void NavigationFocus()
        {
            base.NavigationFocus();

            if (IsFocused)
                (Parent as ContentView)?.Select(this);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var size = Size;
            var style = Style.Current;
            var view = Parent as ContentView;
            var isSelected = view.IsSelected(this);
            var clientRect = new Rectangle(Float2.Zero, size);
            var textRect = TextRectangle;
            Rectangle thumbnailRect;
            TextAlignment nameAlignment;
            switch (view.ViewType)
            {
            case ContentViewType.Tiles:
            {
                var thumbnailSize = size.X;
                thumbnailRect = new Rectangle(0, 0, thumbnailSize, thumbnailSize);
                nameAlignment = TextAlignment.Center;

                if (this is ContentFolder)
                {
                    // Small shadow
                    var shadowRect = new Rectangle(2, 2, clientRect.Width + 1, clientRect.Height + 1);
                    var color = Color.Black.AlphaMultiplied(0.2f);
                    Render2D.FillRectangle(shadowRect, color);
                    Render2D.FillRectangle(clientRect, style.Background.RGBMultiplied(1.25f));

                    if (isSelected)
                        Render2D.FillRectangle(clientRect, Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                    else if (IsMouseOver)
                        Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);

                    DrawThumbnail(ref thumbnailRect, false);
                }
                else
                {
                    // Small shadow
                    var shadowRect = new Rectangle(2, 2, clientRect.Width + 1, clientRect.Height + 1);
                    var color = Color.Black.AlphaMultiplied(0.2f);
                    Render2D.FillRectangle(shadowRect, color);

                    Render2D.FillRectangle(clientRect, style.Background.RGBMultiplied(1.25f));
                    Render2D.FillRectangle(TextRectangle, style.LightBackground);

                    var accentHeight = 2 * view.ViewScale;
                    var barRect = new Rectangle(0, thumbnailRect.Height - accentHeight, clientRect.Width, accentHeight);
                    Render2D.FillRectangle(barRect, Color.DimGray);

                    DrawThumbnail(ref thumbnailRect, false);
                    if (isSelected)
                    {
                        Render2D.FillRectangle(textRect, Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                        Render2D.DrawRectangle(clientRect, Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                    }
                    else if (IsMouseOver)
                    {
                        Render2D.FillRectangle(textRect, style.BackgroundHighlighted);
                        Render2D.DrawRectangle(clientRect, style.BackgroundHighlighted);
                    }
                }
                break;
            }
            case ContentViewType.List:
            {
                var thumbnailSize = size.Y - 2 * DefaultMarginSize;
                thumbnailRect = new Rectangle(DefaultMarginSize, DefaultMarginSize, thumbnailSize, thumbnailSize);
                nameAlignment = TextAlignment.Near;

                if (isSelected)
                    Render2D.FillRectangle(clientRect, Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                else if (IsMouseOver)
                    Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);

                DrawThumbnail(ref thumbnailRect);
                break;
            }
            default: throw new ArgumentOutOfRangeException();
            }

            // Draw short name
            Render2D.PushClip(ref textRect);
            var scale = 0.95f * view.ViewScale;
            Render2D.DrawText(style.FontMedium, ShowFileExtension || view.ShowFileExtensions ? FileName : ShortName, textRect, style.Foreground, nameAlignment, TextAlignment.Center, TextWrapping.WrapWords, 1f, scale);
            Render2D.PopClip();

            if (IsBeingCut)
            {
                var color = style.LightBackground.AlphaMultiplied(0.5f);
                Render2D.FillRectangle(clientRect, color);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();

            if (button == MouseButton.Left)
            {
                // Cache data
                _isMouseDown = true;
                _mouseDownStartPos = location;
            }

            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMouseDown)
            {
                // Clear flag
                _isMouseDown = false;

                // Fire event
                (Parent as ContentView).OnItemClick(this);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Focus();

            // Open
            (Parent as ContentView).OnItemDoubleClick(this);

            return true;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            // Check if start drag and drop
            if (_isMouseDown && Float2.Distance(_mouseDownStartPos, location) > 10.0f)
            {
                // Clear flag
                _isMouseDown = false;

                // Start drag drop
                DoDrag();
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Check if start drag and drop
            if (_isMouseDown)
            {
                // Clear flag
                _isMouseDown = false;

                // Start drag drop
                DoDrag();
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            // Open
            (Parent as ContentView).OnItemDoubleClick(this);

            base.OnSubmit();
        }

        /// <inheritdoc />
        public override int Compare(Control other)
        {
            if (other is ContentItem otherItem)
            {
                if (otherItem.IsFolder)
                    return 1;
                return string.Compare(ShortName, otherItem.ShortName, StringComparison.InvariantCulture);
            }

            return base.Compare(other);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Fire event
            while (_references.Count > 0)
            {
                var reference = _references[0];
                reference.OnItemDispose(this);
                RemoveReference(reference);
            }

            // Release thumbnail
            if (_thumbnail.IsValid)
            {
                ReleaseThumbnail();
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Path;
        }
    }
}
