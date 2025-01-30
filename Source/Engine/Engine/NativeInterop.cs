// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_NETCORE

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
#if !USE_AOT
using System.Linq.Expressions;
#endif
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Runtime.CompilerServices;
using FlaxEngine.Assertions;
using System.Collections.Concurrent;
using System.IO;
using System.Text;
using System.Threading;

namespace FlaxEngine.Interop
{
    /// <summary>
    /// Provides a Mono-like API for native code to access managed runtime.
    /// </summary>
    [HideInEditor]
    public static unsafe partial class NativeInterop
    {
        internal static Dictionary<string, string> AssemblyLocations = new();

        private static bool firstAssemblyLoaded = false;

        private static IntPtr boolTruePtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc((int)1, GCHandleType.Pinned));
        private static IntPtr boolFalsePtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc((int)0, GCHandleType.Pinned));

        private static List<ManagedHandle> methodHandles = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegates = new();
        private static Dictionary<Type, (TypeHolder typeHolder, ManagedHandle handle)> managedTypes = new(new TypeComparer());
        private static List<ManagedHandle> fieldHandleCache = new();
        private static List<ManagedHandle> propertyHandleCache = new();
#if FLAX_EDITOR
        private static List<ManagedHandle> methodHandlesCollectible = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegatesCollectible = new();
        private static Dictionary<Type, (TypeHolder typeHolder, ManagedHandle handle)> managedTypesCollectible = new(new TypeComparer());
        private static List<ManagedHandle> fieldHandleCacheCollectible = new();
        private static List<ManagedHandle> propertyHandleCacheCollectible = new();
#endif
        private static Dictionary<object, ManagedHandle> classAttributesCacheCollectible = new();
        private static Dictionary<Assembly, ManagedHandle> assemblyHandles = new();
        private static ConcurrentDictionary<Type, int> _typeSizeCache = new();

        private static Dictionary<string, IntPtr> loadedNativeLibraries = new();
        internal static Dictionary<string, string> libraryPaths = new();
        private static Dictionary<Assembly, string> assemblyOwnedNativeLibraries = new();
        internal static AssemblyLoadContext scriptingAssemblyLoadContext;

        private delegate TInternal ToNativeDelegate<T, TInternal>(T value);
        private delegate T ToManagedDelegate<T, TInternal>(TInternal value);

        [System.Diagnostics.DebuggerStepThrough]
        private static IntPtr NativeLibraryImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? dllImportSearchPath)
        {
            if (!loadedNativeLibraries.TryGetValue(libraryName, out IntPtr nativeLibrary))
            {
                if (!libraryPaths.TryGetValue(libraryName, out var nativeLibraryPath))
                    nativeLibraryPath = libraryName;

                nativeLibrary = NativeLibrary.Load(nativeLibraryPath, assembly, dllImportSearchPath);
                loadedNativeLibraries.Add(libraryName, nativeLibrary);
                assemblyOwnedNativeLibraries.Add(assembly, libraryName);
            }
            return nativeLibrary;
        }

        private static void InitScriptingAssemblyLoadContext()
        {
#if FLAX_EDITOR
            var isCollectible = true;
#else
            var isCollectible = false;
#endif
            scriptingAssemblyLoadContext = new AssemblyLoadContext("Flax", isCollectible);
#if FLAX_EDITOR
            scriptingAssemblyLoadContext.Resolving += OnScriptingAssemblyLoadContextResolving;
#endif
        }

        [UnmanagedCallersOnly]
        internal static unsafe void Init()
        {
            NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), NativeLibraryImportResolver);

            // Change default culture to match with Mono runtime default culture
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture;
            CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.InvariantCulture;
            System.Threading.Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
            System.Threading.Thread.CurrentThread.CurrentUICulture = CultureInfo.InvariantCulture;

            InitScriptingAssemblyLoadContext();
            DelegateHelpers.InitMethods();
        }

#if FLAX_EDITOR
        private static Assembly OnScriptingAssemblyLoadContextResolving(AssemblyLoadContext assemblyLoadContext, AssemblyName assemblyName)
        {
            // FIXME: There should be a better way to resolve the path to EditorTargetPath where the dependencies are stored
            foreach (string libraryPath in libraryPaths.Values)
            {
                string editorTargetPath = Path.GetDirectoryName(libraryPath);

                var assemblyPath = Path.Combine(editorTargetPath, assemblyName.Name + ".dll");
                if (File.Exists(assemblyPath))
                    return assemblyLoadContext.LoadFromAssemblyPath(assemblyPath);
            }
            return null;
        }
#endif

        [UnmanagedCallersOnly]
        internal static unsafe void Exit()
        {
        }

#if !USE_AOT
        [UnsafeAccessor(UnsafeAccessorKind.Field, Name = "__unmanagedPtr")]
        extern static ref IntPtr GetUnmanagedPtrFieldReference(Object obj);

        [UnsafeAccessor(UnsafeAccessorKind.Field, Name = "__internalId")]
        extern static ref Guid GetInternalIdFieldReference(Object obj);

        // Cache offsets to frequently accessed fields of FlaxEngine.Object
        private static int unmanagedPtrFieldOffset = IntPtr.Size + (Unsafe.Read<int>((typeof(FlaxEngine.Object).GetField("__unmanagedPtr", BindingFlags.Instance | BindingFlags.NonPublic).FieldHandle.Value + 4 + IntPtr.Size).ToPointer()) & 0xFFFFFF);
        private static int internalIdFieldOffset = IntPtr.Size + (Unsafe.Read<int>((typeof(FlaxEngine.Object).GetField("__internalId", BindingFlags.Instance | BindingFlags.NonPublic).FieldHandle.Value + 4 + IntPtr.Size).ToPointer()) & 0xFFFFFF);

        [UnmanagedCallersOnly]
        internal static void ScriptingObjectSetInternalValues(ManagedHandle objectHandle, IntPtr unmanagedPtr, IntPtr idPtr)
        {
            object obj = objectHandle.Target;
            if (obj is not Object scriptingObject)
                return;

            GetUnmanagedPtrFieldReference(scriptingObject) = unmanagedPtr;
            if (idPtr != IntPtr.Zero)
                GetInternalIdFieldReference(scriptingObject) = Unsafe.AsRef<Guid>(idPtr.ToPointer());
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle ScriptingObjectCreate(ManagedHandle typeHandle, IntPtr unmanagedPtr, IntPtr idPtr)
        {
            TypeHolder typeHolder = Unsafe.As<TypeHolder>(typeHandle.Target);
            try
            {
                object obj = typeHolder.CreateScriptingObject(unmanagedPtr, idPtr);
                return ManagedHandle.Alloc(obj);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            return new ManagedHandle();
        }
#endif

        internal static void* NativeAlloc(int byteCount)
        {
            return NativeMemory.AlignedAlloc((UIntPtr)byteCount, 16);
        }

        internal static void* NativeAlloc(int elementCount, int elementSize)
        {
            return NativeMemory.AlignedAlloc((UIntPtr)(elementCount * elementSize), 16);
        }

        internal static IntPtr NativeAllocStringAnsi(string str)
        {
            if (str is null)
                return IntPtr.Zero;

            int length = str.Length + 1;
            void* ptr = NativeMemory.AlignedAlloc((UIntPtr)length, 16);
            Span<byte> byteSpan = new Span<byte>(ptr, length);
            Encoding.ASCII.GetBytes(str, byteSpan);
            byteSpan[length - 1] = 0;

            return (IntPtr)ptr;
        }

        internal static void NativeFree(void* ptr)
        {
            NativeMemory.AlignedFree(ptr);
        }

        /// <summary>
        /// Converts a delegate into a function pointer that is callable from unmanaged code via <see cref="Marshal.GetFunctionPointerForDelegate{TDelegate}"/> but cached <paramref name="d"/> delegate to prevent collecting it by GC.
        /// </summary>
        /// <typeparam name="TDelegate">The type of delegate to convert.</typeparam>
        /// <param name="d">The delegate to be passed to unmanaged code.</param>
        /// <returns>A value that can be passed to unmanaged code, which, in turn, can use it to call the underlying managed delegate.</returns>
        public static IntPtr GetFunctionPointerForDelegate<TDelegate>(TDelegate d) where TDelegate : notnull
        {
            // Example use-case: C# script runs actions via JobSystem.Dispatch which causes crash due to GC collecting Delegate object
            ManagedHandle.Alloc(d, GCHandleType.Weak);

            return Marshal.GetFunctionPointerForDelegate<TDelegate>(d);
        }

        /// <summary>
        /// Converts array of GC Handles from native runtime to managed array.
        /// </summary>
        /// <typeparam name="T">Array element type.</typeparam>
        /// <param name="ptrArray">Input array.</param>
        /// <returns>Output array.</returns>
        public static T[] GCHandleArrayToManagedArray<T>(ManagedArray ptrArray) where T : class
        {
            Span<IntPtr> span = ptrArray.ToSpan<IntPtr>();
            T[] managedArray = new T[ptrArray.Length];
            for (int i = 0; i < managedArray.Length; i++)
                managedArray[i] = span[i] != IntPtr.Zero ? (T)ManagedHandle.FromIntPtr(span[i]).Target : default;
            return managedArray;
        }

        /// <summary>
        /// Converts managed array wrapper into array of GC Handles for native runtime.
        /// </summary>
        /// <param name="array">Input array.</param>
        /// <returns>Output array.</returns>
        public static IntPtr[] ManagedArrayToGCHandleArray(Array array)
        {
            if (array.Length == 0)
                return Array.Empty<IntPtr>();
            IntPtr[] pointerArray = new IntPtr[array.Length];
            for (int i = 0; i < pointerArray.Length; i++)
            {
                var obj = array.GetValue(i);
                if (obj != null)
                    pointerArray.SetValue(ManagedHandle.ToIntPtr(obj), i);
            }
            return pointerArray;
        }

        /// <summary>
        /// Converts managed array wrapper into array of GC Handles for native runtime.
        /// </summary>
        /// <param name="array">Input array.</param>
        /// <returns>Output array.</returns>
        public static ManagedArray ManagedArrayToGCHandleWrappedArray(Array array)
        {
            IntPtr[] pointerArray = ManagedArrayToGCHandleArray(array);
            return ManagedArray.WrapNewArray(pointerArray, array.GetType());
        }

        /// <summary>
        /// Converts array with a custom converter function for each element.
        /// </summary>
        /// <typeparam name="TSrc">Input data type.</typeparam>
        /// <typeparam name="TDst">Output data type.</typeparam>
        /// <param name="src">The input array.</param>
        /// <param name="convertFunc">Converter callback.</param>
        /// <returns>The output array.</returns>
        public static TDst[] ConvertArray<TSrc, TDst>(this Span<TSrc> src, Func<TSrc, TDst> convertFunc)
        {
            TDst[] dst = new TDst[src.Length];
            for (int i = 0; i < src.Length; i++)
                dst[i] = convertFunc(src[i]);
            return dst;
        }

        /// <summary>
        /// Converts array with a custom converter function for each element.
        /// </summary>
        /// <typeparam name="TSrc">Input data type.</typeparam>
        /// <typeparam name="TDst">Output data type.</typeparam>
        /// <param name="src">The input array.</param>
        /// <param name="convertFunc">Converter callback.</param>
        /// <returns>The output array.</returns>
        public static TDst[] ConvertArray<TSrc, TDst>(this TSrc[] src, Func<TSrc, TDst> convertFunc)
        {
            TDst[] dst = new TDst[src.Length];
            for (int i = 0; i < src.Length; i++)
                dst[i] = convertFunc(src[i]);
            return dst;
        }

        /// <summary>Find <paramref name="assemblyName"/> among the scripting assemblies.</summary>
        /// <param name="assemblyName">The name to find</param>
        /// <param name="allowPartial">If true, partial names should be allowed to be resolved.</param>
        /// <returns>The resolved assembly, or null if none could be found.</returns>
        internal static Assembly ResolveScriptingAssemblyByName(AssemblyName assemblyName, bool allowPartial = false)
        {
            var lc = scriptingAssemblyLoadContext;

            if (lc is null)
                return null;

            foreach (Assembly assembly in lc.Assemblies)
            {
                var curName = assembly.GetName();

                if (curName == assemblyName)
                    return assembly;
            }

            if (allowPartial) // Check partial names if full name isn't found
            {
                string partialName = assemblyName.Name;

                foreach (Assembly assembly in lc.Assemblies)
                {
                    var curName = assembly.GetName();

                    if (curName.Name == partialName)
                        return assembly;
                }
            }
            return null;
        }

        internal static bool IsBlittable(Type type)
        {
            if (type.IsPrimitive || type.IsEnum)
                return true;
            if (type.IsArray && IsBlittable(type.GetElementType()))
                return true;
            if (type.IsPointer && type.HasElementType && type.GetElementType().IsPrimitive)
                return true;
            if (type.IsClass)
                return false;
            if (type.IsValueType)
            {
                var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
                foreach (FieldInfo field in fields)
                {
                    if (!IsBlittable(field.FieldType))
                        return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Returns blittable internal type for given type.
        /// </summary>
        internal static Type GetInternalType(Type type)
        {
            Type marshallerType = type.GetCustomAttribute<System.Runtime.InteropServices.Marshalling.NativeMarshallingAttribute>()?.NativeType;
            if (marshallerType == null)
                return null;

            Type internalType = marshallerType.GetNestedType($"{type.Name}Internal");
            if (internalType == null)
                return null;

            return internalType;
        }

        internal class ReferenceTypePlaceholder { }
        internal struct ValueTypePlaceholder { }

        internal delegate object MarshalToManagedDelegate(IntPtr nativePtr, bool byRef);
        internal delegate void MarshalToNativeDelegate(object managedObject, IntPtr nativePtr);
        internal delegate void MarshalToNativeFieldDelegate(FieldInfo field, int fieldOffset, object fieldOwner, IntPtr nativePtr, out int fieldSize);

        internal static ConcurrentDictionary<Type, MarshalToManagedDelegate> toManagedMarshallers = new ConcurrentDictionary<Type, MarshalToManagedDelegate>(1, 3);
        internal static ConcurrentDictionary<Type, MarshalToNativeDelegate> toNativeMarshallers = new ConcurrentDictionary<Type, MarshalToNativeDelegate>(1, 3);
        internal static ConcurrentDictionary<Type, MarshalToNativeFieldDelegate> toNativeFieldMarshallers = new ConcurrentDictionary<Type, MarshalToNativeFieldDelegate>(1, 3);

        internal static object MarshalToManaged(IntPtr nativePtr, Type type)
        {
            static MarshalToManagedDelegate Factory(Type type)
            {
                Type marshalType = type;
                if (marshalType.IsByRef)
                    marshalType = marshalType.GetElementType();
                else if (marshalType.IsPointer)
                    marshalType = typeof(IntPtr);

                MethodInfo method = typeof(MarshalHelper<>).MakeGenericType(marshalType).GetMethod(nameof(MarshalHelper<ReferenceTypePlaceholder>.ToManagedWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                return method.CreateDelegate<MarshalToManagedDelegate>();
            }

            if (!toManagedMarshallers.TryGetValue(type, out var deleg))
                deleg = toManagedMarshallers.GetOrAdd(type, Factory);
            return deleg(nativePtr, type.IsByRef);
        }

        internal static void MarshalToNative(object managedObject, IntPtr nativePtr, Type type)
        {
            static MarshalToNativeDelegate Factory(Type type)
            {
                MethodInfo method;
                if (type.IsValueType)
                    method = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNativeWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    method = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                return method.CreateDelegate<MarshalToNativeDelegate>();
            }

            if (!toNativeMarshallers.TryGetValue(type, out var deleg))
                deleg = toNativeMarshallers.GetOrAdd(type, Factory);
            deleg(managedObject, nativePtr);
        }

        internal static MarshalToNativeFieldDelegate GetToNativeFieldMarshallerDelegate(FieldInfo field, Type type)
        {
            static MarshalToNativeFieldDelegate Factory(Type type)
            {
                MethodInfo method;
                if (type.IsValueType)
                    method = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNativeFieldWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    method = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeFieldWrapper), BindingFlags.Static | BindingFlags.NonPublic);
                return method.CreateDelegate<MarshalToNativeFieldDelegate>();
            }

            if (toNativeFieldMarshallers.TryGetValue(type, out var deleg))
                return deleg;
            return toNativeFieldMarshallers.GetOrAdd(type, Factory);
        }

        internal static class FieldHelper
        {
            /// <summary>
            /// Returns the address of the field, relative to field owner.
            /// </summary>
            internal static int GetFieldOffset(FieldInfo field, Type type)
            {
                if (field.IsLiteral)
                    return 0;

                // Get the address of the field, source: https://stackoverflow.com/a/56512720
                int fieldOffset = Unsafe.Read<int>((field.FieldHandle.Value + 4 + IntPtr.Size).ToPointer()) & 0xFFFFFF;
                if (!type.IsValueType)
                    fieldOffset += IntPtr.Size;
                return fieldOffset;
            }

#if USE_AOT
            /// <summary>
            /// Helper utility to set field of the referenced value via reflection.
            /// </summary>
            internal static void SetReferenceTypeField<T>(FieldInfo field, ref T fieldOwner, object fieldValue)
            {
                if (typeof(T).IsValueType)
                {
                    // Value types need setting via boxed object to properly propagate value
                    object fieldOwnerBoxed = fieldOwner;
                    field.SetValue(fieldOwnerBoxed, fieldValue);
                    fieldOwner = (T)fieldOwnerBoxed;
                }
                else
                    field.SetValue(fieldOwner, fieldValue);
            }
#else
            // TODO: Use Flax.Build to generate UnsafeAccessor methods for scripting object fields, fallback to reflection for other types

            /// <summary>
            /// Returns a reference to the value of the field.
            /// </summary>
            internal static ref TField GetReferenceTypeFieldReference<TField>(int fieldOffset, ref object fieldOwner)
            {
                byte* fieldPtr = (byte*)Unsafe.As<object, IntPtr>(ref fieldOwner) + fieldOffset;
                return ref Unsafe.AsRef<TField>(fieldPtr);
            }

            /// <summary>
            /// Returns a reference to the value of the field.
            /// </summary>
            internal static ref TField GetValueTypeFieldReference<T, TField>(int fieldOffset, ref T fieldOwner) //where T : struct
            {
                byte* fieldPtr = (byte*)Unsafe.AsPointer(ref fieldOwner) + fieldOffset;
                return ref Unsafe.AsRef<TField>(fieldPtr);
            }

            /// <summary>
            /// Returns a reference to the value of the field.
            /// </summary>
            internal static ref TField GetReferenceTypeFieldReference<T, TField>(int fieldOffset, ref T fieldOwner) //where T : class
            {
                byte* fieldPtr = (byte*)Unsafe.As<T, IntPtr>(ref fieldOwner) + fieldOffset;
                return ref Unsafe.AsRef<TField>(fieldPtr);
            }
#endif
        }

        /// <summary>
        /// Helper class for managing stored marshaller delegates for each type.
        /// </summary>
        internal static class MarshalHelper<T>
        {
            private delegate void MarshalToNativeTypedDelegate(ref T managedValue, IntPtr nativePtr);
            private delegate void MarshalToManagedTypedDelegate(ref T managedValue, IntPtr nativePtr, bool byRef);
            internal delegate void MarshalFieldTypedDelegate(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize);
            internal delegate void* GetBasePointer(ref T fieldOwner);

            internal static Delegate toManagedDelegate;
            internal static FieldInfo[] marshallableFields;
            internal static int[] marshallableFieldOffsets;
            internal static MarshalFieldTypedDelegate[] toManagedFieldMarshallers;
            internal static MarshalFieldTypedDelegate[] toNativeFieldMarshallers;

            private static MarshalToNativeTypedDelegate toNativeTypedMarshaller;
            private static MarshalToManagedTypedDelegate toManagedTypedMarshaller;

            static MarshalHelper()
            {
                Type type = typeof(T);

                // Setup field-by-field marshallers for reference types or structures containing references
                if (!type.IsPrimitive && !type.IsPointer && type != typeof(bool))
                {
                    marshallableFields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
                    if (type.IsValueType && !marshallableFields.Any(x => (x.FieldType.IsClass && !x.FieldType.IsPointer) || x.FieldType.Name == "Boolean"))
                        marshallableFields = null;
                    else if (!type.IsValueType && !marshallableFields.Any())
                        marshallableFields = null;

                    if (marshallableFields != null)
                    {
                        toManagedFieldMarshallers = new MarshalFieldTypedDelegate[marshallableFields.Length];
                        toNativeFieldMarshallers = new MarshalFieldTypedDelegate[marshallableFields.Length];
                        marshallableFieldOffsets = new int[marshallableFields.Length];
                        BindingFlags bindingFlags = BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public;
                        for (int i = 0; i < marshallableFields.Length; i++)
                        {
                            FieldInfo field = marshallableFields[i];
                            Type fieldType = field.FieldType;
                            MethodInfo toManagedFieldMethod;
                            MethodInfo toNativeFieldMethod;

                            if (fieldType.IsPointer)
                            {
                                if (type.IsValueType)
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToManagedFieldPointerValueType), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToNativeFieldPointerValueType), bindingFlags);
                                }
                                else
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToManagedFieldPointerReferenceType), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToNativeFieldPointerReferenceType), bindingFlags);
                                }
                            }
                            else if (fieldType.IsValueType)
                            {
                                if (Nullable.GetUnderlyingType(fieldType) != null)
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.NullableValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.NullableValueTypeField<ValueTypePlaceholder>.ToManagedField), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.NullableValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.NullableValueTypeField<ValueTypePlaceholder>.ToNativeField), bindingFlags);
                                }
                                else
                                {
                                    if (type.IsValueType)
                                    {
                                        toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedFieldValueType), bindingFlags);
                                        toNativeFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToNativeFieldValueType), bindingFlags);
                                    }
                                    else
                                    {
                                        toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedFieldReferenceType), bindingFlags);
                                        toNativeFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToNativeFieldReferenceType), bindingFlags);
                                    }
                                }
                            }
                            else if (fieldType.IsArray)
                            {
                                Type arrayElementType = fieldType.GetElementType();
                                if (arrayElementType.IsValueType)
                                {
                                    if (type.IsValueType)
                                    {
                                        toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedFieldArrayValueType), bindingFlags);
                                        toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeFieldValueType), bindingFlags);
                                    }
                                    else
                                    {
                                        toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedFieldArrayReferenceType), bindingFlags);
                                        toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeFieldReferenceType), bindingFlags);
                                    }
                                }
                                else
                                {
                                    if (type.IsValueType)
                                    {
                                        toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedFieldArrayValueType), bindingFlags);
                                        toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeFieldValueType), bindingFlags);
                                    }
                                    else
                                    {
                                        toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedFieldArrayReferenceType), bindingFlags);
                                        toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeFieldReferenceType), bindingFlags);
                                    }
                                }
                            }
                            else
                            {
                                if (type.IsValueType)
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedFieldValueType), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeFieldValueType), bindingFlags);
                                }
                                else
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedFieldReferenceType), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeFieldReferenceType), bindingFlags);
                                }
                            }
                            toManagedFieldMarshallers[i] = toManagedFieldMethod.CreateDelegate<MarshalFieldTypedDelegate>();
                            toNativeFieldMarshallers[i] = toNativeFieldMethod.CreateDelegate<MarshalFieldTypedDelegate>();
                            marshallableFieldOffsets[i] = FieldHelper.GetFieldOffset(field, type);
                        }
                    }
                }

                // Setup marshallers for managed and native directions
                MethodInfo toManagedMethod;
                if (type.IsValueType)
                {
                    // Non-POD structures use internal layout (eg. SpriteHandleManaged in C++ with SpriteHandleMarshaller.SpriteHandleInternal in C#) so convert C++ data into C# data
                    var attr = type.GetCustomAttribute<System.Runtime.InteropServices.Marshalling.NativeMarshallingAttribute>();
                    toManagedMethod = attr?.NativeType.GetMethod("ToManaged", BindingFlags.Static | BindingFlags.NonPublic);
                    if (toManagedMethod != null)
                    {
                        // TODO: optimize via delegate call rather than method invoke
                        var internalType = toManagedMethod.GetParameters()[0].ParameterType;
                        var types = new Type[] { type, internalType };
                        toManagedDelegate = toManagedMethod.CreateDelegate(typeof(ToManagedDelegate<,>).MakeGenericType(types));
                        //toManagedDelegate = toManagedMethod.CreateDelegate();//.CreateDelegate(typeof(ToManagedDelegate<,>).MakeGenericType(type, toManagedMethod.GetParameters()[0].ParameterType));
                        string methodName = nameof(MarshalInternalHelper<ValueTypePlaceholder, ValueTypePlaceholder>.ToManagedMarshaller);
                        toManagedMethod = typeof(MarshalInternalHelper<,>).MakeGenericType(types).GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
                    }
                    else
                    {
                        string methodName;
                        if (type == typeof(IntPtr))
                            methodName = nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedPointer);
                        else if (type == typeof(ManagedHandle))
                            methodName = nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedHandle);
                        else if (marshallableFields != null)
                            methodName = nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedWithMarshallableFields);
                        else
                            methodName = nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManaged);
                        toManagedMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
                    }
                }
                else if (type.IsArray)
                {
                    Type elementType = type.GetElementType();
                    if (elementType.IsValueType)
                    {
                        string methodName;
                        if (ArrayFactory.GetMarshalledType(elementType) == elementType)
                            methodName = nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedArray);
                        else
                            methodName = nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedArrayMarshalled);
                        toManagedMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type.GetElementType()).GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
                    }
                    else
                        toManagedMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type.GetElementType()).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedArray), BindingFlags.Static | BindingFlags.NonPublic);
                }
                else
                {
                    string methodName;
                    if (type == typeof(string))
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedString);
                    else if (type == typeof(Type))
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedType);
                    else if (type.IsClass)
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedClass);
                    else if (type.IsInterface) // Dictionary
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedInterface);
                    else
                        throw new NativeInteropException($"Unsupported type '{type.FullName}'");
                    toManagedMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
                }
                toManagedTypedMarshaller = toManagedMethod.CreateDelegate<MarshalToManagedTypedDelegate>();

                MethodInfo toNativeMethod;
                if (type.IsValueType)
                {
                    if (type.IsByRef)
                        throw new NotImplementedException(); // Is this possible?
                    if (marshallableFields != null)
                        toNativeMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNativeWithMarshallableFields), BindingFlags.Static | BindingFlags.NonPublic);
                    else
                        toNativeMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNative), BindingFlags.Static | BindingFlags.NonPublic);
                }
                else
                {
                    string methodName;
                    if (type == typeof(string))
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeString);
                    else if (type == typeof(Type))
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeType);
                    else if (type.IsPointer)
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativePointer);
                    else if (type.IsArray)
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNativeArray);
                    else
                        methodName = nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNative);
                    toNativeMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
                }
                toNativeTypedMarshaller = toNativeMethod.CreateDelegate<MarshalToNativeTypedDelegate>();
            }

            internal static object ToManagedWrapper(IntPtr nativePtr, bool byRef)
            {
                T managed = default;
                toManagedTypedMarshaller(ref managed, nativePtr, byRef);
                return managed;
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                toManagedTypedMarshaller(ref managedValue, nativePtr, byRef);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            internal static T ToManagedUnbox(IntPtr nativePtr)
            {
                T value = default;
                if (nativePtr == IntPtr.Zero)
                    return value;
                if (typeof(T).IsValueType)
                    value = (T)ManagedHandle.FromIntPtr(nativePtr).Target;
                else
                    MarshalHelper<T>.ToManaged(ref value, nativePtr, false);
                return value;
            }

            internal static Array ToManagedArray(Span<IntPtr> ptrSpan)
            {
                T[] arr = new T[ptrSpan.Length];
                for (int i = 0; i < arr.Length; i++)
                    toManagedTypedMarshaller(ref arr[i], ptrSpan[i], false);
                return arr;
            }

            internal static Array ToManagedArray(ManagedArray nativeArray)
            {
                T[] arr = new T[nativeArray.Length];
                IntPtr nativePtr = nativeArray.Pointer;
                for (int i = 0; i < arr.Length; i++)
                {
                    toManagedTypedMarshaller(ref arr[i], nativePtr, false);
                    nativePtr += nativeArray.ElementSize;
                }
                return arr;
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                toNativeTypedMarshaller(ref managedValue, nativePtr);
            }

            internal static void ToNativeField(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativePtr, out int fieldSize)
            {
                if (marshallableFields != null)
                {
                    for (int i = 0; i < marshallableFields.Length; i++)
                    {
                        if (marshallableFieldOffsets[i] == fieldOffset)
                        {
                            toNativeFieldMarshallers[i](field, fieldOffset, ref fieldOwner, nativePtr, out fieldSize);
                            return;
                        }
                    }
                }
                throw new NativeInteropException($"Invalid field with offset {fieldOffset} to marshal for type {typeof(T).Name}");
            }

            private static void ToManagedFieldPointerValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
            {
#if USE_AOT
                IntPtr fieldValue = Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer());
                FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#else
                ref IntPtr fieldValueRef = ref FieldHelper.GetValueTypeFieldReference<T, IntPtr>(fieldOffset, ref fieldOwner);
                fieldValueRef = Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer());
#endif
                fieldSize = IntPtr.Size;
            }

            private static void ToManagedFieldPointerReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
            {
#if USE_AOT
                IntPtr fieldValue = Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer());
                FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#else
                ref IntPtr fieldValueRef = ref FieldHelper.GetReferenceTypeFieldReference<T, IntPtr>(fieldOffset, ref fieldOwner);
                fieldValueRef = Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer());
#endif
                fieldSize = IntPtr.Size;
            }

            private static void ToNativeFieldPointerValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
            {
#if USE_AOT
                object boxed = field.GetValue(fieldOwner);
                IntPtr fieldValue = new IntPtr(Pointer.Unbox(boxed));
#else
                IntPtr fieldValue = FieldHelper.GetValueTypeFieldReference<T, IntPtr>(fieldOffset, ref fieldOwner);
#endif
                Unsafe.Write<IntPtr>(nativeFieldPtr.ToPointer(), fieldValue);
                fieldSize = IntPtr.Size;
            }

            private static void ToNativeFieldPointerReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
            {
#if USE_AOT
                object boxed = field.GetValue(fieldOwner);
                IntPtr fieldValue = new IntPtr(Pointer.Unbox(boxed));
#else
                IntPtr fieldValue = FieldHelper.GetReferenceTypeFieldReference<T, IntPtr>(fieldOffset, ref fieldOwner);
#endif
                Unsafe.Write<IntPtr>(nativeFieldPtr.ToPointer(), fieldValue);
                fieldSize = IntPtr.Size;
            }

            private static IntPtr EnsureAlignment(IntPtr ptr, int alignment)
            {
                if (ptr % alignment != 0)
                    ptr = IntPtr.Add(ptr, alignment - (int)(ptr % alignment));
                return ptr;
            }

            private static class ValueTypeField<TField> where TField : struct
            {
                private static int fieldAlignment;

                static ValueTypeField()
                {
                    Type fieldType = typeof(TField);
                    if (fieldType.IsEnum)
                        fieldType = fieldType.GetEnumUnderlyingType();
                    else if (fieldType == typeof(bool))
                        fieldType = typeof(byte);

                    if (fieldType.IsValueType && !fieldType.IsEnum && !fieldType.IsPrimitive) // Is it a structure?
                    {
                    }
                    else if (fieldType.IsClass || fieldType.IsPointer)
                        fieldAlignment = IntPtr.Size;
                    else
                        fieldAlignment = GetTypeSize(fieldType);
                }

                internal static void ToManagedFieldValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
                {
                    fieldSize = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr fieldStartPtr = nativeFieldPtr;
                        nativeFieldPtr = EnsureAlignment(nativeFieldPtr, fieldAlignment);
                        fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();
                    }

#if USE_AOT
                    TField fieldValue = default;
#else
                    ref TField fieldValue = ref FieldHelper.GetValueTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToManaged(ref fieldValue, nativeFieldPtr, false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToManagedFieldReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
                {
                    fieldSize = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr fieldStartPtr = nativeFieldPtr;
                        nativeFieldPtr = EnsureAlignment(nativeFieldPtr, fieldAlignment);
                        fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();
                    }

#if USE_AOT
                    TField fieldValue = default;
#else
                    ref TField fieldValue = ref FieldHelper.GetReferenceTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToManaged(ref fieldValue, nativeFieldPtr, false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToManagedFieldArrayValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
                {
                    // Follows the same marshalling semantics with reference types
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField[] fieldValue = (TField[])field.GetValue(fieldOwner);
#else
                    ref TField[] fieldValue = ref FieldHelper.GetValueTypeFieldReference<T, TField[]>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField[]>.ToManaged(ref fieldValue, Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer()), false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToManagedFieldArrayReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
                {
                    // Follows the same marshalling semantics with reference types
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField[] fieldValue = null;
#else
                    ref TField[] fieldValue = ref FieldHelper.GetReferenceTypeFieldReference<T, TField[]>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField[]>.ToManaged(ref fieldValue, Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer()), false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToNativeFieldValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
                {
                    fieldSize = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr startPtr = nativeFieldPtr;
                        nativeFieldPtr = EnsureAlignment(nativeFieldPtr, fieldAlignment);
                        fieldSize += (nativeFieldPtr - startPtr).ToInt32();
                    }

#if USE_AOT
                    TField fieldValue = (TField)field.GetValue(fieldOwner);
#else
                    ref TField fieldValue = ref FieldHelper.GetValueTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToNative(ref fieldValue, nativeFieldPtr);
                }

                internal static void ToNativeFieldReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
                {
                    fieldSize = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr startPtr = nativeFieldPtr;
                        nativeFieldPtr = EnsureAlignment(nativeFieldPtr, fieldAlignment);
                        fieldSize += (nativeFieldPtr - startPtr).ToInt32();
                    }

#if USE_AOT
                    TField fieldValue = (TField)field.GetValue(fieldOwner);
#else
                    ref TField fieldValue = ref FieldHelper.GetReferenceTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToNative(ref fieldValue, nativeFieldPtr);
                }
            }

            private static class NullableValueTypeField<TField>
            {
                static NullableValueTypeField()
                {
                }

                internal static void ToManagedField(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize)
                {
                    fieldSize = 0;
                }

                internal static void ToNativeField(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize)
                {
                    fieldSize = 0;
                }
            }

            private static class ReferenceTypeField<TField> where TField : class
            {
                internal static void ToManagedFieldValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
                {
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField fieldValue = null;
#else
                    ref TField fieldValue = ref FieldHelper.GetValueTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToManaged(ref fieldValue, Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer()), false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToManagedFieldReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
                {
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField fieldValue = default;
#else
                    ref TField fieldValue = ref FieldHelper.GetReferenceTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToManaged(ref fieldValue, Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer()), false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToManagedFieldArrayValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
                {
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField[] fieldValue = null;
#else
                    ref TField[] fieldValue = ref FieldHelper.GetValueTypeFieldReference<T, TField[]>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField[]>.ToManaged(ref fieldValue, Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer()), false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToManagedFieldArrayReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
                {
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField[] fieldValue = null;
#else
                    ref TField[] fieldValue = ref FieldHelper.GetReferenceTypeFieldReference<T, TField[]>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField[]>.ToManaged(ref fieldValue, Unsafe.Read<IntPtr>(nativeFieldPtr.ToPointer()), false);
#if USE_AOT
                    FieldHelper.SetReferenceTypeField(field, ref fieldOwner, fieldValue);
#endif
                }

                internal static void ToNativeFieldValueType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : struct
                {
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField fieldValue = (TField)field.GetValue(fieldOwner);
#else
                    ref TField fieldValue = ref FieldHelper.GetValueTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToNative(ref fieldValue, nativeFieldPtr);
                }

                internal static void ToNativeFieldReferenceType(FieldInfo field, int fieldOffset, ref T fieldOwner, IntPtr nativeFieldPtr, out int fieldSize) // where T : class
                {
                    fieldSize = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = nativeFieldPtr;
                    nativeFieldPtr = EnsureAlignment(nativeFieldPtr, IntPtr.Size);
                    fieldSize += (nativeFieldPtr - fieldStartPtr).ToInt32();

#if USE_AOT
                    TField fieldValue = (TField)field.GetValue(fieldOwner);
#else
                    ref TField fieldValue = ref FieldHelper.GetReferenceTypeFieldReference<T, TField>(fieldOffset, ref fieldOwner);
#endif
                    MarshalHelper<TField>.ToNative(ref fieldValue, nativeFieldPtr);
                }
            }
        }

        internal static class MarshalInternalHelper<T, TInternal> where T : struct
                                                                  where TInternal : struct
        {
            internal static void ToManagedMarshaller(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                ToManagedDelegate<T, TInternal> toManaged = Unsafe.As<ToManagedDelegate<T, TInternal>>(MarshalHelper<T>.toManagedDelegate);
                TInternal intern = Unsafe.Read<TInternal>(nativePtr.ToPointer());
                managedValue = toManaged(Unsafe.Read<TInternal>(nativePtr.ToPointer()));
            }
        }

        internal static class MarshalHelperValueType<T> where T : struct
        {
            internal static void ToNativeWrapper(object managedObject, IntPtr nativePtr)
            {
                MarshalHelper<T>.ToNative(ref Unsafe.Unbox<T>(managedObject), nativePtr);
            }

            internal static void ToNativeFieldWrapper(FieldInfo field, int fieldOffset, object fieldOwner, IntPtr nativePtr, out int fieldSize)
            {
                MarshalHelper<T>.ToNativeField(field, fieldOffset, ref Unsafe.Unbox<T>(fieldOwner), nativePtr, out fieldSize);
            }

            internal static void ToManagedPointer(ref IntPtr managedValue, IntPtr nativePtr, bool byRef)
            {
                Type type = typeof(T);
                byRef |= type.IsByRef; // Is this needed?
                if (type.IsByRef)
                    Assert.IsTrue(type.GetElementType().IsValueType);
                managedValue = byRef ? nativePtr : Unsafe.Read<IntPtr>(nativePtr.ToPointer());
            }

            internal static void ToManagedHandle(ref ManagedHandle managedValue, IntPtr nativePtr, bool byRef)
            {
                managedValue = ManagedHandle.FromIntPtr(nativePtr);
            }

            internal static void ToManagedWithMarshallableFields(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                IntPtr fieldPtr = nativePtr;
                var fields = MarshalHelper<T>.marshallableFields;
                var offsets = MarshalHelper<T>.marshallableFieldOffsets;
                var marshallers = MarshalHelper<T>.toManagedFieldMarshallers;
                for (int i = 0; i < fields.Length; i++)
                {
                    marshallers[i](fields[i], offsets[i], ref managedValue, fieldPtr, out int fieldSize);
                    fieldPtr += fieldSize;
                }
                //Assert.IsTrue((fieldPtr - nativePtr) <= GetTypeSize(typeof(T)));
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                managedValue = Unsafe.Read<T>(nativePtr.ToPointer());
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());

                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    managedValue = Unsafe.As<T[]>(managedArray.ToArray<T>());
                }
                else
                    managedValue = null;
            }

            internal static void ToManagedArrayMarshalled(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());

                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray));
                }
                else
                    managedValue = null;
            }

            internal static void ToNativeWithMarshallableFields(ref T managedValue, IntPtr nativePtr)
            {
                IntPtr fieldPtr = nativePtr;
                var fields = MarshalHelper<T>.marshallableFields;
                var offsets = MarshalHelper<T>.marshallableFieldOffsets;
                var marshallers = MarshalHelper<T>.toNativeFieldMarshallers;
                for (int i = 0; i < fields.Length; i++)
                {
                    marshallers[i](fields[i], offsets[i], ref managedValue, nativePtr, out int fieldSize);
                    nativePtr += fieldSize;
                }
                //Assert.IsTrue((nativePtr - fieldPtr) <= GetTypeSize(typeof(T)));
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                Unsafe.AsRef<T>(nativePtr.ToPointer()) = managedValue;
            }
        }

        internal static class MarshalHelperReferenceType<T> where T : class
        {
            internal static void ToNativeWrapper(object managedObject, IntPtr nativePtr)
            {
                T managedValue = Unsafe.As<T>(managedObject);
                MarshalHelper<T>.ToNative(ref managedValue, nativePtr);
            }

            internal static void ToNativeFieldWrapper(FieldInfo field, int fieldOffset, object fieldOwner, IntPtr nativePtr, out int fieldSize)
            {
                T managedValue = Unsafe.As<T>(fieldOwner);
                MarshalHelper<T>.ToNativeField(field, fieldOffset, ref managedValue, nativePtr, out fieldSize);
            }

            internal static void ToManagedString(ref string managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());
                managedValue = ManagedString.ToManaged(nativePtr);
            }

            internal static void ToManagedType(ref Type managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());
                managedValue = nativePtr == IntPtr.Zero ? null : Unsafe.As<TypeHolder>(ManagedHandle.FromIntPtr(nativePtr).Target);
            }

            internal static void ToManagedClass(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());
                managedValue = nativePtr == IntPtr.Zero ? null : Unsafe.As<T>(ManagedHandle.FromIntPtr(nativePtr).Target);
            }

            internal static void ToManagedInterface(ref T managedValue, IntPtr nativePtr, bool byRef) // Dictionary
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());
                managedValue = nativePtr == IntPtr.Zero ? null : Unsafe.As<T>(ManagedHandle.FromIntPtr(nativePtr).Target);
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());

                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray.ToSpan<IntPtr>()));
                }
                else
                    managedValue = null;
            }


            internal static void ToNativeString(ref string managedValue, IntPtr nativePtr)
            {
                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), ManagedString.ToNativeWeak(managedValue));
            }

            internal static void ToNativeType(ref Type managedValue, IntPtr nativePtr)
            {
                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), managedValue != null ? ManagedHandle.ToIntPtr(GetTypeManagedHandle(managedValue)) : IntPtr.Zero);
            }

            internal static void ToNativePointer(ref T managedValue, IntPtr nativePtr)
            {
                IntPtr managedPtr;
                if (Pointer.Unbox(managedValue) == null)
                    managedPtr = IntPtr.Zero;
                else if (managedValue is FlaxEngine.Object obj)
                    managedPtr = FlaxEngine.Object.GetUnmanagedPtr(obj);
                else
                    managedPtr = ManagedHandle.ToIntPtr(managedValue, GCHandleType.Weak);
                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), managedPtr);
            }

            internal static void ToNativeArray(ref T managedValue, IntPtr nativePtr)
            {
                IntPtr managedPtr;
                if (managedValue == null)
                    managedPtr = IntPtr.Zero;
                else
                {
                    Type type = typeof(T);
                    var elementType = type.GetElementType();
                    var arr = Unsafe.As<Array>(managedValue);
                    var marshalledType = ArrayFactory.GetMarshalledType(elementType);
                    ManagedArray managedArray;
                    if (marshalledType == elementType)
                        managedArray = ManagedArray.WrapNewArray(arr, type);
                    else if (elementType.IsValueType)
                    {
                        // Convert array of custom structures into internal native layout
                        managedArray = ManagedArray.AllocateNewArray(arr.Length, type, marshalledType);
                        IntPtr managedArrayPtr = managedArray.Pointer;
                        for (int i = 0; i < arr.Length; i++)
                        {
                            MarshalToNative(arr.GetValue(i), managedArrayPtr, elementType);
                            managedArrayPtr += managedArray.ElementSize;
                        }
                    }
                    else
                        managedArray = ManagedArrayToGCHandleWrappedArray(arr);
                    managedPtr = ManagedHandle.ToIntPtr(managedArray, GCHandleType.Weak);
                }
                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), managedPtr);
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), managedValue != null ? ManagedHandle.ToIntPtr(managedValue, GCHandleType.Weak) : IntPtr.Zero);
            }
        }

        internal class MethodHolder
        {
            internal Type[] parameterTypes;
            internal MethodInfo method;
            internal Type returnType;
#if !USE_AOT
            private Invoker.MarshalAndInvokeDelegate invokeDelegate;
            private object delegInvoke;
#endif

            internal MethodHolder(MethodInfo method)
            {
                this.method = method;
                returnType = method.ReturnType;
                parameterTypes = method.GetParameterTypes();
            }

#if !USE_AOT
            internal bool TryGetDelegate(out Invoker.MarshalAndInvokeDelegate outDeleg, out object outDelegInvoke)
            {
                // Skip using in-built delegate for value types (eg. Transform) to properly handle instance value passing to method
                if (invokeDelegate == null && !method.DeclaringType.IsValueType)
                {
                    // Thread-safe creation
                    lock (method)
                    {
                        if (invokeDelegate == null)
                        {
                            TryCreateDelegate();
                        }
                    }
                }
                outDeleg = invokeDelegate;
                outDelegInvoke = delegInvoke;
                return outDeleg != null;
            }

            private void TryCreateDelegate()
            {
                var methodTypes = new List<Type>();
                if (!method.IsStatic)
                    methodTypes.Add(method.DeclaringType);
                if (returnType != typeof(void))
                    methodTypes.Add(returnType);
                methodTypes.AddRange(parameterTypes);

                var genericParamTypes = new List<Type>();
                foreach (var type in methodTypes)
                {
                    if (type.IsByRef)
                        genericParamTypes.Add(type.GetElementType());
                    else if (type.IsPointer)
                        genericParamTypes.Add(typeof(IntPtr));
                    else
                        genericParamTypes.Add(type);
                }

                string invokerTypeName = $"{typeof(Invoker).FullName}+Invoker{(method.IsStatic ? "Static" : "")}{(returnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                Type invokerType = Type.GetType(invokerTypeName);
                if (invokerType != null)
                {
                    if (genericParamTypes.Count != 0)
                        invokerType = invokerType.MakeGenericType(genericParamTypes.ToArray());
                    delegInvoke = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });
                    invokeDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.MarshalAndInvoke), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.MarshalAndInvokeDelegate>();
                }
            }
#endif
        }

        internal static ManagedHandle GetMethodGCHandle(MethodInfo method)
        {
            MethodHolder methodHolder = new MethodHolder(method);
            ManagedHandle handle = ManagedHandle.Alloc(methodHolder);
#if FLAX_EDITOR
            if (methodHolder.parameterTypes.Any(x => x.IsCollectible) || method.IsCollectible)
                methodHandlesCollectible.Add(handle);
            else
#endif
            {
                methodHandles.Add(handle);
            }
            return handle;
        }

        internal static ManagedHandle GetAssemblyHandle(Assembly assembly)
        {
            if (!assemblyHandles.TryGetValue(assembly, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(assembly);
                assemblyHandles.Add(assembly, handle);
            }
            return handle;
        }

        internal class FieldHolder
        {
            internal FieldInfo field;
            internal MarshalToNativeFieldDelegate toNativeMarshaller;
            internal int fieldOffset;

            internal FieldHolder(FieldInfo field, Type type)
            {
                this.field = field;
                toNativeMarshaller = GetToNativeFieldMarshallerDelegate(field, type);
                fieldOffset = FieldHelper.GetFieldOffset(field, type);
            }
        }

        internal class TypeComparer : IEqualityComparer<Type>
        {
            public bool Equals(Type x, Type y) => x == y;
            public int GetHashCode(Type obj) => obj.GetHashCode();
        }

        internal class TypeHolder
        {
            internal Type type;
            internal Type wrappedType;
            internal ConstructorInfo ctor;
            internal IntPtr managedClassPointer; // MClass*

            internal TypeHolder(Type type)
            {
                this.type = type;
                wrappedType = type;

                if (type.IsAbstract)
                {
                    // Dotnet doesn't allow to instantiate abstract type thus allow to use generated mock class usage (eg. for Script or GPUResource) for generated abstract types
                    var abstractWrapper = type.GetNestedType("AbstractWrapper", BindingFlags.NonPublic);
                    if (abstractWrapper != null)
                        wrappedType = abstractWrapper;
                }

                ctor = wrappedType.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, Type.EmptyTypes, null);
            }

            internal object CreateObject()
            {
                return RuntimeHelpers.GetUninitializedObject(wrappedType);
            }

#if !USE_AOT
            internal object CreateScriptingObject(IntPtr unmanagedPtr, IntPtr idPtr)
            {
                object obj = RuntimeHelpers.GetUninitializedObject(wrappedType);
                if (obj is Object scriptingObject)
                {
                    GetUnmanagedPtrFieldReference(scriptingObject) = unmanagedPtr;
                    if (idPtr != IntPtr.Zero)
                        GetInternalIdFieldReference(scriptingObject) = Unsafe.AsRef<Guid>(idPtr.ToPointer());
                }
                if (ctor != null)
                    ctor.Invoke(obj, null);
                else
                    throw new NativeInteropException($"Missing empty constructor in type '{wrappedType}'.");
                return obj;
            }
#endif

            public static implicit operator Type(TypeHolder holder) => holder?.type;
            public bool Equals(TypeHolder other) => type == other.type;
            public bool Equals(Type other) => type == other;
            public override int GetHashCode() => type.GetHashCode();
        }

        internal static class ArrayFactory
        {
            private delegate Array CreateArrayDelegate(long size);

            private static ConcurrentDictionary<Type, Type> marshalledTypes = new ConcurrentDictionary<Type, Type>(1, 3);
            private static ConcurrentDictionary<Type, Type> arrayTypes = new ConcurrentDictionary<Type, Type>(1, 3);
            private static ConcurrentDictionary<Type, CreateArrayDelegate> createArrayDelegates = new ConcurrentDictionary<Type, CreateArrayDelegate>(1, 3);

            internal static Type GetMarshalledType(Type elementType)
            {
                static Type Factory(Type type)
                {
                    Type marshalType;
                    if (IsBlittable(type))
                        marshalType = type;
                    else
                        marshalType = GetInternalType(type) ?? typeof(IntPtr);
                    return marshalType;
                }

                if (marshalledTypes.TryGetValue(elementType, out var marshalledType))
                    return marshalledType;
                return marshalledTypes.GetOrAdd(elementType, Factory);
            }

            internal static Type GetArrayType(Type elementType)
            {
                static Type Factory(Type type) => type.MakeArrayType();

                if (arrayTypes.TryGetValue(elementType, out var arrayType))
                    return arrayType;
                return arrayTypes.GetOrAdd(elementType, Factory);
            }

            internal static Array CreateArray(Type type, long size)
            {
                static CreateArrayDelegate Factory(Type type)
                {
                    Type marshalledType = GetMarshalledType(type);
                    MethodInfo method = typeof(Internal<>).MakeGenericType(marshalledType).GetMethod(nameof(Internal<ValueTypePlaceholder>.CreateArrayDelegate), BindingFlags.Static | BindingFlags.NonPublic);
                    return method.CreateDelegate<CreateArrayDelegate>();
                }

                if (createArrayDelegates.TryGetValue(type, out var deleg))
                    return deleg(size);
                return createArrayDelegates.GetOrAdd(type, Factory)(size);
            }


            private static class Internal<T>
            {
                internal static Array CreateArrayDelegate(long size) => new T[size];
            }
        }

        internal static class ValueTypeUnboxer
        {
            private static GCHandle[] pinnedBoxedValues = new GCHandle[256];
            private static uint pinnedBoxedValuesPointer = 0;
            private static (IntPtr ptr, int size)[] pinnedAllocations = new (IntPtr ptr, int size)[256];
            private static uint pinnedAllocationsPointer = 0;

            private delegate IntPtr UnboxerDelegate(object value, object converter);

            private static ConcurrentDictionary<Type, (UnboxerDelegate deleg, object toNativeDeleg)> unboxers = new(1, 3);
            private static MethodInfo unboxerMethod = typeof(ValueTypeUnboxer).GetMethod(nameof(ValueTypeUnboxer.UnboxPointer), BindingFlags.Static | BindingFlags.NonPublic);
            private static MethodInfo unboxerToNativeMethod = typeof(ValueTypeUnboxer).GetMethod(nameof(ValueTypeUnboxer.UnboxPointerWithConverter), BindingFlags.Static | BindingFlags.NonPublic);

            internal static IntPtr GetPointer(object value, Type type)
            {
                if (!unboxers.TryGetValue(type, out var tuple))
                {
                    // Non-POD structures use internal layout (eg. SpriteHandleManaged in C++ with SpriteHandleMarshaller.SpriteHandleInternal in C#) so convert C# data into C++ data
                    var attr = type.GetCustomAttribute<System.Runtime.InteropServices.Marshalling.NativeMarshallingAttribute>();
                    var toNativeMethod = attr?.NativeType.GetMethod("ToNative", BindingFlags.Static | BindingFlags.NonPublic);
                    if (toNativeMethod != null)
                    {
                        tuple.deleg = unboxerToNativeMethod.MakeGenericMethod(type, toNativeMethod.ReturnType).CreateDelegate<UnboxerDelegate>();
                        tuple.toNativeDeleg = toNativeMethod.CreateDelegate(typeof(ToNativeDelegate<,>).MakeGenericType(type, toNativeMethod.ReturnType));
                    }
                    else
                    {
                        tuple.deleg = unboxerMethod.MakeGenericMethod(type).CreateDelegate<UnboxerDelegate>();
                    }
                    tuple = unboxers.GetOrAdd(type, tuple);
                }
                return tuple.deleg(value, tuple.toNativeDeleg);
            }

            private static void PinValue(object value)
            {
                // Prevent garbage collector from relocating the boxed value by pinning it temporarily.
                // The pointer should remain valid quite long time but will be eventually unpinned.
                uint index = Interlocked.Increment(ref pinnedBoxedValuesPointer) % (uint)pinnedBoxedValues.Length;
                ref GCHandle handle = ref pinnedBoxedValues[index];
                if (handle.IsAllocated)
                    handle.Free();
                handle = GCHandle.Alloc(value, GCHandleType.Pinned);
            }

            private static IntPtr PinValue<T>(T value) where T : struct
            {
                // Store the converted value in unmanaged memory so it will not be relocated by the garbage collector.
                int size = TypeHelpers<T>.MarshalSize;
                uint index = Interlocked.Increment(ref pinnedAllocationsPointer) % (uint)pinnedAllocations.Length;
                ref (IntPtr ptr, int size) alloc = ref pinnedAllocations[index];
                if (alloc.size < size)
                {
                    if (alloc.ptr != IntPtr.Zero)
                        NativeFree(alloc.ptr.ToPointer());
                    alloc.ptr = new IntPtr(NativeAlloc(size));
                    alloc.size = size;
                }
                Unsafe.Write<T>(alloc.ptr.ToPointer(), value);
                return alloc.ptr;
            }

            private static IntPtr UnboxPointer<T>(object value, object converter) where T : struct
            {
                if (RuntimeHelpers.IsReferenceOrContainsReferences<T>()) // Cannot pin structure with references
                    return IntPtr.Zero;
                PinValue(value);
                return new IntPtr(Unsafe.AsPointer(ref Unsafe.Unbox<T>(value)));
            }

            private static IntPtr UnboxPointerWithConverter<T, TInternal>(object value, object converter) where T : struct
                                                                                                          where TInternal : struct
            {
                ToNativeDelegate<T, TInternal> toNative = Unsafe.As<ToNativeDelegate<T, TInternal>>(converter);
                return PinValue<TInternal>(toNative(Unsafe.Unbox<T>(value)));
            }
        }

        private delegate IntPtr InvokeThunkDelegate(ManagedHandle instanceHandle, IntPtr param1, IntPtr param2, IntPtr param3, IntPtr param4, IntPtr param5, IntPtr param6, IntPtr param7);

        /// <summary>
        /// Returns all types owned by this assembly.
        /// </summary>
        private static Type[] GetAssemblyTypes(Assembly assembly)
        {
            var referencedAssemblies = assembly.GetReferencedAssemblies();
            var allAssemblies = Utils.GetAssemblies();
            var referencedTypes = new List<string>();
            foreach (var assemblyName in referencedAssemblies)
            {
                var asm = allAssemblies.FirstOrDefault(x => x.GetName().Name == assemblyName.Name);
                if (asm == null)
                    continue;
                referencedTypes.AddRange(asm.DefinedTypes.Select(x => x.FullName).ToArray());
            }

            // TODO: use MetadataReader to read types without loading any of the referenced assemblies?
            // https://makolyte.com/csharp-get-a-list-of-types-defined-in-an-assembly-without-loading-it/

            // We need private types of this assembly too, DefinedTypes contains a lot of types from other assemblies...
            var types = referencedTypes.Any() ? assembly.DefinedTypes.Where(x => !referencedTypes.Contains(x.FullName)).ToArray() : assembly.DefinedTypes.ToArray();

            Assert.IsTrue(Utils.GetAssemblies().Count(x => x.GetName().Name == "FlaxEngine.CSharp") == 1);
            return types;
        }

        internal static TypeHolder GetTypeHolder(Type type)
        {
            if (managedTypes.TryGetValue(type, out (TypeHolder typeHolder, ManagedHandle handle) tuple))
                return tuple.typeHolder;
#if FLAX_EDITOR
            if (managedTypesCollectible.TryGetValue(type, out tuple))
                return tuple.typeHolder;
#endif
            return RegisterType(type, true).typeHolder;
        }

        internal static (TypeHolder typeHolder, ManagedHandle handle) GetTypeHolderAndManagedHandle(Type type)
        {
            if (managedTypes.TryGetValue(type, out (TypeHolder typeHolder, ManagedHandle handle) tuple))
                return tuple;
#if FLAX_EDITOR
            if (managedTypesCollectible.TryGetValue(type, out tuple))
                return tuple;
#endif
            return RegisterType(type, true);
        }

        /// <summary>
        /// Returns a static ManagedHandle to TypeHolder for given Type, and caches it if needed.
        /// </summary>
        internal static ManagedHandle GetTypeManagedHandle(Type type)
        {
            if (type.IsInterface && type.IsGenericType)
                type = type.GetGenericTypeDefinition(); // Generic type to use type definition handle
            if (managedTypes.TryGetValue(type, out (TypeHolder typeHolder, ManagedHandle handle) tuple))
                return tuple.handle;
#if FLAX_EDITOR
            if (managedTypesCollectible.TryGetValue(type, out tuple))
                return tuple.handle;
#endif
            return RegisterType(type, true).handle;
        }

        internal static (TypeHolder typeHolder, ManagedHandle handle) RegisterType(Type type, bool registerNativeType = false)
        {
            // TODO: should this strip by-ref?

            (TypeHolder typeHolder, ManagedHandle handle) tuple;
            tuple.typeHolder = new TypeHolder(type);
            tuple.handle = ManagedHandle.Alloc(tuple.typeHolder);
#if FLAX_EDITOR
            bool isCollectible = type.IsCollectible;
            if (!isCollectible && type.IsGenericType && !type.Assembly.IsCollectible)
            {
                // The owning assembly of a generic type with type arguments referencing
                // collectible assemblies must be one of the collectible assemblies.
                foreach (var genericType in type.GetGenericArguments())
                {
                    if (genericType.Assembly.IsCollectible)
                    {
                        isCollectible = true;
                        break;
                    }
                }
            }

            if (isCollectible)
                managedTypesCollectible.Add(type, tuple);
            else
#endif
            {
                managedTypes.Add(type, tuple);
            }

            if (registerNativeType)
                RegisterNativeClassFromType(tuple.typeHolder, tuple.handle);

            return tuple;
        }

        internal static class TypeHelpers<T>
        {
            public static readonly int MarshalSize;

            static TypeHelpers()
            {
                Type type = typeof(T);
                try
                {
                    var marshalType = type;
                    if (type.IsEnum)
                        marshalType = type.GetEnumUnderlyingType();
                    MarshalSize = Marshal.SizeOf(marshalType);
                }
                catch
                {
                    // Workaround the issue where structure defined within generic type instance (eg. MyType<int>.MyStruct) fails to get size
                    // https://github.com/dotnet/runtime/issues/46426
                    var obj = RuntimeHelpers.GetUninitializedObject(type);
                    MarshalSize = Marshal.SizeOf(obj);
                }
            }
        }

        internal static int GetTypeSize(Type type)
        {
            if (!_typeSizeCache.TryGetValue(type, out int size))
            {
                var marshalSizeField = typeof(TypeHelpers<>).MakeGenericType(type).GetField(nameof(TypeHelpers<int>.MarshalSize), BindingFlags.Static | BindingFlags.Public);
                size = (int)marshalSizeField.GetValue(null);
                _typeSizeCache.AddOrUpdate(type, size, (t, v) => size);
            }
            return size;
        }

        private static class DelegateHelpers
        {
#if USE_AOT
            internal static void InitMethods()
            {
            }
#else
            private static Func<Type[], Type> MakeNewCustomDelegateFunc;
#if FLAX_EDITOR
            private static Func<Type[], Type> MakeNewCustomDelegateFuncCollectible;
#endif

            internal static void InitMethods()
            {
                MakeNewCustomDelegateFunc =
                typeof(Expression).Assembly.GetType("System.Linq.Expressions.Compiler.DelegateHelpers")
                                  .GetMethod("MakeNewCustomDelegate", BindingFlags.NonPublic | BindingFlags.Static).CreateDelegate<Func<Type[], Type>>();

#if FLAX_EDITOR
                // Load System.Linq.Expressions assembly to collectible ALC.
                // The dynamic assembly where delegates are stored is cached in the DelegateHelpers class, so we should
                // use the DelegateHelpers in collectible ALC to make sure the delegates are also stored in the same ALC.
                Assembly assembly = scriptingAssemblyLoadContext.LoadFromAssemblyPath(typeof(Expression).Assembly.Location);
                MakeNewCustomDelegateFuncCollectible =
                assembly.GetType("System.Linq.Expressions.Compiler.DelegateHelpers")
                        .GetMethod("MakeNewCustomDelegate", BindingFlags.NonPublic | BindingFlags.Static).CreateDelegate<Func<Type[], Type>>();

                // Create dummy delegates to force the dynamic Snippets assembly to be loaded in correcet ALCs
                MakeNewCustomDelegateFunc(new[] { typeof(void) });
                {
                    // Ensure the new delegate is placed in the collectible ALC
                    using var ctx = scriptingAssemblyLoadContext.EnterContextualReflection();
                    MakeNewCustomDelegateFuncCollectible(new[] { typeof(void) });
                }
#endif
            }

            internal static Type MakeNewCustomDelegate(Type[] parameters)
            {
#if FLAX_EDITOR
                if (parameters.Any(x => x.IsCollectible))
                    return MakeNewCustomDelegateFuncCollectible(parameters);
#endif
                return MakeNewCustomDelegateFunc(parameters);
            }
#endif
        }

#if !USE_AOT
        /// <summary>
        /// Wrapper class for invoking function pointers from unmanaged code.
        /// </summary>
        internal class ThunkContext
        {
            internal MethodInfo method;
            internal Type[] parameterTypes;
            internal Invoker.InvokeThunkDelegate methodDelegate;
            internal object methodDelegateContext;

            internal ThunkContext(MethodInfo method)
            {
                this.method = method;
                parameterTypes = method.GetParameterTypes();

                // Thunk delegates don't support IsByRef parameters (use generic invocation that handles 'out' and 'ref' prams)
                foreach (var type in parameterTypes)
                {
                    if (type.IsByRef)
                        return;
                }

                List<Type> methodTypes = new List<Type>();
                if (!method.IsStatic)
                    methodTypes.Add(method.DeclaringType);
                if (method.ReturnType != typeof(void))
                    methodTypes.Add(method.ReturnType);
                methodTypes.AddRange(parameterTypes);

                List<Type> genericParamTypes = new List<Type>();
                foreach (var type in methodTypes)
                {
                    if (type.IsByRef)
                        genericParamTypes.Add(type.GetElementType());
                    else if (type.IsPointer)
                        genericParamTypes.Add(typeof(IntPtr));
                    else
                        genericParamTypes.Add(type);
                }

                string invokerTypeName = $"{typeof(Invoker).FullName}+Invoker{(method.IsStatic ? "Static" : "")}{(method.ReturnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                Type invokerType = Type.GetType(invokerTypeName);
                if (invokerType != null)
                {
                    if (genericParamTypes.Count != 0)
                        invokerType = invokerType.MakeGenericType(genericParamTypes.ToArray());
                    methodDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.InvokeThunk), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.InvokeThunkDelegate>();
                    methodDelegateContext = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateInvokerDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });
                }

                if (methodDelegate != null)
                    Assert.IsTrue(methodDelegateContext != null);
            }

            public IntPtr InvokeThunk(ManagedHandle instanceHandle, IntPtr param1, IntPtr param2, IntPtr param3, IntPtr param4, IntPtr param5, IntPtr param6, IntPtr param7)
            {
                IntPtr* nativePtrs = stackalloc IntPtr[] { param1, param2, param3, param4, param5, param6, param7 };
                if (methodDelegate != null)
                {
                    IntPtr returnValue;
                    try
                    {
                        returnValue = methodDelegate(methodDelegateContext, instanceHandle, nativePtrs);
                    }
                    catch (Exception exception)
                    {
                        // Returned exception is the last parameter
                        IntPtr exceptionPtr = nativePtrs[parameterTypes.Length];
                        if (exceptionPtr != IntPtr.Zero)
                            Unsafe.Write<IntPtr>(exceptionPtr.ToPointer(), ManagedHandle.ToIntPtr(exception, GCHandleType.Weak));
                        return IntPtr.Zero;
                    }
                    return returnValue;
                }
                else
                {
                    // The parameters are wrapped (boxed) in GCHandles
                    object returnObject;
                    int numParams = parameterTypes.Length;
                    object[] methodParameters = new object[numParams];
                    for (int i = 0; i < numParams; i++)
                    {
                        IntPtr nativePtr = nativePtrs[i];
                        object managed = null;
                        if (nativePtr != IntPtr.Zero)
                        {
                            Type type = parameterTypes[i];
                            Type elementType = type.GetElementType();
                            if (type.IsByRef)
                            {
                                // References use indirection to support value returning
                                nativePtr = Unsafe.Read<IntPtr>(nativePtr.ToPointer());
                                type = elementType;
                            }
                            if (type.IsArray)
                                managed = MarshalToManaged(nativePtr, type); // Array might be in internal format of custom structs so unbox if need to
                            else
                                managed = ManagedHandle.FromIntPtr(nativePtr).Target;
                        }
                        methodParameters[i] = managed;
                    }

                    try
                    {
                        returnObject = method.Invoke(instanceHandle.IsAllocated ? instanceHandle.Target : null, methodParameters);
                    }
                    catch (Exception exception)
                    {
                        // Returned exception is the last parameter
                        IntPtr exceptionPtr = nativePtrs[numParams];
                        if (exceptionPtr != IntPtr.Zero)
                            Unsafe.Write<IntPtr>(exceptionPtr.ToPointer(), ManagedHandle.ToIntPtr(exception, GCHandleType.Weak));
                        return IntPtr.Zero;
                    }

                    // Marshal reference parameters back to original unmanaged references
                    for (int i = 0; i < numParams; i++)
                    {
                        IntPtr nativePtr = nativePtrs[i];
                        Type type = parameterTypes[i];
                        object managed = methodParameters[i];
                        if (nativePtr != IntPtr.Zero && type.IsByRef)
                        {
                            type = type.GetElementType();
                            if (managed == null)
                                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), IntPtr.Zero);
                            else if (type.IsArray)
                                MarshalToNative(managed, nativePtr, type);
                            else
                                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed, GCHandleType.Weak)));
                        }
                    }

                    // Return value
                    return Invoker.MarshalReturnValueThunkGeneric(method.ReturnType, returnObject);
                }
            }
        }
#endif
    }

    internal class NativeInteropException : Exception
    {
        public NativeInteropException(string message)
        : base(message)
        {
#if !BUILD_RELEASE
            Debug.Logger.LogHandler.LogWrite(LogType.Error, "Native interop exception!");
#endif
        }
    }
}

#endif
