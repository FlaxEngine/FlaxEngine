// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    /// <summary>
    /// Basic pseudo numbers generator utility.
    /// </summary>
    public static class RandomUtil
    {
        /// <summary>
        /// Random numbers generator.
        /// </summary>
        public static readonly Random Random = new Random();

        /// <summary>
        /// Generates a pseudo-random number from normalized range [0;1].
        /// </summary>
        /// <returns>The random number.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Rand()
        {
            return Random.Next(0, int.MaxValue) / (float)int.MaxValue;
        }

        /// <summary>
        ///Generates a random number within a specified range.
        /// </summary>
        /// <param name="min">The minimum value of the range.</param>
        /// <param name="max">The maximum value of the range.</param>
        /// <returns>The random number.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float RandRange(float min, float max)
        {
            return Rand() * (max - min) + min;
        }

        /// <summary>
        ///Generates a random number within a specified range.
        /// </summary>
        /// <param name="min">The minimum value of the range.</param>
        /// <param name="max">The maximum value of the range.</param>
        /// <returns>The random number.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static double RandRange(double min, double max)
        {
            return Random.NextDouble() * (max - min) + min;
        }

        /// <summary>
        /// Generates a random number within a specified range.
        /// </summary>
        /// <param name="min">The minimum value of the range.</param>
        /// <param name="max">The maximum value of the range.</param>
        /// <returns>The random number.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int RandRange(int min, int max)
        {
            return Random.Next(min, max);
        }

        /// <summary>
        /// Generates a random point inside a sphere with radius 1.0.
        /// </summary>
        /// <returns>A random point within the sphere.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float3 InsideSphere()
        {
            float u = RandRange(-1, 1); 
            float v = RandRange(-1, 1); 
            float w = RandRange(-1, 1); 

            // Normalize the vector to ensure it's inside the unit sphere
            float length = Mathf.Sqrt(u * u + v * v + w * w);
            u /= length;
            v /= length;
            w /= length;

            return new Float3(u, v, w);
        }

        /// <summary>
        /// Generates a random point on the surface of a sphere with radius 1.0.
        /// </summary>
        /// <returns>A random point on the sphere's surface.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float3 OnSphere()
        {
            float phi = Rand() * 2f * Mathf.Pi;
            float cosTheta = Rand() * 2f - 1f;
            float sinTheta = Mathf.Sqrt(1f - cosTheta * cosTheta);

            float x = Mathf.Cos(phi) * sinTheta;
            float y = Mathf.Sin(phi) * sinTheta;
            float z = cosTheta;

            return new Float3(x, y, z);
        }

    }
}
