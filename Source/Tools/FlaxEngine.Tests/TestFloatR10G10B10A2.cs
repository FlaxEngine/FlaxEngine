// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestFloatR10G10B10A2
    {
        [Test]
        public void TestConversion()
        {
            Assert.AreEqual(Vector4.Zero, new FloatR10G10B10A2(Vector4.Zero).ToVector4());
            Assert.AreEqual(Vector4.One, new FloatR10G10B10A2(Vector4.One).ToVector4());
            Assert.AreEqual(new Vector4(0.5004888f, 0.5004888f, 0.5004888f, 0.666667f), new FloatR10G10B10A2(new Vector4(0.5f)).ToVector4());
            Assert.AreEqual(new Vector4(1, 0, 0, 0), new FloatR10G10B10A2(new Vector4(1, 0, 0, 0)).ToVector4());
            Assert.AreEqual(new Vector4(0, 1, 0, 0), new FloatR10G10B10A2(new Vector4(0, 1, 0, 0)).ToVector4());
            Assert.AreEqual(new Vector4(0, 0, 1, 0), new FloatR10G10B10A2(new Vector4(0, 0, 1, 0)).ToVector4());
            Assert.AreEqual(new Vector4(0, 0, 0, 1), new FloatR10G10B10A2(new Vector4(0, 0, 0, 1)).ToVector4());
        }
    }
}
