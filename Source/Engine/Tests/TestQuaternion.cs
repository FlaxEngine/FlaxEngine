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
            Assert.IsTrue(Quaternion.NearEqual(Quaternion.Euler(90, 0, 0), new Quaternion(0.7071068f, 0, 0, 0.7071068f)));
            Assert.IsTrue(Quaternion.NearEqual(Quaternion.Euler(25, 0, 10), new Quaternion(0.215616f, -0.018864f, 0.0850898f, 0.9725809f)));
            Assert.IsTrue(Float3.NearEqual(new Float3(25, 0, 10), Quaternion.Euler(25, 0, 10).EulerAngles, 0.00001f));
            Assert.IsTrue(Float3.NearEqual(new Float3(25, -5, 10), Quaternion.Euler(25, -5, 10).EulerAngles, 0.00001f));
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
            Assert.IsTrue(Quaternion.NearEqual(Quaternion.Euler(0, 90, 0), q));
        }
    }
}
#endif
