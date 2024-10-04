// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_NETCORE
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using FlaxEngine.Utilities;

#pragma warning disable 1591

namespace FlaxEngine.Interop
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeClassDefinitions
    {
        internal ManagedHandle typeHandle;
        internal IntPtr nativePointer;
        internal IntPtr name;
        internal IntPtr fullname;
        internal IntPtr @namespace;
        internal uint typeAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeMethodDefinitions
    {
        internal IntPtr name;
        internal int numParameters;
        internal ManagedHandle typeHandle;
        internal uint methodAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeFieldDefinitions
    {
        internal IntPtr name;
        internal ManagedHandle fieldHandle;
        internal ManagedHandle fieldTypeHandle;
        internal int fieldOffset;
        internal uint fieldAttributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativePropertyDefinitions
    {
        internal IntPtr name;
        internal ManagedHandle propertyHandle;
        internal ManagedHandle getterHandle;
        internal ManagedHandle setterHandle;
        internal uint getterAttributes;
        internal uint setterAttributes;
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct NativeVariant
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeVariantType
        {
            internal VariantUtils.VariantType types;
            internal IntPtr TypeName; // char*
        }

        [FieldOffset(0)]
        NativeVariantType Type;

        [FieldOffset(8)]
        byte AsBool;

        [FieldOffset(8)]
        short AsInt16;

        [FieldOffset(8)]
        ushort AsUint16;

        [FieldOffset(8)]
        int AsInt;

        [FieldOffset(8)]
        uint AsUint;

        [FieldOffset(8)]
        long AsInt64;

        [FieldOffset(8)]
        ulong AsUint64;

        [FieldOffset(8)]
        float AsFloat;

        [FieldOffset(8)]
        double AsDouble;

        [FieldOffset(8)]
        IntPtr AsPointer;

        [FieldOffset(8)]
        int AsData0;

        [FieldOffset(12)]
        int AsData1;

        [FieldOffset(16)]
        int AsData2;

        [FieldOffset(20)]
        int AsData3;

        [FieldOffset(24)]
        int AsData4;

        [FieldOffset(28)]
        int AsData5;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeVersion
    {
        internal int _Major;
        internal int _Minor;
        internal int _Build;
        internal int _Revision;

        internal NativeVersion(Version ver)
        {
            _Major = ver.Major;
            _Minor = ver.Minor;
            _Build = ver.Build;
            _Revision = ver.Revision;
        }

        internal Version GetVersion()
        {
            return new Version(_Major, _Minor, _Build, _Revision);
        }
    }

    unsafe partial class NativeInterop
    {
        [LibraryImport("FlaxEngine", EntryPoint = "NativeInterop_CreateClass", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void NativeInterop_CreateClass(ref NativeClassDefinitions managedClass, ManagedHandle assemblyHandle);

        internal enum MTypes : uint
        {
            End = 0x00,
            Void = 0x01,
            Boolean = 0x02,
            Char = 0x03,
            I1 = 0x04,
            U1 = 0x05,
            I2 = 0x06,
            U2 = 0x07,
            I4 = 0x08,
            U4 = 0x09,
            I8 = 0x0a,
            U8 = 0x0b,
            R4 = 0x0c,
            R8 = 0x0d,
            String = 0x0e,
            Ptr = 0x0f,
            ByRef = 0x10,
            ValueType = 0x11,
            Class = 0x12,
            Var = 0x13,
            Array = 0x14,
            GenericInst = 0x15,
            TypeByRef = 0x16,
            I = 0x18,
            U = 0x19,
            Fnptr = 0x1b,
            Object = 0x1c,
            SzArray = 0x1d,
            MVar = 0x1e,
            CmodReqd = 0x1f,
            CmodOpt = 0x20,
            Internal = 0x21,
            Modifier = 0x40,
            Sentinel = 0x41,
            Pinned = 0x45,
            Enum = 0x55,
        };

        [UnmanagedCallersOnly]
        internal static void RegisterNativeLibrary(IntPtr moduleNamePtr, IntPtr modulePathPtr)
        {
            string moduleName = Marshal.PtrToStringAnsi(moduleNamePtr);
            string modulePath = Marshal.PtrToStringUni(modulePathPtr);
            libraryPaths[moduleName] = modulePath;
        }

        [UnmanagedCallersOnly]
        internal static void* AllocMemory(int size, bool coTaskMem)
        {
            if (coTaskMem)
                return Marshal.AllocCoTaskMem(size).ToPointer();
            else
                return NativeMemory.AlignedAlloc((UIntPtr)size, 16);
        }

        [UnmanagedCallersOnly]
        internal static void FreeMemory(void* ptr, bool coTaskMem)
        {
            if (coTaskMem)
                Marshal.FreeCoTaskMem(new IntPtr(ptr));
            else
                NativeMemory.AlignedFree(ptr);
        }

        private static Assembly GetOwningAssembly(Type type)
        {
            Assembly assembly = null;
            if (type.IsGenericType && !type.Assembly.IsCollectible)
            {
                // The owning assembly of a generic type with type arguments referencing
                // collectible assemblies must be one of the collectible assemblies.
                foreach (var genericType in type.GetGenericArguments())
                {
                    if (genericType.Assembly.IsCollectible)
                    {
                        assembly = genericType.Assembly;
                        break;
                    }
                }
            }
            if (assembly == null)
                assembly = type.Assembly;
            return assembly;
        }

        private static NativeClassDefinitions CreateNativeClassDefinitions(Type type, out ManagedHandle assemblyHandle)
        {
            assemblyHandle = GetAssemblyHandle(GetOwningAssembly(type));
            return CreateNativeClassDefinitions(type);
        }

        private static NativeClassDefinitions CreateNativeClassDefinitions(Type type)
        {
            return new NativeClassDefinitions()
            {
                typeHandle = RegisterType(type).handle,
                name = NativeAllocStringAnsi(type.Name),
                fullname = NativeAllocStringAnsi(type.GetTypeName()),
                @namespace = NativeAllocStringAnsi(type.Namespace ?? ""),
                typeAttributes = (uint)type.Attributes,
            };
        }

        private static NativeClassDefinitions CreateNativeClassDefinitions(Type type, ManagedHandle typeHandle, out ManagedHandle assemblyHandle)
        {
            assemblyHandle = GetAssemblyHandle(GetOwningAssembly(type));
            return new NativeClassDefinitions()
            {
                typeHandle = typeHandle,
                name = NativeAllocStringAnsi(type.Name),
                fullname = NativeAllocStringAnsi(type.GetTypeName()),
                @namespace = NativeAllocStringAnsi(type.Namespace ?? ""),
                typeAttributes = (uint)type.Attributes,
            };
        }

        [UnmanagedCallersOnly]
        internal static void GetManagedClasses(ManagedHandle assemblyHandle, NativeClassDefinitions** managedClasses, int* managedClassCount)
        {
            Assembly assembly = Unsafe.As<Assembly>(assemblyHandle.Target);
            Type[] assemblyTypes = GetAssemblyTypes(assembly);

            *managedClasses = (NativeClassDefinitions*)NativeAlloc(assemblyTypes.Length, Unsafe.SizeOf<NativeClassDefinitions>());
            *managedClassCount = assemblyTypes.Length;
            Span<NativeClassDefinitions> span = new Span<NativeClassDefinitions>(*managedClasses, assemblyTypes.Length);
            for (int i = 0; i < assemblyTypes.Length; i++)
            {
                Type type = assemblyTypes[i];
                ref var managedClass = ref span[i];
                managedClass = CreateNativeClassDefinitions(type);
            }
        }

        [UnmanagedCallersOnly]
        internal static void RegisterManagedClassNativePointers(NativeClassDefinitions** managedClasses, int managedClassCount)
        {
            Span<NativeClassDefinitions> span = new Span<NativeClassDefinitions>(Unsafe.Read<IntPtr>(managedClasses).ToPointer(), managedClassCount);
            foreach (ref NativeClassDefinitions managedClass in span)
            {
                TypeHolder typeHolder = Unsafe.As<TypeHolder>(managedClass.typeHandle.Target);
                typeHolder.managedClassPointer = managedClass.nativePointer;
            }
        }

        [UnmanagedCallersOnly]
        internal static void GetManagedClassFromType(ManagedHandle typeHandle, NativeClassDefinitions* managedClass, ManagedHandle* assemblyHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            *managedClass = CreateNativeClassDefinitions(type, out ManagedHandle handle);
            *assemblyHandle = handle;
        }

        private static void RegisterNativeClassFromType(TypeHolder typeHolder, ManagedHandle typeHandle)
        {
            NativeClassDefinitions managedClass = CreateNativeClassDefinitions(typeHolder.type, typeHandle, out ManagedHandle assemblyHandle);
            NativeInterop_CreateClass(ref managedClass, assemblyHandle);
            typeHolder.managedClassPointer = managedClass.nativePointer;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassMethods(ManagedHandle typeHandle, NativeMethodDefinitions** classMethods, int* classMethodsCount)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);

            var methods = new List<MethodInfo>();
            var staticMethods = type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            var instanceMethods = type.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            methods.AddRange(staticMethods);
            methods.AddRange(instanceMethods);

            var arr = (NativeMethodDefinitions*)NativeAlloc(methods.Count, Unsafe.SizeOf<NativeMethodDefinitions>());
            for (int i = 0; i < methods.Count; i++)
            {
                var method = methods[i];
                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeMethodDefinitions>() * i);
                var classMethod = new NativeMethodDefinitions
                {
                    name = NativeAllocStringAnsi(method.Name),
                    numParameters = method.GetParameters().Length,
                    methodAttributes = (uint)method.Attributes,
                };
                classMethod.typeHandle = GetMethodGCHandle(method);
                Unsafe.Write(ptr.ToPointer(), classMethod);
            }
            *classMethods = arr;
            *classMethodsCount = methods.Count;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassFields(ManagedHandle typeHandle, NativeFieldDefinitions** classFields, int* classFieldsCount)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            var fields = type.GetFields(BindingFlags.Static | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            NativeFieldDefinitions* arr = (NativeFieldDefinitions*)NativeAlloc(fields.Length, Unsafe.SizeOf<NativeFieldDefinitions>());
            for (int i = 0; i < fields.Length; i++)
            {
                FieldHolder fieldHolder = new FieldHolder(fields[i], type);

                ManagedHandle fieldHandle = ManagedHandle.Alloc(fieldHolder);
#if FLAX_EDITOR
                if (type.IsCollectible)
                    fieldHandleCacheCollectible.Add(fieldHandle);
                else
#endif
                {
                    fieldHandleCache.Add(fieldHandle);
                }

                NativeFieldDefinitions classField = new NativeFieldDefinitions()
                {
                    name = NativeAllocStringAnsi(fieldHolder.field.Name),
                    fieldHandle = fieldHandle,
                    fieldTypeHandle = GetTypeManagedHandle(fieldHolder.field.FieldType),
                    fieldOffset = fieldHolder.fieldOffset,
                    fieldAttributes = (uint)fieldHolder.field.Attributes,
                };
                Unsafe.Write(IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativeFieldDefinitions>() * i).ToPointer(), classField);
            }
            *classFields = arr;
            *classFieldsCount = fields.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassProperties(ManagedHandle typeHandle, NativePropertyDefinitions** classProperties, int* classPropertiesCount)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            var properties = type.GetProperties(BindingFlags.Static | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

            var arr = (NativePropertyDefinitions*)NativeAlloc(properties.Length, Unsafe.SizeOf<NativePropertyDefinitions>());
            for (int i = 0; i < properties.Length; i++)
            {
                var property = properties[i];

                ManagedHandle propertyHandle = ManagedHandle.Alloc(property);
#if FLAX_EDITOR
                if (type.IsCollectible)
                    propertyHandleCacheCollectible.Add(propertyHandle);
                else
#endif
                {
                    propertyHandleCache.Add(propertyHandle);
                }

                IntPtr ptr = IntPtr.Add(new IntPtr(arr), Unsafe.SizeOf<NativePropertyDefinitions>() * i);

                var getterMethod = property.GetGetMethod(true);
                var setterMethod = property.GetSetMethod(true);

                var classProperty = new NativePropertyDefinitions
                {
                    name = NativeAllocStringAnsi(property.Name),
                    propertyHandle = propertyHandle,
                };
                if (getterMethod != null)
                {
                    classProperty.getterHandle = GetMethodGCHandle(getterMethod);
                    classProperty.getterAttributes = (uint)getterMethod.Attributes;
                }
                if (setterMethod != null)
                {
                    classProperty.setterHandle = GetMethodGCHandle(setterMethod);
                    classProperty.setterAttributes = (uint)setterMethod.Attributes;
                }
                Unsafe.Write(ptr.ToPointer(), classProperty);
            }
            *classProperties = arr;
            *classPropertiesCount = properties.Length;
        }

        internal static void GetAttributes(object[] attributeValues, ManagedHandle** classAttributes, int* classAttributesCount)
        {
            ManagedHandle* arr = (ManagedHandle*)NativeAlloc(attributeValues.Length, Unsafe.SizeOf<ManagedHandle>());
            for (int i = 0; i < attributeValues.Length; i++)
            {
                if (!classAttributesCacheCollectible.TryGetValue(attributeValues[i], out ManagedHandle attributeHandle))
                {
                    attributeHandle = ManagedHandle.Alloc(attributeValues[i]);
                    classAttributesCacheCollectible.Add(attributeValues[i], attributeHandle);
                }
                arr[i] = attributeHandle;
            }
            *classAttributes = arr;
            *classAttributesCount = attributeValues.Length;
        }

        [UnmanagedCallersOnly]
        internal static void GetClassAttributes(ManagedHandle typeHandle, ManagedHandle** classAttributes, int* classAttributesCount)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            object[] attributeValues = type.GetCustomAttributes(false);
            GetAttributes(attributeValues, classAttributes, classAttributesCount);
        }

        [UnmanagedCallersOnly]
        internal static void GetMethodAttributes(ManagedHandle methodHandle, ManagedHandle** classAttributes, int* classAttributesCount)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            object[] attributeValues = methodHolder.method.GetCustomAttributes(false);
            GetAttributes(attributeValues, classAttributes, classAttributesCount);
        }

        [UnmanagedCallersOnly]
        internal static void GetFieldAttributes(ManagedHandle fieldHandle, ManagedHandle** classAttributes, int* classAttributesCount)
        {
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            object[] attributeValues = field.field.GetCustomAttributes(false);
            GetAttributes(attributeValues, classAttributes, classAttributesCount);
        }

        [UnmanagedCallersOnly]
        internal static void GetPropertyAttributes(ManagedHandle propertyHandle, ManagedHandle** classAttributes, int* classAttributesCount)
        {
            PropertyInfo property = Unsafe.As<PropertyInfo>(propertyHandle.Target);
            object[] attributeValues = property.GetCustomAttributes(false);
            GetAttributes(attributeValues, classAttributes, classAttributesCount);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetCustomAttribute(ManagedHandle typeHandle, ManagedHandle attributeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            var attributes = type.GetCustomAttributes(false);
            object attrib;
            if (attributeHandle.IsAllocated)
            {
                // Check for certain attribute type
                Type attributeType = Unsafe.As<TypeHolder>(attributeHandle.Target);
                attrib = attributes.FirstOrDefault(x => x.GetType() == attributeType);
            }
            else
            {
                // Check if has any attribute
                attrib = attributes.FirstOrDefault();
            }
            if (attrib != null)
            {
                if (!classAttributesCacheCollectible.TryGetValue(attrib, out var handle))
                {
                    handle = ManagedHandle.Alloc(attrib);
                    classAttributesCacheCollectible.Add(attrib, handle);
                }
                return handle;
            }
            return new ManagedHandle();
        }

        [UnmanagedCallersOnly]
        internal static void GetClassInterfaces(ManagedHandle typeHandle, IntPtr* classInterfaces, int* classInterfacesCount)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            Type[] interfaces = type.GetInterfaces();

            // Match mono_class_get_interfaces which doesn't return interfaces from base class
            var baseTypeInterfaces = type.BaseType?.GetInterfaces();
            if (baseTypeInterfaces != null)
            {
                if (baseTypeInterfaces.Length == interfaces.Length)
                {
                    // Both base and this type have the same amount of interfaces so assume that this one has none unique
                    interfaces = Array.Empty<Type>();
                }
                else if (baseTypeInterfaces.Length != 0)
                {
                    // Count unique interface types
                    int uniqueCount = interfaces.Length;
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        for (int j = 0; j < baseTypeInterfaces.Length; j++)
                        {
                            if (interfaces[i] == baseTypeInterfaces[j])
                            {
                                uniqueCount--;
                                break;
                            }
                        }
                    }

                    // Get only unique interface types
                    var unique = new Type[uniqueCount];
                    uniqueCount = 0;
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        bool isUnique = true;
                        for (int j = 0; j < baseTypeInterfaces.Length; j++)
                        {
                            if (interfaces[i] == baseTypeInterfaces[j])
                            {
                                isUnique = false;
                                break;
                            }
                        }
                        if (isUnique)
                            unique[uniqueCount++] = interfaces[i];
                    }
                    interfaces = unique;
                }
            }

            IntPtr arr = (IntPtr)NativeAlloc(interfaces.Length, IntPtr.Size);
            for (int i = 0; i < interfaces.Length; i++)
            {
                ManagedHandle handle = GetTypeManagedHandle(interfaces[i]);
                Unsafe.Write<ManagedHandle>(IntPtr.Add(arr, IntPtr.Size * i).ToPointer(), handle);
            }
            *classInterfaces = arr;
            *classInterfacesCount = interfaces.Length;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetMethodReturnType(ManagedHandle methodHandle)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            Type returnType = methodHolder.returnType;
            return GetTypeManagedHandle(returnType);
        }

        [UnmanagedCallersOnly]
        internal static void GetMethodParameterTypes(ManagedHandle methodHandle, IntPtr* typeHandles)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            Type[] parameterTypes = methodHolder.parameterTypes;
            IntPtr arr = (IntPtr)NativeAlloc(parameterTypes.Length, IntPtr.Size);
            for (int i = 0; i < parameterTypes.Length; i++)
            {
                ManagedHandle typeHandle = GetTypeManagedHandle(parameterTypes[i]);
                Unsafe.Write<ManagedHandle>(IntPtr.Add(new IntPtr(arr), IntPtr.Size * i).ToPointer(), typeHandle);
            }
            *typeHandles = arr;
        }

        /// <summary>
        /// Returns pointer to the string's internal structure, containing the buffer and length of the string.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static char* GetStringPointer(ManagedHandle stringHandle, int* len)
        {
            string str = Unsafe.As<string>(stringHandle.Target);
            *len = str.Length;
            fixed (char* p = str)
                return p;
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewObject(ManagedHandle typeHandle)
        {
            TypeHolder typeHolder = Unsafe.As<TypeHolder>(typeHandle.Target);
            try
            {
                object value = typeHolder.CreateObject();
                return ManagedHandle.Alloc(value);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            return new ManagedHandle();
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewArray(ManagedHandle typeHandle, long size)
        {
            Type elementType = Unsafe.As<TypeHolder>(typeHandle.Target);
            Type marshalledType = ArrayFactory.GetMarshalledType(elementType);
            Type arrayType = ArrayFactory.GetArrayType(elementType);
            if (marshalledType.IsValueType)
            {
                ManagedArray managedArray = ManagedArray.AllocateNewArray((int)size, arrayType, marshalledType);
                return ManagedHandle.Alloc(managedArray);
            }
            else
            {
                Array arr = ArrayFactory.CreateArray(elementType, size);
                ManagedArray managedArray = ManagedArray.WrapNewArray(arr, arrayType);
                return ManagedHandle.Alloc(managedArray);
            }
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArrayTypeFromElementType(ManagedHandle elementTypeHandle)
        {
            Type elementType = Unsafe.As<TypeHolder>(elementTypeHandle.Target);
            Type arrayType = ArrayFactory.GetArrayType(elementType);
            return GetTypeManagedHandle(arrayType);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArrayTypeFromWrappedArray(ManagedHandle arrayHandle)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            Type elementType = managedArray.ArrayType.GetElementType();
            Type arrayType = ArrayFactory.GetArrayType(elementType);
            return GetTypeManagedHandle(arrayType);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetArrayPointer(ManagedHandle arrayHandle)
        {
            if (!arrayHandle.IsAllocated)
                return IntPtr.Zero;
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            if (managedArray.Length == 0)
                return IntPtr.Zero;
            return managedArray.Pointer;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetArray(ManagedHandle handle)
        {
            if (!handle.IsAllocated)
                return IntPtr.Zero;
            object value = handle.Target;
            if (value is ManagedArray)
                return (IntPtr)handle;
            if (value is Array)
                return Invoker.MarshalReturnValueGeneric(value.GetType(), value);
            return IntPtr.Zero;
        }

        [UnmanagedCallersOnly]
        internal static int GetArrayLength(ManagedHandle arrayHandle)
        {
            if (!arrayHandle.IsAllocated)
                return 0;
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            return managedArray.Length;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetStringEmpty()
        {
            return ManagedHandle.ToIntPtr(ManagedString.EmptyStringHandle);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewStringUTF16(char* text, int length)
        {
            return ManagedString.ToNativeWeak(new string(new ReadOnlySpan<char>(text, length)));
        }

        [UnmanagedCallersOnly]
        internal static IntPtr NewStringUTF8(sbyte* text, int length)
        {
            return ManagedString.ToNativeWeak(new string(text, 0, length, System.Text.Encoding.UTF8));
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetObjectType(ManagedHandle handle)
        {
            object obj = handle.Target;
            Type classType = obj.GetType();
            if (classType == typeof(ManagedArray))
                classType = ((ManagedArray)obj).ArrayType;
            return GetTypeManagedHandle(classType);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetObjectString(ManagedHandle handle)
        {
            object obj = handle.Target;
            string result = string.Empty;
            try
            {
                result = obj.ToString();
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            return ManagedHandle.ToIntPtr(result);
        }

        [UnmanagedCallersOnly]
        internal static int GetObjectHashCode(ManagedHandle handle)
        {
            object obj = handle.Target;
            int result = 0;
            try
            {
                result = obj.GetHashCode();
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            return result;
        }

        /// <summary>
        /// Creates a managed copy of the value, and stores it in a boxed reference.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static ManagedHandle BoxValue(ManagedHandle typeHandle, IntPtr valuePtr)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            object value = MarshalToManaged(valuePtr, type);
            return ManagedHandle.Alloc(value, GCHandleType.Weak);
        }

        /// <summary>
        /// Returns the address of the boxed value type.
        /// </summary>
        [UnmanagedCallersOnly]
        internal static IntPtr UnboxValue(ManagedHandle handle)
        {
            object value = handle.Target;
            Type type = value.GetType();
            if (!type.IsValueType)
                return ManagedHandle.ToIntPtr(handle);
            return ValueTypeUnboxer.GetPointer(value, type);
        }

        [UnmanagedCallersOnly]
        internal static void RaiseException(ManagedHandle exceptionHandle)
        {
            Exception exception = Unsafe.As<Exception>(exceptionHandle.Target);
            throw exception;
        }

        [UnmanagedCallersOnly]
        internal static void ObjectInit(ManagedHandle objectHandle)
        {
            try
            {
                object obj = objectHandle.Target;
                Type type = obj.GetType();
                ConstructorInfo ctor = type.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, Type.EmptyTypes, null);
                if (ctor == null)
                    throw new Exception($"Missing empty constructor in type '{type}'.");
                ctor.Invoke(obj, null);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetObjectClass(ManagedHandle objectHandle)
        {
            object obj = objectHandle.Target;
            TypeHolder typeHolder = GetTypeHolder(obj.GetType());
            return typeHolder.managedClassPointer;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr InvokeMethod(ManagedHandle instanceHandle, ManagedHandle methodHandle, IntPtr paramPtr, IntPtr exceptionPtr)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
#if !USE_AOT
            if (methodHolder.TryGetDelegate(out var methodDelegate, out var methodDelegateContext))
            {
                // Fast path, invoke the method with minimal allocations
                IntPtr returnValue;
                try
                {
                    returnValue = methodDelegate(methodDelegateContext, instanceHandle, paramPtr);
                }
                catch (Exception exception)
                {
                    if (exceptionPtr != IntPtr.Zero)
                        Unsafe.Write<IntPtr>(exceptionPtr.ToPointer(), ManagedHandle.ToIntPtr(exception, GCHandleType.Weak));
                    return IntPtr.Zero;
                }
                return returnValue;
            }
            else
#endif
            {
                // Slow path, method parameters needs to be stored in heap
                object returnObject;
                int numParams = methodHolder.parameterTypes.Length;
                object[] methodParameters = new object[numParams];

                for (int i = 0; i < numParams; i++)
                {
                    IntPtr nativePtr = Unsafe.Read<IntPtr>((IntPtr.Add(paramPtr, sizeof(IntPtr) * i)).ToPointer());
                    methodParameters[i] = MarshalToManaged(nativePtr, methodHolder.parameterTypes[i]);
                }

                try
                {
                    returnObject = methodHolder.method.Invoke(instanceHandle.IsAllocated ? instanceHandle.Target : null, methodParameters);
                }
                catch (Exception exception)
                {
                    // The internal exception thrown in MethodInfo.Invoke is caught here
                    Exception realException = exception;
                    if (exception.InnerException != null && exception.TargetSite.ReflectedType.Name == "MethodInvoker")
                        realException = exception.InnerException;

                    if (exceptionPtr != IntPtr.Zero)
                        Unsafe.Write<IntPtr>(exceptionPtr.ToPointer(), ManagedHandle.ToIntPtr(realException, GCHandleType.Weak));
                    else
                        throw realException;
                    return IntPtr.Zero;
                }

                // Marshal reference parameters back to original unmanaged references
                for (int i = 0; i < numParams; i++)
                {
                    Type parameterType = methodHolder.parameterTypes[i];
                    if (parameterType.IsByRef)
                    {
                        IntPtr nativePtr = Unsafe.Read<IntPtr>((IntPtr.Add(paramPtr, sizeof(IntPtr) * i)).ToPointer());
                        MarshalToNative(methodParameters[i], nativePtr, parameterType.GetElementType());
                    }
                }

                // Return value
                return Invoker.MarshalReturnValueGeneric(methodHolder.returnType, returnObject);
            }
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetThunk(ManagedHandle methodHandle)
        {
#if USE_AOT
            Debug.LogError("GetThunk is not supported in C# AOT mode");
            return IntPtr.Zero;
#else
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);

            // Wrap the method call, this is needed to get the object instance from ManagedHandle and to pass the exception back to native side
            ThunkContext context = new ThunkContext((MethodInfo)methodHolder.method);
            Delegate methodDelegate = typeof(ThunkContext).GetMethod(nameof(ThunkContext.InvokeThunk)).CreateDelegate<InvokeThunkDelegate>(context);
            IntPtr functionPtr = Marshal.GetFunctionPointerForDelegate(methodDelegate);

            // Keep a reference to the delegate to prevent it from being garbage collected
#if FLAX_EDITOR
            if (methodHolder.method.IsCollectible)
                cachedDelegatesCollectible[functionPtr] = methodDelegate;
            else
#endif
            {
                cachedDelegates[functionPtr] = methodDelegate;
            }
            return functionPtr;
#endif
        }

        [UnmanagedCallersOnly]
        internal static void FieldSetValue(ManagedHandle fieldOwnerHandle, ManagedHandle fieldHandle, IntPtr valuePtr)
        {
            object fieldOwner = fieldOwnerHandle.IsAllocated ? fieldOwnerHandle.Target : null;
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            object value = MarshalToManaged(valuePtr, field.field.FieldType);
            field.field.SetValue(fieldOwner, value);
        }

        [UnmanagedCallersOnly]
        internal static int FieldGetOffset(ManagedHandle fieldHandle)
        {
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            return field.fieldOffset;
        }

        [UnmanagedCallersOnly]
        internal static void FieldGetValue(ManagedHandle fieldOwnerHandle, ManagedHandle fieldHandle, IntPtr valuePtr)
        {
            object fieldOwner = fieldOwnerHandle.Target;
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            field.toNativeMarshaller(field.field, field.fieldOffset, fieldOwner, valuePtr, out int fieldSize);
        }

        [UnmanagedCallersOnly]
        internal static void FieldGetValueReference(ManagedHandle fieldOwnerHandle, ManagedHandle fieldHandle, int fieldOffset, IntPtr valuePtr)
        {
            object fieldOwner = fieldOwnerHandle.Target;
            IntPtr fieldRef;
#if USE_AOT
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            fieldRef = IntPtr.Zero;
            Debug.LogError("Not supported FieldGetValueReference");
#else
            if (fieldOwner.GetType().IsValueType)
            {
                fieldRef = FieldHelper.GetValueTypeFieldReference<object, IntPtr>(fieldOffset, ref fieldOwner);
            }
            else
            {
                fieldRef = FieldHelper.GetReferenceTypeFieldReference<object, IntPtr>(fieldOffset, ref fieldOwner);
            }
#endif
            Unsafe.Write<IntPtr>(valuePtr.ToPointer(), fieldRef);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr FieldGetValueBoxed(ManagedHandle fieldOwnerHandle, ManagedHandle fieldHandle)
        {
            FieldHolder field = Unsafe.As<FieldHolder>(fieldHandle.Target);
            object fieldOwner = field.field.IsStatic ? null : fieldOwnerHandle.Target;
            object fieldValue = field.field.GetValue(fieldOwner);
            return Invoker.MarshalReturnValueGeneric(field.field.FieldType, fieldValue);
        }

        [UnmanagedCallersOnly]
        internal static void WriteArrayReference(ManagedHandle arrayHandle, IntPtr valueHandle, int index)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            managedArray.ToSpan<IntPtr>()[index] = valueHandle;
        }

        [UnmanagedCallersOnly]
        internal static void WriteArrayReferences(ManagedHandle arrayHandle, IntPtr spanPtr, int spanLength)
        {
            ManagedArray managedArray = Unsafe.As<ManagedArray>(arrayHandle.Target);
            var unmanagedSpan = new Span<IntPtr>(spanPtr.ToPointer(), spanLength);
            unmanagedSpan.CopyTo(managedArray.ToSpan<IntPtr>());
        }

        [UnmanagedCallersOnly]
        internal static void GetAssemblyName(ManagedHandle assemblyHandle, IntPtr* assemblyName, IntPtr* assemblyFullName)
        {
            Assembly assembly = Unsafe.As<Assembly>(assemblyHandle.Target);
            *assemblyName = NativeAllocStringAnsi(assembly.GetName().Name);
            *assemblyFullName = NativeAllocStringAnsi(assembly.FullName);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle LoadAssemblyImage(IntPtr assemblyPathPtr)
        {
            if (!firstAssemblyLoaded)
            {
                // This assembly was already loaded when initializing the host context
                firstAssemblyLoaded = true;

                Assembly flaxEngineAssembly = AssemblyLoadContext.Default.Assemblies.First(x => x.GetName().Name == "FlaxEngine.CSharp");
                return GetAssemblyHandle(flaxEngineAssembly);
            }
            try
            {
                string assemblyPath = Marshal.PtrToStringUni(assemblyPathPtr);

                Assembly assembly;
#if FLAX_EDITOR
                // Load assembly from loaded bytes to prevent file locking in Editor
                var assemblyBytes = File.ReadAllBytes(assemblyPath);
                using MemoryStream stream = new MemoryStream(assemblyBytes);
                var pdbPath = Path.ChangeExtension(assemblyPath, "pdb");
                if (File.Exists(pdbPath))
                {
                    // Load including debug symbols
                    using FileStream pdbStream = new FileStream(Path.ChangeExtension(assemblyPath, "pdb"), FileMode.Open);
                    assembly = scriptingAssemblyLoadContext.LoadFromStream(stream, pdbStream);
                }
                else
                {
                    assembly = scriptingAssemblyLoadContext.LoadFromStream(stream);
                }
#else
                // Load assembly from file
                assembly = scriptingAssemblyLoadContext.LoadFromAssemblyPath(assemblyPath);
#endif
                if (assembly == null)
                    return new ManagedHandle();
                NativeLibrary.SetDllImportResolver(assembly, NativeLibraryImportResolver);

                // Assemblies loaded via streams have no Location: https://github.com/dotnet/runtime/issues/12822
                AssemblyLocations.Add(assembly.FullName, assemblyPath);

                return GetAssemblyHandle(assembly);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            return new ManagedHandle();
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetAssemblyByName(IntPtr namePtr)
        {
            string name = Marshal.PtrToStringAnsi(namePtr);
            Assembly assembly = Utils.GetAssemblies().FirstOrDefault(x => x.GetName().Name == name);
            if (assembly == null)
                return new ManagedHandle();
            return GetAssemblyHandle(assembly);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetRuntimeInformation()
        {
            return NativeAllocStringAnsi(System.Runtime.InteropServices.RuntimeInformation.FrameworkDescription);
        }

        [UnmanagedCallersOnly]
        internal static void CloseAssembly(ManagedHandle assemblyHandle)
        {
            Assembly assembly = Unsafe.As<Assembly>(assemblyHandle.Target);
            assemblyHandle.Free();
            assemblyHandles.Remove(assembly);

            AssemblyLocations.Remove(assembly.FullName);

            // Unload native library handles associated for this assembly
            string nativeLibraryName = assemblyOwnedNativeLibraries.GetValueOrDefault(assembly);
            if (nativeLibraryName != null && loadedNativeLibraries.TryGetValue(nativeLibraryName, out IntPtr nativeLibrary))
            {
                NativeLibrary.Free(nativeLibrary);
                loadedNativeLibraries.Remove(nativeLibraryName);
            }
            if (nativeLibraryName != null)
                libraryPaths.Remove(nativeLibraryName);
        }

        [UnmanagedCallersOnly]
        internal static void ReloadScriptingAssemblyLoadContext()
        {
#if FLAX_EDITOR
            // Clear all caches which might hold references to assemblies in collectible ALC
            cachedDelegatesCollectible.Clear();
            foreach (var pair in managedTypesCollectible)
                pair.Value.handle.Free();
            managedTypesCollectible.Clear();
            foreach (var handle in methodHandlesCollectible)
                handle.Free();
            methodHandlesCollectible.Clear();
            foreach (var handle in fieldHandleCacheCollectible)
                handle.Free();
            fieldHandleCacheCollectible.Clear();
            foreach (var handle in propertyHandleCacheCollectible)
                handle.Free();
            propertyHandleCacheCollectible.Clear();

            _typeSizeCache.Clear();

            foreach (var pair in classAttributesCacheCollectible)
                pair.Value.Free();
            classAttributesCacheCollectible.Clear();

            FlaxEngine.Json.JsonSerializer.ResetCache();

            // Unload the ALC
            bool unloading = true;
            scriptingAssemblyLoadContext.Unloading += (alc) => { unloading = false; };
            scriptingAssemblyLoadContext.Unload();

            while (unloading)
                System.Threading.Thread.Sleep(1);

            InitScriptingAssemblyLoadContext();
            DelegateHelpers.InitMethods();
#endif
        }

        [UnmanagedCallersOnly]
        internal static int NativeSizeOf(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            Type nativeType = GetInternalType(type) ?? type;
            if (nativeType == typeof(Version))
                nativeType = typeof(NativeVersion);
            int size;
            if (nativeType.IsClass)
                size = sizeof(IntPtr);
            else
                size = GetTypeSize(nativeType);
            return size;
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsSubclassOf(ManagedHandle typeHandle, ManagedHandle otherTypeHandle, byte checkInterfaces)
        {
            if (typeHandle == otherTypeHandle)
                return 1;

            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            Type otherType = Unsafe.As<TypeHolder>(otherTypeHandle.Target);

            if (type == otherType)
                return 1;

            if (checkInterfaces != 0 && otherType.IsInterface)
            {
                if (type.GetInterfaces().Contains(otherType))
                    return 1;
            }

            return type.IsSubclassOf(otherType) ? (byte)1 : (byte)0;
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsAssignableFrom(ManagedHandle typeHandle, ManagedHandle otherTypeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            Type otherType = Unsafe.As<TypeHolder>(otherTypeHandle.Target);
            return (byte)(type.IsAssignableFrom(otherType) ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsValueType(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            return (byte)(type.IsValueType ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte TypeIsEnum(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            return (byte)(type.IsEnum ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetClassParent(ManagedHandle typeHandle)
        {
            TypeHolder typeHolder = Unsafe.As<TypeHolder>(typeHandle.Target);
            TypeHolder baseTypeHolder = GetTypeHolder(typeHolder.type.BaseType);
            return baseTypeHolder.managedClassPointer;
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetElementClass(ManagedHandle typeHandle)
        {
            TypeHolder typeHolder = Unsafe.As<TypeHolder>(typeHandle.Target);
            TypeHolder elementTypeHolder = GetTypeHolder(typeHolder.type.GetElementType());
            return elementTypeHolder.managedClassPointer;
        }

        [UnmanagedCallersOnly]
        internal static byte GetMethodParameterIsOut(ManagedHandle methodHandle, int parameterNum)
        {
            MethodHolder methodHolder = Unsafe.As<MethodHolder>(methodHandle.Target);
            ParameterInfo parameterInfo = methodHolder.method.GetParameters()[parameterNum];
            return (byte)(parameterInfo.IsOut ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetNullReferenceException()
        {
            var exception = new NullReferenceException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetException(IntPtr msgPtr)
        {
            string msg = Marshal.PtrToStringAnsi(msgPtr);
            var exception = new Exception(msg);
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetNotSupportedException()
        {
            var exception = new NotSupportedException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArgumentNullException()
        {
            var exception = new ArgumentNullException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArgumentException()
        {
            var exception = new ArgumentException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle GetArgumentOutOfRangeException()
        {
            var exception = new ArgumentOutOfRangeException();
            return ManagedHandle.Alloc(exception, GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewGCHandle(ManagedHandle valueHandle, byte pinned)
        {
            return ManagedHandle.Alloc(valueHandle.Target, pinned != 0 ? GCHandleType.Pinned : GCHandleType.Normal);
        }

        [UnmanagedCallersOnly]
        internal static ManagedHandle NewGCHandleWeak(ManagedHandle valueHandle, byte trackResurrection)
        {
            return ManagedHandle.Alloc(valueHandle.Target, trackResurrection != 0 ? GCHandleType.WeakTrackResurrection : GCHandleType.Weak);
        }

        [UnmanagedCallersOnly]
        internal static void FreeGCHandle(ManagedHandle valueHandle)
        {
            valueHandle.Free();
        }

        [UnmanagedCallersOnly]
        internal static void GCCollect(int generation, int mode, bool blocking, bool compacting)
        {
            GC.Collect(generation, (GCCollectionMode)mode, blocking, compacting);
        }

        [UnmanagedCallersOnly]
        internal static int GCMaxGeneration()
        {
            return GC.MaxGeneration;
        }

        [UnmanagedCallersOnly]
        internal static void GCWaitForPendingFinalizers()
        {
            GC.WaitForPendingFinalizers();
        }

        [UnmanagedCallersOnly]
        internal static IntPtr GetTypeClass(ManagedHandle typeHandle)
        {
            TypeHolder typeHolder = Unsafe.As<TypeHolder>(typeHandle.Target);
            if (typeHolder.type.IsByRef)
            {
                // Drop reference type (&) to get actual value type
                return GetTypeHolder(typeHolder.type.GetElementType()).managedClassPointer;
            }
            return typeHolder.managedClassPointer;
        }

        [UnmanagedCallersOnly]
        internal static bool GetTypeIsPointer(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            return type.IsPointer;
        }

        [UnmanagedCallersOnly]
        internal static bool GetTypeIsReference(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            return !type.IsValueType; // Maybe also type.IsByRef?
        }

        [UnmanagedCallersOnly]
        internal static uint GetTypeMTypesEnum(ManagedHandle typeHandle)
        {
            Type type = Unsafe.As<TypeHolder>(typeHandle.Target);
            if (type.IsByRef)
                type = type.GetElementType(); // Drop reference type (&) to get actual value type
            MTypes monoType;
            switch (type)
            {
            case Type _ when type == typeof(void):
                monoType = MTypes.Void;
                break;
            case Type _ when type == typeof(bool):
                monoType = MTypes.Boolean;
                break;
            case Type _ when type == typeof(sbyte):
            case Type _ when type == typeof(short):
                monoType = MTypes.I2;
                break;
            case Type _ when type == typeof(byte):
            case Type _ when type == typeof(ushort):
                monoType = MTypes.U2;
                break;
            case Type _ when type == typeof(int):
                monoType = MTypes.I4;
                break;
            case Type _ when type == typeof(uint):
                monoType = MTypes.U4;
                break;
            case Type _ when type == typeof(long):
                monoType = MTypes.I8;
                break;
            case Type _ when type == typeof(ulong):
                monoType = MTypes.U8;
                break;
            case Type _ when type == typeof(float):
                monoType = MTypes.R4;
                break;
            case Type _ when type == typeof(double):
                monoType = MTypes.R8;
                break;
            case Type _ when type == typeof(string):
                monoType = MTypes.String;
                break;
            case Type _ when type == typeof(IntPtr):
                monoType = MTypes.Ptr;
                break;
            case Type _ when type.IsPointer:
                monoType = MTypes.Ptr;
                break;
            case Type _ when type.IsEnum:
                monoType = MTypes.Enum;
                break;
            case Type _ when type.IsArray:
            case Type _ when type == typeof(ManagedArray):
                monoType = MTypes.Array;
                break;
            case Type _ when type.IsValueType && !type.IsEnum && !type.IsPrimitive:
                monoType = MTypes.ValueType;
                break;
            case Type _ when type.IsGenericType:
                monoType = MTypes.GenericInst;
                break;
            case Type _ when type.IsClass:
                monoType = MTypes.Object;
                break;
            default: throw new NativeInteropException($"Unsupported type '{type.FullName}'");
            }
            return (uint)monoType;
        }
    }
}

#endif
