// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ScriptingType.h"
#include "Engine/Core/Object.h"
#include "Engine/Core/Delegate.h"
#include "ManagedCLR/MTypes.h"

#define SCRIPTING_OBJECT_CAST_WITH_CSHARP (USE_CSHARP)

/// <summary>
/// Represents object from unmanaged memory that can use accessed via scripting.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API ScriptingObject : public Object
{
    friend class Scripting;
    friend class BinaryModule;
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(ScriptingObject);

public:
    typedef ScriptingObjectSpawnParams SpawnParams;

protected:
    MGCHandle _gcHandle;
    ScriptingTypeHandle _type;
    Guid _id;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ScriptingObject"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    explicit ScriptingObject(const SpawnParams& params);

    /// <summary>
    /// Finalizes an instance of the <see cref="ScriptingObject"/> class.
    /// </summary>
    virtual ~ScriptingObject();

public:
    // Spawns a new objects of the given type.
    static ScriptingObject* NewObject(const ScriptingTypeHandle& typeHandle);

    template<typename T>
    static T* NewObject()
    {
        return (T*)NewObject(T::TypeInitializer);
    }

    template<typename T>
    static T* NewObject(const ScriptingTypeHandle& typeHandle)
    {
        auto obj = NewObject(typeHandle);
        if (obj && !obj->Is<T>())
        {
            Delete(obj);
            obj = nullptr;
        }
        return (T*)obj;
    }

public:
    /// <summary>
    /// Event fired when object gets deleted.
    /// </summary>
    Delegate<ScriptingObject*> Deleted;

public:
    /// <summary>
    /// Gets the managed instance object.
    /// </summary>
    MObject* GetManagedInstance() const;

    /// <summary>
    /// Gets the managed instance object or creates it if missing.
    /// </summary>
    MObject* GetOrCreateManagedInstance() const;

    /// <summary>
    /// Determines whether managed instance is alive.
    /// </summary>
    FORCE_INLINE bool HasManagedInstance() const
    {
        return GetManagedInstance() != nullptr;
    }

    /// <summary>
    /// Gets the unique object ID.
    /// </summary>
    FORCE_INLINE const Guid& GetID() const
    {
        return _id;
    }

    /// <summary>
    /// Gets the scripting type handle of this object.
    /// </summary>
    FORCE_INLINE const ScriptingTypeHandle& GetTypeHandle() const
    {
        return _type;
    }

    /// <summary>
    /// Gets the scripting type of this object.
    /// </summary>
    FORCE_INLINE const ScriptingType& GetType() const
    {
        return _type.GetType();
    }

    /// <summary>
    /// Gets the type class of this object.
    /// </summary>
    MClass* GetClass() const;

public:
    // Tries to cast native interface object to scripting object instance. Returns null if fails.
    static ScriptingObject* FromInterface(void* interfaceObj, const ScriptingTypeHandle& interfaceType);

    template<typename T>
    static ScriptingObject* FromInterface(T* interfaceObj)
    {
        return FromInterface(interfaceObj, T::TypeInitializer);
    }

    static void* ToInterface(ScriptingObject* obj, const ScriptingTypeHandle& interfaceType);

    template<typename T>
    static T* ToInterface(ScriptingObject* obj)
    {
        return (T*)ToInterface(obj, T::TypeInitializer);
    }

    static ScriptingObject* ToNative(MObject* obj);

    FORCE_INLINE static MObject* ToManaged(const ScriptingObject* obj)
    {
        return obj ? obj->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Checks if can cast one scripting object type into another type.
    /// </summary>
    /// <param name="from">The object type for the cast.</param>
    /// <param name="to">The destination type to the cast.</param>
    /// <returns>True if can, otherwise false.</returns>
    static bool CanCast(const ScriptingTypeHandle& from, const ScriptingTypeHandle& to);

    /// <summary>
    /// Checks if can cast one scripting object type into another type.
    /// </summary>
    /// <param name="from">The object class for the cast.</param>
    /// <param name="to">The destination class to the cast.</param>
    /// <returns>True if can, otherwise false.</returns>
    static bool CanCast(const MClass* from, const MClass* to);

    template<typename T>
    static T* Cast(ScriptingObject* obj)
    {
#if SCRIPTING_OBJECT_CAST_WITH_CSHARP
        return obj && CanCast(obj->GetClass(), T::GetStaticClass()) ? static_cast<T*>(obj) : nullptr;
#else
        return obj && CanCast(obj->GetTypeHandle(), T::TypeInitializer) ? static_cast<T*>(obj) : nullptr;
#endif
    }

    bool Is(const ScriptingTypeHandle& type) const;

    bool Is(const MClass* type) const
    {
        return CanCast(GetClass(), type);
    }

    template<typename T>
    bool Is() const
    {
#if SCRIPTING_OBJECT_CAST_WITH_CSHARP
        return CanCast(GetClass(), T::GetStaticClass());
#else
        return CanCast(GetTypeHandle(), T::TypeInitializer);
#endif
    }

public:
    /// <summary>
    /// Changes the object id (both managed and unmanaged). Warning! Use with caution as object ID is what it identifies it and change might cause issues.
    /// </summary>
    /// <param name="newId">The new ID.</param>
    virtual void ChangeID(const Guid& newId);

public:
    virtual void SetManagedInstance(MObject* instance);
    virtual void OnManagedInstanceDeleted();
    virtual void OnScriptingDispose();

    virtual bool CreateManaged();
    virtual void DestroyManaged();

public:
    /// <summary>
    /// Determines whether this object is registered or not (can be found by the queries and used in a game).
    /// </summary>
    FORCE_INLINE bool IsRegistered() const
    {
        return (Flags & ObjectFlags::IsRegistered) != ObjectFlags::None;
    }

    /// <summary>
    /// Registers the object (cannot be called when objects has been already registered).
    /// </summary>
    void RegisterObject();

    /// <summary>
    /// Unregisters the object (cannot be called when objects has not been already registered).
    /// </summary>
    void UnregisterObject();

protected:
#if USE_CSHARP
    /// <summary>
    /// Create a new managed object.
    /// </summary>
    MObject* CreateManagedInternal();
#endif

public:
    // [Object]
    void OnDeleteObject() override;
    String ToString() const override;
};

/// <summary>
/// Managed object that uses weak GC handle to track the target object location in memory. 
/// Can be destroyed by the GC.
/// Used by the objects that lifetime is controlled by the C# side.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API ManagedScriptingObject : public ScriptingObject
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ManagedScriptingObject"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    explicit ManagedScriptingObject(const SpawnParams& params);

public:
    // [ScriptingObject]
    void SetManagedInstance(MObject* instance) override;
    void OnManagedInstanceDeleted() override;
    void OnScriptingDispose() override;
    bool CreateManaged() override;
};

/// <summary>
/// Use ScriptingObject instead.
/// [Deprecated on 5.01.2022, expires on 5.01.2024]
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API PersistentScriptingObject : public ScriptingObject
{
public:
    PersistentScriptingObject(const SpawnParams& params);
};

extern FLAXENGINE_API class ScriptingObject* FindObject(const Guid& id, class MClass* type);
