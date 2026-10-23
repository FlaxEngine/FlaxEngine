// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests of managed allocations made by the engine on its frequent paths (called from TestManagedAllocations.cpp).
    /// </summary>
    public static class TestManagedAllocations
    {
        /// <summary>
        /// Gets the managed bytes allocated by the current thread so far.
        /// </summary>
        public static long GetAllocatedBytes()
        {
            return GC.GetAllocatedBytesForCurrentThread();
        }
    }
}
#endif
