// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestModulusOperator
    {
        [Test]
        public void TestVector3Modulus()
        {
            Assert.AreEqual(new Vector3(10, 10, 10) % 3, new Vector3(1, 1, 1));
            Assert.AreEqual(10 % new Vector3(3, 3, 3), new Vector3(1, 1, 1));
            Assert.AreEqual(new Vector3(10, 10, 10) % new Vector3(3, 2, 3), new Vector3(1, 0, 1));
        }

        [Test]
        public void TestVector2Modulus()
        {
            Assert.AreEqual(new Vector2(10, 10) % 3, new Vector2(1, 1));
            Assert.AreEqual(10 % new Vector2(3, 3), new Vector2(1, 1));
            Assert.AreEqual(new Vector2(10, 10) % new Vector2(3, 2), new Vector2(1, 0));
        }
        
        [Test]
        public void TestVector4Modulus()
        {
            Assert.AreEqual(new Vector4(10, 10, 10, 10) % 3, new Vector4(1, 1, 1, 1));
            Assert.AreEqual(10 % new Vector4(3, 3, 3, 3), new Vector4(1, 1, 1, 1));
            Assert.AreEqual(new Vector4(10, 10, 10, 10) % new Vector4(3, 2, 3, 2), new Vector4(1, 0, 1, 0));
        }
        
        [Test]
        public void TestInt2Modulus()
        {
            Assert.AreEqual(new Int2(10, 10) % 3, new Int2(1, 1));
            Assert.AreEqual(10 % new Int2(3, 3), new Int2(1, 1));
            Assert.AreEqual(new Int2(10, 10) % new Int2(3, 2), new Int2(1, 0));
        }
        
        [Test]
        public void TestInt3Modulus()
        {
            Assert.AreEqual(new Int3(10, 10, 10) % 3, new Int3(1, 1, 1));
            Assert.AreEqual(10 % new Int3(3, 3, 3), new Int3(1, 1, 1));
            Assert.AreEqual(new Int3(10, 10, 10) % new Int3(3, 2, 3), new Int3(1, 0, 1));
        }
        
        [Test]
        public void TestInt4Modulus()
        {
            Assert.AreEqual(new Int4(10, 10, 10, 10) % 3, new Int4(1, 1, 1, 1));
            Assert.AreEqual(10 % new Int4(3, 3, 3, 3), new Int4(1, 1, 1, 1));
            Assert.AreEqual(new Int4(10, 10, 10, 10) % new Int4(3, 2, 3, 2), new Int4(1, 0, 1, 0));
        }
    }
}
#endif
