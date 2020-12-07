// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
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

        /// <inheritdoc />
        protected override void UpdateTooltipText()
        {
            var sb = new StringBuilder();
            OnBuildTooltipText(sb);
            TooltipText = sb.ToString();
        }

        private sealed class TooltipDoubleClickHook : Control
        {
            public AssetItem Item;

            public TooltipDoubleClickHook()
            {
                AnchorPreset = AnchorPresets.StretchAll;
                Offsets = Margin.Zero;
            }

            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                return Item.OnMouseDoubleClick(Item.ScreenToClient(ClientToScreen(location)), button);
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

        /// <summary>
        /// Called when building tooltip text.
        /// </summary>
        /// <param name="sb">The String Builder.</param>
        protected virtual void OnBuildTooltipText(StringBuilder sb)
        {
            sb.Append("Type: ").Append(TypeName).AppendLine();
            sb.Append("Size: ").Append(Utilities.Utils.FormatBytesCount((int)new FileInfo(Path).Length)).AppendLine();
            sb.Append("Path: ").Append(Path).AppendLine();
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Asset;

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
