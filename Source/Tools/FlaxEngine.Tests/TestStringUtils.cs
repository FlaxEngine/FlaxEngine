// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using NUnit.Framework;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests for <see cref="StringUtils"/>.
    /// </summary>
    [TestFixture]
    public class TestStringUtils
    {
        /// <summary>
        /// Test incrementing name numbers.
        /// </summary>
        [Test]
        public void TestIncName()
        {
            Assert.AreEqual("my Name", StringUtils.IncrementNameNumber("my Name", null));
            Assert.AreEqual("my Name", StringUtils.IncrementNameNumber("my Name", x => true));
            Assert.AreEqual("my Name 10", StringUtils.IncrementNameNumber("my Name 1", x => x.EndsWith("10")));
            Assert.AreEqual("my Name (10)", StringUtils.IncrementNameNumber("my Name (1)", x => x.EndsWith("10)")));
            Assert.AreEqual("my Name110", StringUtils.IncrementNameNumber("my Name100", x => x.EndsWith("110")));
        }
    }
}
