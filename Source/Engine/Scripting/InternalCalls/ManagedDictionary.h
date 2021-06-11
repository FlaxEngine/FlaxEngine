// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/StdTypesContainer.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/MException.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

struct FLAXENGINE_API ManagedDictionary
{
    MonoObject* Instance;

    ManagedDictionary(MonoObject* instance = nullptr)
    {
        Instance = instance;
    }

    template<typename KeyType, typename ValueType>
    static MonoObject* ToManaged(const Dictionary<KeyType, ValueType>& data, MonoType* keyType, MonoType* valueType)
    {
        MConverter<KeyType> keysConverter;
        MConverter<ValueType> valueConverter;
        ManagedDictionary result = New(keyType, valueType);
        MonoClass* keyClass = mono_type_get_class(keyType);
        MonoClass* valueClass = mono_type_get_class(valueType);
        for (auto i = data.Begin(); i.IsNotEnd(); ++i)
        {
            MonoObject* keyManaged = keysConverter.Box(i->Key, keyClass);
            MonoObject* valueManaged = valueConverter.Box(i->Value, valueClass);
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
    static Dictionary<KeyType, ValueType> ToNative(MonoObject* managed)
    {
        Dictionary<KeyType, ValueType> result;
        const ManagedDictionary wrapper(managed);
        MonoArray* managedKeys = wrapper.GetKeys();
        auto length = managedKeys ? (int32)mono_array_length(managedKeys) : 0;
        Array<KeyType> keys;
        keys.Resize(length);
        result.EnsureCapacity(length);
        MConverter<KeyType> keysConverter;
        for (int32 i = 0; i < keys.Count(); i++)
        {
            auto& key = keys[i];
            MonoObject* keyManaged = mono_array_get(managedKeys, MonoObject*, i);
            keysConverter.Unbox(key, keyManaged);
        }
        MConverter<ValueType> valueConverter;
        for (int32 i = 0; i < keys.Count(); i++)
        {
            auto& key = keys[i];
            MonoObject* keyManaged = mono_array_get(managedKeys, MonoObject*, i);
            MonoObject* valueManaged = wrapper.GetValue(keyManaged);
            auto& value = result[key];
            valueConverter.Unbox(value, valueManaged);
        }
        return result;
    }

    static ManagedDictionary New(MonoType* keyType, MonoType* valueType)
    {
        ManagedDictionary result;

        auto domain = mono_domain_get();
        auto scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, result);
        auto makeGenericMethod = scriptingClass->GetMethod("MakeGenericType", 2);
        CHECK_RETURN(makeGenericMethod, result);
        auto createMethod = StdTypesContainer::Instance()->ActivatorClass->GetMethod("CreateInstance", 2);
        CHECK_RETURN(createMethod, result);

        auto genericType = MUtils::GetType(StdTypesContainer::Instance()->DictionaryClass->GetNative());
        auto genericArgs = mono_array_new(domain, mono_get_object_class(), 2);
        mono_array_set(genericArgs, MonoReflectionType*, 0, mono_type_get_object(domain, keyType));
        mono_array_set(genericArgs, MonoReflectionType*, 1, mono_type_get_object(domain, valueType));

        void* params[2];
        params[0] = genericType;
        params[1] = genericArgs;
        MonoObject* exception = nullptr;
        auto dictionaryType = makeGenericMethod->Invoke(nullptr, params, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT(""));
            return result;
        }

        params[0] = dictionaryType;
        params[1] = nullptr;
        auto instance = createMethod->Invoke(nullptr, params, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT(""));
            return result;
        }

        result.Instance = instance;
        return result;
    }

    void Add(MonoObject* key, MonoObject* value)
    {
        CHECK(Instance);
        auto scriptingClass = Scripting::GetStaticClass();
        CHECK(scriptingClass);
        auto addDictionaryItemMethod = scriptingClass->GetMethod("AddDictionaryItem", 3);
        CHECK(addDictionaryItemMethod);
        void* params[3];
        params[0] = Instance;
        params[1] = key;
        params[2] = value;
        MonoObject* exception = nullptr;
        mono_runtime_invoke(addDictionaryItemMethod->GetNative(), Instance, params, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT(""));
        }
    }

    MonoArray* GetKeys() const
    {
        CHECK_RETURN(Instance, nullptr);
        auto scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, nullptr);
        auto getDictionaryKeysMethod = scriptingClass->GetMethod("GetDictionaryKeys", 1);
        CHECK_RETURN(getDictionaryKeysMethod, nullptr);
        void* params[1];
        params[0] = Instance;
        return (MonoArray*)mono_runtime_invoke(getDictionaryKeysMethod->GetNative(), nullptr, params, nullptr);
    }

    MonoObject* GetValue(MonoObject* key) const
    {
        CHECK_RETURN(Instance, nullptr);
        auto klass = mono_object_get_class(Instance);
        auto getItemMethod = mono_class_get_method_from_name(klass, "System.Collections.IDictionary.get_Item", 1);
        CHECK_RETURN(getItemMethod, nullptr);
        void* params[1];
        params[0] = key;
        return mono_runtime_invoke(getItemMethod, Instance, params, nullptr);
    }
};
