#if USE_NETCORE

using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Diagnostics;

namespace FlaxEngine
{
    internal unsafe static partial class NativeInterop
    {
        internal static class Invoker
        {
            internal delegate IntPtr MarshalAndInvokeDelegate(object delegateContext, IntPtr instancePtr, IntPtr paramPtr);
            internal delegate IntPtr InvokeThunkDelegate(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs);

            internal static T* ToPointer<T>(IntPtr ptr) where T : unmanaged
            {
                return (T*)ptr.ToPointer();
            }

            /// <summary>
            /// Creates a delegate for invoker to pass parameters as references.
            /// </summary>
            internal static Delegate CreateDelegateFromMethod(MethodInfo method, bool byRefParameters = true)
            {
                Type[] methodParameters = method.GetParameters().Select(x => x.ParameterType).ToArray();
                Type[] delegateParameters = method.GetParameters().Select(x => x.ParameterType.IsPointer ? typeof(IntPtr) : x.ParameterType).ToArray();
                if (byRefParameters)
                    delegateParameters = delegateParameters.Select(x => x.IsByRef ? x : x.MakeByRefType()).ToArray();

                List<ParameterExpression> parameterExpressions = new List<ParameterExpression>(delegateParameters.Select(x => Expression.Parameter(x)));
                List<Expression> callExpressions = new List<Expression>(parameterExpressions);
                for (int i = 0; i < callExpressions.Count; i++)
                    if (methodParameters[i].IsPointer)
                        callExpressions[i] = Expression.Call(null, typeof(Invoker).GetMethod(nameof(Invoker.ToPointer), BindingFlags.Static | BindingFlags.NonPublic).MakeGenericMethod(methodParameters[i].GetElementType()), parameterExpressions[i]);

                var firstParamExp = Expression.Parameter(method.DeclaringType);
                var callDelegExp = Expression.Call(!method.IsStatic ? firstParamExp : null, method, callExpressions.ToArray());

                if (!method.IsStatic)
                    parameterExpressions.Insert(0, firstParamExp);
                var lambda = Expression.Lambda(callDelegExp, parameterExpressions.ToArray());

                return lambda.Compile();
            }

            internal static IntPtr MarshalReturnValue<TRet>(ref TRet returnValue)
            {
                if (typeof(TRet) == typeof(string))
                    return ManagedString.ToNative(Unsafe.As<string>(returnValue));
                else if (typeof(TRet) == typeof(IntPtr))
                    return (IntPtr)(object)returnValue;
                else if (typeof(TRet) == typeof(bool))
                    return (bool)(object)returnValue ? boolTruePtr : boolFalsePtr;
                else if (typeof(TRet) == typeof(Type))
                    return returnValue != null ? GCHandle.ToIntPtr(GetTypeGCHandle(Unsafe.As<Type>(returnValue))) : IntPtr.Zero;
                else if (typeof(TRet) == typeof(object[]))
                    return returnValue != null ? GCHandle.ToIntPtr(GCHandle.Alloc(ManagedArray.WrapNewArray(ManagedArrayToGCHandleArray(Unsafe.As<object[]>(returnValue))), GCHandleType.Weak)) : IntPtr.Zero;
                else
                    return returnValue != null ? GCHandle.ToIntPtr(GCHandle.Alloc(returnValue, GCHandleType.Weak)) : IntPtr.Zero;
            }


            internal static class InvokerNoRet0<TInstance>
            {
                internal delegate void InvokerDelegate(object instance);
                internal delegate void ThunkInvokerDelegate(object instance);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet1<TInstance, T1>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1);
                internal delegate void ThunkInvokerDelegate(object instance, T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, param1);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet2<TInstance, T1, T2>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1, ref T2 param2);
                internal delegate void ThunkInvokerDelegate(object instance, T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, param1, param2);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet3<TInstance, T1, T2, T3>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3);
                internal delegate void ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, param1, param2, param3);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerNoRet4<TInstance, T1, T2, T3, T4>
            {
                internal delegate void InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);
                internal delegate void ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
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
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero) MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);
                    T4 param4 = paramPtrs[3] != IntPtr.Zero ? (T4)GCHandle.FromIntPtr(paramPtrs[3]).Target : default(T4);

                    deleg(GCHandle.FromIntPtr(instancePtr).Target, param1, param2, param3, param4);

                    return IntPtr.Zero;
                }
            }

            internal static class InvokerStaticNoRet0
            {
                internal delegate void InvokerDelegate();
                internal delegate void ThunkInvokerDelegate();

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    deleg();

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
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
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    deleg(ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);

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
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    deleg(ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);

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
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    deleg(ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);

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
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
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
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero) MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    deleg(ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return IntPtr.Zero;
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);
                    T4 param4 = paramPtrs[3] != IntPtr.Zero ? (T4)GCHandle.FromIntPtr(paramPtrs[3]).Target : default(T4);

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
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerRet1<TInstance, TRet, T1>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1);
                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, param1);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerRet2<TInstance, TRet, T1, T2>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1, ref T2 param2);
                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, param1, param2);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerRet3<TInstance, TRet, T1, T2, T3>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3);
                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, param1, param2, param3);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerRet4<TInstance, TRet, T1, T2, T3, T4>
            {
                internal delegate TRet InvokerDelegate(object instance, ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);
                internal delegate TRet ThunkInvokerDelegate(object instance, T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
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
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero) MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);
                    T4 param4 = paramPtrs[3] != IntPtr.Zero ? (T4)GCHandle.FromIntPtr(paramPtrs[3]).Target : default(T4);

                    TRet ret = deleg(GCHandle.FromIntPtr(instancePtr).Target, param1, param2, param3, param4);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerStaticRet0<TRet>
            {
                internal delegate TRet InvokerDelegate();
                internal delegate TRet ThunkInvokerDelegate();

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    TRet ret = deleg();

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    TRet ret = deleg();

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerStaticRet1<TRet, T1>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1);
                internal delegate TRet ThunkInvokerDelegate(T1 param1);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);

                    T1 param1 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);

                    TRet ret = deleg(ref param1);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);

                    TRet ret = deleg(param1);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerStaticRet2<TRet, T1, T2>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1, ref T2 param2);
                internal delegate TRet ThunkInvokerDelegate(T1 param1, T2 param2);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);

                    TRet ret = deleg(ref param1, ref param2);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);

                    TRet ret = deleg(param1, param2);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerStaticRet3<TRet, T1, T2, T3>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1, ref T2 param2, ref T3 param3);
                internal delegate TRet ThunkInvokerDelegate(T1 param1, T2 param2, T3 param3);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
                {
                    (Type[] types, InvokerDelegate deleg) = (Tuple<Type[], InvokerDelegate>)(delegateContext);

                    IntPtr param1Ptr = Marshal.ReadIntPtr(paramPtr);
                    IntPtr param2Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size);
                    IntPtr param3Ptr = Marshal.ReadIntPtr(paramPtr + IntPtr.Size + IntPtr.Size);

                    T1 param1 = default;
                    T2 param2 = default;
                    T3 param3 = default;
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);

                    TRet ret = deleg(ref param1, ref param2, ref param3);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);

                    TRet ret = deleg(param1, param2, param3);

                    return MarshalReturnValue(ref ret);
                }
            }

            internal static class InvokerStaticRet4<TRet, T1, T2, T3, T4>
            {
                internal delegate TRet InvokerDelegate(ref T1 param1, ref T2 param2, ref T3 param3, ref T4 param4);
                internal delegate TRet ThunkInvokerDelegate(T1 param1, T2 param2, T3 param3, T4 param4);

                internal static object CreateDelegate(MethodInfo method)
                {
                    return new Tuple<Type[], InvokerDelegate>(method.GetParameters().Select(x => x.ParameterType).ToArray(), Unsafe.As<InvokerDelegate>(CreateDelegateFromMethod(method)));
                }

                internal static object CreateInvokerDelegate(MethodInfo method)
                {
                    return Unsafe.As<ThunkInvokerDelegate>(CreateDelegateFromMethod(method, false));
                }

                [DebuggerStepThrough]
                internal static IntPtr MarshalAndInvoke(object delegateContext, IntPtr instancePtr, IntPtr paramPtr)
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
                    if (param1Ptr != IntPtr.Zero) MarshalHelper<T1>.ToManaged(ref param1, param1Ptr, types[0].IsByRef);
                    if (param2Ptr != IntPtr.Zero) MarshalHelper<T2>.ToManaged(ref param2, param2Ptr, types[1].IsByRef);
                    if (param3Ptr != IntPtr.Zero) MarshalHelper<T3>.ToManaged(ref param3, param3Ptr, types[2].IsByRef);
                    if (param4Ptr != IntPtr.Zero) MarshalHelper<T4>.ToManaged(ref param4, param4Ptr, types[3].IsByRef);

                    TRet ret = deleg(ref param1, ref param2, ref param3, ref param4);

                    // Marshal reference parameters back to original unmanaged references
                    if (types[0].IsByRef)
                        MarshalHelper<T1>.ToNative(ref param1, param1Ptr);
                    if (types[1].IsByRef)
                        MarshalHelper<T2>.ToNative(ref param2, param2Ptr);
                    if (types[2].IsByRef)
                        MarshalHelper<T3>.ToNative(ref param3, param3Ptr);
                    if (types[3].IsByRef)
                        MarshalHelper<T4>.ToNative(ref param4, param4Ptr);

                    return MarshalReturnValue(ref ret);
                }

                [DebuggerStepThrough]
                internal static unsafe IntPtr InvokeThunk(object delegateContext, IntPtr instancePtr, IntPtr* paramPtrs)
                {
                    ThunkInvokerDelegate deleg = Unsafe.As<ThunkInvokerDelegate>(delegateContext);

                    T1 param1 = paramPtrs[0] != IntPtr.Zero ? (T1)GCHandle.FromIntPtr(paramPtrs[0]).Target : default(T1);
                    T2 param2 = paramPtrs[1] != IntPtr.Zero ? (T2)GCHandle.FromIntPtr(paramPtrs[1]).Target : default(T2);
                    T3 param3 = paramPtrs[2] != IntPtr.Zero ? (T3)GCHandle.FromIntPtr(paramPtrs[2]).Target : default(T3);
                    T4 param4 = paramPtrs[3] != IntPtr.Zero ? (T4)GCHandle.FromIntPtr(paramPtrs[3]).Target : default(T4);

                    TRet ret = deleg(param1, param2, param3, param4);

                    return MarshalReturnValue(ref ret);
                }
            }
        }
    }
}

#endif
