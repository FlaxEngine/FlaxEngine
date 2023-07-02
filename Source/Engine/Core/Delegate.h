// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Allocation.h"

/// <summary>
/// The function object that supports binding static, member and lambda functions.
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

    struct Lambda
    {
        int64 Refs;
        void (*Dtor)(void*);
    };

    void* _callee;
    StubSignature _function;
    Lambda* _lambda;

    FORCE_INLINE void LambdaCtor() const
    {
        Platform::InterlockedIncrement((int64 volatile*)&_lambda->Refs);
    }
    FORCE_INLINE void LambdaDtor()
    {
        if (Platform::InterlockedDecrement(&_lambda->Refs) == 0)
        {
            ((Lambda*)_lambda)->Dtor(_callee);
            Allocator::Free(_lambda);
        }
    }
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Function"/> class.
    /// </summary>
    Function()
    {
        _callee = nullptr;
        _function = nullptr;
        _lambda = nullptr;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Function"/> class.
    /// </summary>
    Function(Signature method)
    {
        _callee = (void*)method;
        _function = &StaticPointerMethodStub;
        _lambda = nullptr;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Function"/> class.
    /// </summary>
    template<typename T>
    Function(const T& lambda)
    {
        _lambda = nullptr;
        Bind<T>(lambda);
    }

    Function(const Function& other)
        : _callee(other._callee)
        , _function(other._function)
        , _lambda(other._lambda)
    {
        if (_lambda)
            LambdaCtor();
    }

    Function(Function&& other) noexcept
        : _callee(other._callee)
        , _function(other._function)
        , _lambda(other._lambda)
    {
        other._lambda = nullptr;
    }

    ~Function()
    {
        if (_lambda)
            LambdaDtor();  
    }

public:
    /// <summary>
    /// Binds a static function.
    /// </summary>
    template<ReturnType (*Method)(Params ...)>
    void Bind()
    {
        if (_lambda)
            LambdaDtor();
        _callee = nullptr;
        _function = &StaticMethodStub<Method>;
        _lambda = nullptr;
    }

    /// <summary>
    /// Binds a member function.
    /// </summary>
    /// <param name="callee">The object instance.</param>
    template<class T, ReturnType(T::*Method)(Params ...)>
    void Bind(T* callee)
    {
        if (_lambda)
            LambdaDtor();
        _callee = callee;
        _function = &ClassMethodStub<T, Method>;
        _lambda = nullptr;
    }

    /// <summary>
    /// Binds a function.
    /// </summary>
    /// <param name="method">The method.</param>
    void Bind(Signature method)
    {
        if (_lambda)
            LambdaDtor();
        _callee = (void*)method;
        _function = &StaticPointerMethodStub;
        _lambda = nullptr;
    }

    /// <summary>
    /// Binds a lambda.
    /// </summary>
    /// <param name="lambda">The lambda.</param>
    template<typename T>
    void Bind(const T& lambda)
    {
        if (_lambda)
            LambdaDtor();
        _lambda = (Lambda*)Allocator::Allocate(sizeof(Lambda) + sizeof(T));
        _lambda->Refs = 1;
        _lambda->Dtor = [](void* callee) -> void { static_cast<T*>(callee)->~T(); };
        _function = [](void* callee, Params ... params) -> ReturnType { return (*static_cast<T*>(callee))(Forward<Params>(params)...); };
        _callee = (byte*)_lambda + sizeof(Lambda);
        new(_callee) T(lambda);
    }

    /// <summary>
    /// Unbinds the function.
    /// </summary>
    void Unbind()
    {
        if (_lambda)
            LambdaDtor();
        _callee = nullptr;
        _function = nullptr;
        _lambda = nullptr;
    }

public:
    /// <summary>
    /// Returns true if any function has been binded.
    /// </summary>
    FORCE_INLINE bool IsBinded() const
    {
        return _function != nullptr;
    }

    /// <summary>
    /// Calls the binded function (it must be assigned).
    /// </summary>
    /// <param name="params">A list of parameters for the function invocation.</param>
    /// <returns>Function result</returns>
    FORCE_INLINE ReturnType operator()(Params ... params) const
    {
        ASSERT(_function);
        return _function(_callee, Forward<Params>(params)...);
    }

    Function& operator=(const Function& other)
    {
        if (this == &other)
            return *this;
        _callee = other._callee;
        _function = other._function;
        _lambda = other._lambda;
        if (_lambda)
            LambdaCtor();
        return *this;
    }

    Function& operator=(Function&& other) noexcept
    {
        if (this == &other)
            return *this;
        _callee = other._callee;
        _function = other._function;
        _lambda = other._lambda;
        other._callee = nullptr;
        other._function = nullptr;
        other._lambda = nullptr;
        return *this;
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
    Delegate()
    {
    }

    Delegate(const Delegate& other)
    {
        const intptr newSize = other._size;
        auto newBindings = (FunctionType*)Allocator::Allocate(newSize * sizeof(FunctionType));
        Platform::MemoryCopy((void*)newBindings, (const void*)other._ptr, newSize * sizeof(FunctionType));
        for (intptr i = 0; i < newSize; i++)
        {
            FunctionType& f = newBindings[i];
            if (f._function && f._lambda)
                f.LambdaCtor();
        }
        _ptr = (intptr)newBindings;
        _size = newSize;
    }

    Delegate(Delegate&& other) noexcept
    {
        _ptr = other._ptr;
        _size = other._size;
        other._ptr = 0;
        other._size = 0;
    }

    ~Delegate()
    {
        auto ptr = (FunctionType*)_ptr;
        if (ptr)
        {
            while (_size--)
            {
                if (ptr->_lambda)
                    ptr->LambdaDtor();  
                ++ptr;
            }
            Allocator::Free((void*)_ptr);
        }
    }

    Delegate& operator=(const Delegate& other)
    {
        if (this != &other)
        {
            UnbindAll();
            const intptr size = Platform::AtomicRead((intptr volatile*)&other._size);
            FunctionType* bindings = (FunctionType*)Platform::AtomicRead((intptr volatile*)&other._ptr);
            for (intptr i = 0; i < size; i++)
                Bind(bindings[i]);
        }
        return *this;
    }

    Delegate& operator=(Delegate&& other) noexcept
    {
        if (this != &other)
        {
            _ptr = other._ptr;
            _size = other._size;
            other._ptr = 0;
            other._size = 0;
        }
        return *this;
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
    /// Binds a lambda.
    /// </summary>
    /// <param name="lambda">The lambda.</param>
    template<typename T>
    void Bind(const T& lambda)
    {
        FunctionType f;
        f.template Bind<T>(lambda);
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
                    bindings[i]._callee = f._callee;
                    bindings[i]._lambda = f._lambda;
                    if (f._lambda)
                        f.LambdaCtor();
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
    /// Binds a static function (if not binded yet).
    /// </summary>
    template<void(*Method)(Params ...)>
    void BindUnique()
    {
        FunctionType f;
        f.template Bind<Method>();
        BindUnique(f);
    }

    /// <summary>
    /// Binds a member function (if not binded yet).
    /// </summary>
    /// <param name="callee">The object instance.</param>
    template<class T, void(T::*Method)(Params ...)>
    void BindUnique(T* callee)
    {
        FunctionType f;
        f.template Bind<T, Method>(callee);
        BindUnique(f);
    }

    /// <summary>
    /// Binds a function (if not binded yet).
    /// </summary>
    /// <param name="method">The method.</param>
    void BindUnique(Signature method)
    {
        FunctionType f(method);
        BindUnique(f);
    }

    /// <summary>
    /// Binds a function (if not binded yet).
    /// </summary>
    /// <param name="f">The function to bind.</param>
    void BindUnique(const FunctionType& f)
    {
        const intptr size = Platform::AtomicRead(&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead(&_ptr);
        if (bindings)
        {
            // Skip if already binded
            for (intptr i = 0; i < size; i++)
            {
                if (Platform::AtomicRead((intptr volatile*)&bindings[i]._callee) == (intptr)f._callee && Platform::AtomicRead((intptr volatile*)&bindings[i]._function) == (intptr)f._function)
                    return;
            }
        }
        Bind(f);
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
    void Unbind(const FunctionType& f)
    {
        // Find slot with that function
        const intptr size = Platform::AtomicRead(&_size);
        FunctionType* bindings = (FunctionType*)Platform::AtomicRead(&_ptr);
        for (intptr i = 0; i < size; i++)
        {
            if (Platform::AtomicRead((intptr volatile*)&bindings[i]._callee) == (intptr)f._callee && Platform::AtomicRead((intptr volatile*)&bindings[i]._function) == (intptr)f._function)
            {
                if (bindings[i]._lambda)
                {
                    bindings[i].LambdaDtor();
                    bindings[i]._lambda = nullptr;
                }
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
            if (bindings[i]._lambda)
            {
                bindings[i].LambdaDtor();
                bindings[i]._lambda = nullptr;
            }
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
    /// Gets the current capacity of delegate table (amount of function to store before resizing).
    /// </summary>
    int32 Capacity() const
    {
        return (int32)Platform::AtomicRead((intptr volatile*)&_size);
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
                buffer[count]._lambda = (typename FunctionType::Lambda*)Platform::AtomicRead((intptr volatile*)&bindings[i]._lambda);
                if (buffer[count]._lambda)
                    buffer[count].LambdaCtor();
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
            auto function = (StubSignature)Platform::AtomicRead((intptr volatile*)&bindings->_function);
            auto callee = (void*)Platform::AtomicRead((intptr volatile*)&bindings->_callee);
            if (function != nullptr && function == (StubSignature)Platform::AtomicRead((intptr volatile*)&bindings->_function))
                function(callee, Forward<Params>(params)...);
            ++bindings;
        }
    }
};

/// <summary>
/// Action delegate.
/// </summary>
typedef Delegate<> Action;
