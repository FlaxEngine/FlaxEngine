// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.MaterialBase"/> rendering.
    /// </summary>
    /// <seealso cref="IBrush" />
    public sealed class MaterialBrush : IBrush, IEquatable<MaterialBrush>
    {
        /// <summary>
        /// The material.
        /// </summary>
        [ExpandGroups, Tooltip("The material to use for GUI control area rendering. It must be GUI domain.")]
        public MaterialBase Material;

        /// <summary>
        /// Initializes a new instance of the <see cref="MaterialBrush"/> class.
        /// </summary>
        public MaterialBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MaterialBrush"/> struct.
        /// </summary>
        /// <param name="material">The material.</param>
        public MaterialBrush(MaterialBase material)
        {
            Material = material;
        }

        /// <inheritdoc />
        public Float2 Size => Float2.One;

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color)
        {
            Render2D.DrawMaterial(Material, rect, color);
        }

        /// <inheritdoc />
        public bool Equals(MaterialBrush other)
        {
            return other != null && Material == other.Material;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is MaterialBrush other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return HashCode.Combine(Material);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return Equals(obj) ? 1 : 0;
        }
    }
}
