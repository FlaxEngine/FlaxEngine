// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#if COMPILE_WITH_PROFILER
#include "Engine/Profiler/ProfilerSrcLoc.h"
#endif
#include "MTypes.h"

/// <summary>
/// Encapsulates information about a single Mono (managed) method belonging to some managed class. This object also allows you to invoke the method.
/// </summary>
class FLAXENGINE_API MMethod
{
    friend MClass;
    friend MProperty;
    friend MEvent;
    friend MCore;

protected:
#if USE_MONO
    MonoMethod* _monoMethod;
#elif USE_NETCORE
    void* _handle;
    int32 _paramsCount;
    mutable void* _returnType;
    mutable Array<void*, InlinedAllocation<8>> _parameterTypes;
    void CacheSignature() const;
#endif
    MClass* _parentClass;
    StringAnsi _name;
    MVisibility _visibility;
#if !USE_MONO_AOT
    void* _cachedThunk = nullptr;
#endif

    mutable int32 _hasCachedAttributes : 1;
#if USE_NETCORE
    mutable int32 _hasCachedSignature : 1;
#endif
    int32 _isStatic : 1;

    mutable Array<MObject*> _attributes;

public:
#if USE_MONO
    explicit MMethod(MonoMethod* monoMethod, MClass* parentClass);
    explicit MMethod(MonoMethod* monoMethod, const char* name, MClass* parentClass);
#elif USE_NETCORE
    MMethod(MClass* parentClass, StringAnsi&& name, void* handle, int32 paramsCount, MMethodAttributes attributes);
#endif

public:
#if COMPILE_WITH_PROFILER
    StringAnsi ProfilerName;
    SourceLocationData ProfilerData;
#endif

    /// <summary>
    /// Invokes the method on the provided object instance. This does not respect polymorphism and will invoke the exact method of the class this object was retrieved from.Use invokeVirtual() if you need polymorphism.
    /// </summary>
    /// <remarks>
    /// This method is slow! Use it ONLY if you will call it only once for whole application life cycle, or you don't have 64 bytes of memory available.
    /// </remarks>
    /// <param name="instance">Instance of the object to invoke the method on. Can be null for static methods.</param>
    /// <param name="params">Array of parameters to pass to the method. Caller must ensure they match method
    /// parameter count and type.For value types parameters should be pointers to the
    /// values and for reference types they should be pointers to MObject.</param>
    /// <param name="exception">An optional pointer to the exception value to store exception object reference.</param>
    /// <returns>A boxed return value, or null if method has no return value.</returns>
    MObject* Invoke(void* instance, void** params, MObject** exception) const;

    /// <summary>
    /// Invokes the method on the provided object instance. If the instance has an override of this method it  will be called.
    /// </summary>
    /// <remarks>
    /// This method is slow! Use it ONLY if you will call it only once for whole application life cycle, or you don't have 64 bytes of memory available.
    /// </remarks>
    /// <param name="instance">Instance of the object to invoke the method on.</param>
    /// <param name="params">
    /// Array of parameters to pass to the method. Caller must ensure they match method
    ///	parameter count and type. For value types parameters should be pointers to the
    /// values and for reference types they should be pointers to MObject.
    /// </param>
    /// <param name="exception">An optional pointer to the exception value to store exception object reference.</param>
    /// <returns>A boxed return value, or null if method has no return value.</returns>
    MObject* InvokeVirtual(MObject* instance, void** params, MObject** exception) const;

#if !USE_MONO_AOT
    /// <summary>
    /// Gets a thunk for this method. A thunk is a C++ like function pointer that you can use for calling the method.
    /// </summary>
    /// <remarks>
    /// This is the fastest way of calling managed code.
    /// Get thunk from class if you want to call static method. You need to call it from method of a instance wrapper to call a specific instance.
    /// Thunks return boxed value but for some smaller types (eg. bool, int, float) the return is inlined into pointer.
    /// </remarks>
    /// <returns>The method thunk pointer.</returns>
    void* GetThunk();
#endif

    /// <summary>
    /// Creates a method that is inflated out of generic method.
    /// </summary>
    /// <returns>The inflated generic method.</returns>
    MMethod* InflateGeneric() const;

    /// <summary>
    /// Gets the method name.
    /// </summary>
    FORCE_INLINE const StringAnsi& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Returns the parent class that this method is contained with.
    /// </summary>
    FORCE_INLINE MClass* GetParentClass() const
    {
        return _parentClass;
    }

    /// <summary>
    /// Returns the type of the return value. Returns null if method has no return value.
    /// </summary>
    MType* GetReturnType() const;

    /// <summary>
    /// Returns the number of parameters the method expects.
    /// </summary>
    int32 GetParametersCount() const;

    /// <summary>
    /// Returns the type of the method parameter at the specified index.
    /// </summary>
    /// <param name="paramIdx">The parameter type.</param>
    /// <returns>The parameter type.</returns>
    MType* GetParameterType(int32 paramIdx) const;

    /// <summary>
    /// Returns the value indicating whenever the method parameter  at the specified index is marked as output parameter.
    /// </summary>
    /// <param name="paramIdx">The parameter type.</param>
    /// <returns>True if parameter is marked as output, otherwise false.</returns>
    bool GetParameterIsOut(int32 paramIdx) const;

    /// <summary>
    /// Gets method visibility in the class.
    /// </summary>
    FORCE_INLINE MVisibility GetVisibility() const
    {
        return _visibility;
    }

    /// <summary>
    /// Returns true if the method doesn't require a class instance.
    /// </summary>
    FORCE_INLINE bool IsStatic() const
    {
        return _isStatic != 0;
    }

#if USE_MONO
    /// <summary>
    /// Gets the Mono method handle.
    /// </summary>
    FORCE_INLINE MonoMethod* GetNative() const
    {
        return _monoMethod;
    }
#endif

public:
    /// <summary>
    /// Checks if method has an attribute of the specified type.
    /// </summary>
    /// <param name="klass">The attribute class to check.</param>
    /// <returns>True if has attribute of that class type, otherwise false.</returns>
    bool HasAttribute(const MClass* klass) const;

    /// <summary>
    /// Checks if method has an attribute of any type.
    /// </summary>
    /// <returns>True if has any custom attribute, otherwise false.</returns>
    bool HasAttribute() const;

    /// <summary>
    /// Returns an instance of an attribute of the specified type. Returns null if the method doesn't have such an attribute.
    /// </summary>
    /// <param name="klass">The attribute Class to take.</param>
    /// <returns>The attribute object.</returns>
    MObject* GetAttribute(const MClass* klass) const;

    /// <summary>
    /// Returns an instance of all attributes connected with given method. Returns null if the method doesn't have any attributes.
    /// </summary>
    /// <returns>The array of attribute objects.</returns>
    const Array<MObject*>& GetAttributes() const;
};
