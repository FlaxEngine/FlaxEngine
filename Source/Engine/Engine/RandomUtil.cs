// Copyright (c) Wojciech Figat. All rights reserved.

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
    }
}
