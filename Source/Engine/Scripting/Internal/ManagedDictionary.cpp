// Copyright (c) Wojciech Figat. All rights reserved.

#include "ManagedDictionary.h"

#if USE_CSHARP
Dictionary<ManagedDictionary::KeyValueType, MTypeObject*> ManagedDictionary::CachedTypes;
#if !USE_MONO_AOT
ManagedDictionary::MakeGenericTypeThunk ManagedDictionary::MakeGenericType;
ManagedDictionary::CreateInstanceThunk ManagedDictionary::CreateInstance;
ManagedDictionary::AddDictionaryItemThunk ManagedDictionary::AddDictionaryItem;
ManagedDictionary::GetDictionaryKeysThunk ManagedDictionary::GetDictionaryKeys;
#else
MMethod* ManagedDictionary::MakeGenericType;
MMethod* ManagedDictionary::CreateInstance;
MMethod* ManagedDictionary::AddDictionaryItem;
MMethod* ManagedDictionary::GetDictionaryKeys;
#endif

ManagedDictionary::ManagedDictionary(MObject* instance)
{
    Instance = instance;

#if !USE_MONO_AOT
    // Cache the thunks of the dictionary helper methods
    if (MakeGenericType == nullptr)
    {
        MClass* scriptingClass = Scripting::GetStaticClass();
        CHECK(scriptingClass);

        MMethod* makeGenericTypeMethod = scriptingClass->GetMethod("MakeGenericType", 2);
        CHECK(makeGenericTypeMethod);
        MakeGenericType = (MakeGenericTypeThunk)makeGenericTypeMethod->GetThunk();

        MMethod* createInstanceMethod = StdTypesContainer::Instance()->ActivatorClass->GetMethod("CreateInstance", 2);
        CHECK(createInstanceMethod);
        CreateInstance = (CreateInstanceThunk)createInstanceMethod->GetThunk();

        MMethod* addDictionaryItemMethod = scriptingClass->GetMethod("AddDictionaryItem", 3);
        CHECK(addDictionaryItemMethod);
        AddDictionaryItem = (AddDictionaryItemThunk)addDictionaryItemMethod->GetThunk();

        MMethod* getDictionaryKeysItemMethod = scriptingClass->GetMethod("GetDictionaryKeys", 1);
        CHECK(getDictionaryKeysItemMethod);
        GetDictionaryKeys = (GetDictionaryKeysThunk)getDictionaryKeysItemMethod->GetThunk();
    }
#else
    if (MakeGenericType == nullptr)
    {
        MClass* scriptingClass = Scripting::GetStaticClass();
        CHECK(scriptingClass);

        MakeGenericType = scriptingClass->GetMethod("MakeGenericType", 2);
        CHECK(MakeGenericType);

        CreateInstance = StdTypesContainer::Instance()->ActivatorClass->GetMethod("CreateInstance", 2);
        CHECK(CreateInstance);

        AddDictionaryItem = scriptingClass->GetMethod("AddDictionaryItem", 3);
        CHECK(AddDictionaryItem);

        GetDictionaryKeys = scriptingClass->GetMethod("GetDictionaryKeys", 1);
        CHECK(GetDictionaryKeys);
    }
#endif
}

MTypeObject* ManagedDictionary::GetClass(MType* keyType, MType* valueType)
{
    // Check if the generic type was generated earlier
    KeyValueType cacheKey = { keyType, valueType };
    MTypeObject* dictionaryType;
    if (CachedTypes.TryGet(cacheKey, dictionaryType))
        return dictionaryType;

    MTypeObject* genericType = MUtils::GetType(StdTypesContainer::Instance()->DictionaryClass);
#if USE_NETCORE
    MArray* genericArgs = MCore::Array::New(MCore::TypeCache::IntPtr, 2);
#else
    MArray* genericArgs = MCore::Array::New(MCore::TypeCache::Object, 2);
#endif
    MTypeObject** genericArgsPtr = MCore::Array::GetAddress<MTypeObject*>(genericArgs);
    genericArgsPtr[0] = INTERNAL_TYPE_GET_OBJECT(keyType);
    genericArgsPtr[1] = INTERNAL_TYPE_GET_OBJECT(valueType);

    MObject* exception = nullptr;
#if !USE_MONO_AOT
    dictionaryType = MakeGenericType(nullptr, genericType, genericArgs, &exception);
#else
    void* params[2];
    params[0] = genericType;
    params[1] = genericArgs;
    dictionaryType = (MTypeObject*)MakeGenericType->Invoke(nullptr, params, &exception);
#endif
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT(""));
        return nullptr;
    }
    CachedTypes.Add(cacheKey, dictionaryType);
    return dictionaryType;
}

ManagedDictionary ManagedDictionary::New(MType* keyType, MType* valueType)
{
    ManagedDictionary result;
    MTypeObject* dictionaryType = GetClass(keyType, valueType);
    if (!dictionaryType)
        return result;

    MObject* exception = nullptr;
#if !USE_MONO_AOT
    MObject* instance = CreateInstance(nullptr, dictionaryType, nullptr, &exception);
#else
    void* params[2];
    params[0] = dictionaryType;
    params[1] = nullptr;
    MObject* instance = CreateInstance->Invoke(nullptr, params, &exception);
#endif
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT(""));
        return result;
    }

    result.Instance = instance;
    return result;
}

void ManagedDictionary::Add(MObject* key, MObject* value)
{
    CHECK(Instance);

    MObject* exception = nullptr;
#if !USE_MONO_AOT
    AddDictionaryItem(nullptr, Instance, key, value, &exception);
#else
    void* params[3];
    params[0] = Instance;
    params[1] = key;
    params[2] = value;
    AddDictionaryItem->Invoke(Instance, params, &exception);
#endif
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT(""));
    }
}

MArray* ManagedDictionary::GetKeys() const
{
    CHECK_RETURN(Instance, nullptr);
#if !USE_MONO_AOT
    return GetDictionaryKeys(nullptr, Instance, nullptr);
#else
    void* params[1];
    params[0] = Instance;
    return (MArray*)GetDictionaryKeys->Invoke(nullptr, params, nullptr);
#endif
}

MObject* ManagedDictionary::GetValue(MObject* key) const
{
    CHECK_RETURN(Instance, nullptr);
    MClass* klass = MCore::Object::GetClass(Instance);
    MMethod* getItemMethod = klass->GetMethod("System.Collections.IDictionary.get_Item", 1);
    CHECK_RETURN(getItemMethod, nullptr);
    void* params[1];
    params[0] = key;
    return getItemMethod->Invoke(Instance, params, nullptr);
}
#endif
