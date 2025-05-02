// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using FlaxEditor.Utilities;
using NUnit.Framework;

namespace FlaxEditor.Tests
{
    [TestFixture]
    public class TestPropertyNameUI
    {
        [Test]
        public void TestFormatting()
        {
            Assert.AreEqual("Property", Utils.GetPropertyNameUI("property"));
            Assert.AreEqual("Property", Utils.GetPropertyNameUI("_property"));
            Assert.AreEqual("Property", Utils.GetPropertyNameUI("m_property"));
            Assert.AreEqual("Property", Utils.GetPropertyNameUI("g_property"));
            Assert.AreEqual("Property", Utils.GetPropertyNameUI("Property"));
            Assert.AreEqual("Property 1", Utils.GetPropertyNameUI("Property1"));
            Assert.AreEqual("Property Name", Utils.GetPropertyNameUI("PropertyName"));
            Assert.AreEqual("Property Name", Utils.GetPropertyNameUI("Property_Name"));
            Assert.AreEqual("Property 123", Utils.GetPropertyNameUI("Property_123"));
            Assert.AreEqual("Property TMP", Utils.GetPropertyNameUI("Property_TMP"));
            Assert.AreEqual("Property TMP", Utils.GetPropertyNameUI("PropertyTMP"));
            Assert.AreEqual("Property T", Utils.GetPropertyNameUI("PropertyT"));
            Assert.AreEqual("Property TMP Word", Utils.GetPropertyNameUI("PropertyTMPWord"));
            Assert.AreEqual("Generate SDF On Model Import", Utils.GetPropertyNameUI("GenerateSDFOnModelImport"));
        }
    }
}
#endif
