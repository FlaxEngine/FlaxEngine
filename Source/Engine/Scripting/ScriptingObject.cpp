// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ScriptingObject.h"
#include "Scripting.h"
#include "BinaryModule.h"
#include "InternalCalls.h"
#include "Engine/Level/Actor.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include "ManagedCLR/MAssembly.h"
#include "ManagedCLR/MClass.h"
#include "ManagedCLR/MUtils.h"
#include "ManagedCLR/MField.h"
#if PLATFORM_LINUX
#include "ManagedCLR/MCore.h"
#endif
#include "FlaxEngine.Gen.h"
#include <ThirdParty/mono-2.0/mono/metadata/object.h>
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

#define ScriptingObject_unmanagedPtr "__unmanagedPtr"
#define ScriptingObject_id "__internalId"

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
    // Ensure that GC handle is empty
    ASSERT(_gcHandle == 0);

    // Ensure that object has been already unregistered
    if (IsRegistered())
        UnregisterObject();

    Deleted(this);

    // Handle custom scripting objects removing
    if (Flags & ObjectFlags::IsCustomScriptingType)
    {
        _type.Module->OnObjectDeleted(this);
    }
}

MonoObject* ScriptingObject::GetManagedInstance() const
{
    return _gcHandle ? mono_gchandle_get_target(_gcHandle) : nullptr;
}

MClass* ScriptingObject::GetClass() const
{
    return _type ? _type.GetType().ManagedClass : nullptr;
}

ScriptingObject* ScriptingObject::ToNative(MonoObject* obj)
{
    ScriptingObject* ptr = nullptr;
    if (obj)
    {
        // TODO: cache the field offset from object and read directly from object pointer
        const auto ptrField = mono_class_get_field_from_name(mono_object_get_class(obj), ScriptingObject_unmanagedPtr);
        CHECK_RETURN(ptrField, nullptr);
        mono_field_get_value(obj, ptrField, &ptr);
    }
    return ptr;
}

void ScriptingObject::ChangeID(const Guid& newId)
{
    ASSERT(newId.IsValid() && newId != _id);

    const Guid prevId = _id;
    _id = newId;

    // Update managed instance
    const auto managedInstance = GetManagedInstance();
    const auto monoClass = GetClass();
    if (managedInstance && monoClass)
    {
        const MField* monoIdField = monoClass->GetField(ScriptingObject_id);
        if (monoIdField)
            monoIdField->SetValue(managedInstance, &_id);
    }

    // Update scripting
    if (IsRegistered())
        Scripting::OnObjectIdChanged(this, prevId);
    _type.GetType().Module->OnObjectIdChanged(this, prevId);
}

void ScriptingObject::OnManagedInstanceDeleted()
{
    // Release the handle
    if (_gcHandle)
    {
        mono_gchandle_free(_gcHandle);
        _gcHandle = 0;
    }

    // Unregister object
    if (IsRegistered())
        UnregisterObject();

    // Self destruct
    DeleteObject();
}

void ScriptingObject::OnScriptingDispose()
{
    // Delete C# object
    DestroyManaged();

    // Delete C++ object
    DeleteObject();
}

MonoObject* ScriptingObject::CreateManagedInternal()
{
#if BUILD_DEBUG
    ASSERT(!HasManagedInstance());
#endif

    // Get class
    MClass* monoClass = GetClass();
    if (monoClass == nullptr)
    {
        LOG(Warning, "Missing managed class for object with id {0}", GetID());
        return nullptr;
    }

    // Allocate managed instance
    MonoObject* managedInstance = mono_object_new(mono_domain_get(), monoClass->GetNative());
    if (managedInstance == nullptr)
    {
        LOG(Warning, "Failed to create new instance of the object of type {0}", String(monoClass->GetFullName()));
    }

    // Set handle to unmanaged object
    const MField* monoUnmanagedPtrField = monoClass->GetField(ScriptingObject_unmanagedPtr);
    if (monoUnmanagedPtrField)
    {
        const void* value = this;
        monoUnmanagedPtrField->SetValue(managedInstance, &value);
    }

    // Set object id
    const MField* monoIdField = monoClass->GetField(ScriptingObject_id);
    if (monoIdField)
    {
        monoIdField->SetValue(managedInstance, (void*)&_id);
    }

    // Initialize managed instance (calls constructor)
    mono_runtime_object_init(managedInstance);

    return managedInstance;
}

void ScriptingObject::DestroyManaged()
{
    // Get managed instance
    const auto managedInstance = GetManagedInstance();

    // Reset managed to unmanaged pointer
    if (managedInstance)
    {
        if (const auto monoClass = GetClass())
        {
            const MField* monoUnmanagedPtrField = monoClass->GetField(ScriptingObject_unmanagedPtr);
            if (monoUnmanagedPtrField)
            {
                void* param = nullptr;
                monoUnmanagedPtrField->SetValue(managedInstance, &param);
            }
        }
    }

    // Clear the handle
    if (_gcHandle)
    {
        mono_gchandle_free(_gcHandle);
        _gcHandle = 0;
    }
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

bool ScriptingObject::CanCast(MClass* from, MClass* to)
{
    if (!from && !to)
        return true;
    CHECK_RETURN(from && to, false);

#if PLATFORM_LINUX
    // Cannot enter GC unsafe region if the thread is not attached
    MCore::AttachThread();
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
    RemovableObject::OnDeleteObject();
}

String ScriptingObject::ToString() const
{
    return _type ? _type.GetType().ManagedClass->ToString() : String::Empty;
}

ManagedScriptingObject::ManagedScriptingObject(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void ManagedScriptingObject::CreateManaged()
{
    MonoObject* managedInstance = CreateManagedInternal();
    if (managedInstance)
    {
        // Cache the GC handle to the object (used to track the target object because it can be moved in a memory)
        _gcHandle = mono_gchandle_new_weakref(managedInstance, false);

        // Ensure to be registered
        if (!IsRegistered())
            RegisterObject();
    }
}

PersistentScriptingObject::PersistentScriptingObject(const SpawnParams& params)
    : ScriptingObject(params)
{
}

PersistentScriptingObject::~PersistentScriptingObject()
{
    PersistentScriptingObject::DestroyManaged();
}

void PersistentScriptingObject::OnManagedInstanceDeleted()
{
    // Cleanup
    if (_gcHandle)
    {
        mono_gchandle_free(_gcHandle);
        _gcHandle = 0;
    }

    // But do not delete itself
}

void PersistentScriptingObject::OnScriptingDispose()
{
    // Delete C# object
    if (IsRegistered())
        UnregisterObject();
    DestroyManaged();

    // Don't delete C++ object
}

void PersistentScriptingObject::CreateManaged()
{
    MonoObject* managedInstance = CreateManagedInternal();
    if (managedInstance)
    {
        // Prevent form object GC destruction and moving
        _gcHandle = mono_gchandle_new(managedInstance, false);

        // Ensure to be registered
        if (!IsRegistered())
            RegisterObject();
    }
}

class ScriptingObjectInternal
{
public:

    static MonoObject* Create1(MonoReflectionType* type)
    {
        // Peek class for that type (handle generic class cases)
        if (!type)
            DebugLog::ThrowArgumentNull("type");
        MonoType* monoType = mono_reflection_type_get_type(type);
        const int32 monoTypeType = mono_type_get_type(monoType);
        if (monoTypeType == MONO_TYPE_GENERICINST)
        {
            LOG(Error, "Generic scripts are not supported.");
            return nullptr;
        }
        MonoClass* typeClass = mono_type_get_class(monoType);
        if (typeClass == nullptr)
        {
            LOG(Error, "Invalid type.");
            return nullptr;
        }

        // Get the assembly with that class
        // TODO: consider caching lookup table MonoImage* -> ManagedBinaryModule*
        ManagedBinaryModule* module = nullptr;
        MonoImage* mImage = mono_class_get_image(typeClass);
        auto& modules = BinaryModule::GetModules();
        for (auto e : modules)
        {
            auto managedModule = dynamic_cast<ManagedBinaryModule*>(e);
            if (managedModule && managedModule->Assembly->GetMonoImage() == mImage)
            {
                module = managedModule;
                break;
            }
        }
        if (module == nullptr)
        {
            LOG(Error, "Cannot find scripting assembly for type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            return nullptr;
        }

        // Try to find the scripting type for this class
        int32 typeIndex;
        if (!module->ClassToTypeIndex.TryGet(typeClass, typeIndex))
        {
            LOG(Error, "Cannot spawn objects of type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            return nullptr;
        }
        const ScriptingType& scriptingType = module->Types[typeIndex];

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(Guid::New(), ScriptingTypeHandle(module, typeIndex));
        ScriptingObject* obj = scriptingType.Class.Spawn(params);
        if (obj == nullptr)
        {
            LOG(Error, "Failed to spawn object of type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            return nullptr;
        }

        // Set default name for actors
        if (auto* actor = dynamic_cast<Actor*>(obj))
        {
            actor->SetName(String(mono_class_get_name(typeClass)));
        }

        // Create managed object
        obj->CreateManaged();
        MonoObject* managedInstance = obj->GetManagedInstance();
        if (managedInstance == nullptr)
        {
            LOG(Error, "Cannot create managed instance for type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            Delete(obj);
        }

        return managedInstance;
    }

    static MonoObject* Create2(MonoString* typeNameObj)
    {
        // Get typename
        if (typeNameObj == nullptr)
            DebugLog::ThrowArgumentNull("typeName");
        const StringAsANSI<> typeNameData((const Char*)mono_string_chars(typeNameObj), (int32)mono_string_length(typeNameObj));
        const StringAnsiView typeName(typeNameData.Get(), (int32)mono_string_length(typeNameObj));

        // Try to find the scripting type for this typename
        const ScriptingTypeHandle type = Scripting::FindScriptingType(typeName);
        if (!type)
        {
            LOG(Error, "Cannot find scripting type for \'{0}\'.", String(typeName));
            return nullptr;
        }

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(Guid::New(), type);
        ScriptingObject* obj = type.GetType().Class.Spawn(params);
        if (obj == nullptr)
        {
            LOG(Error, "Failed to spawn object of type \'{0}\'.", String(typeName));
            return nullptr;
        }

        // Create managed object
        obj->CreateManaged();
        MonoObject* managedInstance = obj->GetManagedInstance();
        if (managedInstance == nullptr)
        {
            LOG(Error, "Cannot create managed instance for type \'{0}\'.", String(typeName));
            Delete(obj);
        }

        return managedInstance;
    }

    static void ManagedInstanceCreated(MonoObject* managedInstance)
    {
        MonoClass* typeClass = mono_object_get_class(managedInstance);

        // Get the assembly with that class
        // TODO: consider caching lookup table MonoImage* -> ManagedBinaryModule*
        ManagedBinaryModule* module = nullptr;
        MonoImage* mimage = mono_class_get_image(typeClass);
        auto& modules = BinaryModule::GetModules();
        for (auto e : modules)
        {
            auto managedModule = dynamic_cast<ManagedBinaryModule*>(e);
            if (managedModule && managedModule->Assembly->GetMonoImage() == mimage)
            {
                module = managedModule;
                break;
            }
        }
        if (module == nullptr)
        {
            LOG(Error, "Cannot find scripting assembly for type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            return;
        }

        // Try to find the scripting type for this class
        int32 typeIndex;
        if (!module->ClassToTypeIndex.TryGet(typeClass, typeIndex))
        {
            LOG(Error, "Cannot spawn objects of type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            return;
        }
        const ScriptingType& scriptingType = module->Types[typeIndex];

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(Guid::New(), ScriptingTypeHandle(module, typeIndex));
        ScriptingObject* obj = scriptingType.Class.Spawn(params);
        if (obj == nullptr)
        {
            LOG(Error, "Failed to spawn object of type \'{0}.{1}\'.", String(mono_class_get_namespace(typeClass)), String(mono_class_get_name(typeClass)));
            return;
        }

        // Set default name for actors
        if (auto* actor = dynamic_cast<Actor*>(obj))
        {
            actor->SetName(String(mono_class_get_name(typeClass)));
        }

        // Link created managed instance to the unmanaged object
        if (auto* managedScriptingObject = dynamic_cast<ManagedScriptingObject*>(obj))
        {
            // Managed
            managedScriptingObject->_gcHandle = mono_gchandle_new_weakref(managedInstance, false);
        }
        else
        {
            // Persistent
            obj->_gcHandle = mono_gchandle_new(managedInstance, false);
        }

        MClass* monoClass = obj->GetClass();

        // Set handle to unmanaged object
        const MField* monoUnmanagedPtrField = monoClass->GetField(ScriptingObject_unmanagedPtr);
        if (monoUnmanagedPtrField)
        {
            const void* value = obj;
            monoUnmanagedPtrField->SetValue(managedInstance, &value);
        }

        // Set object id
        const MField* monoIdField = monoClass->GetField(ScriptingObject_id);
        if (monoIdField)
        {
            monoIdField->SetValue(managedInstance, (void*)&obj->_id);
        }

        // Register object
        if (!obj->IsRegistered())
            obj->RegisterObject();
    }

    static void Destroy(ManagedScriptingObject* obj, float timeLeft)
    {
        // Use scaled game time for removing actors/scripts by the user (maybe expose it to the api?)
        const bool useGameTime = timeLeft > ZeroTolerance;

        if (obj)
            obj->DeleteObject(timeLeft, useGameTime);
    }

    static MonoString* GetTypeName(ManagedScriptingObject* obj)
    {
        INTERNAL_CALL_CHECK_RETURN(obj, nullptr);
        return MUtils::ToString(obj->GetType().Fullname);
    }

    static MonoObject* FindObject(Guid* id, MonoReflectionType* type)
    {
        ScriptingObject* obj = Scripting::FindObject(*id, Scripting::FindClass(MUtils::GetClass(type)));
        return obj ? obj->GetOrCreateManagedInstance() : nullptr;
    }

    static MonoObject* TryFindObject(Guid* id, MonoReflectionType* type)
    {
        ScriptingObject* obj = Scripting::TryFindObject(*id, Scripting::FindClass(MUtils::GetClass(type)));
        return obj ? obj->GetOrCreateManagedInstance() : nullptr;
    }

    static void ChangeID(ManagedScriptingObject* obj, Guid* id)
    {
        INTERNAL_CALL_CHECK(obj);
        obj->ChangeID(*id);
    }

    static void InitRuntime()
    {
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_Create1", &Create1);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_Create2", &Create2);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_ManagedInstanceCreated", &ManagedInstanceCreated);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_ManagedInstanceDeleted", &Scripting::OnManagedInstanceDeleted);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_Destroy", &Destroy);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_GetTypeName", &GetTypeName);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_FindObject", &FindObject);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_TryFindObject", &TryFindObject);
        ADD_INTERNAL_CALL("FlaxEngine.Object::Internal_ChangeID", &ChangeID);
    }
};

IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(ScriptingObject, FlaxEngine, "FlaxEngine.Object", nullptr, nullptr);
