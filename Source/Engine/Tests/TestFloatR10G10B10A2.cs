// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestFloatR10G10B10A2
    {
        [Test]
        public void TestConversion()
        {
            Assert.IsTrue(Float4.NearEqual(Float4.Zero, new FloatR10G10B10A2(Float4.Zero).ToFloat4()));
            Assert.IsTrue(Float4.NearEqual(Float4.One, new FloatR10G10B10A2(Float4.One).ToFloat4()));
            Assert.IsTrue(Float4.NearEqual(new Float4(0.5004888f, 0.5004888f, 0.5004888f, 0.666667f), new FloatR10G10B10A2(new Float4(0.5f)).ToFloat4()));
            Assert.IsTrue(Float4.NearEqual(new Float4(1, 0, 0, 0), new FloatR10G10B10A2(new Float4(1, 0, 0, 0)).ToFloat4()));
            Assert.IsTrue(Float4.NearEqual(new Float4(0, 1, 0, 0), new FloatR10G10B10A2(new Float4(0, 1, 0, 0)).ToFloat4()));
            Assert.IsTrue(Float4.NearEqual(new Float4(0, 0, 1, 0), new FloatR10G10B10A2(new Float4(0, 0, 1, 0)).ToFloat4()));
            Assert.IsTrue(Float4.NearEqual(new Float4(0, 0, 0, 1), new FloatR10G10B10A2(new Float4(0, 0, 0, 1)).ToFloat4()));
        }
    }
}
#endif
