// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    partial struct LayersMask : IComparable, IComparable<LayersMask>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LayersMask"/> struct.
        /// </summary>
        /// <param name="mask">The bit mask.</param>
        public LayersMask(int mask) => Mask = (uint)mask;

        /// <summary>
        /// Initializes a new instance of the <see cref="LayersMask"/> struct.
        /// </summary>
        /// <param name="mask">The bit mask.</param>
        public LayersMask(uint mask) => Mask = mask;

        /// <summary>
        /// Determines whether the specified layer index is set in the mask.
        /// </summary>
        /// <param name="layerIndex">Index of the layer (zero-based).</param>
        /// <returns><c>true</c> if the specified layer is set; otherwise, <c>false</c>.</returns>
        public bool HasLayer(int layerIndex)
        {
            return (Mask & (1 << layerIndex)) != 0;
        }

        /// <summary>
        /// Determines whether the specified layer is set in the mask.
        /// </summary>
        /// <param name="layerName">Name of the layer (from Layers settings).</param>
        /// <returns><c>true</c> if the specified layer is set; otherwise, <c>false</c>.</returns>
        public bool HasLayer(string layerName)
        {
            return HasLayer(Level.GetLayerIndex(layerName));
        }

        /// <summary>
        /// Gets a layer mask based on a specific layer names.
        /// </summary>
        /// <param name="layerNames">The names of the layers (from Layers settings).</param>
        /// <returns>A layer mask with the mask set to the layers found. Returns a mask with 0 if not found.</returns>
        public static LayersMask GetMask(params string[] layerNames)
        {
            LayersMask mask = new LayersMask();
            foreach (var layerName in layerNames)
            {
                // Ignore blank entries
                if (string.IsNullOrEmpty(layerName))
                    continue;
                int index = Level.GetLayerIndex(layerName);
                if (index != -1)
                    mask.Mask |= (uint)(1 << index);
            }
            return mask;
        }
        
        /// <summary>
        /// Gets the layer index based on the layer name.
        /// </summary>
        /// <param name="layerName">The name of the layer.</param>
        /// <returns>The index if found, otherwise, returns -1.</returns>
        public static int GetLayerIndex(string layerName)
        {
            return Level.GetLayerIndex(layerName);
        }

        /// <summary>
        /// Gets the layer name based on the layer index.
        /// </summary>
        /// <param name="layerIndex">The index of the layer.</param>
        /// <returns>The name of the layer if found, otherwise, a blank string.</returns>
        public static string GetLayerName(int layerIndex)
        {
            return Level.GetLayerName(layerIndex);
        }

        /// <summary>
        /// Adds two masks.
        /// </summary>
        /// <param name="left">The left mask.</param>
        /// <param name="right">The right mask.</param>
        /// <returns>The sum of the two masks.</returns>
        public static LayersMask operator +(LayersMask left, LayersMask right) => new LayersMask(left.Mask | right.Mask);

        /// <summary>
        /// Removes one mask from another.
        /// </summary>
        /// <param name="left">The left mask.</param>
        /// <param name="right">The right mask.</param>
        /// <returns>The left mask without right mask.</returns>
        public static LayersMask operator -(LayersMask left, LayersMask right) => new LayersMask(left.Mask & ~right.Mask);

        /// <summary>
        /// Performance bitwise OR of the masks.
        /// </summary>
        /// <param name="left">The left mask.</param>
        /// <param name="right">The right mask.</param>
        /// <returns>The sum of the two masks.</returns>
        public static LayersMask operator |(LayersMask left, LayersMask right) => new LayersMask(left.Mask | right.Mask);

        /// <summary>
        /// Performance bitwise AND of the masks.
        /// </summary>
        /// <param name="left">The left mask.</param>
        /// <param name="right">The right mask.</param>
        /// <returns>The conjunction of the two masks.</returns>
        public static LayersMask operator &(LayersMask left, LayersMask right) => new LayersMask(left.Mask & right.Mask);

        /// <summary>
        /// Performance bitwise XOR of the masks.
        /// </summary>
        /// <param name="left">The left mask.</param>
        /// <param name="right">The right mask.</param>
        /// <returns>The difference of the two masks.</returns>
        public static LayersMask operator ^(LayersMask left, LayersMask right) => new LayersMask(left.Mask ^ right.Mask);

        /// <summary>
        /// Performance bitwise NOT of the mask.
        /// </summary>
        /// <param name="left">The mask.</param>
        /// <returns>The negated mask.</returns>
        public static LayersMask operator ~(LayersMask left) => new LayersMask(~left.Mask);

        /// <summary>
        /// Performs an implicit conversion from <see cref="LayersMask"/> to <see cref="System.UInt32"/>.
        /// </summary>
        /// <param name="mask">The mask.</param>
        /// <returns>The mask value.</returns>
        public static implicit operator uint(LayersMask mask) => mask.Mask;

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(LayersMask left, LayersMask right)
        {
            return left.Mask == right.Mask;
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(LayersMask left, LayersMask right)
        {
            return left.Mask != right.Mask;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Mask.GetHashCode();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Mask.ToString();
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="other">The other value to compare.</param>
        /// <returns>True if both values are equal, otherwise false.</returns>
        public bool Equals(LayersMask other)
        {
            return Mask == other.Mask;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is LayersMask other && Equals(other);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is LayersMask other)
                return Mask.CompareTo(other.Mask);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(LayersMask other)
        {
            return Mask.CompareTo(other.Mask);
        }
    }
}
