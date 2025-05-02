// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial struct GPUSamplerDescription : IEquatable<GPUSamplerDescription>
    {
        /// <summary>
        /// Clears description.
        /// </summary>
        public void Clear()
        {
            this = new GPUSamplerDescription();
            MaxMipLevel = float.MaxValue;
        }

        /// <summary>
        /// Creates a new <see cref="GPUSamplerDescription" /> with default settings.
        /// </summary>
        /// <param name="filter">The filtering method.</param>
        /// <param name="addressMode">The addressing mode.</param>
        /// <returns>A new instance of <see cref="GPUSamplerDescription" /> class.</returns>
        public static GPUSamplerDescription New(GPUSamplerFilter filter = GPUSamplerFilter.Point, GPUSamplerAddressMode addressMode = GPUSamplerAddressMode.Wrap)
        {
            return new GPUSamplerDescription
            {
                Filter = filter,
                AddressU = addressMode,
                AddressV = addressMode,
                AddressW = addressMode,
                MaxMipLevel = float.MaxValue,
            };
        }

        /// <inheritdoc />
        public bool Equals(GPUSamplerDescription other)
        {
            return Filter == other.Filter &&
                   AddressU == other.AddressU &&
                   AddressV == other.AddressV &&
                   AddressW == other.AddressW &&
                   Mathf.NearEqual(MipBias, other.MipBias) &&
                   Mathf.NearEqual(MinMipLevel, other.MinMipLevel) &&
                   Mathf.NearEqual(MaxMipLevel, other.MaxMipLevel) &&
                   MaxAnisotropy == other.MaxAnisotropy &&
                   BorderColor == other.BorderColor &&
                   ComparisonFunction == other.ComparisonFunction;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is GPUSamplerDescription other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = (int)Filter;
                hashCode = (hashCode * 397) ^ (int)AddressU;
                hashCode = (hashCode * 397) ^ (int)AddressV;
                hashCode = (hashCode * 397) ^ (int)AddressW;
                hashCode = (hashCode * 397) ^ (int)MipBias;
                hashCode = (hashCode * 397) ^ (int)MinMipLevel;
                hashCode = (hashCode * 397) ^ (int)MaxMipLevel;
                hashCode = (hashCode * 397) ^ MaxAnisotropy;
                hashCode = (hashCode * 397) ^ (int)BorderColor;
                hashCode = (hashCode * 397) ^ (int)ComparisonFunction;
                return hashCode;
            }
        }
    }
}
