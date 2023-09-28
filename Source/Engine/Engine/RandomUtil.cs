// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;
using FlaxEngine.Utilities;

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
        /// <returns>The random <see cref="float"/>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float RandRange(float min, float max)
        {
            return Random.NextFloat(min, max);
        }

        /// <summary>
        ///Generates a random number within a specified range.
        /// </summary>
        /// <param name="min">The minimum value of the range.</param>
        /// <param name="max">The maximum value of the range.</param>
        /// <returns>The random <see cref="double"/>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static double RandRange(double min, double max)
        {
            return Random.NextDouble(min, max);
        }

        /// <summary>
        /// Generates a random number within a specified range.
        /// </summary>
        /// <param name="min">The minimum value of the range.</param>
        /// <param name="max">The maximum value of the range.</param>
        /// <returns>The random <see cref="int"/>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int RandRange(int min, int max)
        {
            return Random.Next(min, max);
        }

        /// <summary>
        /// Generates a random point inside a sphere with radius 1.0.
        /// </summary>
        /// <returns>A random <see cref="Float3"/> within the sphere.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float3 GetPointInsideSphere()
        {
            return Random.NextUnitVector3();
        }

        /// <summary>
        /// Generates a random point on the surface of a sphere with radius 1.0.
        /// </summary>
        /// <returns>A random <see cref="Float3"/>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float3 GetPointOnSphere()
        {
            float z = 2 * Random.NextFloat() - 1;
            float t = 2 * Mathf.Pi * Random.NextFloat();

            float r = Mathf.Sqrt(1 - z * z);
            float x = r * Mathf.Cos(t);
            float y = r * Mathf.Sin(t);

            return new Float3(x, y, z);
        }

        /// <summary>
        /// Generates a random uniform rotation quaternion.
        /// </summary>
        /// <returns>A random <see cref="Quaternion"/>.</returns>
        /// <param name="randomRoll">Should the roll value be randomized.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion GetUniformOrientation(bool randomRoll = false)
        {
            return Random.NextQuaternion(randomRoll);
        }
    }
}
