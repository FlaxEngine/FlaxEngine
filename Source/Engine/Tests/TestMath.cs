// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests for <see cref="Mathf"/> and <see cref="Mathd"/>.
    /// </summary>
    [TestFixture]
    public class TestMath
    {
        /// <summary>
        /// Test unwinding angles.
        /// </summary>
        [Test]
        public void TestUnwind()
        {
            Assert.AreEqual(0.0f, Mathf.UnwindDegreesAccurate(0.0f));
            Assert.AreEqual(45.0f, Mathf.UnwindDegreesAccurate(45.0f));
            Assert.AreEqual(90.0f, Mathf.UnwindDegreesAccurate(90.0f));
            Assert.AreEqual(180.0f, Mathf.UnwindDegreesAccurate(180.0f));
            Assert.AreEqual(0.0f, Mathf.UnwindDegreesAccurate(360.0f));
            Assert.AreEqual(0.0f, Mathf.UnwindDegrees(0.0f));
            Assert.AreEqual(45.0f, Mathf.UnwindDegrees(45.0f));
            Assert.AreEqual(90.0f, Mathf.UnwindDegrees(90.0f));
            Assert.AreEqual(180.0f, Mathf.UnwindDegrees(180.0f));
            Assert.AreEqual(0.0f, Mathf.UnwindDegrees(360.0f));
            var fError = 0.001f;
            var dError = 0.00001;
            for (float f = -400.0f; f <= 400.0f; f += 0.1f)
            {
                var f1 = Mathf.UnwindDegreesAccurate(f);
                var f2 = Mathf.UnwindDegrees(f);
                if (Mathf.Abs(f1 - f2) >= fError)
                    throw new Exception($"Failed on angle={f}, {f1} != {f2}");
                var d = (double)f;
                var d1 = Mathd.UnwindDegreesAccurate(d);
                var d2 = Mathd.UnwindDegrees(d);
                if (Mathd.Abs(d1 - d2) >= dError)
                    throw new Exception($"Failed on angle={d}, {d1} != {d2}");
            }
        }
    }
}
#endif
