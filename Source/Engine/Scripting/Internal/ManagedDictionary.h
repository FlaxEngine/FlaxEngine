// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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

/// <summary>
/// Utility interop between C++ and C# for Dictionary collection.
/// </summary>
struct FLAXENGINE_API ManagedDictionary
{
    MObject* Instance;

    ManagedDictionary(MObject* instance = nullptr)
    {
        Instance = instance;
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
        MClass* scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, nullptr);
        MMethod* makeGenericMethod = scriptingClass->GetMethod("MakeGenericType", 2);
        CHECK_RETURN(makeGenericMethod, nullptr);

        MTypeObject* genericType = MUtils::GetType(StdTypesContainer::Instance()->DictionaryClass);
#if USE_NETCORE
        MArray* genericArgs = MCore::Array::New(MCore::TypeCache::IntPtr, 2);
#else
        MArray* genericArgs = MCore::Array::New(MCore::TypeCache::Object, 2);
#endif
        MTypeObject** genericArgsPtr = MCore::Array::GetAddress<MTypeObject*>(genericArgs);
        genericArgsPtr[0] = INTERNAL_TYPE_GET_OBJECT(keyType);
        genericArgsPtr[1] = INTERNAL_TYPE_GET_OBJECT(valueType);

        void* params[2];
        params[0] = genericType;
        params[1] = genericArgs;
        MObject* exception = nullptr;
        MObject* dictionaryType = makeGenericMethod->Invoke(nullptr, params, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT(""));
            return nullptr;
        }
        return (MTypeObject*)dictionaryType;
    }

    static ManagedDictionary New(MType* keyType, MType* valueType)
    {
        ManagedDictionary result;
        MTypeObject* dictionaryType = GetClass(keyType, valueType);
        if (!dictionaryType)
            return result;

        MClass* scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, result);
        MMethod* createMethod = StdTypesContainer::Instance()->ActivatorClass->GetMethod("CreateInstance", 2);
        CHECK_RETURN(createMethod, result);

        MObject* exception = nullptr;
        void* params[2];
        params[0] = dictionaryType;
        params[1] = nullptr;
        MObject* instance = createMethod->Invoke(nullptr, params, &exception);
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
        MClass* scriptingClass = Scripting::GetStaticClass();
        CHECK(scriptingClass);
        MMethod* addDictionaryItemMethod = scriptingClass->GetMethod("AddDictionaryItem", 3);
        CHECK(addDictionaryItemMethod);
        void* params[3];
        params[0] = Instance;
        params[1] = key;
        params[2] = value;
        MObject* exception = nullptr;
        addDictionaryItemMethod->Invoke(Instance, params, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT(""));
        }
    }

    MArray* GetKeys() const
    {
        CHECK_RETURN(Instance, nullptr);
        MClass* scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, nullptr);
        MMethod* getDictionaryKeysMethod = scriptingClass->GetMethod("GetDictionaryKeys", 1);
        CHECK_RETURN(getDictionaryKeysMethod, nullptr);
        void* params[1];
        params[0] = Instance;
        return (MArray*)getDictionaryKeysMethod->Invoke( nullptr, params, nullptr);
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

#endif
