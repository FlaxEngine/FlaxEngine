// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ScriptingObject.h"
#include "SerializableScriptingObject.h"
#include "Scripting.h"
#include "BinaryModule.h"
#include "Engine/Level/Actor.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/LogContext.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/Content.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/ThreadLocal.h"
#include "Engine/Serialization/SerializationFwd.h"
#include "ManagedCLR/MAssembly.h"
#include "ManagedCLR/MClass.h"
#include "ManagedCLR/MUtils.h"
#include "ManagedCLR/MField.h"
#include "ManagedCLR/MCore.h"
#include "Internal/InternalCalls.h"
#include "Internal/ManagedSerialization.h"
#include "FlaxEngine.Gen.h"

#define ScriptingObject_unmanagedPtr "__unmanagedPtr"
#define ScriptingObject_id "__internalId"

// TODO: don't leak memory (use some kind of late manual GC for those wrapper objects)
typedef Pair<ScriptingObject*, ScriptingTypeHandle> ScriptingObjectsInterfaceKey;
Dictionary<ScriptingObjectsInterfaceKey, void*> ScriptingObjectsInterfaceWrappers;

SerializableScriptingObject::SerializableScriptingObject(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void SerializableScriptingObject::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(SerializableScriptingObject);

#if !COMPILE_WITHOUT_CSHARP
    // Handle C# objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType))
    {
        stream.JKEY("V");
        if (other)
        {
            ManagedSerialization::SerializeDiff(stream, GetOrCreateManagedInstance(), other->GetOrCreateManagedInstance());
        }
        else
        {
            ManagedSerialization::Serialize(stream, GetOrCreateManagedInstance());
        }
    }
#endif

    // Handle custom scripting objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
    {
        stream.JKEY("D");
        _type.Module->SerializeObject(stream, this, other);
    }
}

void SerializableScriptingObject::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
#if !COMPILE_WITHOUT_CSHARP
    // Handle C# objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType))
    {
        auto* const v = SERIALIZE_FIND_MEMBER(stream, "V");
        if (v != stream.MemberEnd() && v->value.IsObject() && v->value.MemberCount() != 0)
        {
            ManagedSerialization::Deserialize(v->value, GetOrCreateManagedInstance());
        }
    }
#endif

    // Handle custom scripting objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
    {
        auto* const v = SERIALIZE_FIND_MEMBER(stream, "D");
        if (v != stream.MemberEnd() && v->value.IsObject() && v->value.MemberCount() != 0)
        {
            _type.Module->DeserializeObject(v->value, this, modifier);
        }
    }
}

ScriptingObject::ScriptingObject(const SpawnParams& params)
    : _gcHandle(0)
    , _type(params.Type)
    , _id(params.ID)
{
    // Managed objects must have valid and unique ID
    ASSERT(_id.IsValid());
}

ScriptingObject::~ScriptingObject()
{
    Deleted(this);

    // Get rid of managed object
    ScriptingObject::DestroyManaged();
    ASSERT(_gcHandle == 0);

    // Handle custom scripting objects removing
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
    {
        _type.Module->OnObjectDeleted(this);
    }

    // Ensure that object has been already unregistered
    if (IsRegistered())
        UnregisterObject();
}

ScriptingObject* ScriptingObject::NewObject(const ScriptingTypeHandle& typeHandle)
{
    if (!typeHandle)
        return nullptr;
    auto& type = typeHandle.GetType();
    if (type.Type != ScriptingTypes::Script)
        return nullptr;
    const ScriptingObjectSpawnParams params(Guid::New(), typeHandle);
    return type.Script.Spawn(params);
}

MObject* ScriptingObject::GetManagedInstance() const
{
#if USE_NETCORE
    const MGCHandle handle = Platform::AtomicRead((int64*)&_gcHandle);
#elif USE_MONO
    const MGCHandle handle = Platform::AtomicRead((int32*)&_gcHandle);
#endif
#if USE_CSHARP
    return handle ? MCore::GCHandle::GetTarget(handle) : nullptr;
#else
    return nullptr;
#endif
}

MObject* ScriptingObject::GetOrCreateManagedInstance() const
{
    MObject* managedInstance = GetManagedInstance();
    if (!managedInstance)
    {
        const_cast<ScriptingObject*>(this)->CreateManaged();
        managedInstance = GetManagedInstance();
    }
    return managedInstance;
}

MClass* ScriptingObject::GetClass() const
{
    return _type ? _type.GetType().ManagedClass : nullptr;
}

ScriptingObject* ScriptingObject::FromInterface(void* interfaceObj, const ScriptingTypeHandle& interfaceType)
{
    if (!interfaceObj || !interfaceType)
        return nullptr;
    PROFILE_CPU();

    // Find the type which implements this interface and has the same vtable as interface object
    // TODO: implement vtableInterface->type hashmap caching in Scripting service to optimize sequential interface casts
    auto& modules = BinaryModule::GetModules();
    for (auto module : modules)
    {
        for (auto& type : module->Types)
        {
            if (type.Type != ScriptingTypes::Script)
                continue;
            auto interfaceImpl = type.GetInterface(interfaceType);
            if (!interfaceImpl || !interfaceImpl->IsNative)
                continue;

            // Get vtable for this type
            void* vtable = type.Script.VTable;
            if (!vtable && type.GetDefaultInstance())
            {
                // Use vtable from default instance of this type
                vtable = *(void***)type.GetDefaultInstance();
            }

            // Check if object interface vtable matches the type interface vtable value
            ScriptingObject* predictedObj = (ScriptingObject*)((byte*)interfaceObj - interfaceImpl->VTableOffset);
            void* predictedVTable = *(void***)predictedObj;
            if (vtable == predictedVTable)
            {
                ASSERT(predictedObj->GetType().GetInterface(interfaceType));
                return predictedObj;
            }

            // Check for case of passing object directly
            predictedObj = (ScriptingObject*)interfaceObj;
            predictedVTable = *(void***)predictedObj;
            if (vtable == predictedVTable)
            {
                ASSERT(predictedObj->GetType().GetInterface(interfaceType));
                return predictedObj;
            }
        }
    }

    // Special case for interface wrapper object
    for (const auto& e : ScriptingObjectsInterfaceWrappers)
    {
        if (e.Value == interfaceObj)
            return e.Key.First;
    }

    return nullptr;
}

void* ScriptingObject::ToInterface(ScriptingObject* obj, const ScriptingTypeHandle& interfaceType)
{
    if (!obj || !interfaceType)
        return nullptr;
    const ScriptingType& objectType = obj->GetType();
    const ScriptingType::InterfaceImplementation* interface = objectType.GetInterface(interfaceType);
    void* result = nullptr;
    if (interface && interface->IsNative)
    {
        // Native interface so just offset pointer to the interface vtable start
        result = (byte*)obj + interface->VTableOffset;
    }
    else if (interface)
    {
        // Interface implemented in scripting (eg. C# class inherits C++ interface)
        const ScriptingObjectsInterfaceKey key(obj, interfaceType);
        if (!ScriptingObjectsInterfaceWrappers.TryGet(key, result))
        {
            result = interfaceType.GetType().Interface.GetInterfaceWrapper(obj);
            ScriptingObjectsInterfaceWrappers.Add(key, result);
        }
    }
    return result;
}

ScriptingObject* ScriptingObject::ToNative(MObject* obj)
{
    ScriptingObject* ptr = nullptr;
#if USE_CSHARP
    if (obj)
    {
#if USE_MONO || USE_MONO_AOT
        const auto ptrField = MCore::Object::GetClass(obj)->GetField(ScriptingObject_unmanagedPtr);
        CHECK_RETURN(ptrField, nullptr);
        ptrField->GetValue(obj, &ptr);
#else
        static const MField* ptrField = MCore::Object::GetClass(obj)->GetField(ScriptingObject_unmanagedPtr);
        ptrField->GetValueReference(obj, &ptr);
#endif
    }
#endif
    return ptr;
}

bool ScriptingObject::Is(const ScriptingTypeHandle& type) const
{
    CHECK_RETURN(type, false);
#if SCRIPTING_OBJECT_CAST_WITH_CSHARP
    return _type == type || CanCast(GetClass(), type.GetType().ManagedClass);
#else
    return CanCast(GetTypeHandle(), type);
#endif
}

void ScriptingObject::ChangeID(const Guid& newId)
{
    ASSERT(newId.IsValid() && newId != _id);

    const Guid prevId = _id;
    _id = newId;

    // Update managed instance
    const auto managedInstance = GetManagedInstance();
    const auto klass = GetClass();
    if (managedInstance && klass)
    {
        const MField* monoIdField = klass->GetField(ScriptingObject_id);
        if (monoIdField)
            monoIdField->SetValue(managedInstance, &_id);
    }

    // Update scripting
    if (IsRegistered())
        Scripting::OnObjectIdChanged(this, prevId);
    _type.GetType().Module->OnObjectIdChanged(this, prevId);
}

void ScriptingObject::SetManagedInstance(MObject* instance)
{
    ASSERT(_gcHandle == 0);
#if USE_NETCORE
    _gcHandle = (MGCHandle)instance;
#elif !COMPILE_WITHOUT_CSHARP
    _gcHandle = MCore::GCHandle::New(instance);
#endif
}

void ScriptingObject::OnManagedInstanceDeleted()
{
    // Release the handle
    if (_gcHandle)
    {
#if USE_CSHARP
        MCore::GCHandle::Free(_gcHandle);
#endif
        _gcHandle = 0;
    }

    // Unregister object
    if (IsRegistered())
        UnregisterObject();
}

void ScriptingObject::OnScriptingDispose()
{
    // Delete C# object
    if (IsRegistered())
        UnregisterObject();
    DestroyManaged();
}

bool ScriptingObject::CreateManaged()
{
#if USE_CSHARP
    MObject* managedInstance = CreateManagedInternal();
    if (!managedInstance)
        return true;

    // Prevent from object GC destruction
#if USE_NETCORE
    auto handle = (MGCHandle)managedInstance;
    auto oldHandle = Platform::InterlockedCompareExchange((int64*)&_gcHandle, *(int64*)&handle, 0);
    if (*(uint64*)&oldHandle != 0)
#else
    auto handle = MCore::GCHandle::New(managedInstance, false);
    auto oldHandle = Platform::InterlockedCompareExchange((int32*)&_gcHandle, *(int32*)&handle, 0);
    if (*(uint32*)&oldHandle != 0)
#endif
    {
        // Other thread already created the object before
        if (const auto klass = GetClass())
        {
            // Reset managed to unmanaged pointer
            MCore::ScriptingObject::SetInternalValues(klass, managedInstance, nullptr, nullptr);
        }
        MCore::GCHandle::Free(handle);
        return true;
    }
#endif

    // Ensure to be registered
    if (!IsRegistered())
        RegisterObject();

    return false;
}

#if USE_CSHARP

MObject* ScriptingObject::CreateManagedInternal()
{
    // Get class
    MClass* klass = GetClass();
    if (klass == nullptr)
    {
        LOG(Warning, "Missing managed class for object with id {0}", GetID());
        return nullptr;
    }

    MObject* managedInstance = MCore::ScriptingObject::CreateScriptingObject(klass, this, &_id);
    if (managedInstance == nullptr)
    {
        LOG(Warning, "Failed to create new instance of the object of type {0}", String(klass->GetFullName()));
    }

    return managedInstance;
}

#endif

void ScriptingObject::DestroyManaged()
{
#if USE_CSHARP
    // Get managed instance
    const auto managedInstance = GetManagedInstance();

    // Reset managed to unmanaged pointer
    if (managedInstance)
    {
        if (const auto klass = GetClass())
        {
            MCore::ScriptingObject::SetInternalValues(klass, managedInstance, nullptr, nullptr);
        }
    }

    // Clear the handle
    if (_gcHandle)
    {
        MCore::GCHandle::Free(_gcHandle);
        _gcHandle = 0;
    }
#else
    _gcHandle = 0;
#endif
}

void ScriptingObject::RegisterObject()
{
    ASSERT(!IsRegistered());
    Flags |= ObjectFlags::IsRegistered;
    Scripting::RegisterObject(this);
}

void ScriptingObject::UnregisterObject()
{
    ASSERT(IsRegistered());
    Flags &= ~ObjectFlags::IsRegistered;
    Scripting::UnregisterObject(this);
}

bool ScriptingObject::CanCast(const ScriptingTypeHandle& from, const ScriptingTypeHandle& to)
{
    if (from == to)
        return true;
    if (!from && !to)
        return true;
    CHECK_RETURN(from && to, false);
#if SCRIPTING_OBJECT_CAST_WITH_CSHARP
    return CanCast(from.GetType().ManagedClass, to.GetType().ManagedClass);
#else
    return to.IsAssignableFrom(from);
#endif
}

bool ScriptingObject::CanCast(const MClass* from, const MClass* to)
{
    if (!from && !to)
        return true;
    CHECK_RETURN(from && to, false);

#if DOTNET_HOST_MONO
    // Cannot enter GC unsafe region if the thread is not attached
    MCore::Thread::Attach();
#endif

    return from->IsSubClassOf(to);
}

void ScriptingObject::OnDeleteObject()
{
    // Cleanup managed object
    DestroyManaged();

    // Unregister
    if (IsRegistered())
        UnregisterObject();

    // Base
    Object::OnDeleteObject();
}

String ScriptingObject::ToString() const
{
    return _type ? String(_type.GetType().Fullname) : String::Empty;
}

ManagedScriptingObject::ManagedScriptingObject(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void ManagedScriptingObject::SetManagedInstance(MObject* instance)
{
    ASSERT(_gcHandle == 0);
#if USE_NETCORE
    _gcHandle = (MGCHandle)instance;
#elif !COMPILE_WITHOUT_CSHARP
    _gcHandle = MCore::GCHandle::NewWeak(instance);
#endif
}

void ManagedScriptingObject::OnManagedInstanceDeleted()
{
    // Base
    ScriptingObject::OnManagedInstanceDeleted();

    // Self destruct
    DeleteObject();
}

void ManagedScriptingObject::OnScriptingDispose()
{
    // Base
    ScriptingObject::OnScriptingDispose();

    // Self destruct
    DeleteObject();
}

bool ManagedScriptingObject::CreateManaged()
{
#if USE_CSHARP
    MObject* managedInstance = CreateManagedInternal();
    if (!managedInstance)
        return true;

    // Cache the GC handle to the object (used to track the target object because it can be moved in a memory)
#if USE_NETCORE
    auto handle = (MGCHandle)managedInstance;
    auto oldHandle = Platform::InterlockedCompareExchange((int64*)&_gcHandle, *(int64*)&handle, 0);
    if (*(uint64*)&oldHandle != 0)
#else
    auto handle = MCore::GCHandle::NewWeak(managedInstance, false);
    auto oldHandle = Platform::InterlockedCompareExchange((int32*)&_gcHandle, *(int32*)&handle, 0);
    if (*(uint32*)&oldHandle != 0)
#endif
    {
        // Other thread already created the object before
        if (const auto klass = GetClass())
        {
            // Reset managed to unmanaged pointer
            MCore::ScriptingObject::SetInternalValues(klass, managedInstance, nullptr, nullptr);
        }
        MCore::GCHandle::Free(handle);
        return true;
    }
#endif

    // Ensure to be registered
    if (!IsRegistered())
        RegisterObject();

    return false;
}

PersistentScriptingObject::PersistentScriptingObject(const SpawnParams& params)
    : ScriptingObject(params)
{
}

#if USE_CSHARP

DEFINE_INTERNAL_CALL(MObject*) ObjectInternal_Create1(MTypeObject* type)
{
    // Peek class for that type (handle generic class cases)
    if (!type)
        DebugLog::ThrowArgumentNull("type");
    MType* mType = INTERNAL_TYPE_OBJECT_GET(type);
    const MTypes mTypeType = MCore::Type::GetType(mType);
    if (mTypeType == MTypes::GenericInst)
    {
        LOG(Error, "Generic scripts are not supported.");
        return nullptr;
    }
    MClass* typeClass = MCore::Type::GetClass(mType);
    if (typeClass == nullptr)
    {
        LOG(Error, "Invalid type.");
        return nullptr;
    }

    // Get the assembly with that class
    auto module = ManagedBinaryModule::FindModule(typeClass);
    if (module == nullptr)
    {
        LOG(Error, "Cannot find scripting assembly for type \'{0}\'.", String(typeClass->GetFullName()));
        return nullptr;
    }

    // Try to find the scripting type for this class
    int32 typeIndex;
    if (!module->ClassToTypeIndex.TryGet(typeClass, typeIndex))
    {
        LOG(Error, "Cannot spawn objects of type \'{0}\'.", String(typeClass->GetFullName()));
        return nullptr;
    }
    const ScriptingType& scriptingType = module->Types[typeIndex];

    // Create unmanaged object
    const ScriptingObjectSpawnParams params(Guid::New(), ScriptingTypeHandle(module, typeIndex));
    ScriptingObject* obj = scriptingType.Script.Spawn(params);
    if (obj == nullptr)
    {
        LOG(Error, "Failed to spawn object of type \'{0}\'.", String(typeClass->GetFullName()));
        return nullptr;
    }

    // Set default name for actors
    if (auto* actor = dynamic_cast<Actor*>(obj))
    {
        actor->SetName(String(typeClass->GetName()));
    }

    // Create managed object
    obj->CreateManaged();
    MObject* managedInstance = obj->GetManagedInstance();
    if (managedInstance == nullptr)
    {
        LOG(Error, "Cannot create managed instance for type \'{0}\'.", String(typeClass->GetFullName()));
        Delete(obj);
    }

    return managedInstance;
}

DEFINE_INTERNAL_CALL(MObject*) ObjectInternal_Create2(MString* typeNameObj)
{
    // Get typename
    if (typeNameObj == nullptr)
        DebugLog::ThrowArgumentNull("typeName");
    const StringView typeNameChars = MCore::String::GetChars(typeNameObj);
    const StringAsANSI<100> typeNameData(typeNameChars.Get(), typeNameChars.Length());
    const StringAnsiView typeName(typeNameData.Get(), typeNameChars.Length());

    // Try to find the scripting type for this typename
    const ScriptingTypeHandle type = Scripting::FindScriptingType(typeName);
    if (!type)
    {
        LOG(Error, "Cannot find scripting type for \'{0}\'.", String(typeName));
        return nullptr;
    }

    // Create unmanaged object
    const ScriptingObjectSpawnParams params(Guid::New(), type);
    ScriptingObject* obj = type.GetType().Script.Spawn(params);
    if (obj == nullptr)
    {
        LOG(Error, "Failed to spawn object of type \'{0}\'.", String(typeName));
        return nullptr;
    }

    // Create managed object
    obj->CreateManaged();
    MObject* managedInstance = obj->GetManagedInstance();
    if (managedInstance == nullptr)
    {
        LOG(Error, "Cannot create managed instance for type \'{0}\'.", String(typeName));
        Delete(obj);
    }

    return managedInstance;
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_ManagedInstanceCreated(MObject* managedInstance, MClass* typeClass)
{
    // Get the assembly with that class
    auto module = ManagedBinaryModule::FindModule(typeClass);
    if (module == nullptr)
    {
        LOG(Error, "Cannot find scripting assembly for type \'{0}\'.", String(typeClass->GetFullName()));
        return;
    }

    // Try to find the scripting type for this class
    int32 typeIndex;
    if (!module->ClassToTypeIndex.TryGet(typeClass, typeIndex))
    {
        LOG(Error, "Cannot spawn objects of type \'{0}\'.", String(typeClass->GetFullName()));
        return;
    }
    const ScriptingType& scriptingType = module->Types[typeIndex];

    // Create unmanaged object
    const ScriptingObjectSpawnParams params(Guid::New(), ScriptingTypeHandle(module, typeIndex));
    ScriptingObject* obj = scriptingType.Script.Spawn(params);
    if (obj == nullptr)
    {
        LOG(Error, "Failed to spawn object of type \'{0}\'.", String(typeClass->GetFullName()));
        return;
    }

    // Link created managed instance to the unmanaged object
    obj->SetManagedInstance(managedInstance);

    // Set default name for actors
    if (auto* actor = dynamic_cast<Actor*>(obj))
    {
        actor->SetName(String(typeClass->GetName()));
    }

    MClass* klass = obj->GetClass();
    const Guid id = obj->GetID();
    MCore::ScriptingObject::SetInternalValues(klass, managedInstance, obj, &id);

    // Register object
    if (!obj->IsRegistered())
        obj->RegisterObject();
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_ManagedInstanceDeleted(ScriptingObject* obj)
{
    Scripting::OnManagedInstanceDeleted(obj);
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_Destroy(ScriptingObject* obj, float timeLeft)
{
    // Use scaled game time for removing actors/scripts by the user (maybe expose it to the api?)
    const bool useGameTime = timeLeft > ZeroTolerance;

    if (obj)
        obj->DeleteObject(timeLeft, useGameTime);
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_DestroyNow(ScriptingObject* obj)
{
    if (obj)
        obj->DeleteObjectNow();
}

DEFINE_INTERNAL_CALL(MString*) ObjectInternal_GetTypeName(ScriptingObject* obj)
{
    INTERNAL_CALL_CHECK_RETURN(obj, nullptr);
    return MUtils::ToString(obj->GetType().Fullname);
}

DEFINE_INTERNAL_CALL(MObject*) ObjectInternal_FindObject(Guid* id, MTypeObject* type, bool skipLog = false)
{
    if (!id->IsValid())
        return nullptr;
    MClass* klass = MUtils::GetClass(type);
    ScriptingObject* obj = Scripting::TryFindObject(*id);
    if (!obj)
    {
        if (!klass || klass == ScriptingObject::GetStaticClass() || klass->IsSubClassOf(Asset::GetStaticClass()))
        {
            obj = Content::LoadAsync<Asset>(*id);
        }
    }
    if (obj)
    {
        if (klass && !obj->Is(klass))
        {
            if (!skipLog)
            {
                LOG(Warning, "Found scripting object with ID={0} of type {1} that doesn't match type {2}", *id, String(obj->GetType().Fullname), String(klass->GetFullName()));
                LogContext::Print(LogType::Warning);
            }
            return nullptr;
        }
        return obj->GetOrCreateManagedInstance();
    }

    if (!skipLog)
    {
        if (klass)
            LOG(Warning, "Unable to find scripting object with ID={0} of type {1}", *id, String(klass->GetFullName()));
        else
            LOG(Warning, "Unable to find scripting object with ID={0}", *id);
        LogContext::Print(LogType::Warning);
    }
    return nullptr;
}

DEFINE_INTERNAL_CALL(MObject*) ObjectInternal_TryFindObject(Guid* id, MTypeObject* type)
{
    ScriptingObject* obj = Scripting::TryFindObject(*id);
    if (obj && !obj->Is(MUtils::GetClass(type)))
        obj = nullptr;
    return obj ? obj->GetOrCreateManagedInstance() : nullptr;
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_ChangeID(ScriptingObject* obj, Guid* id)
{
    INTERNAL_CALL_CHECK(obj);
    obj->ChangeID(*id);
}

DEFINE_INTERNAL_CALL(void*) ObjectInternal_GetUnmanagedInterface(ScriptingObject* obj, MTypeObject* type)
{
    if (obj && type)
    {
        MClass* typeClass = MUtils::GetClass(type);
        const ScriptingTypeHandle interfaceType = ManagedBinaryModule::FindType(typeClass);
        if (interfaceType)
        {
            return ScriptingObject::ToInterface(obj, interfaceType);
        }
    }
    return nullptr;
}

DEFINE_INTERNAL_CALL(MObject*) ObjectInternal_FromUnmanagedPtr(ScriptingObject* obj)
{
    MObject* result = nullptr;
    if (obj)
        result = obj->GetOrCreateManagedInstance();
    return result;
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_MapObjectID(Guid* id)
{
    const auto idsMapping = Scripting::ObjectsLookupIdMapping.Get();
    if (idsMapping && id->IsValid())
        idsMapping->TryGet(*id, *id);
}

DEFINE_INTERNAL_CALL(void) ObjectInternal_RemapObjectID(Guid* id)
{
    const auto idsMapping = Scripting::ObjectsLookupIdMapping.Get();
    if (idsMapping && id->IsValid())
        idsMapping->KeyOf(*id, id);
}
#endif

class ScriptingObjectInternal
{
public:
    static void InitRuntime()
    {
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_Create1", &ObjectInternal_Create1);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_Create2", &ObjectInternal_Create2);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_ManagedInstanceCreated", &ObjectInternal_ManagedInstanceCreated);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_ManagedInstanceDeleted", &ObjectInternal_ManagedInstanceDeleted);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_Destroy", &ObjectInternal_Destroy);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_DestroyNow", &ObjectInternal_DestroyNow);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_GetTypeName", &ObjectInternal_GetTypeName);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_FindObject", &ObjectInternal_FindObject);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_TryFindObject", &ObjectInternal_TryFindObject);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_ChangeID", &ObjectInternal_ChangeID);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_GetUnmanagedInterface", &ObjectInternal_GetUnmanagedInterface);
        ADD_INTERNAL_CALL("FlaxEngine.Object::FromUnmanagedPtr", &ObjectInternal_FromUnmanagedPtr);
        ADD_INTERNAL_CALL("FlaxEngine.Object::MapObjectID", &ObjectInternal_MapObjectID);
        ADD_INTERNAL_CALL("FlaxEngine.Object::RemapObjectID", &ObjectInternal_RemapObjectID);
    }

    static ScriptingObject* Spawn(const ScriptingObjectSpawnParams& params)
    {
        return New<ScriptingObject>(params);
    }
};

ScriptingTypeInitializer ScriptingObject::TypeInitializer(GetBinaryModuleFlaxEngine(), StringAnsiView("FlaxEngine.Object", ARRAY_COUNT("FlaxEngine.Object") - 1), sizeof(ScriptingObject), &ScriptingObjectInternal::InitRuntime, &ScriptingObjectInternal::Spawn);
