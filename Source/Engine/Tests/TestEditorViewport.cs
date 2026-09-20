// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using FlaxEditor.Viewport;
using FlaxEngine;
using NUnit.Framework;

namespace FlaxEditor.Tests
{
    [TestFixture]
    public class TestEditorViewport
    {
        [Test]
        public void TestMouseAnchorSnapsToDevicePixels()
        {
            const float dpiScale = 1.25f;
            var logicalPosition = new Float2(984.7999f, 580.7999f);

            var snapped = EditorViewport.SnapMousePositionToDevicePixels(logicalPosition, dpiScale);
            var physicalPosition = snapped * dpiScale;

            Assert.AreEqual(1231.0f, physicalPosition.X, 0.0001f);
            Assert.AreEqual(726.0f, physicalPosition.Y, 0.0001f);
            Assert.AreEqual(snapped, EditorViewport.SnapMousePositionToDevicePixels(snapped, dpiScale));
        }
    }
}
#endif
