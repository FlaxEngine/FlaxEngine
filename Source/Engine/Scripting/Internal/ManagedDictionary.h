// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Scripting/Scripting.h"
#if USE_CSHARP
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/Internal/StdTypesContainer.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// Utility interop between C++ and C# for Dictionary collection.
/// </summary>
struct FLAXENGINE_API ManagedDictionary
{
public:
    struct KeyValueType
    {
        MType* keyType;
        MType* valueType;

        bool operator==(const KeyValueType& other) const
        {
            return keyType == other.keyType && valueType == other.valueType;
        }
    };

private:
    static Dictionary<KeyValueType, MTypeObject*> CachedDictionaryTypes;

#if !USE_MONO_AOT
    typedef MTypeObject* (*MakeGenericTypeThunk)(MObject* instance, MTypeObject* genericType, MArray* genericArgs, MObject** exception);
    static MakeGenericTypeThunk MakeGenericType;

    typedef MObject* (*CreateInstanceThunk)(MObject* instance, MTypeObject* type, void* arr, MObject** exception);
    static CreateInstanceThunk CreateInstance;

    typedef void (*AddDictionaryItemThunk)(MObject* instance, MObject* dictionary, MObject* key, MObject* value, MObject** exception);
    static AddDictionaryItemThunk AddDictionaryItem;

    typedef MArray* (*GetDictionaryKeysThunk)(MObject* instance, MObject* dictionary, MObject** exception);
    static GetDictionaryKeysThunk GetDictionaryKeys;
#else
    static MMethod* MakeGenericType;
    static MMethod* CreateInstance;
    static MMethod* AddDictionaryItem;
    static MMethod* GetDictionaryKeys;
#endif

public:
    MObject* Instance;

    ManagedDictionary(MObject* instance = nullptr)
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

    template<typename KeyType, typename ValueType>
    static MObject* ToManaged(const Dictionary<KeyType, ValueType>& data, MType* keyType, MType* valueType)
    {
        MConverter<KeyType> keysConverter;
        MConverter<ValueType> valueConverter;
        ManagedDictionary result = New(keyType, valueType);
        MClass* keyClass = MCore::Type::GetClass(keyType);
        MClass* valueClass = MCore::Type::GetClass(valueType);
        for (auto i = data.Begin(); i.IsNotEnd(); ++i)
        {
            MObject* keyManaged = keysConverter.Box(i->Key, keyClass);
            MObject* valueManaged = valueConverter.Box(i->Value, valueClass);
            result.Add(keyManaged, valueManaged);
        }
        return result.Instance;
    }

    /// <summary>
    /// Converts the managed dictionary objects into the native dictionary collection.
    /// </summary>
    /// <param name="managed">The managed dictionary object.</param>
    /// <returns>The output array.</returns>
    template<typename KeyType, typename ValueType>
    static Dictionary<KeyType, ValueType> ToNative(MObject* managed)
    {
        Dictionary<KeyType, ValueType> result;
        const ManagedDictionary wrapper(managed);
        MArray* managedKeys = wrapper.GetKeys();
        if (managedKeys == nullptr)
            return result;
        int32 length = MCore::Array::GetLength(managedKeys);
        Array<KeyType> keys;
        keys.Resize(length);
        result.EnsureCapacity(length);
        MConverter<KeyType> keysConverter;
        MConverter<ValueType> valueConverter;
        MObject** managedKeysPtr = MCore::Array::GetAddress<MObject*>(managedKeys);
        for (int32 i = 0; i < keys.Count(); i++)
        {
            KeyType& key = keys[i];
            MObject* keyManaged = managedKeysPtr[i];
            keysConverter.Unbox(key, keyManaged);
            MObject* valueManaged = wrapper.GetValue(keyManaged);
            ValueType& value = result[key];
            valueConverter.Unbox(value, valueManaged);
        }
        return result;
    }

    static MTypeObject* GetClass(MType* keyType, MType* valueType)
    {
        // Check if the generic type was generated earlier
        KeyValueType cacheKey = { keyType, valueType };
        MTypeObject* dictionaryType;
        if (CachedDictionaryTypes.TryGet(cacheKey, dictionaryType))
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
        CachedDictionaryTypes.Add(cacheKey, dictionaryType);
        return dictionaryType;
    }

    static ManagedDictionary New(MType* keyType, MType* valueType)
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

    void Add(MObject* key, MObject* value)
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

    MArray* GetKeys() const
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

    MObject* GetValue(MObject* key) const
    {
        CHECK_RETURN(Instance, nullptr);
        MClass* klass = MCore::Object::GetClass(Instance);
        MMethod* getItemMethod = klass->GetMethod("System.Collections.IDictionary.get_Item", 1);
        CHECK_RETURN(getItemMethod, nullptr);
        void* params[1];
        params[0] = key;
        return getItemMethod->Invoke(Instance, params, nullptr);
    }
};

inline uint32 GetHash(const ManagedDictionary::KeyValueType& other)
{
    uint32 hash = ::GetHash((void*)other.keyType);
    CombineHash(hash, ::GetHash((void*)other.valueType));
    return hash;
}

#endif
