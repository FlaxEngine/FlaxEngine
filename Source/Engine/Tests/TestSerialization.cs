// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using FlaxEngine.Json;
using NUnit.Framework;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests for <see cref="JsonSerializer"/>.
    /// </summary>
    [TestFixture]
    public class TestSerialization
    {
        public class ObjectOne
        {
            public float MyValue;
            public Vector2 MyVector;
            public int[] MyArray;
        }

        /// <summary>
        /// Test object ID serialization to and from string.
        /// </summary>
        [Test]
        public void TestConvertID()
        {
            var id1 = new Guid(11, 22, 33, 44, 55, 66, 77, 88, 99, 0, 1);
            string id1Str = JsonSerializer.GetStringID(id1);

            Assert.AreEqual("0000000b002100164d42372c01006358", id1Str);

            JsonSerializer.ParseID(id1Str, out var id1Tmp);
            Assert.AreEqual(id1, id1Tmp);
        }

        public static string FilterLineBreak(string str)
        {
            return str.Replace("\r", "");
        }

        /// <summary>
        /// Test object serialization to JSON.
        /// </summary>
        [Test]
        public void TestSerialize()
        {
            ObjectOne obj = new ObjectOne();

            Assert.AreEqual("{\n\t\"MyValue\": 0.0,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 0.0\n\t},\n\t\"MyArray\": null\n}", FilterLineBreak(JsonSerializer.Serialize(obj)));

            obj.MyValue = 1.2f;

            Assert.AreEqual("{\n\t\"MyValue\": 1.2,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 0.0\n\t},\n\t\"MyArray\": null\n}", FilterLineBreak(JsonSerializer.Serialize(obj)));

            obj.MyVector.Y = 2.0f;

            Assert.AreEqual("{\n\t\"MyValue\": 1.2,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 2.0\n\t},\n\t\"MyArray\": null\n}", FilterLineBreak(JsonSerializer.Serialize(obj)));

            obj.MyArray = new[]
            {
                1,
                2,
                3,
                4
            };

            Assert.AreEqual("{\n\t\"MyValue\": 1.2,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 2.0\n\t},\n\t\"MyArray\": [\n\t\t1,\n\t\t2,\n\t\t3,\n\t\t4\n\t]\n}", FilterLineBreak(JsonSerializer.Serialize(obj)));
        }

        /// <summary>
        /// Test object deserialization from JSON.
        /// </summary>
        [Test]
        public void TestDeserialize()
        {
            ObjectOne obj = new ObjectOne();

            JsonSerializer.Deserialize(obj, "{\n\t\"MyValue\": 0.0,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 0.0\n\t}\n}");

            Assert.AreEqual(0.0f, obj.MyValue);
            Assert.AreEqual(Vector2.Zero, obj.MyVector);
            Assert.IsNull(obj.MyArray);

            JsonSerializer.Deserialize(obj, "{\n\t\"MyValue\": 1.2,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 0.0\n\t}\n}");

            Assert.AreEqual(1.2f, obj.MyValue);
            Assert.AreEqual(Vector2.Zero, obj.MyVector);
            Assert.IsNull(obj.MyArray);

            JsonSerializer.Deserialize(obj, "{\n\t\"MyValue\": 1.2,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 2.0\n\t}\n}");

            Assert.AreEqual(1.2f, obj.MyValue);
            Assert.AreEqual(new Vector2(0.0f, 2.0f), obj.MyVector);
            Assert.IsNull(obj.MyArray);

            JsonSerializer.Deserialize(obj, "{\n\t\"MyValue\": 1.2,\n\t\"MyVector\": {\n\t\t\"X\": 0.0,\n\t\t\"Y\": 2.0\n\t},\n\t\"MyArray\": [\n\t\t1,\n\t\t2,\n\t\t3,\n\t\t4\n\t]\n}");

            Assert.AreEqual(1.2f, obj.MyValue);
            Assert.AreEqual(new Vector2(0.0f, 2.0f), obj.MyVector);
            Assert.IsNotNull(obj.MyArray);
            Assert.IsTrue(Utils.ArraysEqual(obj.MyArray, new[]
            {
                1,
                2,
                3,
                4
            }));
        }

        /// <summary>
        /// Test object diff serialization to JSON.
        /// </summary>
        [Test]
        public void TestSerializeDiff()
        {
            ObjectOne obj = new ObjectOne();
            ObjectOne other = new ObjectOne();

            Assert.AreEqual("{}", JsonSerializer.SerializeDiff(obj, other));

            obj.MyValue = 2.0f;

            Assert.AreEqual("{\n\t\"MyValue\": 2.0\n}", FilterLineBreak(JsonSerializer.SerializeDiff(obj, other)));

            obj.MyValue = 2.0f;
            other.MyValue = 2.0f;

            Assert.AreEqual("{}", JsonSerializer.SerializeDiff(obj, other));

            other.MyArray = new[] { 1 };

            Assert.AreEqual("{\n\t\"MyArray\": null\n}", FilterLineBreak(JsonSerializer.SerializeDiff(obj, other)));

            obj.MyArray = other.MyArray;

            Assert.AreEqual("{}", JsonSerializer.SerializeDiff(obj, other));

            obj.MyArray = new[] { 1 };

            Assert.AreEqual("{}", JsonSerializer.SerializeDiff(obj, other));

            obj.MyArray = new[] { 2 };

            Assert.AreEqual("{\n\t\"MyArray\": [\n\t\t2\n\t]\n}", FilterLineBreak(JsonSerializer.SerializeDiff(obj, other)));

            other.MyArray = null;

            Assert.AreEqual("{\n\t\"MyArray\": [\n\t\t2\n\t]\n}", FilterLineBreak(JsonSerializer.SerializeDiff(obj, other)));
        }
    }
}
#endif
