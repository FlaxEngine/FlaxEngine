// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup that shows the list of content items to pick. Supports searching and basic items filtering.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu" />
    public class ContentSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The content item.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu.Item" />
        public class ContentItemView : Item, IContentItemOwner
        {
            /// <summary>
            /// The icon size (in pixels).
            /// </summary>
            public const float IconSize = 28;

            /// <summary>
            /// Gets the item.
            /// </summary>
            public ContentItem ContentItem;

            /// <summary>
            /// Initializes a new instance of the <see cref="ContentItemView"/> class.
            /// </summary>
            /// <param name="item">The item.</param>
            public ContentItemView(ContentItem item)
            {
                ContentItem = item;
                ContentItem.AddReference(this);

                OnItemRenamed(item);
                TooltipText = item.Path;

                Height = IconSize + 4;
            }

            /// <inheritdoc />
            protected override void GetTextRect(out Rectangle rect)
            {
                var height = Height;
                rect = new Rectangle(IconSize + 4, (height - 16) * 0.5f, Width - IconSize - 6, 16);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw icon
                var iconRect = new Rectangle(2, 2, IconSize, IconSize);
                ContentItem.DrawThumbnail(ref iconRect);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (ContentItem != null)
                {
                    ContentItem.RemoveReference(this);
                    ContentItem = null;
                }

                base.OnDestroy();
            }

            /// <inheritdoc />
            public void OnItemDeleted(ContentItem item)
            {
                Dispose();
            }

            /// <inheritdoc />
            public void OnItemRenamed(ContentItem item)
            {
                Name = item.ShortName;
                if (item is ScriptItem)
                    Name = item.FileName; // Show extension for scripts (esp. for .h and .cpp files of the same name)
            }

            /// <inheritdoc />
            public void OnItemReimported(ContentItem item)
            {
            }

            /// <inheritdoc />
            public void OnItemDispose(ContentItem item)
            {
                Dispose();
            }
        }

        /// <summary>
        /// Validates if the given content item can be used to pick it.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(ContentItem item);

        private IsValidDelegate _isValid;
        private Action<ContentItem> _selected;

        /// <inheritdoc />
        protected ContentSearchPopup()
        {
        }

        /// <inheritdoc />
        protected ContentSearchPopup(IsValidDelegate isValid, Action<ContentItem> selected)
        {
            _isValid = isValid;
            _selected = selected;

            ItemClicked += OnItemClicked;

            // TODO: use async thread to search workspace items
            foreach (var project in Editor.Instance.ContentDatabase.Projects)
            {
                if (project.Content != null)
                    FindItems(project.Content.Folder);
            }
            SortItems();
        }

        private void OnItemClicked(Item item)
        {
            _selected.Invoke(((ContentItemView)item).ContentItem);
        }

        private void FindItems(ContentFolder folder)
        {
            for (int i = 0; i < folder.Children.Count; i++)
            {
                if (folder.Children[i] is ContentItem item && _isValid(item))
                {
                    AddItem(new ContentItemView(item));
                }
            }

            for (int i = 0; i < folder.Children.Count; i++)
            {
                if (folder.Children[i] is ContentFolder child)
                {
                    FindItems(child);
                }
            }
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="isValid">Event called to check if a given content item is valid to be used.</param>
        /// <param name="selected">Event called on content item pick.</param>
        /// <returns>The dialog.</returns>
        public static ContentSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid, Action<ContentItem> selected)
        {
            var popup = new ContentSearchPopup(isValid, selected);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _isValid = null;
            _selected = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// Popup that shows the list of assets to pick. Supports searching and basic items filtering.
    /// </summary>
    public class AssetSearchPopup : ContentSearchPopup
    {
        /// <summary>
        /// The asset item.
        /// </summary>
        public class AssetItemView : ContentItemView
        {
            /// <summary>
            /// Gets the asset.
            /// </summary>
            public AssetItem AssetItem => (AssetItem)ContentItem;

            /// <summary>
            /// Initializes a new instance of the <see cref="AssetItemView"/> class.
            /// </summary>
            /// <param name="asset">The asset.</param>
            public AssetItemView(AssetItem asset)
            : base(asset)
            {
            }
        }

        /// <summary>
        /// Validates if the given asset item can be used to pick it.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <returns>True if is valid.</returns>
        public new delegate bool IsValidDelegate(AssetItem asset);

        private IsValidDelegate _isValid;
        private Action<AssetItem> _selected;

        private AssetSearchPopup(IsValidDelegate isValid, Action<AssetItem> selected)
        {
            _isValid = isValid;
            _selected = selected;

            ItemClicked += OnItemClicked;

            // TODO: use async thread to search workspace items
            foreach (var project in Editor.Instance.ContentDatabase.Projects)
            {
                if (project.Content != null)
                    FindAssets(project.Content.Folder);
            }
            SortItems();
        }

        private void OnItemClicked(Item item)
        {
            _selected(((AssetItemView)item).AssetItem);
        }

        private void FindAssets(ContentFolder folder)
        {
            for (int i = 0; i < folder.Children.Count; i++)
            {
                if (folder.Children[i] is AssetItem asset && _isValid(asset))
                {
                    AddItem(new AssetItemView(asset));
                }
            }

            for (int i = 0; i < folder.Children.Count; i++)
            {
                if (folder.Children[i] is ContentFolder child)
                {
                    FindAssets(child);
                }
            }
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="isValid">Event called to check if a given asset item is valid to be used.</param>
        /// <param name="selected">Event called on asset item pick.</param>
        /// <returns>The dialog.</returns>
        public static AssetSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid, Action<AssetItem> selected)
        {
            var popup = new AssetSearchPopup(isValid, selected);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _isValid = null;
            _selected = null;

            base.OnDestroy();
        }
    }
}
