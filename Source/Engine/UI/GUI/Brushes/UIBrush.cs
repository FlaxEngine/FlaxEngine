// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Asset with <see cref="IBrush"/> that can be reused in different UI controls.
    /// </summary>
    /// <seealso cref="IBrush" />
    /// <seealso cref="UIBrush" />
    public class UIBrushAsset
    {
        /// <summary>
        /// Brush object.
        /// </summary>
        public IBrush Brush;
    }

    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.VideoPlayer"/> frame displaying.
    /// </summary>
    /// <seealso cref="IBrush" />
    /// <seealso cref="UIBrushAsset" />
    public sealed class UIBrush : IBrush
    {
        /// <summary>
        /// The UI Brush asset to use.
        /// </summary>
        public JsonAssetReference<UIBrushAsset> Asset;

        /// <summary>
        /// Initializes a new instance of the <see cref="UIBrush"/> class.
        /// </summary>
        public UIBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UIBrush"/> struct.
        /// </summary>
        /// <param name="asset">The UI Brush asset to use.</param>
        public UIBrush(JsonAssetReference<UIBrushAsset> asset)
        {
            Asset = asset;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UIBrush"/> struct.
        /// </summary>
        /// <param name="asset">The UI Brush asset to use.</param>
        public UIBrush(JsonAsset asset)
        {
            Asset = asset;
        }

        /// <inheritdoc />
        public Float2 Size
        {
            get
            {
                var asset = (UIBrushAsset)Asset.Asset?.Instance;
                if (asset != null && asset.Brush != null)
                    return asset.Brush.Size;
                return Float2.Zero;
            }
        }

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            var asset = (UIBrushAsset)Asset.Asset?.Instance;
            if (asset != null && asset.Brush != null)
                asset.Brush.Draw(rect, color);
        }
    }
}
