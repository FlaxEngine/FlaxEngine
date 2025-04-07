// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using System.Collections;
using FlaxEngine.Collections;
using NUnit.Framework;
using Assert = FlaxEngine.Assertions.Assert;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestsCircularBuffer
    {
        [Test]
        public void CircularBufferTestFrontOverwrite()
        {
            var buffer = new CircularBuffer<long>(3);
            for (int i = 0; i < 5; i++)
            {
                buffer.PushFront(i);
            }
            Assert.AreEqual(2, buffer[0]);
            Assert.AreEqual(3, buffer[1]);
            Assert.AreEqual(4, buffer[2]);
            Assert.AreEqual(4, buffer.Front());
            Assert.AreEqual(2, buffer.Back());

            Assert.AreEqual(buffer.Count, 3);
            Assert.AreEqual(buffer.Capacity, 3);
        }

        [Test]
        public void CircularBufferTestBackOverwrite()
        {
            var buffer = new CircularBuffer<long>(3);
            for (int i = 0; i < 5; i++)
            {
                buffer.PushBack(i);
            }
            Assert.AreEqual(4, buffer[0]);
            Assert.AreEqual(3, buffer[1]);
            Assert.AreEqual(2, buffer[2]);
            Assert.AreEqual(2, buffer.Front());
            Assert.AreEqual(4, buffer.Back());

            Assert.AreEqual(buffer.Count, 3);
            Assert.AreEqual(buffer.Capacity, 3);
        }

        [Test]
        public void CircularBufferTestMixedOverwrite()
        {
            var buffer = new CircularBuffer<long>(3);
            buffer.PushFront(0);

            buffer.PushFront(1);
            buffer.PushBack(-1);
            Assert.AreEqual(1, buffer.Front());
            Assert.AreEqual(-1, buffer.Back());
            buffer.PushFront(2);
            Assert.AreEqual(2, buffer.Front());
            Assert.AreEqual(0, buffer.Back());
            buffer.PushBack(-2);
            Assert.AreEqual(1, buffer.Front());
            buffer.PushFront(3);
            Assert.AreEqual(0, buffer.Back());
            buffer.PushBack(-3);

            Assert.AreEqual(-3, buffer[0]);
            Assert.AreEqual(0, buffer[1]);
            Assert.AreEqual(1, buffer[2]);
            Assert.AreEqual(1, buffer.Front());
            Assert.AreEqual(-3, buffer.Back());

            Assert.AreEqual(buffer.Count, 3);
            Assert.AreEqual(buffer.Capacity, 3);
        }

        [Test]
        public void CircularBufferTestFrontUnderwrite()
        {
            var buffer = new CircularBuffer<long>(5);
            for (int i = 0; i < 4; i++)
            {
                buffer.PushFront(i);
            }
            buffer.PopFront();
            buffer.PushFront(4);
            Assert.AreEqual(4, buffer.Front());
            Assert.AreEqual(0, buffer.Back());
            buffer.PopFront();
            buffer.PushFront(5);
            Assert.AreEqual(0, buffer[0]);
            Assert.AreEqual(1, buffer[1]);
            Assert.AreEqual(2, buffer[2]);
            Assert.AreEqual(5, buffer[3]);

            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () =>
            {
                var test = buffer[4];
            });

            Assert.AreEqual(5, buffer.Front());
            Assert.AreEqual(0, buffer.Back());

            Assert.AreEqual(buffer.Count, 4);
            Assert.AreEqual(buffer.Capacity, 5);
        }

        [Test]
        public void CircularBufferTestBackUnderwrite()
        {
            var buffer = new CircularBuffer<long>(5);
            for (int i = 0; i < 4; i++)
            {
                buffer.PushBack(i);
            }
            buffer.PopBack();
            buffer.PushBack(4);
            Assert.AreEqual(4, buffer.Back());
            Assert.AreEqual(0, buffer.Front());
            buffer.PopBack();
            buffer.PushBack(5);
            Assert.AreEqual(5, buffer[0]);
            Assert.AreEqual(2, buffer[1]);
            Assert.AreEqual(1, buffer[2]);
            Assert.AreEqual(0, buffer[3]);

            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () =>
            {
                var test = buffer[4];
            });

            Assert.AreEqual(0, buffer.Front());
            Assert.AreEqual(5, buffer.Back());

            Assert.AreEqual(buffer.Count, 4);
            Assert.AreEqual(buffer.Capacity, 5);
        }

        [Test]
        public void CircularBufferTestMixedUnderwrite()
        {
            var buffer = new CircularBuffer<long>(5);
            buffer.PushFront(0);

            buffer.PushFront(1);
            buffer.PushBack(-1);
            Assert.AreEqual(1, buffer.Front());
            Assert.AreEqual(-1, buffer.Back());

            buffer.PushFront(2);
            Assert.AreEqual(2, buffer.Front());
            Assert.AreEqual(-1, buffer.Back());

            buffer.PopBack();
            Assert.AreEqual(0, buffer.Back());
            Assert.AreEqual(2, buffer.Front());

            buffer.PushBack(-2);
            Assert.AreEqual(2, buffer.Front());
            Assert.AreEqual(-2, buffer.Back());

            Assert.AreEqual(-2, buffer[0]);
            Assert.AreEqual(0, buffer[1]);
            Assert.AreEqual(1, buffer[2]);
            Assert.AreEqual(2, buffer[3]);

            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () =>
            {
                var test = buffer[4];
            });

            Assert.AreEqual(buffer.Count, 4);
            Assert.AreEqual(buffer.Capacity, 5);
        }

        [Test]
        public void CircularBufferTestEnumerationFrontWhenOverflown()
        {
            var buffer = new CircularBuffer<long>(5);
            int i;
            for (i = 0; i < 11; i++)
            {
                buffer.PushFront(i);
            }
            for (i = 0; i < buffer.Capacity; i++)
            {
                Assert.AreEqual(buffer.Count + i + 1, buffer[i]);
            }
            i = 6;
            foreach (var item in buffer)
            {
                Assert.AreEqual(i++, item);
            }
            i = 6;
            var e = ((IEnumerable)buffer).GetEnumerator();
            while (e.MoveNext())
            {
                Assert.AreEqual(i++, (long)e.Current);
            }
        }

        [Test]
        public void CircularBufferTestEnumerationBackWhenOverflown()
        {
            var buffer = new CircularBuffer<long>(5);
            int i;
            for (i = 0; i < 11; i++)
            {
                buffer.PushBack(i);
            }
            for (i = 0; i < buffer.Capacity; i++)
            {
                Assert.AreEqual(10 - i, buffer[i]);
            }
            i = 10;
            foreach (var item in buffer)
            {
                Assert.AreEqual(i--, item);
            }
            i = 10;
            var e = ((IEnumerable)buffer).GetEnumerator();
            while (e.MoveNext())
            {
                Assert.AreEqual(i--, (long)e.Current);
            }
        }

        [Test]
        public void CircularBufferTestEnumerationWhenPartiallyFull()
        {
            var buffer = new CircularBuffer<long>(3);
            buffer.PushFront(1);
            buffer.PushBack(0);
            var i = 0;
            foreach (var item in buffer)
            {
                Assert.AreEqual(i++, item);
            }
            Assert.AreEqual(i, 2);
            Assert.AreEqual(buffer.Count, 2);
            Assert.AreEqual(buffer.Capacity, 3);
        }

        [Test]
        public void CircularBufferTestEnumerationWhenPartiallyFullLarge()
        {
            var buffer = new CircularBuffer<long>(20000);
            int i = 0;
            for (i = 0; i < 10000; i++)
            {
                buffer.PushFront(i);
            }
            i = 0;
            foreach (var value in buffer)
            {
                Assert.AreEqual(i++, value);
            }
            Assert.AreEqual(i, 10000);

            Assert.AreEqual(buffer.Count, 10000);
            Assert.AreEqual(buffer.Capacity, 20000);
        }

        [Test]
        public void CircularBufferTestEnumerationWhenEmpty()
        {
            var buffer = new CircularBuffer<long>(3);
            foreach (var value in buffer)
            {
                Assert.Fail("Unexpected Value: " + value);
            }
        }

        [Test]
        public void CircularBufferTestCopyToArray()
        {
            var buffer = new CircularBuffer<long>(3);
            for (int i = 0; i < 5; i++)
            {
                buffer.PushFront(i);
            }
            var testArray = buffer.ToArray();
            Assert.AreEqual(2, testArray[0]);
            Assert.AreEqual(3, testArray[1]);
            Assert.AreEqual(4, testArray[2]);

            testArray = new long[3];
            buffer.CopyTo(testArray, 0);
            Assert.AreEqual(2, testArray[0]);
            Assert.AreEqual(3, testArray[1]);
            Assert.AreEqual(4, testArray[2]);

            testArray = new long[5];
            buffer.CopyTo(testArray, 2);
            Assert.AreEqual(2, testArray[2]);
        }

        [Test]
        public void CircularBufferTestExceptions()
        {
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { new CircularBuffer<long>(0); });
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { new CircularBuffer<long>(-5); });
            var buffer = new CircularBuffer<long>(1);
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer.Front(); });
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer.Back(); });
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () =>
            {
                var test = buffer[-1];
            });
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () =>
            {
                var test = buffer[1];
            });
            Assert.ExceptionExpected(typeof(ArgumentOutOfRangeException), () => { buffer[-1] = 0; });
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer[1] = 0; });
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer.PopBack(); });
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer.PopFront(); });
        }

        [Test]
        public void CircularBufferTestForceSet()
        {
            var buffer = new CircularBuffer<long>(15);
            for (int i = 0; i < 5; i++)
            {
                buffer.PushFront(i);
            }
            for (int i = 0; i < buffer.Count; i++)
            {
                buffer[i] = 1;
            }
            foreach (var item in buffer)
            {
                Assert.AreEqual(1, item);
            }
            for (int i = 0; i < buffer.Count; i++)
            {
                buffer[i] = i;
            }
            int j = 0;
            foreach (var item in buffer)
            {
                Assert.AreEqual(j++, item);
            }
        }

        [Test]
        public void CircularBufferTestFrontAndBack()
        {
            var rand = new Random();
            var buffer = new CircularBuffer<long>(3);
            for (int i = 0; i < 25; i++)
            {
                var cur = rand.Next();
                buffer.PushFront(cur);
                Assert.AreEqual(cur, buffer.Front());
                buffer.PopFront();
            }
            for (int i = 0; i < 25; i++)
            {
                var cur = rand.Next();
                buffer.PushBack(cur);
                Assert.AreEqual(cur, buffer.Back());
                buffer.PopBack();
            }
        }

        [Test]
        public void CircularBufferTestConstructors()
        {
            var array = new long[5];
            for (int i = 0; i < 5; i++)
            {
                array[i] = i + 10;
            }
            var buffer = new CircularBuffer<long>(15, array, 5);
            Assert.AreEqual(10, buffer.Back());
            Assert.AreEqual(14, buffer.Front());
            Assert.AreEqual(5, buffer.Count);
            Assert.AreEqual(15, buffer.Capacity);
            Assert.AreEqual(false, buffer.IsEmpty);
        }

        [Test]
        public void CircularBufferTestClear()
        {
            var buffer = new CircularBuffer<long>(15);
            for (int i = 0; i < 5; i++)
            {
                buffer.PushFront(i);
            }
            buffer.Clear();
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer.PopBack(); });
            Assert.ExceptionExpected(typeof(IndexOutOfRangeException), () => { buffer.PopFront(); });
            Assert.AreEqual(0, buffer.Count);
            Assert.AreEqual(15, buffer.Capacity);
        }

        [Test]
        public void CircularBufferTestMultipleIterations()
        {
            var buffer = new CircularBuffer<long>(50);
            for (int i = 0; i < 80; i++)
            {
                buffer.PushFront(i);
            }
            for (int i = 0; i < 50; i++)
            {
                Assert.AreEqual(79 - i, buffer.PopFront());
            }
            for (int i = 0; i < 20; i++)
            {
                buffer.PushFront(i);
            }
            for (int i = 0; i < 20; i++)
            {
                Assert.AreEqual(i, buffer[i]);
            }
        }

        [Test]
        public void CircularBufferTestEvents()
        {
            //Front
            var buffer = new CircularBuffer<long>(5);
            var overflown = 0;
            var added = 0;
            var removed = 7;

            var overflownHandler = new CircularBuffer<long>.ItemOverflownEventHandler((sender, args) =>
            {
                Assert.AreEqual(overflown++, args.Item);
                Assert.AreEqual(false, args.WasFrontItem);
            });
            buffer.OnItemOverflown += overflownHandler;
            var addHandler = new CircularBuffer<long>.ItemAddedEventHandler((sender, args) =>
            {
                Assert.AreEqual(added++, args.Item);
                Assert.AreEqual(buffer.Count - 1, args.Index);
            });
            buffer.OnItemAdded += addHandler;
            var removeHandler = new CircularBuffer<long>.ItemRemovedEventHandler((sender, args) =>
            {
                Assert.AreEqual(removed--, args.Item);
                Assert.AreEqual(true, args.WasFrontItem);
            });
            buffer.OnItemRemoved += removeHandler;

            for (int i = 0; i < 8; i++)
            {
                buffer.PushFront(i);
            }
            for (int i = 0; i < 5; i++)
            {
                buffer.PopFront();
            }
            buffer.OnItemOverflown -= overflownHandler;
            buffer.OnItemAdded -= addHandler;
            buffer.OnItemRemoved -= removeHandler;

            //Back
            buffer.Clear();
            overflown = 0;
            added = 0;
            removed = 7;

            overflownHandler = (sender, args) =>
            {
                Assert.AreEqual(overflown++, args.Item);
                Assert.AreEqual(true, args.WasFrontItem);
            };
            buffer.OnItemOverflown += overflownHandler;
            addHandler = (sender, args) =>
            {
                Assert.AreEqual(added++, args.Item);
                Assert.AreEqual(0, args.Index);
            };
            buffer.OnItemAdded += addHandler;
            removeHandler = (sender, args) =>
            {
                Assert.AreEqual(removed--, args.Item);
                Assert.AreEqual(false, args.WasFrontItem);
            };
            buffer.OnItemRemoved += removeHandler;

            for (int i = 0; i < 8; i++)
            {
                buffer.PushBack(i);
            }
            for (int i = 0; i < 5; i++)
            {
                buffer.PopBack();
            }
            buffer.OnItemOverflown -= overflownHandler;
            buffer.OnItemAdded -= addHandler;
            buffer.OnItemRemoved -= removeHandler;
        }

        [Test]
        public void CircularBufferTestJsonConversion()
        {
            var buffer = new CircularBuffer<long>(3);
            buffer.PushFront(0);

            buffer.PushFront(1);
            buffer.PushBack(-1);
            buffer.PushFront(2);
            buffer.PushBack(-2);
            buffer.PushFront(3);
            buffer.PushBack(-3);

            var json = Json.JsonSerializer.Serialize(buffer);
            var deserializedBuffer = Json.JsonSerializer.Deserialize<CircularBuffer<long>>(json);

            Assert.AreEqual(-3, deserializedBuffer[0]);
            Assert.AreEqual(0, deserializedBuffer[1]);
            Assert.AreEqual(1, deserializedBuffer[2]);
            Assert.AreEqual(1, deserializedBuffer.Front());
            Assert.AreEqual(-3, deserializedBuffer.Back());

            Assert.AreEqual(deserializedBuffer.Count, 3);
            Assert.AreEqual(deserializedBuffer.Capacity, 3);
        }
    }
}
#endif
