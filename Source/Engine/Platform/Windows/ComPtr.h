// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Core/Templates.h"
#include "../Win32/IncludeWindowsHeaders.h"

/// <summary>
/// Helper object that makes IUnknown methods private.
/// </summary>
template<typename T>
class RemoveIUnknownBase : public T
{
public:

    ~RemoveIUnknownBase()
    {
    }

    // STDMETHOD macro implies virtual.
    // ComPtr can be used with any class that implements the 3 methods of IUnknown.
    // ComPtr does not require these methods to be virtual.
    // When ComPtr is used with a class without a virtual table, marking the functions
    // as virtual in this class adds unnecessary overhead.
    HRESULT __stdcall QueryInterface(REFIID riid, _COM_Outptr_ void** ppvObject);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    template<class Q>
    HRESULT __stdcall QueryInterface(_COM_Outptr_ Q** pp)
    {
        return QueryInterface(__uuidof(Q), (void**)pp);
    }
};

template<typename T>
struct RemoveIUnknown
{
    typedef RemoveIUnknownBase<T> ReturnType;
};

template<typename T>
struct RemoveIUnknown<const T>
{
    typedef const RemoveIUnknownBase<T> ReturnType;
};

template<typename T>
class ComPtr
{
public:

    typedef T InterfaceType;

protected:

    InterfaceType* ptr_;
    template<class U>
    friend class ComPtr;

    void InternalAddRef() const
    {
        if (ptr_ != nullptr)
        {
            ptr_->AddRef();
        }
    }

    unsigned long InternalRelease()
    {
        unsigned long ref = 0;
        T* temp = ptr_;

        if (temp != nullptr)
        {
            ptr_ = nullptr;
            ref = temp->Release();
        }

        return ref;
    }

public:

    ComPtr()
        : ptr_(nullptr)
    {
    }

    ComPtr(decltype(__nullptr))
        : ptr_(nullptr)
    {
    }

    template<class U>
    ComPtr(U* other)
        : ptr_(other)
    {
        InternalAddRef();
    }

    ComPtr(const ComPtr& other)
        : ptr_(other.ptr_)
    {
        InternalAddRef();
    }

    // copy constructor that allows to instantiate class when U* is convertible to T*
    template<class U>
    ComPtr(const ComPtr<U>& other, typename TEnableIf<__is_convertible_to(U*, T*), void*>::Type* = 0)
        : ptr_(other.ptr_)
    {
        InternalAddRef();
    }

    ComPtr(ComPtr&& other) noexcept
        : ptr_(nullptr)
    {
        if (this != reinterpret_cast<ComPtr*>(&reinterpret_cast<unsigned char&>(other)))
        {
            Swap(other);
        }
    }

    // Move constructor that allows instantiation of a class when U* is convertible to T*
    template<class U>
    ComPtr(ComPtr<U>&& other, typename TEnableIf<__is_convertible_to(U*, T*), void*>::Type* = 0)
        : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }

    ~ComPtr()
    {
        InternalRelease();
    }

public:

    ComPtr& operator=(decltype(__nullptr))
    {
        InternalRelease();
        return *this;
    }

    ComPtr& operator=(T* other)
    {
        if (ptr_ != other)
        {
            ComPtr(other).Swap(*this);
        }
        return *this;
    }

    template<typename U>
    ComPtr& operator=(U* other)
    {
        ComPtr(other).Swap(*this);
        return *this;
    }

    ComPtr& operator=(const ComPtr& other)
    {
        if (ptr_ != other.ptr_)
        {
            ComPtr(other).Swap(*this);
        }
        return *this;
    }

    template<class U>
    ComPtr& operator=(const ComPtr<U>& other)
    {
        ComPtr(other).Swap(*this);
        return *this;
    }

    ComPtr& operator=(ComPtr&& other) noexcept
    {
        ComPtr(static_cast<ComPtr&&>(other)).Swap(*this);
        return *this;
    }

    template<class U>
    ComPtr& operator=(ComPtr<U>&& other)
    {
        ComPtr(static_cast<ComPtr<U>&&>(other)).Swap(*this);
        return *this;
    }

public:

    void Swap(ComPtr&& r)
    {
        T* tmp = ptr_;
        ptr_ = r.ptr_;
        r.ptr_ = tmp;
    }

    void Swap(ComPtr& r)
    {
        T* tmp = ptr_;
        ptr_ = r.ptr_;
        r.ptr_ = tmp;
    }

    operator bool() const
    {
        return Get() != nullptr;
    }

    operator T*() const
    {
        return ptr_;
    }

    T* Get() const
    {
        return ptr_;
    }

#if BUILD_DEBUG
    typename RemoveIUnknown<InterfaceType>::ReturnType* operator->() const
    {
        return static_cast<typename RemoveIUnknown<InterfaceType>::ReturnType*>(ptr_);
    }
#else
    typename InterfaceType* operator->() const
    {
        return ptr_;
    }
#endif

    T** operator&()
    {
        return &ptr_;
    }

    const T** operator&() const
    {
        return &ptr_;
    }

    T* const* GetAddressOf() const
    {
        return &ptr_;
    }

    T** GetAddressOf()
    {
        return &ptr_;
    }

    T** ReleaseAndGetAddressOf()
    {
        InternalRelease();
        return &ptr_;
    }

    T* Detach()
    {
        T* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }

    void Attach(InterfaceType* other)
    {
        if (ptr_ != nullptr)
        {
            auto ref = ptr_->Release();
        }

        ptr_ = other;
    }

    unsigned long Reset()
    {
        return InternalRelease();
    }

    // query for U interface
    template<typename U>
    HRESULT As(ComPtr<U>* p) const
    {
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }
};

// Comparison operators - don't compare COM object identity
template<class T, class U>
bool operator==(const ComPtr<T>& a, const ComPtr<U>& b)
{
    static_assert(__is_base_of(T, U) || __is_base_of(U, T), "'T' and 'U' pointers must be comparable");
    return a.Get() == b.Get();
}

template<class T>
bool operator==(const ComPtr<T>& a, decltype(__nullptr))
{
    return a.Get() == nullptr;
}

template<class T>
bool operator==(decltype(__nullptr), const ComPtr<T>& a)
{
    return a.Get() == nullptr;
}

template<class T, class U>
bool operator!=(const ComPtr<T>& a, const ComPtr<U>& b)
{
    static_assert(__is_base_of(T, U) || __is_base_of(U, T), "'T' and 'U' pointers must be comparable");
    return a.Get() != b.Get();
}

template<class T>
bool operator!=(const ComPtr<T>& a, decltype(__nullptr))
{
    return a.Get() != nullptr;
}

template<class T>
bool operator!=(decltype(__nullptr), const ComPtr<T>& a)
{
    return a.Get() != nullptr;
}

template<class T, class U>
bool operator<(const ComPtr<T>& a, const ComPtr<U>& b)
{
    static_assert(__is_base_of(T, U) || __is_base_of(U, T), "'T' and 'U' pointers must be comparable");
    return a.Get() < b.Get();
}

#endif
