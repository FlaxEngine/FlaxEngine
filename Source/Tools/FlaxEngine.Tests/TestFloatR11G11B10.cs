// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestFloatR11G11B10
    {
        [Test]
        public void TestConversion()
        {
            Assert.AreEqual(Vector3.Zero, new FloatR11G11B10(Vector3.Zero).ToVector3());
            Assert.AreEqual(Vector3.One, new FloatR11G11B10(Vector3.One).ToVector3());
            Assert.AreEqual(new Vector3(0.5f, 0.5f, 0.5f), new FloatR11G11B10(new Vector3(0.5f)).ToVector3());
            Assert.AreEqual(new Vector3(1, 0, 0), new FloatR11G11B10(new Vector3(1, 0, 0)).ToVector3());
            Assert.AreEqual(new Vector3(0, 1, 0), new FloatR11G11B10(new Vector3(0, 1, 0)).ToVector3());
            Assert.AreEqual(new Vector3(0, 0, 1), new FloatR11G11B10(new Vector3(0, 0, 1)).ToVector3());
            Assert.AreEqual(new Vector3(10, 11, 12), new FloatR11G11B10(new Vector3(10, 11, 12)).ToVector3());
        }
    }
}
