// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using System.Globalization;
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests for <see cref="FlaxEditor.Utilities.Utils"/>.
    /// </summary>
    [TestFixture]
    public class TestEditorUtils
    {
        /// <summary>
        /// Test floating point values formatting to readable text.
        /// </summary>
        [Test]
        public void TestFormatFloat()
        {
            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;

            Assert.AreEqual("0", FlaxEditor.Utilities.Utils.FormatFloat(0.0f));
            Assert.AreEqual("0", FlaxEditor.Utilities.Utils.FormatFloat(0.0d));
            Assert.AreEqual("0.1234", FlaxEditor.Utilities.Utils.FormatFloat(0.1234f));
            Assert.AreEqual("0.1234", FlaxEditor.Utilities.Utils.FormatFloat(0.1234d));
            Assert.AreEqual("1234", FlaxEditor.Utilities.Utils.FormatFloat(1234.0f));
            Assert.AreEqual("1234", FlaxEditor.Utilities.Utils.FormatFloat(1234.0d));
            Assert.AreEqual("1234.123", FlaxEditor.Utilities.Utils.FormatFloat(1234.123f));
            Assert.AreEqual("1234.1234", FlaxEditor.Utilities.Utils.FormatFloat(1234.1234d));

            double[] values =
            {
                123450000000000000.0, 1.0 / 7, 10000000000.0 / 7, 100000000000000000.0 / 7, 0.001 / 7, 0.0001 / 7, 100000000000000000.0, 0.00000000001,
                1.23e-2, 1.234e-5, 1.2345E-10, 1.23456E-20, 5E-20, 1.23E+2, 1.234e5, 1.2345E10, -7.576E-05, 1.23456e20, 5e+20, 9.1093822E-31, 5.9736e24,
                double.Epsilon, Mathd.Epsilon, Mathf.Epsilon
            };
            foreach (int sign in new[] { 1, -1 })
            {
                foreach (double value in values)
                {
                    double value1 = sign * value;
                    string text = FlaxEditor.Utilities.Utils.FormatFloat(value1);
                    Assert.IsFalse(text.Contains("e", StringComparison.Ordinal));
                    Assert.IsFalse(text.Contains("E", StringComparison.Ordinal));
                    double value2 = double.Parse(text);
                    Assert.AreEqual(value2, value1);
                }
            }
            foreach (int sign in new[] { 1, -1 })
            {
                foreach (double value in values)
                {
                    float value1 = (float)(sign * value);
                    string text = FlaxEditor.Utilities.Utils.FormatFloat(value1);
                    Assert.IsFalse(text.Contains("e", StringComparison.Ordinal));
                    Assert.IsFalse(text.Contains("E", StringComparison.Ordinal));
                    float value2 = float.Parse(text);
                    Assert.AreEqual(value2, value1);
                }
            }
        }
    }
}
#endif
