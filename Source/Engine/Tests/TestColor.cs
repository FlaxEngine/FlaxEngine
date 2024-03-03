// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestColor
    {
        [Test]
        public void TestRGB2HSVConversion()
        {
            Assert.AreEqual(new ColorHSV(312, 1, 1, 1), ColorHSV.FromColor(new Color(1, 0, 0.8f, 1)));
            Assert.AreEqual(new ColorHSV(0, 0, 0, 1), ColorHSV.FromColor(Color.Black));
            Assert.AreEqual(new ColorHSV(0, 0, 1, 1), ColorHSV.FromColor(Color.White));
            Assert.AreEqual(new ColorHSV(0, 1, 1, 1), ColorHSV.FromColor(Color.Red));
            Assert.AreEqual(new ColorHSV(120, 1, 1, 1), ColorHSV.FromColor(Color.Lime));
            Assert.AreEqual(new ColorHSV(240, 1, 1, 1), ColorHSV.FromColor(Color.Blue));
            Assert.AreEqual(new ColorHSV(60, 1, 1, 1), ColorHSV.FromColor(Color.Yellow));
            Assert.AreEqual(new ColorHSV(180, 1, 1, 1), ColorHSV.FromColor(Color.Cyan));
            Assert.AreEqual(new ColorHSV(300, 1, 1, 1), ColorHSV.FromColor(Color.Magenta));
            Assert.AreEqual(new ColorHSV(0, 0, 0.7529412f, 1), ColorHSV.FromColor(Color.Silver));
            Assert.AreEqual(new ColorHSV(0, 0, 0.5019608f, 1), ColorHSV.FromColor(Color.Gray));
            Assert.AreEqual(new ColorHSV(0, 1, 0.5019608f, 1), ColorHSV.FromColor(Color.Maroon));
        }

        [Test]
        public void TestHSV2RGBConversion()
        {
            Assert.AreEqual(Color.Black, ColorHSV.FromColor(Color.Black).ToColor());
            Assert.AreEqual(Color.White, ColorHSV.FromColor(Color.White).ToColor());
            Assert.AreEqual(Color.Red, ColorHSV.FromColor(Color.Red).ToColor());
            Assert.AreEqual(Color.Lime, ColorHSV.FromColor(Color.Lime).ToColor());
            Assert.AreEqual(Color.Blue, ColorHSV.FromColor(Color.Blue).ToColor());
            Assert.AreEqual(Color.Silver, ColorHSV.FromColor(Color.Silver).ToColor());
            Assert.AreEqual(Color.Maroon, ColorHSV.FromColor(Color.Maroon).ToColor());
            Assert.AreEqual(new Color(184, 209, 219, 255).ToRgba(), ColorHSV.FromColor(new Color(184, 209, 219, 255)).ToColor().ToRgba());
        }
    }
}
#endif
