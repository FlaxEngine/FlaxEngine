// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_NETCORE
using System;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Diagnostics;
using System.Collections.Generic;

namespace FlaxEngine.Interop
{
    unsafe partial class NativeInterop
    {
        /// <summary>
        /// Helper class for invoking managed methods from delegates.
        /// </summary>
        internal static class Invoker
        {
            // TODO: Use .NET8 Unsafe.BitCast<TRet, ValueType>(returnValue) for more efficient casting of value types over boxing cast

            internal static class InvokerMarshallers<T>
            {
                internal delegate IntPtr Delegate(ref T value);
                internal static Delegate deleg;
                internal static Delegate delegThunk;

                static InvokerMarshallers()
                {
                    Type type = typeof(T);
                    if (type == typeof(string))
                        deleg = typeof(Invoker).GetMethod(nameof(MarshalReturnValueString), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(ManagedHandle))
                        deleg = typeof(Invoker).GetMethod(nameof(MarshalReturnValueManagedHandle), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(Type))
                        deleg = typeof(Invoker).GetMethod(nameof(MarshalReturnValueType), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type.IsArray)
                        deleg = typeof(Invoker).GetMethod(nameof(MarshalReturnValueArray), BindingFlags.Static | BindingFlags.NonPublic).MakeGenericMethod(type).CreateDelegate<Delegate>();
                    else if (type == typeof(bool))
                        deleg = typeof(Invoker).GetMethod(nameof(MarshalReturnValueBool), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else
                        deleg = typeof(Invoker).GetMethod(nameof(MarshalReturnValueWrapped), BindingFlags.Static | BindingFlags.NonPublic).MakeGenericMethod(type).CreateDelegate<Delegate>();

                    if (type == typeof(string))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueString), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(ManagedHandle))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueManagedHandle), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(Type))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueType), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type.IsArray)
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueArray), BindingFlags.Static | BindingFlags.NonPublic).MakeGenericMethod(type).CreateDelegate<Delegate>();
                    else if (type == typeof(System.Boolean))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueMonoBoolean), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(IntPtr))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueIntPtr), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(System.Int16))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueInt16), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(System.Int32))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueInt32), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(System.Int64))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueInt64), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(System.UInt16))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueUInt16), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(System.UInt32))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueUInt32), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else if (type == typeof(System.UInt64))
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueUInt64), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<Delegate>();
                    else
                        delegThunk = typeof(Invoker).GetMethod(nameof(MarshalReturnValueWrapped), BindingFlags.Static | BindingFlags.NonPublic).MakeGenericMethod(type).CreateDelegate<Delegate>();
                }
            }

            internal static IntPtr MarshalReturnValueString(ref string returnValue)
            {
                return returnValue != null ? ManagedString.ToNativeWeak(returnValue) : IntPtr.Zero;
            }

            internal static IntPtr MarshalReturnValueManagedHandle(ref ManagedHandle returnValue)
            {
                return returnValue != null ? ManagedHandle.ToIntPtr(returnValue) : IntPtr.Zero;
            }

            internal static IntPtr MarshalReturnValueType(ref Type returnValue)
            {
                return returnValue != null ? ManagedHandle.ToIntPtr(GetTypeManagedHandle(returnValue)) : IntPtr.Zero;
            }

            internal static IntPtr MarshalReturnValueArray<TRet>(ref TRet returnValue)
            {
                if (returnValue == null)
                    return IntPtr.Zero;
                var elementType = typeof(TRet).GetElementType();
                if (ArrayFactory.GetMarshalledType(elementType) == elementType)
                    return ManagedHandle.ToIntPtr(ManagedArray.WrapNewArray(Unsafe.As<Array>(returnValue)), GCHandleType.Weak);
                return ManagedHandle.ToIntPtr(ManagedArrayToGCHandleWrappedArray(Unsafe.As<Array>(returnValue)), GCHandleType.Weak);
            }

            internal static IntPtr MarshalReturnValueBool(ref bool returnValue)
            {
                return returnValue ? boolTruePtr : boolFalsePtr;
            }

            internal static IntPtr MarshalReturnValueIntPtr(ref IntPtr returnValue)
            {
                return returnValue;
            }

            internal static IntPtr MarshalReturnValueMonoBoolean(ref bool returnValue)
            {
                return returnValue ? 1 : 0;
            }

            internal static IntPtr MarshalReturnValueInt16(ref Int16 returnValue)
            {
                return returnValue;
            }

            internal static IntPtr MarshalReturnValueInt32(ref Int32 returnValue)
            {
                return returnValue;
            }

            internal static IntPtr MarshalReturnValueInt64(ref Int64 returnValue)
            {
                return new IntPtr(returnValue);
            }

            internal static IntPtr MarshalReturnValueUInt16(ref UInt16 returnValue)
            {
                return returnValue;
            }

            internal static IntPtr MarshalReturnValueUInt32(ref UInt32 returnValue)
            {
                return new IntPtr(returnValue);
            }

            internal static IntPtr MarshalReturnValueUInt64(ref UInt64 returnValue)
            {
                return new IntPtr((long)returnValue);
            }

            internal static IntPtr MarshalReturnValueWrapped<TRet>(ref TRet returnValue)
            {
                return returnValue != null ? ManagedHandle.ToIntPtr(returnValue, GCHandleType.Weak) : IntPtr.Zero;
            }

            internal static IntPtr MarshalReturnValue<TRet>(ref TRet returnValue)
            {
                return InvokerMarshallers<TRet>.deleg(ref returnValue);
            }

            internal static IntPtr MarshalReturnValueGeneric(Type returnType, object returnObject)
            {
                if (returnObject == null)
                    return IntPtr.Zero;
                if (returnType == typeof(string))
                    return ManagedString.ToNativeWeak(Unsafe.As<string>(returnObject));
                if (returnType == typeof(ManagedHandle))
                    return ManagedHandle.ToIntPtr((ManagedHandle)(object)returnObject);
                if (returnType == typeof(bool))
                    return (bool)returnObject ? boolTruePtr : boolFalsePtr;
                if (returnType == typeof(Type) || returnType == typeof(TypeHolder))
                    return ManagedHandle.ToIntPtr(GetTypeManagedHandle(Unsafe.As<Type>(returnObject)));
                if (returnType.IsArray && ArrayFactory.GetMarshalledType(returnType.GetElementType()) == returnType.GetElementType())
                    return ManagedHandle.ToIntPtr(ManagedArray.WrapNewArray(Unsafe.As<Array>(returnObject)), GCHandleType.Weak);
                if (returnType.IsArray)
                    return ManagedHandle.ToIntPtr(ManagedArrayToGCHandleWrappedArray(Unsafe.As<Array>(returnObject)), GCHandleType.Weak);
                return ManagedHandle.ToIntPtr(returnObject, GCHandleType.Weak);
            }

            internal static IntPtr MarshalReturnValueThunk<TRet>(ref TRet returnValue)
            {
                return InvokerMarshallers<TRet>.delegThunk(ref returnValue);
            }

            internal static IntPtr MarshalReturnValueThunkGeneric(Type returnType, object returnObject)
            {
                if (returnObject == null)
                    return IntPtr.Zero;
                if (returnType == typeof(string))
                    return ManagedString.ToNativeWeak(Unsafe.As<string>(returnObject));
                if (returnType == typeof(IntPtr))
                    return (IntPtr)(object)returnObject;
                if (returnType == typeof(ManagedHandle))
                    return ManagedHandle.ToIntPtr((ManagedHandle)(object)returnObject);
                if (returnType == typeof(Type) || returnType == typeof(TypeHolder))
                    return returnObject != null ? ManagedHandle.ToIntPtr(GetTypeManagedHandle(Unsafe.As<Type>(returnObject))) : IntPtr.Zero;
                if (returnType.IsArray)
                {
                    var elementType = returnType.GetElementType();
                    if (ArrayFactory.GetMarshalledType(elementType) == elementType)
                        return ManagedHandle.ToIntPtr(ManagedArray.WrapNewArray(Unsafe.As<Array>(returnObject)), GCHandleType.Weak);
                    return ManagedHandle.ToIntPtr(ManagedArrayToGCHandleWrappedArray(Unsafe.As<Array>(returnObject)), GCHandleType.Weak);
                }
                // Match Mono bindings and pass value as pointer to prevent boxing it
                if (returnType == typeof(System.Boolean))
                    return new IntPtr(((System.Boolean)(object)returnObject) ? 1 : 0);
                if (returnType == typeof(System.Int16))
                    return new IntPtr((int)(System.Int16)(object)returnObject);
                if (returnType == typeof(System.Int32))
                    return new IntPtr((int)(System.Int32)(object)returnObject);
                if (returnType == typeof(System.Int64))
                    return new IntPtr((long)(System.Int64)(object)returnObject);
                if (returnType == typeof(System.UInt16))
                    return (IntPtr)new UIntPtr((ulong)(System.UInt16)(object)returnObject);
                if (returnType == typeof(System.UInt32))
                    return (IntPtr)new UIntPtr((ulong)(System.UInt32)(object)returnObject);
                if (returnType == typeof(System.UInt64))
                    return (IntPtr)new UIntPtr((ulong)(System.UInt64)(object)returnObject);
                return ManagedHandle.ToIntPtr(returnObject, GCHandleType.Weak);
            }

#if !USE_AOT
            internal delegate IntPtr MarshalAndInvokeDelegate(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr);

            internal delegate IntPtr InvokeThunkDelegate(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs);

            /// <summary>
            /// Casts managed pointer to unmanaged pointer.
            /// </summary>
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            internal static T* ToPointer<T>(IntPtr ptr) where T : unmanaged
            {
                return (T*)ptr.ToPointer();
            }

            internal static MethodInfo ToPointerMethod = typeof(Invoker).GetMethod(nameof(Invoker.ToPointer), BindingFlags.Static | BindingFlags.NonPublic);

            /// <summary>
            /// Creates a delegate for invoker to pass parameters as references.
            /// </summary>
            internal static Delegate CreateDelegateFromMethod(MethodInfo method, bool passParametersByRef = true)
            {
                Type[] methodParameters;
                if (method.IsStatic)
                    methodParameters = method.GetParameterTypes();
                else
                    methodParameters = method.GetParameters().Select(x => x.ParameterType).Prepend(method.DeclaringType).ToArray();

                // Pass delegate parameters by reference
                Type[] delegateParameters = methodParameters.Select(x => x.IsPointer ? typeof(IntPtr) : x).Select(x => passParametersByRef && !x.IsByRef ? x.MakeByRefType() : x).ToArray();
                if (!method.IsStatic && passParametersByRef)
                    delegateParameters[0] = method.DeclaringType;

                // Convert unmanaged pointer parameters to IntPtr
                ParameterExpression[] parameterExpressions = delegateParameters.Select(Expression.Parameter).ToArray();
                Expression[] callExpressions = new Expression[methodParameters.Length];
                for (int i = 0; i < methodParameters.Length; i++)
                {
                    Type parameterType = methodParameters[i];
                    if (parameterType.IsPointer)
                        callExpressions[i] = Expression.Call(null, ToPointerMethod.MakeGenericMethod(parameterType.GetElementType()), parameterExpressions[i]);
                    else
                        callExpressions[i] = parameterExpressions[i];
                }

                // Create and compile the delegate
                MethodCallExpression callDelegExp;
                if (method.IsStatic)
                    callDelegExp = Expression.Call(null, method, callExpressions.ToArray());
                else
                    callDelegExp = Expression.Call(parameterExpressions[0], method, callExpressions.Skip(1).ToArray());
                Type delegateType = DelegateHelpers.MakeNewCustomDelegate(delegateParameters.Append(method.ReturnType).ToArray());
                return Expression.Lambda(delegateType, callDelegExp, parameterExpressions).Compile();
            }

            internal static class InvokerNoRet0<TInstance>
            {
                internal delegate void InvokerDelegate(object instance);

                internal delegate void ThunkInvokerDelegate(object instance);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    deleg(instancePtr.Target);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    deleg(instancePtr.Target);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet1<TInstance, T1>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1);

                internal delegate void ThunkInvokerDelegate(object instance, T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    deleg(instancePtr.Target, ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);

                    deleg(instancePtr.Target, param1);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet2<TInstance, T1, T2>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1, ref T2 param2);

                internal delegate void ThunkInvokerDelegate(object instance, T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    deleg(instancePtr.Target, ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);

                    deleg(instancePtr.Target, param1, param2);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet3<TInstance, T1, T2, T3>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3);

                internal delegate void ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    deleg(instancePtr.Target, ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);

                    deleg(instancePtr.Target, param1, param2, param3);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet4<TInstance, T1, T2, T3, T4>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);

                internal delegate void ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);
                    IntPtr param4Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    T4 param4 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero)
                        MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    deleg(instancePtr.Target, ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (param4Ptr != IntPtr.Zero && types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);
                    T4 param4 = MarshalHelper<T4>.ToManagedUnbox(paramPtrs[3]);

                    deleg(instancePtr.Target, param1, param2, param3, param4);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerStaticNoRet0
            {
                internal delegate void InvokerDelegate();

                internal delegate void ThunkInvokerDelegate();

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    deleg();

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    deleg();

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerStaticNoRet1<T1>
            {
                internal delegate void InvokerDelegate(ref T1 param1);

                internal delegate void ThunkInvokerDelegate(T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    deleg(ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);

                    deleg(param1);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerStaticNoRet2<T1, T2>
            {
                internal delegate void InvokerDelegate(ref T1 param1, ref T2 param2);

                internal delegate void ThunkInvokerDelegate(T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    deleg(ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);

                    deleg(param1, param2);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerStaticNoRet3<T1, T2, T3>
            {
                internal delegate void InvokerDelegate(ref T1 param1, ref T2 param2, ref T3 param3);

                internal delegate void ThunkInvokerDelegate(T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    deleg(ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);

                    deleg(param1, param2, param3);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerStaticNoRet4<T1, T2, T3, T4>
            {
                internal delegate void InvokerDelegate(ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);

                internal delegate void ThunkInvokerDelegate(T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);
                    IntPtr param4Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    T4 param4 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero)
                        MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    deleg(ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (param4Ptr != IntPtr.Zero && types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return IntPtr.Zero;
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);
                    T4 param4 = MarshalHelper<T4>.ToManagedUnbox(paramPtrs[3]);

                    deleg(param1, param2, param3, param4);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerRet0<TInstance, TRet>
            {
                internal delegate TRet InvokerDelegate(object instance);

                internal delegate TRet ThunkInvokerDelegate(object instance);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    TRet ret = deleg(instancePtr.Target);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    TRet ret = deleg(instancePtr.Target);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerRet1<TInstance, TRet, T1>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1);

                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    TRet ret = deleg(instancePtr.Target, ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);

                    TRet ret = deleg(instancePtr.Target, param1);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerRet2<TInstance, TRet, T1, T2>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1, ref T2 param2);

                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    TRet ret = deleg(instancePtr.Target, ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);

                    TRet ret = deleg(instancePtr.Target, param1, param2);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerRet3<TInstance, TRet, T1, T2, T3>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3);

                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    TRet ret = deleg(instancePtr.Target, ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);

                    TRet ret = deleg(instancePtr.Target, param1, param2, param3);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerRet4<TInstance, TRet, T1, T2, T3, T4>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);

                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);
                    IntPtr param4Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    T4 param4 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero)
                        MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    TRet ret = deleg(instancePtr.Target, ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (param4Ptr != IntPtr.Zero && types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);
                    T4 param4 = MarshalHelper<T4>.ToManagedUnbox(paramPtrs[3]);

                    TRet ret = deleg(instancePtr.Target, param1, param2, param3, param4);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerStaticRet0<TRet>
            {
                internal delegate TRet InvokerDelegate();

                internal delegate TRet ThunkInvokerDelegate();

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    TRet ret = deleg();

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    TRet ret = deleg();

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerStaticRet1<TRet, T1>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1);

                internal delegate TRet ThunkInvokerDelegate(T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    TRet ret = deleg(ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);

                    TRet ret = deleg(param1);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerStaticRet2<TRet, T1, T2>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1, ref T2 param2);

                internal delegate TRet ThunkInvokerDelegate(T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    TRet ret = deleg(ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);

                    TRet ret = deleg(param1, param2);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerStaticRet3<TRet, T1, T2, T3>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1, ref T2 param2, ref T3 param3);

                internal delegate TRet ThunkInvokerDelegate(T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    TRet ret = deleg(ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);

                    TRet ret = deleg(param1, param2, param3);

                    return MarshalReturnValueThunk(ref ret);
                }
            }

            internal static class InvokerStaticRet4<TRet, T1, T2, T3, T4>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);

                internal delegate TRet ThunkInvokerDelegate(T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameterTypes(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                //[DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, ManagedHandle instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);
                    IntPtr param4Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    T4 param4 = default;
                    if (param1Ptr != IntPtr.Zero)
                        MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero)
                        MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero)
                        MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero)
                        MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    TRet ret = deleg(ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (param1Ptr != IntPtr.Zero && types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (param2Ptr != IntPtr.Zero && types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (param3Ptr != IntPtr.Zero && types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (param4Ptr != IntPtr.Zero && types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return MarshalReturnValue(ref ret);
                }

                //[DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, ManagedHandle instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = MarshalHelper<T1>.ToManagedUnbox(paramPtrs[0]);
                    T2 param2 = MarshalHelper<T2>.ToManagedUnbox(paramPtrs[1]);
                    T3 param3 = MarshalHelper<T3>.ToManagedUnbox(paramPtrs[2]);
                    T4 param4 = MarshalHelper<T4>.ToManagedUnbox(paramPtrs[3]);

                    TRet ret = deleg(param1, param2, param3, param4);

                    return MarshalReturnValueThunk(ref ret);
                }
            }
#endif
        }
    }
}

#endif
