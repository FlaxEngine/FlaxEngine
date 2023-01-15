// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    /// <summary>
    /// Basic pseudo numbers generator utility.
    /// </summary>
    [HideInEditor]
    public class RandomUtil
    {
        private static readonly Random _random = new Random();

        /// <summary>
        /// Generates a pseudo-random number from normalized range [0;1].
        /// </summary>
        /// <returns>The random number.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Rand()
        {
            return _random.Next(0, int.MaxValue) / (float)int.MaxValue;
        }
    }
}
