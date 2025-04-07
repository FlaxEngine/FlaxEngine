// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
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
            Assert.AreEqual("my Name", FlaxEditor.Utilities.Utils.IncrementNameNumber("my Name", null));
            Assert.AreEqual("my Name", FlaxEditor.Utilities.Utils.IncrementNameNumber("my Name", x => true));
            Assert.AreEqual("my Name 10", FlaxEditor.Utilities.Utils.IncrementNameNumber("my Name 1", x => x.EndsWith("10")));
            Assert.AreEqual("my Name (10)", FlaxEditor.Utilities.Utils.IncrementNameNumber("my Name (1)", x => x.EndsWith("10)")));
            Assert.AreEqual("my Name110", FlaxEditor.Utilities.Utils.IncrementNameNumber("my Name100", x => x.EndsWith("110")));
        }
    }
}
#endif
