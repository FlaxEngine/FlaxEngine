// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using FlaxEditor.Utilities;
using NUnit.Framework;

namespace FlaxEditor.Tests
{
    [TestFixture]
    public class TestQueryFilterHelper
    {
        [Test]
        public void TestMatching()
        {
            // Simple matching
            Assert.IsFalse(QueryFilterHelper.Match("text", "txt"));
            Assert.IsFalse(QueryFilterHelper.Match("text", "txt txt"));
            Assert.IsFalse(QueryFilterHelper.Match("text", "txttxt"));
            Assert.IsTrue(QueryFilterHelper.Match("text", "text"));
            Assert.IsTrue(QueryFilterHelper.Match("text", "ss text"));
            Assert.IsTrue(QueryFilterHelper.Match("text", "text1"));
            Assert.IsTrue(QueryFilterHelper.Match("text", "text 1"));
            Assert.IsTrue(QueryFilterHelper.Match("text", "1text1"));
            Assert.IsTrue(QueryFilterHelper.Match("text", "text text"));

            // Exact matching
            QueryFilterHelper.Range[] range;
            Assert.IsTrue(QueryFilterHelper.Match("text", "text", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(0, 4), range[0]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("text", "text text", out range));
            Assert.AreEqual(2, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(0, 4), range[0]);
            Assert.AreEqual(new QueryFilterHelper.Range(5, 4), range[1]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("bool", "is it bool or boolean, boolish bol", out range));
            Assert.AreEqual(3, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(6, 4), range[0]);
            Assert.AreEqual(new QueryFilterHelper.Range(14, 4), range[1]);
            Assert.AreEqual(new QueryFilterHelper.Range(23, 4), range[2]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("boolean", "is it bool or boolean, boolish bol", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(14, 7), range[0]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("boolean (0)", "is it bool or boolean (0)", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(14, 11), range[0]);

            // More for searching in scene graph examples
            Assert.IsFalse(QueryFilterHelper.Match("actor", "acctor", out range));
            //
            Assert.IsTrue(QueryFilterHelper.Match("actor", "actor (0)", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(0, 5), range[0]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("actor ", "actor (0)", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(0, 6), range[0]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("actor (0)", "actor (0) more", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(0, 9), range[0]);
            //
            Assert.IsFalse(QueryFilterHelper.Match("actor (0)", "actor (1) more", out range));

            // Even more
            Assert.IsTrue(QueryFilterHelper.Match("magic", "Are you looking forTheMagic?", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(22, 5), range[0]);
            //
            Assert.IsTrue(QueryFilterHelper.Match("magic", "black Magical stuff", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(6, 5), range[0]);

            // Sth more tricky
            Assert.IsTrue(QueryFilterHelper.Match("magic ma", "magic magic magic", out range));
            Assert.AreEqual(1, range.Length);
            Assert.AreEqual(new QueryFilterHelper.Range(0, 8), range[0]);
        }
    }
}
#endif
