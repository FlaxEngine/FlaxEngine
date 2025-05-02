// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace FlaxEngine.Tests
{
    /// <summary>
    /// Tests for scripting.
    /// </summary>
    public class TestScripting
    {
        /// <summary>
        /// Tests all <see cref="LibraryImportAttribute"/> usages in the engine to verify all bindings are correct to work with P/Invoke.
        /// </summary>
        public static int TestLibraryImports()
        {
            var result = 0;
            var libraryName = "FlaxEngine";
            var library = NativeLibrary.Load(Interop.NativeInterop.libraryPaths[libraryName]);
            if (library == IntPtr.Zero)
                return -1;
            var types = typeof(FlaxEngine.Object).Assembly.GetTypes();
            foreach (var type in types)
            {
                var methods = type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
                foreach (var method in methods)
                {
                    var libraryImport = method.GetCustomAttribute<LibraryImportAttribute>();
                    if (libraryImport == null || libraryImport.LibraryName != libraryName || libraryImport.EntryPoint == null)
                        continue;
                    bool found = NativeLibrary.TryGetExport(library, libraryImport.EntryPoint, out var addr);
                    if (!found)
                    {
                        Debug.LogError("Missing library import: " + libraryImport.EntryPoint + " on " + type.FullName + "::" + method.Name);
                        result++;
                    }
                }
            }
            NativeLibrary.Free(library);
            return result;
        }
    }
}

namespace FlaxEngine
{
    partial struct TestStruct : System.IEquatable<TestStruct>
    {
        /// <summary></summary>
        public static bool operator ==(TestStruct left, TestStruct right)
        {
            return left.Equals(right);
        }

        /// <summary></summary>
        public static bool operator !=(TestStruct left, TestStruct right)
        {
            return !left.Equals(right);
        }

        /// <inheritdoc />
        public bool Equals(TestStruct other)
        {
            return Vector.Equals(other.Vector) && Equals(Object, other.Object);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is TestStruct other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return (Vector.GetHashCode() * 397) ^ (Object != null ? Object.GetHashCode() : 0);
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return $"Vector={Vector}, Object={Object?.ToString() ?? "null"}";
        }
    }

    /// <summary>
    /// Test class.
    /// </summary>
    public class TestClassManaged : TestClassNative
    {
        TestClassManaged()
        {
            // Test setting C++ values from C#
            SimpleField = 2;
            SimpleStruct = new TestStruct
            {
                Vector = Float3.UnitX,
                Object = this,
            };
            SimpleEvent += OnSimpleEvent;
        }

        /// <inheritdoc />
        public override int TestMethod(string str, ref TestStructPOD pod, ref TestStruct nonPod, TestStruct[] struct1, ref TestStruct[] struct2, out Object[] objects)
        {
            objects = new Object[3];
            if (struct1 == null || struct1.Length != 1)
                return -1;
            if (struct2 == null || struct2.Length != 1)
                return -2;
            if (pod.Vector != Float3.One)
                return -3;
            struct2 = new TestStruct[2]
            {
                struct1[0],
                SimpleStruct,
            };
            pod.Vector = Float3.Half;

            // Test C++ base method invocation
            return str.Length + base.TestMethod(str, ref pod, ref nonPod, struct1, ref struct2, out _);
        }

        /// <inheritdoc />
        public override int TestInterfaceMethod(string str)
        {
            // Test C++ base method invocation
            return str.Length + base.TestInterfaceMethod(str);
        }

        private void OnSimpleEvent(int arg1, Float3 arg2, string arg3, ref string arg4, ref TestStruct nonPod, TestStruct[] arg5, ref TestStruct[] arg6)
        {
            // Verify that C++ passed proper data to C# via event bindings
            if (arg1 == 1 &&
                arg2 == Float3.One &&
                arg3 == "1" &&
                arg4 == "2" &&
                nonPod.Object == null &&
                nonPod.Vector == Float3.One &&
                arg5 != null && arg5.Length == 1 && arg5[0] == SimpleStruct &&
                arg6 != null && arg6.Length == 1 && arg6[0] == SimpleStruct)
            {
                // Test passing data back from C# to C++
                SimpleField = 4;
                nonPod.Object = this;
                nonPod.Vector = Float3.UnitY;
                arg4 = "4";
                arg6 = new TestStruct[2]
                {
                    new TestStruct
                    {
                        Vector = Float3.Half,
                        Object = null,
                    },
                    SimpleStruct,
                };
            }
        }
    }

    /// <summary>
    /// Test interface in C#.
    /// </summary>
    public class TestInterfaceManaged : Object, ITestInterface
    {
        /// <inheritdoc />
        public int TestInterfaceMethod(string str)
        {
            return str.Length;
        }
    }
}
#endif
