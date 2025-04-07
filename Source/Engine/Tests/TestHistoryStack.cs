// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using FlaxEditor.History;
using NUnit.Framework;
using Assert = FlaxEngine.Assertions.Assert;

namespace FlaxEditor.Tests
{
    [TestFixture]
    public class TestHistoryStack
    {
        public class HistoryTestObject : IHistoryAction
        {
            public int Item;

            public HistoryTestObject(int item)
            {
                Item = item;
            }

            public override bool Equals(object obj)
            {
                var historyTestObject = (HistoryTestObject)obj;
                return historyTestObject != null && Item == historyTestObject.Item;
            }

            public static implicit operator int(HistoryTestObject obj)
            {
                return obj.Item;
            }

            public static implicit operator HistoryTestObject(int obj)
            {
                return new HistoryTestObject(obj);
            }

            public override string ToString()
            {
                return Item.ToString();
            }

            public override int GetHashCode()
            {
                var hashCode = -606588576;
                hashCode = hashCode * -1521134295 + Item.GetHashCode();
                return hashCode;
            }

            public string ActionString { get; set; }

            public void Dispose()
            {
            }
        }

        [Test]
        public void HistoryStackTestBasic()
        {
            var stack = new HistoryStack(50);
            for (int i = 0; i < 80; i++)
            {
                stack.Push(new HistoryTestObject(i));
            }
            for (int i = 80 - 1; i >= 80 - 50; i--)
            {
                Assert.AreEqual(i, (int)(HistoryTestObject)stack.PopHistory());
                Assert.AreEqual(i, (int)(HistoryTestObject)stack.PeekReverse());
            }
            for (int i = 80 - 50; i < 80; i++)
            {
                Assert.AreEqual(i, (int)(HistoryTestObject)stack.PopReverse());
                Assert.AreEqual(i, (int)(HistoryTestObject)stack.PeekHistory());
            }
        }

        [Test]
        public void HistoryStackTestEmptyHistory()
        {
            var stack = new HistoryStack(50);
            Assert.AreEqual(null, stack.PopHistory());
            Assert.AreEqual(null, stack.PopReverse());
        }

        [Test]
        public void HistoryStackTestTravel()
        {
            var stack = new HistoryStack(50);
            for (int i = 0; i < 80; i++)
            {
                stack.Push(new HistoryTestObject(i));
            }
            Assert.AreEqual(0, stack.ReverseCount);
            Assert.AreEqual(60, (int)(HistoryTestObject)stack.TravelBack(20));
            Assert.AreEqual(20, stack.ReverseCount);
            Assert.AreEqual(30, stack.HistoryCount);
            Assert.AreEqual(74, (int)(HistoryTestObject)stack.TravelReverse(15));
            Assert.AreEqual(5, stack.ReverseCount);
            Assert.AreEqual(45, stack.HistoryCount);
            Assert.AreEqual(79, (int)(HistoryTestObject)stack.TravelReverse(5));
            Assert.AreEqual(0, stack.ReverseCount);
            Assert.AreEqual(50, stack.HistoryCount);
            Assert.AreEqual(30, (int)(HistoryTestObject)stack.TravelBack(50));
            Assert.AreEqual(50, stack.ReverseCount);
            Assert.AreEqual(0, stack.HistoryCount);
        }

        [Test]
        public void HistoryStackTestExceptions()
        {
            var stack = new HistoryStack(50);
            for (int i = 0; i < 80; i++)
            {
                stack.Push(new HistoryTestObject(i));
            }
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { stack.TravelBack(-5); });
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { stack.TravelBack(0); });
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { stack.TravelReverse(-5); });
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { stack.TravelReverse(0); });
        }

        [Test]
        public void HistoryStackTestDropReverse()
        {
            var stack = new HistoryStack(50);
            for (int i = 0; i < 40; i++)
            {
                stack.Push(new HistoryTestObject(i));
            }
            stack.TravelBack(5);
            stack.Push(new HistoryTestObject(100));
            Assert.AreEqual(0, stack.ReverseCount);
            Assert.AreEqual(36, stack.HistoryCount);
            Assert.AreEqual(100, (int)(HistoryTestObject)stack.PeekHistory());
            Assert.AreEqual(null, stack.PeekReverse());
        }
    }
}
#endif
