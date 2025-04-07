// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using FlaxEngine.GUI;
using NUnit.Framework;
using Assert = FlaxEngine.Assertions.Assert;

namespace FlaxEngine.Tests
{
    [TestFixture]
    public class TestContainerControl
    {
        public class MyControl : Control
        {
            public MyControl(float x, float y, float width, float height)
            : base(x, y, width, height)
            {
            }
        }

        public class MyContainerControl : ContainerControl
        {
            public MyContainerControl(float x, float y, float width, float height)
            : base(x, y, width, height)
            {
            }
        }

        [Test]
        public void TestChildren()
        {
            var c1 = new MyControl(0, 0, 10, 20);
            var c2 = new MyControl(0, 10, 10, 20);
            var c3 = new MyControl(10, 0, 10, 20);

            var cc1 = new MyContainerControl(0, 0, 100, 100);
            var cc2 = new MyContainerControl(10, 0, 100, 100);
            var cc3 = new MyContainerControl(10, 10, 100, 100);

            cc1.AddChild(cc2);
            cc2.AddChild(cc3);
            cc3.AddChild(c1);
            cc3.AddChild(c2);
            cc3.AddChild(c3);

            Assert.IsTrue(cc1.Parent == null);
            Assert.IsTrue(cc2.Parent == cc1);
            Assert.IsTrue(cc3.Parent == cc2);
            Assert.IsTrue(c1.Parent == cc3);
            Assert.IsTrue(c2.Parent == cc3);
            Assert.IsTrue(c3.Parent == cc3);

            Assert.AreEqual(cc1.GetChildAt(new Vector2(15, 5)), cc2);
            Assert.AreEqual(cc1.GetChildAtRecursive(new Vector2(35, 25)), c3);
        }
    }
}
#endif
