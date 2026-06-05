// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.GPUTexture"/>.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class GPUTextureBrush : IBrush, IEquatable<GPUTextureBrush>
    {
        /// <summary>
        /// The GPU texture.
        /// </summary>
        [HideInEditor]
        public GPUTexture Texture;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        [ExpandGroups]
        public BrushFilter Filter = BrushFilter.Linear;

        /// <summary>
        /// Initializes a new instance of the <see cref="GPUTextureBrush"/> class.
        /// </summary>
        public GPUTextureBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GPUTextureBrush"/> struct.
        /// </summary>
        /// <param name="texture">The GPU texture.</param>
        public GPUTextureBrush(GPUTexture texture)
        {
            Texture = texture;
        }

        /// <inheritdoc />
        public Float2 Size => Texture != null ? Texture.Size : Float2.One;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            if (Texture && Texture.Dimensions != TextureDimensions.Texture)
            {
                // TODO: add cube and volume texture preview
                Render2D.FillRectangle(rect, color);
                return;
            }
            if (Filter == BrushFilter.Point)
                Render2D.DrawTexturePoint(Texture, rect, color);
            else
                Render2D.DrawTexture(Texture, rect, color);
        }

        /// <inheritdoc />
        public bool Equals(GPUTextureBrush other)
        {
            return other != null && Texture == other.Texture && Filter == other.Filter;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is GPUTextureBrush other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return HashCode.Combine(Texture, (int)Filter);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return Equals(obj) ? 1 : 0;
        }
    }
}
