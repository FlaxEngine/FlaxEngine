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
        MType* KeyType;
        MType* ValueType;

        bool operator==(const KeyValueType& other) const
        {
            return KeyType == other.KeyType && ValueType == other.ValueType;
        }
    };

private:
    friend class Scripting;
    static Dictionary<KeyValueType, MTypeObject*> CachedTypes;

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

    ManagedDictionary(MObject* instance = nullptr);

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

    static MTypeObject* GetClass(MType* keyType, MType* valueType);

    static ManagedDictionary New(MType* keyType, MType* valueType);

    void Add(MObject* key, MObject* value);

    MArray* GetKeys() const;

    MObject* GetValue(MObject* key) const;
};

inline uint32 GetHash(const ManagedDictionary::KeyValueType& other)
{
    uint32 hash = ::GetHash((void*)other.KeyType);
    CombineHash(hash, ::GetHash((void*)other.ValueType));
    return hash;
}

#endif
