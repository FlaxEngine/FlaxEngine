// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Tools.Terrain.Brushes
{
    /// <summary>
    /// Terrain brush that has circle shape and uses radial falloff.
    /// </summary>
    /// <seealso cref="FlaxEditor.Tools.Terrain.Brushes.Brush" />
    [HideInEditor]
    public class CircleBrush : Brush
    {
        /// <summary>
        /// Circle brush falloff types.
        /// </summary>
        public enum FalloffTypes
        {
            /// <summary>
            /// A linear falloff that has been smoothed to round off the sharp edges where the falloff begins and ends.
            /// </summary>
            Smooth = 0,

            /// <summary>
            /// A sharp linear falloff, without rounded edges.
            /// </summary>
            Linear = 1,

            /// <summary>
            /// A half-ellipsoid-shaped falloff that begins smoothly and ends sharply.
            /// </summary>
            Spherical = 2,

            /// <summary>
            /// A falloff with an abrupt start and a smooth ellipsoidal end. The opposite of the Sphere falloff.
            /// </summary>
            Tip = 3,
        }

        /// <summary>
        /// The brush falloff that defines the percentage from the brush's extents where the falloff should begin. Essentially, this describes how hard the brush's edges are. A falloff of 0 means the brush will have full effect throughout with hard edges. A falloff of 1 means the brush will only have full effect at its center, and the effect will be reduced throughout its entire area to the edge.
        /// </summary>
        [EditorOrder(10), Limit(0, 1, 0.01f), Tooltip("The brush falloff that defines the percentage from the brush's extents where the falloff should begin.")]
        public float Falloff = 0.5f;

        /// <summary>
        /// The brush falloff type. Defines circle brush falloff mode.
        /// </summary>
        [EditorOrder(20), Tooltip("The brush falloff type. Defines circle brush falloff mode.")]
        public FalloffTypes FalloffType = FalloffTypes.Smooth;

        private delegate float CalculateFalloffDelegate(float distance, float radius, float falloff);

        private float CalculateFalloff_Smooth(float distance, float radius, float falloff)
        {
            // Smooth-step linear falloff
            float alpha = CalculateFalloff_Linear(distance, radius, falloff);
            return alpha * alpha * (3 - 2 * alpha);
        }

        private float CalculateFalloff_Linear(float distance, float radius, float falloff)
        {
            return distance < radius ? 1.0f : falloff > 0.0f ? Mathf.Max(0.0f, 1.0f - (distance - radius) / falloff) : 0.0f;
        }

        private float CalculateFalloff_Spherical(float distance, float radius, float falloff)
        {
            if (distance <= radius)
            {
                return 1.0f;
            }

            if (distance > radius + falloff)
            {
                return 0.0f;
            }

            // Elliptical falloff
            return Mathf.Sqrt(1.0f - Mathf.Square((distance - radius) / falloff));
        }

        private float CalculateFalloff_Tip(float distance, float radius, float falloff)
        {
            if (distance <= radius)
            {
                return 1.0f;
            }

            if (distance > radius + falloff)
            {
                return 0.0f;
            }

            // Inverse elliptical falloff
            return 1.0f - Mathf.Sqrt(1.0f - Mathf.Square((falloff + radius - distance) / falloff));
        }

        /// <inheritdoc />
        public override MaterialInstance GetBrushMaterial(ref RenderContext renderContext, ref Vector3 position, ref Color color)
        {
            var material = CacheMaterial(EditorAssets.TerrainCircleBrushMaterial);
            if (material)
            {
                // Data 0: XYZ: position, W: radius
                // Data 1: X: falloff, Y: type
                float halfSize = Size * 0.5f;
                float falloff = halfSize * Falloff;
                float radius = halfSize - falloff;
                material.SetParameterValue("Color", color);
                material.SetParameterValue("BrushData0", new Float4(position - renderContext.View.Origin, radius));
                material.SetParameterValue("BrushData1", new Float4(falloff, (float)FalloffType, 0, 0));
            }
            return material;
        }

        /// <inheritdoc />
        public override float Sample(ref Vector3 brushPosition, ref Vector3 samplePosition)
        {
            Vector3.DistanceXZ(ref brushPosition, ref samplePosition, out var distanceXZ);
            float distance = (float)distanceXZ;
            float halfSize = Size * 0.5f;
            float falloff = halfSize * Falloff;
            float radius = halfSize - falloff;

            switch (FalloffType)
            {
            case FalloffTypes.Smooth: return CalculateFalloff_Smooth(distance, radius, falloff);
            case FalloffTypes.Linear: return CalculateFalloff_Linear(distance, radius, falloff);
            case FalloffTypes.Spherical: return CalculateFalloff_Spherical(distance, radius, falloff);
            case FalloffTypes.Tip: return CalculateFalloff_Tip(distance, radius, falloff);
            default: throw new ArgumentOutOfRangeException();
            }
        }
    }
}
