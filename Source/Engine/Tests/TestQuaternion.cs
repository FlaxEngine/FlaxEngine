// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests for <see cref="Quaternion"/>.
    /// </summary>
    [TestFixture]
    public class TestQuaternion
    {
        /// <summary>
        /// Test euler angle conversion to quaternion.
        /// </summary>
        [Test]
        public void TestEuler()
        {
            Assert.AreEqual(Quaternion.Euler(90, 0, 0), new Quaternion(0.7071068f, 0, 0, 0.7071068f));
            Assert.AreEqual(Quaternion.Euler(25, 0, 10), new Quaternion(0.215616f, -0.018864f, 0.0850898f, 0.9725809f));
            Assert.AreEqual(new Float3(25, 0, 10), Quaternion.Euler(25, 0, 10).EulerAngles);
            Assert.AreEqual(new Float3(25, -5, 10), Quaternion.Euler(25, -5, 10).EulerAngles);
        }

        /// <summary>
        /// Test multiply operation.
        /// </summary>
        [Test]
        public void TestMultiply()
        {
            var q = Quaternion.Identity;
            var delta = Quaternion.Euler(0, 10, 0);
            for (int i = 0; i < 9; i++)
                q *= delta;
            Assert.AreEqual(Quaternion.Euler(0, 90, 0), q);
        }
    }
}
#endif
