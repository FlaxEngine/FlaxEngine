// Copyright (c) Wojciech Figat. All rights reserved.

using NUnit.Framework;

#pragma warning disable CS0649

namespace Flax.Build.Tests
{
    [TestFixture]
    public class TestCommandLine
    {
        [Test, Sequential]
        public void TestParseOptionOnly([Values(
                "-something",
                "something",
                "  \t   \t-\t   \tsomething\t  ",
                "-something=")]
            string commandLine)
        {
            var options = CommandLine.Parse(commandLine);
            Assert.AreEqual(1, options.Length);
            Assert.AreEqual("something", options[0].Name);
            Assert.IsTrue(string.IsNullOrEmpty(options[0].Value));
        }

        [Test, Sequential]
        public void TestParseOneValue([Values(
                "-something=value",
                "something=value",
                "-something=\"value\"",
                "-something=\\\"value\\\"",
                "\"-something=\"value\"\"",
                "\"-something=\\\"value\\\"\"",
                "  \t   \t-\t   \tsomething\t =value  ",
                "-something=value    ")]
            string commandLine)
        {
            var options = CommandLine.Parse(commandLine);
            Assert.AreEqual(1, options.Length);
            Assert.AreEqual("something", options[0].Name);
            Assert.AreEqual("value", options[0].Value);
        }

        [Test, Sequential]
        public void TestParseTwoOptions([Values(
                "-first -something=value",
                "-first \"-something =value\"",
                "  \t  -first  \t-\t   \tsomething\t =value  ",
                "-first\t\t-something=value    ")]
            string commandLine)
        {
            var options = CommandLine.Parse(commandLine);
            Assert.AreEqual(2, options.Length);
            Assert.AreEqual("first", options[0].Name);
            Assert.IsTrue(string.IsNullOrEmpty(options[0].Value));
            Assert.AreEqual("something", options[1].Name);
            Assert.AreEqual("value", options[1].Value);
        }

        [Test, Sequential]
        public void TestParsePathOption([Values(
                "-something=value\\anotherFolder -second",
                "  \t   \t-\t   \tsomething\t =value\\anotherFolder -second  ",
                " -something\t =\"value\\anotherFolder\" -second  ",
                " -something\t =\'value\\anotherFolder\' -second  ",
                "\"-something\t =value\\anotherFolder\" -second  ",
                " \t\t-something=value\\anotherFolder   -second ")]
            string commandLine)
        {
            var options = CommandLine.Parse(commandLine);
            Assert.AreEqual(2, options.Length);
            Assert.AreEqual("something", options[0].Name);
            Assert.AreEqual("value\\anotherFolder", options[0].Value);
            Assert.AreEqual("second", options[1].Name);
            Assert.IsTrue(string.IsNullOrEmpty(options[1].Value));
        }

        public enum Mode
        {
            Mode1,
            Mode2,
            Mode3,
        }

        class TestConfig1
        {
            /// <summary>
            /// The option 1.
            /// </summary>
            [CommandLine("option1", "")]
            public static bool Option1;

            /// <summary>
            /// The option 2.
            /// </summary>
            [CommandLine("option2", "")]
            public static int Option2;

            /// <summary>
            /// The option 3.
            /// </summary>
            [CommandLine("option3", "")]
            public static string Option3;

            /// <summary>
            /// The option 4.
            /// </summary>
            [CommandLine("option4", "")]
            public static Mode Option4;
        }

        class TestConfig2
        {
            /// <summary>
            /// The option 1.
            /// </summary>
            [CommandLine("option1", "")]
            public bool Option1;

            /// <summary>
            /// The option 2.
            /// </summary>
            [CommandLine("option2", "")]
            public int Option2;

            /// <summary>
            /// The option 3.
            /// </summary>
            [CommandLine("option3", "")]
            public string Option3;

            /// <summary>
            /// The option 4.
            /// </summary>
            [CommandLine("option4", "")]
            public Mode Option4;
        }

        class TestConfig3
        {
            [CommandLine("option1", "")]
            public static string[] Option1;
        }

        [Test]
        public void TestConfigure()
        {
            Assert.AreEqual(false, TestConfig1.Option1);
            CommandLine.Configure(typeof(TestConfig1), "-option1 -option");
            Assert.AreEqual(true, TestConfig1.Option1);

            CommandLine.Configure(typeof(TestConfig1), "-option2=11");
            Assert.AreEqual(11, TestConfig1.Option2);

            CommandLine.Configure(typeof(TestConfig1), "-option3=hello -option4=Mode2");
            Assert.AreEqual("hello", TestConfig1.Option3);
            Assert.AreEqual(Mode.Mode2, TestConfig1.Option4);
        }

        [Test]
        public void TestConfigureObject()
        {
            var obj = new TestConfig2();

            Assert.AreEqual(false, obj.Option1);
            CommandLine.Configure(obj, "-Option1 -Option");
            Assert.AreEqual(true, obj.Option1);

            CommandLine.Configure(obj, "-Option2=11");
            Assert.AreEqual(11, obj.Option2);

            CommandLine.Configure(obj, "-Option3=hello -Option4=Mode2");
            Assert.AreEqual("hello", obj.Option3);
            Assert.AreEqual(Mode.Mode2, obj.Option4);
        }

        [Test]
        public void TestArray()
        {
            Assert.AreEqual(null, TestConfig3.Option1);
            CommandLine.Configure(typeof(TestConfig3), "-option1=value1,value2");
            Assert.NotNull(TestConfig3.Option1);
            Assert.AreEqual(2, TestConfig3.Option1.Length);
            Assert.AreEqual("value1", TestConfig3.Option1[0]);
            Assert.AreEqual("value2", TestConfig3.Option1[1]);
        }
    }
}
