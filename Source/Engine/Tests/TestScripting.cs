// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using System.Collections.Generic;
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

        /// <summary>
        /// Tests that one assembly can import several native libraries through the engine's import resolver.
        /// </summary>
        public static int TestNativeLibrariesPerAssembly()
        {
            // Two system libraries under test names: the resolver loads each name once, so the names must be new to it
            string[] systemLibraries;
            if (OperatingSystem.IsWindows())
                systemLibraries = new[] { "kernel32.dll", "user32.dll" };
            else if (OperatingSystem.IsMacOS())
                systemLibraries = new[] { "/usr/lib/libSystem.B.dylib", "/usr/lib/libobjc.A.dylib" };
            else
                systemLibraries = new[] { "libc.so.6", "libm.so.6" };
            var names = new[] { "TestNativeLibrary1", "TestNativeLibrary2" };

            // An assembly that does not import native libraries through the resolver
            var assembly = typeof(System.Linq.Enumerable).Assembly;
            var interop = typeof(Interop.NativeInterop);
            var resolver = interop.GetMethod("NativeLibraryImportResolver", BindingFlags.NonPublic | BindingFlags.Static);
            var loadedLibraries = (Dictionary<string, IntPtr>)interop.GetField("loadedNativeLibraries", BindingFlags.NonPublic | BindingFlags.Static)?.GetValue(null);
            var ownedLibraries = (System.Collections.IDictionary)interop.GetField("assemblyOwnedNativeLibraries", BindingFlags.NonPublic | BindingFlags.Static)?.GetValue(null);
            if (resolver == null || loadedLibraries == null || ownedLibraries == null)
                return -1;

            var result = 0;
            try
            {
                for (int i = 0; i < names.Length; i++)
                {
                    Interop.NativeInterop.libraryPaths[names[i]] = systemLibraries[i];
                    var library = (IntPtr)resolver.Invoke(null, new object[] { names[i], assembly, null });
                    if (library == IntPtr.Zero)
                        result++;
                }
            }
            catch (TargetInvocationException ex)
            {
                Debug.LogError("Native library import failed: " + ex.InnerException?.Message);
                result += 10;
            }
            finally
            {
                // Remove the test libraries from the resolver (also after a failure)
                foreach (var name in names)
                {
                    Interop.NativeInterop.libraryPaths.Remove(name);
                    if (loadedLibraries.Remove(name, out var library))
                        NativeLibrary.Free(library);
                }
                ownedLibraries.Remove(assembly);
            }
            return result;
        }

        /// <summary>
        /// Tests that a native library imported by two assemblies stays loaded until both assemblies release it.
        /// </summary>
        public static int TestNativeLibrarySharedByAssemblies()
        {
            // A system library under a test name, imported by two assemblies that do not import native libraries through the resolver
            string systemLibrary = OperatingSystem.IsWindows() ? "kernel32.dll" : OperatingSystem.IsMacOS() ? "/usr/lib/libSystem.B.dylib" : "libc.so.6";
            const string name = "TestNativeLibrary3";
            var assemblyA = typeof(System.Linq.Enumerable).Assembly;
            var assemblyB = typeof(System.Text.RegularExpressions.Regex).Assembly;
            var interop = typeof(Interop.NativeInterop);
            var resolver = interop.GetMethod("NativeLibraryImportResolver", BindingFlags.NonPublic | BindingFlags.Static);
            var release = interop.GetMethod("ReleaseNativeLibraries", BindingFlags.NonPublic | BindingFlags.Static);
            var loadedLibraries = (Dictionary<string, IntPtr>)interop.GetField("loadedNativeLibraries", BindingFlags.NonPublic | BindingFlags.Static)?.GetValue(null);
            var ownedLibraries = (System.Collections.IDictionary)interop.GetField("assemblyOwnedNativeLibraries", BindingFlags.NonPublic | BindingFlags.Static)?.GetValue(null);
            if (resolver == null || release == null || loadedLibraries == null || ownedLibraries == null)
                return -1;

            var result = 0;
            try
            {
                Interop.NativeInterop.libraryPaths[name] = systemLibrary;
                var libraryA = (IntPtr)resolver.Invoke(null, new object[] { name, assemblyA, null });
                var libraryB = (IntPtr)resolver.Invoke(null, new object[] { name, assemblyB, null });
                if (libraryA == IntPtr.Zero || libraryB != libraryA)
                    result += 1;

                // The first assembly releases its libraries: the other one still uses the library
                release.Invoke(null, new object[] { assemblyA });
                if (!loadedLibraries.ContainsKey(name))
                    result += 2;

                // The last assembly releases its libraries: the library is freed
                release.Invoke(null, new object[] { assemblyB });
                if (loadedLibraries.ContainsKey(name))
                    result += 4;
            }
            catch (TargetInvocationException ex)
            {
                Debug.LogError("Native library import failed: " + ex.InnerException?.Message);
                result += 10;
            }
            finally
            {
                // Remove the test library from the resolver (also after a failure)
                Interop.NativeInterop.libraryPaths.Remove(name);
                if (loadedLibraries.Remove(name, out var library))
                    NativeLibrary.Free(library);
                ownedLibraries.Remove(assemblyA);
                ownedLibraries.Remove(assemblyB);
            }
            return result;
        }

        /// <summary>
        /// Tests <see cref="ScriptingObjectInterfaceReference{T}"/> usage with marshalling.
        /// </summary>
        public static int TestInterfaceReference()
        {
            var native = new TestClassNative();
            native.InterfaceRef = native;
            var returned = native.InterfaceRef;
            if (returned != native)
                return 1;
            returned = native.TestPassInterface(native);
            if (returned != native)
                return 2;
            returned = native.TestPassInterfaceArray(new ScriptingObjectInterfaceReference<ITestInterface>[1] { native })[0];
            if (returned != native)
                return 3;
            var dic = new Dictionary<string, ScriptingObjectInterfaceReference<ITestInterface>>();
            dic.Add("key", native);
            returned = native.TestPassInterfaceDictionary(dic)["key"];
            if (returned != native)
                return 4;
            var res = returned.Interface.TestInterfaceMethod("123");
            if (res != 3)
                return 5;
            return 0;
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
