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
#include "Engine/Core/Delegate.h"
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
            if(OnDestructacted.IsBinded())
            {
                //we are holding shared pointer it is not job of this class to delete it
                OnDestructacted();
                ptr = nullptr;
                SharesWith = nullptr;
                return;
            }
            Delete(ptr);
            ptr = nullptr;
        }
    }

    FORCE_INLINE T& operator*()
    {
        return *ptr; 
    }
    FORCE_INLINE T* operator->()
    {
        return ptr; 
    }
    void RemoveLink() 
    {
        if (OnDestructacted.IsBinded())
        {
            //we are holding shared pointer it is not job of this class to delete it
            OnDestructacted();
            ptr = nullptr;
            SharesWith = nullptr;
        }
    }
    bool GetSharedPtr(ShadredObjectPtr<T>& shared)
    {
        if (SharesWith != nullptr)
        {
            shared = *SharesWith;
            return true;
        }
        return false;
    }
protected:
    /// <summary>
    /// Call back to ShadredObjectPtr to remove the Refrence
    /// </summary>
    Action OnDestructacted;
    ShadredObjectPtr<T> SharesWith;
    friend class ShadredObjectPtr<T>;
};
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
    unsigned long long* refCount = nullptr; //64 bit int
public:
    // Constructor
    ShadredObjectPtr(int* p = nullptr) : ptr(nullptr), refCount(new unsigned long long(0ull)) {};
    ShadredObjectPtr(int* ptr) : ptr(ptr), refCount(new unsigned long long(1ull)) {};

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
    }

    // move constructor
    ShadredObjectPtr(ShadredObjectPtr&& dyingObj)
    {

        this->ptr = dyingObj.ptr; // share the underlying pointer
        this->refCount = dyingObj.refCount; // share refCount

        // cleanup any existing data
        Destroy();
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
        Destroy();

        // clean up dyingObj
        dyingObj.refCount = nullptr;
        dyingObj.ptr = nullptr;
    }

    FORCE_INLINE int* operator->() const
    {
        return this->ptr;
    }
    FORCE_INLINE int& operator*() const
    {
        return *this->ptr;
    }

    FORCE_INLINE unsigned long long SheredCount() const
    {
        return *refCount; // *this->refCount
    }

    /// <summary>
    /// Gets The ObjectPtr
    /// </summary>
    /// <returns>new ObjectPtr</returns>
    ObjectPtr& Get() const
    {
        (*this->refCount)++;
        auto op = ObjectPtr<T>(this->ptr);
        op.SharesWith = *this;
        op.OnDestructacted.Bind
        (
            [](ShadredObjectPtr shared = *this)
            {
                shared.Destroy();
            }
        );
        return op;
    }

    // Destructor
    ~ShadredObjectPtr()
    {
        Destroy();
    }

protected:
    void Destroy()
    {
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
