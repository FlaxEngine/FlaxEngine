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
        /// A static random numbers generator.
        /// If you need a thread safe one, or one that can be seeded, please use [`Random`].
        /// </summary>
        public static readonly Random Random = new Random();
    }
}
