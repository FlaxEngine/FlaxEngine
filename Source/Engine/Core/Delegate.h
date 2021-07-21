// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Allocation.h"

/// <summary>
/// The function object.
/// </summary>
template<typename ReturnType, typename... Params>
class Function<ReturnType(Params ...)>
{
    friend Delegate<Params...>;
public:

    /// <summary>
    /// Signature of the function to call.
    /// </summary>
    typedef ReturnType (*Signature)(Params ...);

private:

    typedef ReturnType (*StubSignature)(void*, Params ...);

    template<ReturnType(*Method)(Params ...)>
    static ReturnType StaticMethodStub(void*, Params ... params)
    {
        return (Method)(Forward<Params>(params)...);
    }

    static ReturnType StaticPointerMethodStub(void* callee, Params ... params)
    {
        return reinterpret_cast<Signature>(callee)(Forward<Params>(params)...);
    }

    template<class T, ReturnType(T::*Method)(Params ...)>
    static ReturnType ClassMethodStub(void* callee, Params ... params)
    {
        return (reinterpret_cast<T*>(callee)->*Method)(Forward<Params>(params)...);
    }

    template<class T, ReturnType(T::*Method)(Params ...) const>
    static ReturnType ClassMethodStub(void* callee, Params ... params)
    {
        return (reinterpret_cast<T*>(callee)->*Method)(Forward<Params>(params)...);
    }

    void* _callee;
    StubSignature _function;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="Function"/> class.
    /// </summary>
    Function()
    {
        _callee = nullptr;
        _function = nullptr;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Function"/> class.
    /// </summary>
    Function(Signature method)
    {
        ASSERT(method);
        _callee = (void*)method;
        _function = &StaticPointerMethodStub;
    }

public:

    /// <summary>
    /// Binds a static function.
    /// </summary>
    template<ReturnType (*Method)(Params ...)>
    void Bind()
    {
        _callee = nullptr;
        _function = &StaticMethodStub<Method>;
    }

    /// <summary>
    /// Binds a member function.
    /// </summary>
    /// <param name="callee">The object instance.</param>
    template<class T, ReturnType(T::*Method)(Params ...)>
    void Bind(T* callee)
    {
        _callee = callee;
        _function = &ClassMethodStub<T, Method>;
    }

    /// <summary>
    /// Binds a function.
    /// </summary>
    /// <param name="method">The method.</param>
    void Bind(Signature method)
    {
        _callee = (void*)method;
        _function = &StaticPointerMethodStub;
    }

    /// <summary>
    /// Unbinds the function.
    /// </summary>
    void Unbind()
    {
        _callee = nullptr;
        _function = nullptr;
    }

public:

    /// <summary>
    /// Returns true if any function has been binded.
    /// </summary>
    /// <returns>True if any function has been binded, otherwise false.</returns>
    FORCE_INLINE bool IsBinded() const
    {
        return _function != nullptr;
    }

    /// <summary>
    /// Calls the binded function if any has been assigned.
    /// </summary>
    /// <param name="params">A list of parameters for the function invocation.</param>
    /// <returns>Function result</returns>
    void TryCall(Params ... params) const
    {
        if (_function)
            _function(_callee, Forward<Params>(params)...);
    }

    /// <summary>
    /// Calls the binded function (it must be assigned).
    /// </summary>
    /// <param name="params">A list of parameters for the function invocation.</param>
    /// <returns>Function result</returns>
    ReturnType operator()(Params ... params) const
    {
        ASSERT(_function);
        return _function(_callee, Forward<Params>(params)...);
    }

    FORCE_INLINE bool operator==(const Function& other) const
    {
        return _function == other._function && _callee == other._callee;
    }

    FORCE_INLINE bool operator!=(const Function& other) const
    {
        return _function != other._function || _callee != other._callee;
    }
};

/// <summary>
/// Delegate object that can be used to bind and call multiply functions. Thread-safe to register/unregister during the call.
/// </summary>
template<typename... Params>
class Delegate
{
public:

    /// <summary>
    /// Signature of the function to call.
    /// </summary>
    typedef void (*Signature)(Params ...);

    /// <summary>
    /// Template for the function.
    /// </summary>
    using FunctionType = Function<void(Params ...)>;

protected:

    // Single allocation for list of binded functions. Thread-safe access via atomic operations. Removing binded function simply clears the entry to handle function unregister during invocation.
    intptr volatile _ptr = 0;
    intptr volatile _size = 0;
    typedef void (*StubSignature)(void*, Params ...);

public:
    NON_COPYABLE(Delegate);

    Delegate()
    {
    }

    ~Delegate()
    {
        Allocator::Free((void*)_ptr);
    }

public:

    /// <summary>
    /// Binds a static function.
    /// </summary>
    template<void(*Method)(Params ...)>
    void Bind()
    {
        FunctionType f;
        f.template Bind<Method>();
        Bind(f);
    }

    /// <summary>
    /// Binds a member function.
    /// </summary>
    /// <param name="callee">The object instance.</param>
    template<class T, void(T::*Method)(Params ...)>
    void Bind(T* callee)
    {
        ASSERT(callee);
        FunctionType f;
        f.template Bind<T, Method>(callee);
        Bind(f);
    }

    /// <summary>
    /// Binds a function.
    /// </summary>
    /// <param name="method">The method.</param>
    void Bind(Signature method)
    {
        FunctionType f(method);
        Bind(f);
    }

    /// <summary>
    /// Binds a function.
    /// </summary>
    /// <param name="f">The function to bind.</param>
    void Bind(const FunctionType& f)
    {
        const intptr size = Platform::AtomicRead(&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead(&_ptr);
        if (bindings)
        {
            // Find a first free slot
            for (intptr i = 0; i < size; i++)
            {
                if (Platform::InterlockedCompareExchange((intptr volatile*)&bindings[i]._function, (intptr)f._function, 0) == 0)
                {
                    Platform::AtomicStore((intptr volatile*)&bindings[i]._callee, (intptr)f._callee);
                    return;
                }
            }
        }

        // Failed to find an empty slot in the list (empty or too small) so perform reallocation
        const intptr newSize = size ? size * 2 : 32;
        auto newBindings = (FunctionType*)Allocator::Allocate(newSize * sizeof(FunctionType));
        Platform::MemoryCopy(newBindings, bindings, size * sizeof(FunctionType));
        Platform::MemoryClear(newBindings + size, (newSize - size) * sizeof(FunctionType));

        // Insert function into a first slot after the old list
        newBindings[size] = f;

        // Set the new list
        auto oldBindings = (FunctionType*)Platform::InterlockedCompareExchange(&_ptr, (intptr)newBindings, (intptr)bindings);
        if (oldBindings != bindings)
        {
            // Other thread already set the new list so free this list and try again
            Allocator::Free(newBindings);
            Bind(f);
        }
        else
        {
            // Free previous bindings and update list size
            Platform::AtomicStore(&_size, newSize);
            // TODO: what is someone read this value before and is using the old table?
            Allocator::Free(bindings);
        }
    }

    /// <summary>
    /// Unbinds a static function.
    /// </summary>
    template<void(*Method)(Params ...)>
    void Unbind()
    {
        FunctionType f;
        f.template Bind<Method>();
        Unbind(f);
    }

    /// <summary>
    /// Unbinds a member function.
    /// </summary>
    /// <param name="callee">The object instance.</param>
    template<class T, void(T::*Method)(Params ...)>
    void Unbind(T* callee)
    {
        ASSERT(callee);
        FunctionType f;
        f.template Bind<T, Method>(callee);
        Unbind(f);
    }

    /// <summary>
    /// Unbinds the specified function.
    /// </summary>
    /// <param name="method">The method.</param>
    void Unbind(Signature method)
    {
        FunctionType f(method);
        Unbind(f);
    }

    /// <summary>
    /// Unbinds the specified function.
    /// </summary>
    /// <param name="f">The function to unbind.</param>
    void Unbind(FunctionType& f)
    {
        // Find slot with that function
        const intptr size = Platform::AtomicRead(&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead(&_ptr);
        for (intptr i = 0; i < size; i++)
        {
            if (Platform::AtomicRead((intptr volatile*)&bindings[i]._callee) == (intptr)f._callee && Platform::AtomicRead((intptr volatile*)&bindings[i]._function) == (intptr)f._function)
            {
                Platform::AtomicStore((intptr volatile*)&bindings[i]._callee, 0);
                Platform::AtomicStore((intptr volatile*)&bindings[i]._function, 0);
                break;
            }
        }
        if ((FunctionType*)Platform::AtomicRead(&_ptr) != bindings)
        {
            // Someone changed the bindings list so retry unbind from the new one
            Unbind(f);
        }
    }

    /// <summary>
    /// Unbinds all the functions.
    /// </summary>
    void UnbindAll()
    {
        const intptr size = Platform::AtomicRead(&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead(&_ptr);
        for (intptr i = 0; i < size; i++)
        {
            Platform::AtomicStore((intptr volatile*)&bindings[i]._function, 0);
            Platform::AtomicStore((intptr volatile*)&bindings[i]._callee, 0);
        }
    }

    /// <summary>
    /// Counts the amount of binded functions.
    /// </summary>
    /// <returns>The binded functions count.</returns>
    int32 Count() const
    {
        int32 count = 0;
        const intptr size = Platform::AtomicRead((intptr volatile*)&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead((intptr volatile*)&_ptr);
        for (intptr i = 0; i < size; i++)
        {
            if (Platform::AtomicRead((intptr volatile*)&bindings[i]._function) != 0)
                count++;
        }
        return count;
    }

    /// <summary>
    /// Determines whether any function is binded.
    /// </summary>
    /// <returns><c>true</c> if any function is binded; otherwise, <c>false</c>.</returns>
    bool IsBinded() const
    {
        const intptr size = Platform::AtomicRead((intptr volatile*)&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead((intptr volatile*)&_ptr);
        for (intptr i = 0; i < size; i++)
        {
            if (Platform::AtomicRead((intptr volatile*)&bindings[i]._function) != 0)
                return true;
        }
        return false;
    }

    /// <summary>
    /// Gets all the binded functions.
    /// </summary>
    /// <param name="buffer">The result bindings functions memory.</param>
    /// <param name="bufferSize">The result bindings functions memory size.</param>
    /// <returns>The amount of written items into the output bindings buffer. Can be equal or less than input bindingsCount capacity.</returns>
    int32 GetBindings(FunctionType* buffer, int32 bufferSize) const
    {
        int32 count = 0;
        const intptr size = Platform::AtomicRead((intptr volatile*)&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead((intptr volatile*)&_ptr);
        for (intptr i = 0; i < size && i < bufferSize; i++)
        {
            buffer[count]._function = (StubSignature)Platform::AtomicRead((intptr volatile*)&bindings[i]._function);
            if (buffer[count]._function != nullptr)
            {
                buffer[count]._callee = (void*)Platform::AtomicRead((intptr volatile*)&bindings[i]._callee);
                count++;
            }
        }
        return count;
    }

    /// <summary>
    /// Calls all the binded functions. Supports unbinding of the called functions during invocation.
    /// </summary>
    /// <param name="params">A list of parameters for the function invocation.</param>
    void operator()(Params ... params) const
    {
        const intptr size = Platform::AtomicRead((intptr volatile*)&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead((intptr volatile*)&_ptr);
        for (intptr i = 0; i < size; i++)
        {
            FunctionType f;
            f._function = (StubSignature)Platform::AtomicRead((intptr volatile*)&bindings[i]._function);
            if (f._function != nullptr)
            {
                f._callee = (void*)Platform::AtomicRead((intptr volatile*)&bindings[i]._callee);
                f._function(f._callee, Forward<Params>(params)...);
            }
        }
    }
};

/// <summary>
/// Action delegate.
/// </summary>
typedef Delegate<> Action;
