using System;
using FlaxEngine;
using FlaxEditor.Content;
using FlaxEditor.Windows;
using FlaxEngine.Experimental.UI;
using System.Runtime;

namespace FlaxEditor.Experimental.UI
{
    /// <inheritdoc />
    public class UIBlueprintAssetItem : JsonAssetItem
    {
        /// <inheritdoc />
        public UIBlueprintAssetItem(string path, Guid id, string typeName)
        : base(path, id, typeName)
        {

            // Use custom icon (Sprite)
            _thumbnail = FlaxEditor.Editor.Instance.Icons.Document128;

        }
    }
    /// <inheritdoc />
    [ContentContextMenu("New/Experimental/UI/Blueprint")]
    public sealed class UIBlueprintAssetProxy : JsonAssetProxy
    {
        /// <inheritdoc />
        public static readonly string AssetTypename = typeof(UIBlueprintAsset).FullName;

        ///// <inheritdoc />
        //public static readonly string Extension = "uiblueprint";

        /// <inheritdoc />
        public override string TypeName => AssetTypename;

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x7eff21);


        ///// <inheritdoc />
        //public override string Name => "UI Blueprint Asset";

        ///// <inheritdoc />
        //public override string FileExtension => Extension;

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new UIDesignerEditor(editor, (UIBlueprintAssetItem)item);
        }

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is UIBlueprintAssetItem;
        }

        /// <inheritdoc />
        public override bool IsProxyFor<T>()
        {
            return typeof(T) == typeof(UIBlueprintAssetItem);
        }
        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new UIBlueprintAssetItem(path, id, typeName);
        }

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }
        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            var asset = FlaxEngine.Content.CreateVirtualAsset<UIBlueprintAsset>();
            Editor.SaveJsonAsset(outputPath, asset);
        }
    }
}
