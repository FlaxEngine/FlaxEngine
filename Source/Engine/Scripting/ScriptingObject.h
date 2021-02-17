// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ScriptingType.h"
#include "Engine/Core/Object.h"
#include "Engine/Core/Delegate.h"
#include "ManagedCLR/MTypes.h"

/// <summary>
/// Represents object from unmanaged memory that can use accessed via C# scripting.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API ScriptingObject : public RemovableObject
{
    friend class Scripting;
    friend class BinaryModule;
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ScriptingObject);
public:

    typedef ScriptingObjectSpawnParams SpawnParams;

protected:

    uint32 _gcHandle;
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

    /// <summary>
    /// Spawns a new objects of the given type.
    /// </summary>
    template<typename T>
    static T* Spawn()
    {
        const SpawnParams params(Guid::New(), T::TypeInitializer);
        return T::New(params);
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
    /// <returns>The Mono managed object or null if not created.</returns>
    MonoObject* GetManagedInstance() const;

    /// <summary>
    /// Gets the managed instance object or creates it if missing.
    /// </summary>
    /// <returns>The Mono managed object.</returns>
    FORCE_INLINE MonoObject* GetOrCreateManagedInstance() const
    {
        MonoObject* managedInstance = GetManagedInstance();
        if (!managedInstance)
        {
            const_cast<ScriptingObject*>(this)->CreateManaged();
            managedInstance = GetManagedInstance();
        }
        return managedInstance;
    }

    /// <summary>
    /// Determines whether managed instance is alive.
    /// </summary>
    /// <returns>True if managed object has been created and exists, otherwise false.</returns>
    FORCE_INLINE bool HasManagedInstance() const
    {
        return GetManagedInstance() != nullptr;
    }

    /// <summary>
    /// Gets the unique object ID.
    /// </summary>
    /// <returns>The unique object ID.</returns>
    FORCE_INLINE const Guid& GetID() const
    {
        return _id;
    }

    /// <summary>
    /// Gets the scripting type handle of this object.
    /// </summary>
    /// <returns>The scripting type handle.</returns>
    FORCE_INLINE const ScriptingTypeHandle& GetTypeHandle() const
    {
        return _type;
    }

    /// <summary>
    /// Gets the scripting type of this object.
    /// </summary>
    /// <returns>The scripting type.</returns>
    FORCE_INLINE const ScriptingType& GetType() const
    {
        return _type.GetType();
    }

    /// <summary>
    /// Gets the type class of this object.
    /// </summary>
    /// <returns>The Mono class.</returns>
    MClass* GetClass() const;

public:

    static ScriptingObject* ToNative(MonoObject* obj);

    static MonoObject* ToManaged(ScriptingObject* obj)
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
    static bool CanCast(MClass* from, MClass* to);

    template<typename T>
    static T* Cast(ScriptingObject* obj)
    {
        return obj && CanCast(obj->GetClass(), T::GetStaticClass()) ? (T*)obj : nullptr;
    }

    bool Is(const ScriptingTypeHandle& type) const;

    bool Is(MClass* type) const
    {
        return CanCast(GetClass(), type);
    }

    template<typename T>
    bool Is() const
    {
        return CanCast(GetClass(), T::GetStaticClass());
    }

public:

    /// <summary>
    /// Changes the object id (both managed and unmanaged). Warning! Use with caution as object ID is what it identifies it and change might cause issues.
    /// </summary>
    /// <param name="newId">The new ID.</param>
    virtual void ChangeID(const Guid& newId);

public:

    virtual void OnManagedInstanceDeleted();
    virtual void OnScriptingDispose();

    virtual void CreateManaged() = 0;
    virtual void DestroyManaged();

public:

    /// <summary>
    /// Determines whether this object is registered or not (can be found by the queries and used in a game).
    /// </summary>
    /// <returns><c>true</c> if this instance is registered; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsRegistered() const
    {
        return (Flags & ObjectFlags::IsRegistered) != 0;
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

    /// <summary>
    /// Create a new managed object.
    /// </summary>
    MonoObject* CreateManagedInternal();

public:

    // [RemovableObject]
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
    void CreateManaged() override;
};

/// <summary>
/// Managed object that uses pinned GC handle to prevent collecting and moving. 
/// Used by the objects that lifetime is controlled by the C++ side.
/// </summary>
API_CLASS(InBuild) class FLAXENGINE_API PersistentScriptingObject : public ScriptingObject
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="PersistentScriptingObject"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    explicit PersistentScriptingObject(const SpawnParams& params);

    /// <summary>
    /// Finalizes an instance of the <see cref="PersistentScriptingObject"/> class.
    /// </summary>
    ~PersistentScriptingObject();

public:

    // [ManagedScriptingObject]
    void OnManagedInstanceDeleted() override;
    void OnScriptingDispose() override;
    void CreateManaged() override;
};
