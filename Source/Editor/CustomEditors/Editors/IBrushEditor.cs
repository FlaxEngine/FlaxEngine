// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit <see cref="IBrush"/> type properties.
    /// </summary>
    /// <seealso cref="IBrush"/>
    /// <seealso cref="ObjectSwitcherEditor"/>
    [CustomEditor(typeof(IBrush)), DefaultEditor]
    public sealed class IBrushEditor : ObjectSwitcherEditor
    {
        /// <inheritdoc />
        protected override OptionType[] Options => new[]
        {
            new OptionType("null", null),
            new OptionType("Texture", typeof(TextureBrush)),
            new OptionType("Sprite", typeof(SpriteBrush)),
            new OptionType("GPU Texture", typeof(GPUTextureBrush)),
            new OptionType("Material", typeof(MaterialBrush)),
            new OptionType("Solid Color", typeof(SolidColorBrush)),
            new OptionType("Linear Gradient", typeof(LinearGradientBrush)),
            new OptionType("Texture 9-Slicing", typeof(Texture9SlicingBrush)),
            new OptionType("Sprite 9-Slicing", typeof(Sprite9SlicingBrush)),
            new OptionType("Video", typeof(VideoBrush)),
            new OptionType("UI Brush", typeof(UIBrush)),
        };
    }
}
