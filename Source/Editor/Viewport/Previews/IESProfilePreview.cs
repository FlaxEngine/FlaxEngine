// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Preview control for <see cref="IESProfile"/> asset.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Previews.TexturePreviewBase" />
    public class IESProfilePreview : TexturePreviewBase
    {
        private IESProfile _asset;
        private MaterialInstance _previewMaterial;

        /// <summary>
        /// Gets or sets the asset to preview.
        /// </summary>
        public IESProfile Asset
        {
            get => _asset;
            set
            {
                if (_asset != value)
                {
                    _asset = value;
                    _previewMaterial.SetParameterValue("Texture", value);
                    UpdateTextureRect();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="IESProfilePreview"/> class.
        /// </summary>
        public IESProfilePreview()
        {
            var baseMaterial = FlaxEngine.Content.LoadAsyncInternal<Material>(EditorAssets.IesProfilePreviewMaterial);

            // Wait for base (don't want to async material parameters set due to async loading)
            if (baseMaterial == null || baseMaterial.WaitForLoaded())
                throw new Exception("Cannot load IES Profile preview material.");

            // Create preview material (virtual)
            _previewMaterial = baseMaterial.CreateVirtualInstance();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Object.Destroy(ref _previewMaterial);

            base.OnDestroy();
        }

        /// <inheritdoc />
        protected override void CalculateTextureRect(out Rectangle rect)
        {
            CalculateTextureRect(new Float2(256), Size, out rect);
        }

        /// <inheritdoc />
        protected override void DrawTexture(ref Rectangle rect)
        {
            // Check if has loaded asset
            if (_asset && _asset.IsLoaded)
            {
                Render2D.DrawMaterial(_previewMaterial, rect);
            }
        }
    }
}
