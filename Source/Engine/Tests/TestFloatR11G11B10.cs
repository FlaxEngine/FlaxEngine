// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestFloatR11G11B10
    {
        [Test]
        public void TestConversion()
        {
            Assert.IsTrue(Float3.NearEqual(Float3.Zero, new FloatR11G11B10(Float3.Zero).ToFloat3()));
            Assert.IsTrue(Float3.NearEqual(Float3.One, new FloatR11G11B10(Float3.One).ToFloat3()));
            Assert.IsTrue(Float3.NearEqual(new Float3(0.5f, 0.5f, 0.5f), new FloatR11G11B10(new Float3(0.5f)).ToFloat3()));
            Assert.IsTrue(Float3.NearEqual(new Float3(1, 0, 0), new FloatR11G11B10(new Float3(1, 0, 0)).ToFloat3()));
            Assert.IsTrue(Float3.NearEqual(new Float3(0, 1, 0), new FloatR11G11B10(new Float3(0, 1, 0)).ToFloat3()));
            Assert.IsTrue(Float3.NearEqual(new Float3(0, 0, 1), new FloatR11G11B10(new Float3(0, 0, 1)).ToFloat3()));
            Assert.IsTrue(Float3.NearEqual(new Float3(10, 11, 12), new FloatR11G11B10(new Float3(10, 11, 12)).ToFloat3()));
        }
    }
}
#endif
