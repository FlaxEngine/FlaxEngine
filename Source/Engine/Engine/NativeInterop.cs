// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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

        private static Dictionary<string, Type> typeCache = new();

        private static IntPtr boolTruePtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc((int)1, GCHandleType.Pinned));
        private static IntPtr boolFalsePtr = ManagedHandle.ToIntPtr(ManagedHandle.Alloc((int)0, GCHandleType.Pinned));

        private static List<ManagedHandle> methodHandles = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegates = new();
        private static Dictionary<Type, ManagedHandle> typeHandleCache = new();
        private static List<ManagedHandle> fieldHandleCache = new();
#if FLAX_EDITOR
        private static List<ManagedHandle> methodHandlesCollectible = new();
        private static ConcurrentDictionary<IntPtr, Delegate> cachedDelegatesCollectible = new();
        private static Dictionary<Type, ManagedHandle> typeHandleCacheCollectible = new();
        private static List<ManagedHandle> fieldHandleCacheCollectible = new();
#endif
        private static Dictionary<object, ManagedHandle> classAttributesCacheCollectible = new();
        private static Dictionary<Assembly, ManagedHandle> assemblyHandles = new();
        private static Dictionary<Type, int> _typeSizeCache = new();

        private static Dictionary<string, IntPtr> loadedNativeLibraries = new();
        internal static Dictionary<string, string> libraryPaths = new();
        private static Dictionary<Assembly, string> assemblyOwnedNativeLibraries = new();
        internal static AssemblyLoadContext scriptingAssemblyLoadContext;

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
        public static TDst[] ConvertArray<TSrc, TDst>(Span<TSrc> src, Func<TSrc, TDst> convertFunc)
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
        public static TDst[] ConvertArray<TSrc, TDst>(TSrc[] src, Func<TSrc, TDst> convertFunc)
        {
            TDst[] dst = new TDst[src.Length];
            for (int i = 0; i < src.Length; i++)
                dst[i] = convertFunc(src[i]);
            return dst;
        }

        private static Type FindType(string typeName)
        {
            if (typeCache.TryGetValue(typeName, out Type type))
                return type;

            type = Type.GetType(typeName, ResolveAssemblyByName, null);
            if (type == null)
            {
                foreach (var assembly in scriptingAssemblyLoadContext.Assemblies)
                {
                    type = assembly.GetType(typeName);
                    if (type != null)
                        break;
                }
            }

            if (type == null)
            {
                string oldTypeName = typeName;
                typeName = typeName.Substring(0, typeName.IndexOf(','));
                type = Type.GetType(typeName, ResolveAssemblyByName, null);
                if (type == null)
                {
                    foreach (var assembly in scriptingAssemblyLoadContext.Assemblies)
                    {
                        type = assembly.GetType(typeName);
                        if (type != null)
                            break;
                    }
                }
                typeName = oldTypeName;
            }

            typeCache.Add(typeName, type);

            return type;
        }

        private static Assembly ResolveAssemblyByName(AssemblyName assemblyName)
        {
            foreach (Assembly assembly in scriptingAssemblyLoadContext.Assemblies)
                if (assembly.GetName() == assemblyName)
                    return assembly;
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
            string[] splits = type.AssemblyQualifiedName.Split(',');
            string @namespace = string.Join('.', splits[0].Split('.').SkipLast(1));
            string className = @namespace.Length > 0 ? splits[0].Substring(@namespace.Length + 1) : splits[0];
            string parentClassName = "";
            if (className.Contains('+'))
            {
                parentClassName = className.Substring(0, className.LastIndexOf('+') + 1);
                className = className.Substring(parentClassName.Length);
            }
            string marshallerName = className + "Marshaller";
            string internalAssemblyQualifiedName = $"{@namespace}.{parentClassName}{marshallerName}+{className}Internal,{String.Join(',', splits.Skip(1))}";
            return FindType(internalAssemblyQualifiedName);
        }

        internal class ReferenceTypePlaceholder
        {
        }

        internal struct ValueTypePlaceholder
        {
        }

        internal delegate object MarshalToManagedDelegate(IntPtr nativePtr, bool byRef);

        internal delegate void MarshalToNativeDelegate(object managedObject, IntPtr nativePtr);

        internal delegate void MarshalToNativeFieldDelegate(FieldInfo field, object fieldOwner, IntPtr nativePtr, out int fieldOffset);

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

        internal static MarshalToNativeFieldDelegate GetToNativeFieldMarshallerDelegate(Type type)
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

        internal static void MarshalToNativeField(FieldInfo field, object fieldOwner, IntPtr nativePtr, out int fieldOffset)
        {
            GetToNativeFieldMarshallerDelegate(fieldOwner.GetType())(field, fieldOwner, nativePtr, out fieldOffset);
        }

        /// <summary>
        /// Helper class for managing stored marshaller delegates for each type.
        /// </summary>
        internal static class MarshalHelper<T>
        {
            private delegate void MarshalToNativeTypedDelegate(ref T managedValue, IntPtr nativePtr);

            private delegate void MarshalToManagedTypedDelegate(ref T managedValue, IntPtr nativePtr, bool byRef);

            internal delegate void MarshalFieldTypedDelegate(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset);

            internal static FieldInfo[] marshallableFields;
            internal static MarshalFieldTypedDelegate[] toManagedFieldMarshallers;
            internal static MarshalFieldTypedDelegate[] toNativeFieldMarshallers;

            private static MarshalToNativeTypedDelegate toNativeTypedMarshaller;
            private static MarshalToManagedTypedDelegate toManagedTypedMarshaller;

            static MarshalHelper()
            {
                Type type = typeof(T);

                // Setup marshallers for managed and native directions
                MethodInfo toManagedMethod;
                if (type.IsValueType)
                    toManagedMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManaged), BindingFlags.Static | BindingFlags.NonPublic);
                else if (type.IsArray && type.GetElementType().IsValueType)
                    toManagedMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type.GetElementType()).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToManagedArray), BindingFlags.Static | BindingFlags.NonPublic);
                else if (type.IsArray && !type.GetElementType().IsValueType)
                    toManagedMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type.GetElementType()).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManagedArray), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    toManagedMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToManaged), BindingFlags.Static | BindingFlags.NonPublic);
                toManagedTypedMarshaller = toManagedMethod.CreateDelegate<MarshalToManagedTypedDelegate>();

                MethodInfo toNativeMethod;
                if (type.IsValueType)
                    toNativeMethod = typeof(MarshalHelperValueType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperValueType<ValueTypePlaceholder>.ToNative), BindingFlags.Static | BindingFlags.NonPublic);
                else
                    toNativeMethod = typeof(MarshalHelperReferenceType<>).MakeGenericType(type).GetMethod(nameof(MarshalHelperReferenceType<ReferenceTypePlaceholder>.ToNative), BindingFlags.Static | BindingFlags.NonPublic);
                toNativeTypedMarshaller = toNativeMethod.CreateDelegate<MarshalToNativeTypedDelegate>();

                if (!type.IsPrimitive && !type.IsPointer && type != typeof(bool))
                {
                    // Setup field-by-field marshallers for reference types or structures containing references
                    marshallableFields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
                    if (type.IsValueType && !marshallableFields.Any(x => (x.FieldType.IsClass && !x.FieldType.IsPointer) || x.FieldType.Name == "Boolean"))
                        marshallableFields = null;
                    else if (!type.IsValueType && !marshallableFields.Any())
                        marshallableFields = null;

                    if (marshallableFields != null)
                    {
                        toManagedFieldMarshallers = new MarshalFieldTypedDelegate[marshallableFields.Length];
                        toNativeFieldMarshallers = new MarshalFieldTypedDelegate[marshallableFields.Length];
                        BindingFlags bindingFlags = BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public;
                        for (int i = 0; i < marshallableFields.Length; i++)
                        {
                            FieldInfo field = marshallableFields[i];
                            Type fieldType = field.FieldType;
                            MethodInfo toManagedFieldMethod;
                            MethodInfo toNativeFieldMethod;

                            if (fieldType.IsPointer)
                            {
                                toManagedFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToManagedFieldPointer), bindingFlags);
                                toNativeFieldMethod = typeof(MarshalHelper<>).MakeGenericType(type).GetMethod(nameof(MarshalHelper<T>.ToNativeFieldPointer), bindingFlags);
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
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedField), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToNativeField), bindingFlags);
                                }
                            }
                            else if (fieldType.IsArray)
                            {
                                Type arrayElementType = fieldType.GetElementType();
                                if (arrayElementType.IsValueType)
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ValueTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ValueTypeField<ValueTypePlaceholder>.ToManagedFieldArray), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeField), bindingFlags);
                                }
                                else
                                {
                                    toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, arrayElementType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedFieldArray), bindingFlags);
                                    toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeField), bindingFlags);
                                }
                            }
                            else
                            {
                                toManagedFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToManagedField), bindingFlags);
                                toNativeFieldMethod = typeof(MarshalHelper<>.ReferenceTypeField<>).MakeGenericType(type, fieldType).GetMethod(nameof(MarshalHelper<T>.ReferenceTypeField<ReferenceTypePlaceholder>.ToNativeField), bindingFlags);
                            }
                            toManagedFieldMarshallers[i] = toManagedFieldMethod.CreateDelegate<MarshalFieldTypedDelegate>();
                            toNativeFieldMarshallers[i] = toNativeFieldMethod.CreateDelegate<MarshalFieldTypedDelegate>();
                        }
                    }
                }
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
                T managed = default;
                if (nativePtr != IntPtr.Zero)
                {
                    Type type = typeof(T);
                    if (type.IsArray)
                        managed = (T)MarshalToManaged(nativePtr, type); // Array might be in internal format of custom structs so unbox if need to
                    else
                        managed = (T)ManagedHandle.FromIntPtr(nativePtr).Target;
                }
                return managed;
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

            internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr nativePtr, out int fieldOffset)
            {
                if (marshallableFields != null)
                {
                    for (int i = 0; i < marshallableFields.Length; i++)
                    {
                        if (marshallableFields[i] == field)
                        {
                            toNativeFieldMarshallers[i](marshallableFields[i], ref fieldOwner, nativePtr, out fieldOffset);
                            return;
                        }
                    }
                }
                throw new NativeInteropException($"Invalid field {field.Name} to marshal for type {typeof(T).Name}");
            }

            private static void ToManagedFieldPointer(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
            {
                ref IntPtr fieldValueRef = ref GetFieldReference<IntPtr>(field, ref fieldOwner);
                fieldValueRef = Unsafe.Read<IntPtr>(fieldPtr.ToPointer());
                fieldOffset = IntPtr.Size;
            }

            private static void ToNativeFieldPointer(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
            {
                ref IntPtr fieldValueRef = ref GetFieldReference<IntPtr>(field, ref fieldOwner);
                Unsafe.Write<IntPtr>(fieldPtr.ToPointer(), fieldValueRef);
                fieldOffset = IntPtr.Size;
            }

            /// <summary>
            /// Returns a reference to the value of the field.
            /// </summary>           
            private static ref TField GetFieldReference<TField>(FieldInfo field, ref T fieldOwner)
            {
                // Get the address of the field, source: https://stackoverflow.com/a/56512720
                if (typeof(T).IsValueType)
                {
                    byte* fieldPtr = (byte*)Unsafe.AsPointer(ref fieldOwner) + (Marshal.ReadInt32(field.FieldHandle.Value + 4 + IntPtr.Size) & 0xFFFFFF);
                    return ref Unsafe.AsRef<TField>(fieldPtr);
                }
                else
                {
                    byte* fieldPtr = (byte*)Unsafe.As<T, IntPtr>(ref fieldOwner) + IntPtr.Size + (Marshal.ReadInt32(field.FieldHandle.Value + 4 + IntPtr.Size) & 0xFFFFFF);
                    return ref Unsafe.AsRef<TField>(fieldPtr);
                }
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

                internal static void ToManagedField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr fieldStartPtr = fieldPtr;
                        fieldPtr = EnsureAlignment(fieldPtr, fieldAlignment);
                        fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();
                    }

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperValueType<TField>.ToManaged(ref fieldValueRef, fieldPtr, false);
                }

                internal static void ToManagedFieldArray(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    // Follows the same marshalling semantics with reference types
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField[] fieldValueRef = ref GetFieldReference<TField[]>(field, ref fieldOwner);
                    MarshalHelperValueType<TField>.ToManagedArray(ref fieldValueRef, Unsafe.Read<IntPtr>(fieldPtr.ToPointer()), false);
                }

                internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<TField>();
                    if (fieldAlignment > 1)
                    {
                        IntPtr startPtr = fieldPtr;
                        fieldPtr = EnsureAlignment(fieldPtr, fieldAlignment);
                        fieldOffset += (fieldPtr - startPtr).ToInt32();
                    }

#if USE_AOT
                    TField fieldValueRef = (TField)field.GetValue(fieldOwner);
#else
                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
#endif
                    MarshalHelperValueType<TField>.ToNative(ref fieldValueRef, fieldPtr);
                }
            }

            private static class NullableValueTypeField<TField>
            {
                static NullableValueTypeField()
                {
                }

                internal static void ToManagedField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = 0;
                }

                internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = 0;
                }
            }

            private static class ReferenceTypeField<TField> where TField : class
            {
                internal static void ToManagedField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToManaged(ref fieldValueRef, Unsafe.Read<IntPtr>(fieldPtr.ToPointer()), false);
                }

                internal static void ToManagedFieldArray(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField[] fieldValueRef = ref GetFieldReference<TField[]>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToManagedArray(ref fieldValueRef, Unsafe.Read<IntPtr>(fieldPtr.ToPointer()), false);
                }

                internal static void ToNativeField(FieldInfo field, ref T fieldOwner, IntPtr fieldPtr, out int fieldOffset)
                {
                    fieldOffset = Unsafe.SizeOf<IntPtr>();
                    IntPtr fieldStartPtr = fieldPtr;
                    fieldPtr = EnsureAlignment(fieldPtr, IntPtr.Size);
                    fieldOffset += (fieldPtr - fieldStartPtr).ToInt32();

                    ref TField fieldValueRef = ref GetFieldReference<TField>(field, ref fieldOwner);
                    MarshalHelperReferenceType<TField>.ToNative(ref fieldValueRef, fieldPtr);
                }
            }
        }

        internal static class MarshalHelperValueType<T> where T : struct
        {
            internal static void ToNativeWrapper(object managedObject, IntPtr nativePtr)
            {
                ToNative(ref Unsafe.Unbox<T>(managedObject), nativePtr);
            }

            internal static void ToNativeFieldWrapper(FieldInfo field, object fieldOwner, IntPtr nativePtr, out int fieldOffset)
            {
                MarshalHelper<T>.ToNativeField(field, ref Unsafe.Unbox<T>(fieldOwner), nativePtr, out fieldOffset);
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                Type type = typeof(T);
                byRef |= type.IsByRef;
                if (byRef)
                {
                    if (type.IsByRef)
                        type = type.GetElementType();
                    Assert.IsTrue(type.IsValueType);
                }

                if (type == typeof(IntPtr) && byRef)
                    managedValue = (T)(object)nativePtr;
                else if (type == typeof(ManagedHandle))
                    managedValue = (T)(object)ManagedHandle.FromIntPtr(nativePtr);
                else if (MarshalHelper<T>.marshallableFields != null)
                {
                    IntPtr fieldPtr = nativePtr;
                    for (int i = 0; i < MarshalHelper<T>.marshallableFields.Length; i++)
                    {
                        MarshalHelper<T>.toManagedFieldMarshallers[i](MarshalHelper<T>.marshallableFields[i], ref managedValue, fieldPtr, out int fieldOffset);
                        fieldPtr += fieldOffset;
                    }
                    Assert.IsTrue((fieldPtr - nativePtr) <= Unsafe.SizeOf<T>());
                }
                else
                    managedValue = Unsafe.Read<T>(nativePtr.ToPointer());
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Marshal.ReadIntPtr(nativePtr);

                Type elementType = typeof(T);
                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    if (ArrayFactory.GetMarshalledType(elementType) == elementType)
                        managedValue = Unsafe.As<T[]>(managedArray.ToArray<T>());
                    else if (elementType.IsValueType)
                        managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray));
                    else
                        managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray.ToSpan<IntPtr>()));
                }
                else
                    managedValue = null;
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                if (typeof(T).IsByRef)
                    throw new NotImplementedException();

                if (MarshalHelper<T>.marshallableFields != null)
                {
                    IntPtr fieldPtr = nativePtr;
                    for (int i = 0; i < MarshalHelper<T>.marshallableFields.Length; i++)
                    {
                        MarshalHelper<T>.toNativeFieldMarshallers[i](MarshalHelper<T>.marshallableFields[i], ref managedValue, nativePtr, out int fieldOffset);
                        nativePtr += fieldOffset;
                    }
                    Assert.IsTrue((nativePtr - fieldPtr) <= Unsafe.SizeOf<T>());
                }
                else
                    Unsafe.AsRef<T>(nativePtr.ToPointer()) = managedValue;
            }
        }

        internal static class MarshalHelperReferenceType<T> where T : class
        {
            internal static void ToNativeWrapper(object managedObject, IntPtr nativePtr)
            {
                T managedValue = Unsafe.As<T>(managedObject);
                ToNative(ref managedValue, nativePtr);
            }

            internal static void ToNativeFieldWrapper(FieldInfo field, object managedObject, IntPtr nativePtr, out int fieldOffset)
            {
                T managedValue = Unsafe.As<T>(managedObject);
                MarshalHelper<T>.ToNativeField(field, ref managedValue, nativePtr, out fieldOffset);
            }

            internal static void ToManaged(ref T managedValue, IntPtr nativePtr, bool byRef)
            {
                Type type = typeof(T);
                if (byRef)
                    nativePtr = Marshal.ReadIntPtr(nativePtr);

                if (type == typeof(string))
                    managedValue = Unsafe.As<T>(ManagedString.ToManaged(nativePtr));
                else if (nativePtr == IntPtr.Zero)
                    managedValue = null;
                else if (type.IsClass)
                    managedValue = Unsafe.As<T>(ManagedHandle.FromIntPtr(nativePtr).Target);
                else if (type.IsInterface) // Dictionary
                    managedValue = Unsafe.As<T>(ManagedHandle.FromIntPtr(nativePtr).Target);
                else
                    throw new NotImplementedException();
            }

            internal static void ToManagedArray(ref T[] managedValue, IntPtr nativePtr, bool byRef)
            {
                if (byRef)
                    nativePtr = Marshal.ReadIntPtr(nativePtr);

                if (nativePtr != IntPtr.Zero)
                {
                    ManagedArray managedArray = Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr(nativePtr).Target);
                    managedValue = Unsafe.As<T[]>(MarshalHelper<T>.ToManagedArray(managedArray.ToSpan<IntPtr>()));
                }
                else
                    managedValue = null;
            }

            internal static void ToNative(ref T managedValue, IntPtr nativePtr)
            {
                Type type = typeof(T);

                IntPtr managedPtr;
                if (type == typeof(string))
                    managedPtr = ManagedString.ToNativeWeak(managedValue as string);
                else if (type.IsPointer)
                {
                    if (Pointer.Unbox(managedValue) == null)
                        managedPtr = IntPtr.Zero;
                    else if (managedValue is FlaxEngine.Object flaxObj)
                        managedPtr = FlaxEngine.Object.GetUnmanagedPtr(flaxObj);
                    else
                        managedPtr = ManagedHandle.ToIntPtr(managedValue, GCHandleType.Weak);
                }
                else if (type == typeof(Type))
                    managedPtr = managedValue != null ? ManagedHandle.ToIntPtr(GetTypeGCHandle((Type)(object)managedValue)) : IntPtr.Zero;
                else if (type.IsArray)
                {
                    if (managedValue == null)
                        managedPtr = IntPtr.Zero;
                    else
                    {
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
                }
                else
                    managedPtr = managedValue != null ? ManagedHandle.ToIntPtr(managedValue, GCHandleType.Weak) : IntPtr.Zero;

                Unsafe.Write<IntPtr>(nativePtr.ToPointer(), managedPtr);
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
                if (invokeDelegate == null)
                {
                    List<Type> methodTypes = new List<Type>();
                    if (!method.IsStatic)
                        methodTypes.Add(method.DeclaringType);
                    if (returnType != typeof(void))
                        methodTypes.Add(returnType);
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

                    string invokerTypeName = $"{typeof(Invoker).FullName}+Invoker{(method.IsStatic ? "Static" : "")}{(returnType != typeof(void) ? "Ret" : "NoRet")}{parameterTypes.Length}{(genericParamTypes.Count > 0 ? "`" + genericParamTypes.Count : "")}";
                    Type invokerType = Type.GetType(invokerTypeName);
                    if (invokerType != null)
                    {
                        if (genericParamTypes.Count != 0)
                            invokerType = invokerType.MakeGenericType(genericParamTypes.ToArray());
                        invokeDelegate = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.MarshalAndInvoke), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Invoker.MarshalAndInvokeDelegate>();
                        delegInvoke = invokerType.GetMethod(nameof(Invoker.InvokerStaticNoRet0.CreateDelegate), BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { method });
                    }
                }

                outDeleg = invokeDelegate;
                outDelegInvoke = delegInvoke;
                return outDeleg != null;
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

            internal FieldHolder(FieldInfo field, Type type)
            {
                this.field = field;
                toNativeMarshaller = GetToNativeFieldMarshallerDelegate(type);
            }
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

            private delegate TInternal ToNativeDelegate<T, TInternal>(T value);

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
                int size = Unsafe.SizeOf<T>();
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

        /// <summary>
        /// Returns a static ManagedHandle for given Type, and caches it if needed.
        /// </summary>
        internal static ManagedHandle GetTypeGCHandle(Type type)
        {
            if (typeHandleCache.TryGetValue(type, out ManagedHandle handle))
                return handle;
#if FLAX_EDITOR
            if (typeHandleCacheCollectible.TryGetValue(type, out handle))
                return handle;
#endif

            handle = ManagedHandle.Alloc(type);
#if FLAX_EDITOR
            if (type.IsCollectible) // check if generic parameters are also collectible?
                typeHandleCacheCollectible.Add(type, handle);
            else
#endif
            {
                typeHandleCache.Add(type, handle);
            }

            return handle;
        }

        internal static int GetTypeSize(Type type)
        {
            if (!_typeSizeCache.TryGetValue(type, out var size))
            {
                try
                {
                    var marshalType = type;
                    if (type.IsEnum)
                        marshalType = type.GetEnumUnderlyingType();
                    size = Marshal.SizeOf(marshalType);
                }
                catch
                {
                    // Workaround the issue where structure defined within generic type instance (eg. MyType<int>.MyStruct) fails to get size
                    // https://github.com/dotnet/runtime/issues/46426
                    var obj = Activator.CreateInstance(type);
                    size = Marshal.SizeOf(obj);
                }
                _typeSizeCache.Add(type, size);
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
                            Marshal.WriteIntPtr(exceptionPtr, ManagedHandle.ToIntPtr(exception, GCHandleType.Weak));
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
                                nativePtr = Marshal.ReadIntPtr(nativePtr);
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
                            Marshal.WriteIntPtr(exceptionPtr, ManagedHandle.ToIntPtr(exception, GCHandleType.Weak));
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
                                Marshal.WriteIntPtr(nativePtr, IntPtr.Zero);
                            else if (type.IsArray)
                                MarshalToNative(managed, nativePtr, type);
                            else
                                Marshal.WriteIntPtr(nativePtr, ManagedHandle.ToIntPtr(ManagedHandle.Alloc(managed, GCHandleType.Weak)));
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
