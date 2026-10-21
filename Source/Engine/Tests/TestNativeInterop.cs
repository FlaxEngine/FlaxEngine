// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using FlaxEngine.Interop;
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestNativeInterop
    {
        [Test]
        public void TestGetTypeSizeAllocations()
        {
            // A cached type size is returned without allocations (the first calls are not measured: the runtime allocates when it compiles the code)
            var type = typeof(Float3);
            for (int i = 0; i < 1000; i++)
                NativeInterop.GetTypeSize(type);
            var before = GC.GetAllocatedBytesForCurrentThread();
            for (int i = 0; i < 1000; i++)
                NativeInterop.GetTypeSize(type);
            Assert.Less(GC.GetAllocatedBytesForCurrentThread() - before, 1000);
        }
    }
}
#endif
