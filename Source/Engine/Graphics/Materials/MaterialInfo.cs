// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct MaterialInfo
    {
        /// <summary>
        /// Creates the default <see cref="MaterialInfo"/>.
        /// </summary>
        /// <returns>The result.</returns>
        public static MaterialInfo CreateDefault()
        {
            return new MaterialInfo
            {
                Domain = MaterialDomain.Surface,
                BlendMode = MaterialBlendMode.Opaque,
                ShadingModel = MaterialShadingModel.Lit,
                UsageFlags = MaterialUsageFlags.None,
                FeaturesFlags = MaterialFeaturesFlags.None,
                DecalBlendingMode = MaterialDecalBlendingMode.Translucent,
                TransparentLightingMode = MaterialTransparentLightingMode.SurfaceNonDirectional,
                PostFxLocation = MaterialPostFxLocation.AfterPostProcessingPass,
                MaskThreshold = 0.3f,
                OpacityThreshold = 0.12f,
                TessellationMode = TessellationMethod.None,
                MaxTessellationFactor = 15,
            };
        }

        /// <summary>
        /// Implements the operator ==.
        /// </summary>
        /// <param name="a">The a.</param>
        /// <param name="b">The b.</param>
        /// <returns>The result of the operator.</returns>
        public static bool operator ==(MaterialInfo a, MaterialInfo b)
        {
            return a.Equals(b);
        }

        /// <summary>
        /// Implements the operator !=.
        /// </summary>
        /// <param name="a">The a.</param>
        /// <param name="b">The b.</param>
        /// <returns>The result of the operator.</returns>
        public static bool operator !=(MaterialInfo a, MaterialInfo b)
        {
            return !a.Equals(b);
        }

        /// <summary>
        /// Compares with the other material info and returns true if both values are equal.
        /// </summary>
        /// <param name="other">The other info.</param>
        /// <returns>True if both objects are equal, otherwise false.</returns>
        public bool Equals(MaterialInfo other)
        {
            return Domain == other.Domain
                   && BlendMode == other.BlendMode
                   && ShadingModel == other.ShadingModel
                   && UsageFlags == other.UsageFlags
                   && FeaturesFlags == other.FeaturesFlags
                   && DecalBlendingMode == other.DecalBlendingMode
                   && TransparentLightingMode == other.TransparentLightingMode
                   && PostFxLocation == other.PostFxLocation
                   && Mathf.NearEqual(MaskThreshold, other.MaskThreshold)
                   && Mathf.NearEqual(OpacityThreshold, other.OpacityThreshold)
                   && TessellationMode == other.TessellationMode
                   && MaxTessellationFactor == other.MaxTessellationFactor;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is MaterialInfo info && Equals(info);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = (int)Domain;
                hashCode = (hashCode * 397) ^ (int)BlendMode;
                hashCode = (hashCode * 397) ^ (int)ShadingModel;
                hashCode = (hashCode * 397) ^ (int)UsageFlags;
                hashCode = (hashCode * 397) ^ (int)FeaturesFlags;
                hashCode = (hashCode * 397) ^ (int)PostFxLocation;
                hashCode = (hashCode * 397) ^ (int)DecalBlendingMode;
                hashCode = (hashCode * 397) ^ (int)TransparentLightingMode;
                hashCode = (hashCode * 397) ^ (int)(MaskThreshold * 1000.0f);
                hashCode = (hashCode * 397) ^ (int)(OpacityThreshold * 1000.0f);
                hashCode = (hashCode * 397) ^ (int)TessellationMode;
                hashCode = (hashCode * 397) ^ MaxTessellationFactor;
                return hashCode;
            }
        }
    }
}
