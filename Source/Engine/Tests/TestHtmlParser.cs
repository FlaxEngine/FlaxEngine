// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System.Collections.Generic;
using FlaxEngine.Utilities;
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestHtmlParser
    {
        [TestCase("")]
        [TestCase("a")]
        [TestCase("a\na")]
        [TestCase("a\ra")]
        [TestCase("<a>")]
        [TestCase("<a/>")]
        [TestCase("<a />")]
        [TestCase("<a>b<a/>")]
        [TestCase("<!-- -->")]
        public void TestValid(string html)
        {
            var parser = new HtmlParser(html);
            while (parser.ParseNext(out var tag))
            {
                Assert.IsNotNull(tag.Name);
                Assert.IsNotNull(tag.Attributes);
            }
        }

        [TestCase("b<a>b", ExpectedResult = new[] { "a" })]
        [TestCase("b<a/>b", ExpectedResult = new[] { "a" })]
        [TestCase("b<a />b", ExpectedResult = new[] { "a" })]
        [TestCase("<a>b<a/>", ExpectedResult = new[] { "a", "a" })]
        [TestCase("<a>b<a />", ExpectedResult = new[] { "a", "a" })]
        [TestCase("<a >b<a/>", ExpectedResult = new[] { "a", "a" })]
        public string[] TestTags(string html)
        {
            var tags = new List<string>();
            var parser = new HtmlParser(html);
            while (parser.ParseNext(out var tag))
            {
                tags.Add(tag.Name);
            }
            return tags.ToArray();
        }

        [TestCase("b<a size=50>b", ExpectedResult = "size=50")]
        [TestCase("b<a size=50 len=60>b", ExpectedResult = "size=50,len=60")]
        [TestCase("b<a size=\"50\">b", ExpectedResult = "size=50")]
        [TestCase("b<a size=\"5 0\">b", ExpectedResult = "size=5 0")]
        [TestCase("b<a size=50%>b", ExpectedResult = "size=50%")]
        [TestCase("b<a size=#FF>b", ExpectedResult = "size=#FF")]
        [TestCase("b<a=value>b", ExpectedResult = "=value")]
        public string TestAttributes(string html)
        {
            var result = string.Empty;
            var parser = new HtmlParser(html);
            while (parser.ParseNext(out var tag))
            {
                foreach (var e in tag.Attributes)
                {
                    if (result.Length != 0)
                        result += ',';
                    result += e.Key + "=" + e.Value;
                }
            }
            return result;
        }

        [TestCase("b<a>", ExpectedResult = false)]
        [TestCase("b<a/>", ExpectedResult = true)]
        [TestCase("b<a />", ExpectedResult = true)]
        [TestCase("b<a  />", ExpectedResult = true)]
        [TestCase("b<a size=50>", ExpectedResult = false)]
        [TestCase("b<a size=50/>", ExpectedResult = true)]
        [TestCase("b</a>", ExpectedResult = true)]
        public bool TestEnding(string html)
        {
            var parser = new HtmlParser(html);
            while (parser.ParseNext(out var tag))
            {
                return tag.IsSlash;
            }
            throw new System.Exception();
        }

        [TestCase("b<a size=50>", ExpectedResult = 1)]
        [TestCase("sd b <a>", ExpectedResult = 5)]
        [TestCase("sd b </a>", ExpectedResult = 5)]
        [TestCase("sd b <a/>", ExpectedResult = 5)]
        public int TestStartPosition(string html)
        {
            var parser = new HtmlParser(html);
            while (parser.ParseNext(out var tag))
            {
                return tag.StartPosition;
            }
            return -1;
        }

        [TestCase("b<a>", ExpectedResult = 4)]
        [TestCase("b<a/>", ExpectedResult = 5)]
        [TestCase("b<a />", ExpectedResult = 6)]
        [TestCase("b<a  />", ExpectedResult = 7)]
        [TestCase("b<a size=50>", ExpectedResult = 12)]
        [TestCase("b<a size=\"50\">", ExpectedResult = 14)]
        public int TestEndPosition(string html)
        {
            var parser = new HtmlParser(html);
            while (parser.ParseNext(out var tag))
            {
                return tag.EndPosition;
            }
            return -1;
        }
    }
}
#endif
