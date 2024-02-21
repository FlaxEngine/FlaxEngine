// curude implementacion of ObjectPtr and SharedObjectPtr writen by Nori_SC
// Usage.
// If u need to track 1 pointer with more then 1 object use SharedObjectPtr
// The SharedObjectPtr.Get() return a ObjectPtr with is shared with SharedObjectPtr
// if u like to make it a not shared Object Ptr after calling get u can call RemoveLink()
// The Shared Object Ptr will get a callback from Object Ptr about ObjectPtr has stopped Sharing with Shared Object Ptr,
// and it will set the pointer of ObjectPtr to null
// if the ObjectPtr with is last Refrence to SharedObjectPtr is last memory will be freed
// u can alwas call back from ObjectPtr to SharedObjectPtr using GetSharedPtr

#pragma once
#include "Engine/Core/Memory/Memory.h"


template<typename T>
class ShadredObjectPtr;
/// <summary>
/// Object pointer when gets deconstructed automatically frees the memory pointer with is pointing to
/// </summary>
/// <typeparam name="T">type of pointer</typeparam>
template<typename T>
class ObjectPtr
{
    /// <summary>
    /// raw pointer
    /// </summary>
    T* ptr;
public:
    // Constructor
    ObjectPtr(T* p = nullptr)
    {
        ptr = p;
    }

    // Destructor
    ~ObjectPtr()
    {
        if (ptr != nullptr)
        {
            if (SharesWith != nullptr)
            {
                ptr = nullptr;
                SharesWith->Destroy();
                return;
            }
            Delete(ptr);
            ptr = nullptr;
        }
    }

    FORCE_INLINE T& operator*() const
    {
        return *ptr;
    }
    FORCE_INLINE T* operator->() const
    {
        return ptr;
    }
    FORCE_INLINE operator T& () const { return *ptr; }
    FORCE_INLINE operator T* () const { return  ptr; }
    FORCE_INLINE operator bool() const { ptr  == nullptr}
    /// <summary>
    /// 
    /// </summary>
    /// <param name="createNew">if is true will create new instance of T </param>
    void RemoveLink(bool createNew = false)
    {
        if (SharesWith != nullptr)
        {
            SharesWith->Destroy();
            return;
        }
        ptr = nullptr;
        SharesWith = nullptr;
        if (createNew)
        {
            ptr = New<T>();
        }
    }
    /// <summary>
    /// Gets shared pointer
    /// </summary>
    /// <returns>Shadred Object Ptr or new Shadred Object Ptr</returns>
    ShadredObjectPtr<T>& ToSharedPtr()
    {
        ShadredObjectPtr<T>* out;
        if (SharesWith != nullptr)
        {
            out = SharesWith;
            SharesWith->Destroy();
            ptr = nullptr;
            SharesWith = nullptr;
        }
        else
        {
            out = New(ShadredObjectPtr<T>(ptr));
            ptr = nullptr;
        }
        return *out;
    }
protected:
    /// <summary>
    /// Call back to ShadredObjectPtr to remove the Refrence
    /// </summary>
    //Action OnDestructacted;

    class ShadredObjectPtr<T>* SharesWith = nullptr;
    friend ShadredObjectPtr<T>;
};
#define SHADREDOBJECTPTRREFRENCETYPE unsigned int
/// <summary>
/// Shared pointer when gets deconstructed if any of other ObjectPtr or other ShadredObjectPtr are not holding its pointer it will destroty it self
/// Automatically free the memory pointer is pointing to
/// </summary>
/// <typeparam name="T">type of pointer</typeparam>
template<typename T>
class ShadredObjectPtr
{
private:
    /// <summary>
    /// shared pointer
    /// </summary>
    T* ptr = nullptr;
    SHADREDOBJECTPTRREFRENCETYPE* refCount = nullptr; //64 bit int
public:
    // Constructor
    ShadredObjectPtr() : ptr(nullptr), refCount(nullptr) {};
    ShadredObjectPtr(int* ptr) : ptr(ptr), refCount(new SHADREDOBJECTPTRREFRENCETYPE(1ull)) {};

    void Create()
    {
        if (refCount == nullptr && ptr == nullptr)
        {
            ptr = New(T());
            refCount = new SHADREDOBJECTPTRREFRENCETYPE(1ull);
        }
    }
    // copy constructor
    ShadredObjectPtr(const ShadredObjectPtr& obj)
    {
        this->ptr = obj.ptr;
        this->refCount = obj.refCount;
        if (nullptr != obj.ptr)
        {
            // if the pointer is not null, increment the refCount
            (*this->refCount)++;
        }
    }
    // copy assignment
    ShadredObjectPtr& operator=(const ShadredObjectPtr& obj)
    {
        // cleanup any existing data
        Destroy();
        // Assign incoming object's data to this object
        this->ptr = obj.ptr; // share the underlying pointer
        this->refCount = obj.refCount; // share refCount
        if (nullptr != obj.ptr)
        {
            // if the pointer is not null, increment the refCount
            (*this->refCount)++;
        }
        return *this;
    }

    // move constructor
    ShadredObjectPtr(ShadredObjectPtr&& dyingObj)
    {

        this->ptr = dyingObj.ptr; // share the underlying pointer
        this->refCount = dyingObj.refCount; // share refCount

        // cleanup any existing data
        dyingObj.Destroy();
        // clean up dyingObj
        dyingObj.refCount = nullptr;
        dyingObj.ptr = nullptr;
    }
    // move assignment
    ShadredObjectPtr& operator=(ShadredObjectPtr&& dyingObj)
    {
        this->ptr = dyingObj.ptr; // share the underlying pointer
        this->refCount = dyingObj.refCount; // share refCount

        // cleanup any existing data
        dyingObj.Destroy();

        // clean up dyingObj
        dyingObj.refCount = nullptr;
        dyingObj.ptr = nullptr;

        return *this;
    }

    FORCE_INLINE T& operator*() const
    {
        return *ptr;
    }
    FORCE_INLINE T* operator->() const
    {
        return ptr;
    }
    FORCE_INLINE operator T& () const { return *ptr; }
    FORCE_INLINE operator T* () const { return  ptr; }
    FORCE_INLINE operator bool() const { ptr == nullptr }

    FORCE_INLINE unsigned long long SheredCount() const
    {
        return *refCount; // *this->refCount
    }

    /// <summary>
    /// Gets The ObjectPtr
    /// </summary>
    /// <returns>new ObjectPtr</returns>
    ObjectPtr<T>& Get() const
    {
        (*this->refCount)++;
        auto op = New(ObjectPtr<T>(this->ptr));
        op->SharesWith = (ShadredObjectPtr<T>*)this;
        return *op;
    }

    // Destructor
    ~ShadredObjectPtr()
    {
        Destroy();
    }

protected:
    friend ObjectPtr<T>;
    void Destroy()
    {
        if (refCount == nullptr)
            return;
        (*refCount)--;
        if (*refCount == 0)
        {
            if (nullptr != ptr)
            {
                Delete(ptr);
                ptr = nullptr;
            }
            delete refCount;
            refCount = nullptr;
        }
    }
};
