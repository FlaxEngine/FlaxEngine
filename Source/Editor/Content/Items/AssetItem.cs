// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Asset item object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentItem" />
    [HideInEditor]
    public abstract class AssetItem : ContentItem
    {
        /// <summary>
        /// Gets the asset unique identifier.
        /// </summary>
        public Guid ID { get; protected set; }

        /// <summary>
        /// Gets the asset type identifier.
        /// </summary>
        public string TypeName { get; }

        /// <summary>
        /// Returns true if asset is now loaded.
        /// </summary>
        public bool IsLoaded => FlaxEngine.Content.GetAsset(ID)?.IsLoaded ?? false;

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="typeName">The asset type name.</param>
        /// <param name="id">The asset identifier.</param>
        protected AssetItem(string path, string typeName, ref Guid id)
        : base(path)
        {
            TypeName = typeName;
            ID = id;
        }

        private sealed class TooltipDoubleClickHook : Control
        {
            public AssetItem Item;

            public TooltipDoubleClickHook()
            {
                AnchorPreset = AnchorPresets.StretchAll;
                Offsets = Margin.Zero;
            }

            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                return Item.OnMouseDoubleClick(Item.PointFromScreen(PointToScreen(location)), button);
            }
        }

        /// <inheritdoc />
        public override void OnTooltipShown(Tooltip tooltip)
        {
            base.OnTooltipShown(tooltip);

            // Inject the hook control for the double-click event (if user double-clicks on tooltip over the asset item it will open that item - helps on small screens)
            var hook = tooltip.GetChild<TooltipDoubleClickHook>();
            if (hook == null)
                hook = tooltip.AddChild<TooltipDoubleClickHook>();
            hook.Item = this;
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Asset;

        /// <inheritdoc />
        public override string TypeDescription
        {
            get
            {
                // Translate asset type name
                var typeName = TypeName;
                string[] typeNamespaces = typeName.Split('.');
                if (typeNamespaces.Length != 0 && typeNamespaces[typeNamespaces.Length - 1].Length != 0)
                {
                    typeName = Utilities.Utils.GetPropertyNameUI(typeNamespaces[typeNamespaces.Length - 1]);
                }
                return typeName;
            }
        }

        /// <summary>
        /// Loads the asset.
        /// </summary>
        /// <returns>The asset object.</returns>
        public Asset LoadAsync()
        {
            return FlaxEngine.Content.LoadAsync<Asset>(ID);
        }

        /// <summary>
        /// Reloads the asset (if it's loaded).
        /// </summary>
        public void Reload()
        {
            var asset = FlaxEngine.Content.GetAsset(ID);
            if (asset != null && asset.IsLoaded)
            {
                asset.Reload();
            }
        }

        /// <summary>
        /// Determines whether asset is of the specified type (included inheritance checks).
        /// </summary>
        /// <typeparam name="T">The type to check.</typeparam>
        /// <returns><c>true</c> if asset is of the specified type (including inherited types); otherwise, <c>false</c>.</returns>
        public bool IsOfType<T>()
        {
            return IsOfType(typeof(T));
        }

        /// <summary>
        /// Determines whether asset is of the specified type (included inheritance checks).
        /// </summary>
        /// <param name="type">The type to check.</param>
        /// <returns><c>true</c> if asset is of the specified type (including inherited types); otherwise, <c>false</c>.</returns>
        public virtual bool IsOfType(Type type)
        {
            return false;
        }

        /// <summary>
        /// Called when user dags this item into editor viewport or scene tree node.
        /// </summary>
        /// <param name="context">The editor context (eg. editor viewport or scene tree node).</param>
        /// <returns>True if item can be dropped in, otherwise false.</returns>
        public virtual bool OnEditorDrag(object context)
        {
            return false;
        }

        /// <summary>
        /// Called when user drops the item into editor viewport or scene tree node.
        /// </summary>
        /// <param name="context">The editor context (eg. editor viewport or scene tree node).</param>
        /// <returns>The spawned object.</returns>
        public virtual Actor OnEditorDrop(object context)
        {
            throw new NotSupportedException($"Asset {GetType()} doesn't support dropping into viewport.");
        }

        /// <inheritdoc />
        protected override bool DrawShadow => true;

        /// <inheritdoc />
        public override ContentItem Find(Guid id)
        {
            return id == ID ? this : null;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Path + ":" + ID;
        }
    }
}
