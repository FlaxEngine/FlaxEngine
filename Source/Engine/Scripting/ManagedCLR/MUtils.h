// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"

#if USE_MONO

#include <mono/metadata/object.h>
#include <mono/metadata/appdomain.h>

#if USE_NETCORE
#include "Engine/Scripting/DotNet/CoreCLR.h"
#endif

struct Version;
template<typename AllocationType>
class BitArray;

namespace MUtils
{
    extern FLAXENGINE_API StringView ToString(MonoString* str);
    extern FLAXENGINE_API StringAnsi ToStringAnsi(MonoString* str);
    extern FLAXENGINE_API void ToString(MonoString* str, String& result);
    extern FLAXENGINE_API void ToString(MonoString* str, StringView& result);
    extern FLAXENGINE_API void ToString(MonoString* str, Variant& result);
    extern FLAXENGINE_API void ToString(MonoString* str, MString& result);

    extern FLAXENGINE_API MonoString* ToString(const char* str);
    extern FLAXENGINE_API MonoString* ToString(const StringAnsi& str);
    extern FLAXENGINE_API MonoString* ToString(const String& str);
    extern FLAXENGINE_API MonoString* ToString(const String& str, MonoDomain* domain);
    extern FLAXENGINE_API MonoString* ToString(const StringAnsiView& str);
    extern FLAXENGINE_API MonoString* ToString(const StringView& str);
    extern FLAXENGINE_API MonoString* ToString(const StringView& str, MonoDomain* domain);

    extern FLAXENGINE_API ScriptingTypeHandle UnboxScriptingTypeHandle(MonoReflectionType* value);
    extern FLAXENGINE_API MonoReflectionType* BoxScriptingTypeHandle(const ScriptingTypeHandle& value);
    extern FLAXENGINE_API VariantType UnboxVariantType(MonoReflectionType* value);
    extern FLAXENGINE_API VariantType UnboxVariantType(MonoType* monoType);
    extern FLAXENGINE_API MonoReflectionType* BoxVariantType(const VariantType& value);
    extern FLAXENGINE_API Variant UnboxVariant(MonoObject* value);
    extern FLAXENGINE_API MonoObject* BoxVariant(const Variant& value);
}

// Converter for data of type T between managed and unmanaged world
template<typename T, typename Enable = void>
struct MConverter
{
    MonoObject* Box(const T& data, MonoClass* klass);
    void Unbox(T& result, MonoObject* data);
    void ToManagedArray(MonoArray* result, const Span<T>& data);
    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<T, AllocationType>& result, MonoArray* data, int32 length);
};

#if USE_NETCORE
// Pass-through converter for ScriptingObjects (passed as GCHandles)
template<>
struct MConverter<void*>
{
    void Unbox(void*& result, MonoObject* data)
    {
        CHECK(data);
        result = data;
    }
};
#endif

// Converter for POD types (that can use raw memory copy).
template<typename T>
struct MConverter<T, typename TEnableIf<TAnd<TIsPODType<T>, TNot<TIsBaseOf<class ScriptingObject, typename TRemovePointer<T>::Type>>>::Value>::Type>
{
    MonoObject* Box(const T& data, MonoClass* klass)
    {
        return mono_value_box(mono_domain_get(), klass, (void*)&data);
    }

    void Unbox(T& result, MonoObject* data)
    {
        CHECK(data);
        Platform::MemoryCopy(&result, mono_object_unbox(data), sizeof(T));
    }

    void ToManagedArray(MonoArray* result, const Span<T>& data)
    {
        Platform::MemoryCopy(mono_array_addr(result, T, 0), data.Get(), data.Length() * sizeof(T));
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<T, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Add(mono_array_addr(data, T, 0), length);
    }
};

// Converter for String.
template<>
struct MConverter<String>
{
    MonoObject* Box(const String& data, MonoClass* klass)
    {
#if USE_NETCORE
        MonoString* str = MUtils::ToString(data);
        return mono_value_box(nullptr, klass, str);
#else
        return (MonoObject*)MUtils::ToString(data);
#endif
    }

    void Unbox(String& result, MonoObject* data)
    {
#if USE_NETCORE
        MonoString* str = (MonoString*)mono_object_unbox(data);
        result = MUtils::ToString(str);
#else
        result = MUtils::ToString((MonoString*)data);
#endif
    }

    void ToManagedArray(MonoArray* result, const Span<String>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, MUtils::ToString(data[i]));
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<String, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            MUtils::ToString(mono_array_get(data, MonoString*, i), result[i]);
    }
};

// Converter for StringAnsi.
template<>
struct MConverter<StringAnsi>
{
    MonoObject* Box(const StringAnsi& data, MonoClass* klass)
    {
        return (MonoObject*)MUtils::ToString(data);
    }

    void Unbox(StringAnsi& result, MonoObject* data)
    {
        result = MUtils::ToStringAnsi((MonoString*)data);
    }

    void ToManagedArray(MonoArray* result, const Span<StringAnsi>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, MUtils::ToString(data[i]));
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<StringAnsi, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            MUtils::ToString(mono_array_get(data, MonoString*, i), result[i]);
    }
};

// Converter for StringView.
template<>
struct MConverter<StringView>
{
    MonoObject* Box(const StringView& data, MonoClass* klass)
    {
        return (MonoObject*)MUtils::ToString(data);
    }

    void Unbox(StringView& result, MonoObject* data)
    {
        result = MUtils::ToString((MonoString*)data);
    }

    void ToManagedArray(MonoArray* result, const Span<StringView>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, MUtils::ToString(data[i]));
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<StringView, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            MUtils::ToString(mono_array_get(data, MonoString*, i), result[i]);
    }
};

// Converter for Variant.
template<>
struct MConverter<Variant>
{
    MonoObject* Box(const Variant& data, MonoClass* klass)
    {
        return MUtils::BoxVariant(data);
    }

    void Unbox(Variant& result, MonoObject* data)
    {
        result = MUtils::UnboxVariant(data);
    }

    void ToManagedArray(MonoArray* result, const Span<Variant>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, MUtils::BoxVariant(data[i]));
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<Variant, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            result[i] = MUtils::UnboxVariant(mono_array_get(data, MonoObject*, i));
    }
};

// Converter for Scripting Objects (collection of pointers).
template<typename T>
struct MConverter<T*, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
{
    MonoObject* Box(T* data, MonoClass* klass)
    {
        return data ? data->GetOrCreateManagedInstance() : nullptr;
    }

    void Unbox(T*& result, MonoObject* data)
    {
        result = (T*)ScriptingObject::ToNative(data);
    }

    void ToManagedArray(MonoArray* result, const Span<T*>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
        {
            auto obj = data[i];
            mono_array_setref(result, i, obj ? obj->GetOrCreateManagedInstance() : nullptr);
        }
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<T*, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            result[i] = (T*)ScriptingObject::ToNative(mono_array_get(data, MonoObject*, i));
    }
};

// Converter for Scripting Objects (collection of values).
template<typename T>
struct MConverter<T, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
{
    MonoObject* Box(const T& data, MonoClass* klass)
    {
        return data.GetOrCreateManagedInstance();
    }

    void Unbox(T& result, MonoObject* data)
    {
        // Not Supported
        CRASH;
    }

    void ToManagedArray(MonoArray* result, const Span<T>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, data[i].GetOrCreateManagedInstance());
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<T, AllocationType>& result, MonoArray* data, int32 length)
    {
        // Not Supported
        CRASH;
    }
};

// Converter for ScriptingObject References.
template<typename T>
class ScriptingObjectReference;

template<typename T>
struct MConverter<ScriptingObjectReference<T>>
{
    MonoObject* Box(const ScriptingObjectReference<T>& data, MonoClass* klass)
    {
        return data.GetManagedInstance();
    }

    void Unbox(ScriptingObjectReference<T>& result, MonoObject* data)
    {
        result = (T*)ScriptingObject::ToNative(data);
    }

    void ToManagedArray(MonoArray* result, const Span<ScriptingObjectReference<T>>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, data[i].GetManagedInstance());
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<ScriptingObjectReference<T>, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            result[i] = (T*)ScriptingObject::ToNative(mono_array_get(data, MonoObject*, i));
    }
};

// Converter for Asset References.
template<typename T>
class AssetReference;

template<typename T>
struct MConverter<AssetReference<T>>
{
    MonoObject* Box(const AssetReference<T>& data, MonoClass* klass)
    {
        return data.GetManagedInstance();
    }

    void Unbox(AssetReference<T>& result, MonoObject* data)
    {
        result = (T*)ScriptingObject::ToNative(data);
    }

    void ToManagedArray(MonoArray* result, const Span<AssetReference<T>>& data)
    {
        for (int32 i = 0; i < data.Length(); i++)
            mono_array_setref(result, i, data[i].GetManagedInstance());
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<AssetReference<T>, AllocationType>& result, MonoArray* data, int32 length)
    {
        result.Resize(length);
        for (int32 i = 0; i < length; i++)
            result[i] = (T*)ScriptingObject::ToNative(mono_array_get(data, MonoObject*, i));
    }
};

// Converter for Array.
template<typename T>
struct MConverter<Array<T>>
{
    MonoObject* Box(const Array<T>& data, MonoClass* klass)
    {
        if (!klass)
            return nullptr;
        // TODO: use shared empty arrays cache
        auto result = mono_array_new(mono_domain_get(), klass, data.Count());
        MConverter<T> converter;
        converter.ToManagedArray(result, Span<T>(data.Get(), data.Count()));
        return (MonoObject*)result;
    }

    void Unbox(Array<T>& result, MonoObject* data)
    {
        auto length = data ? (int32)mono_array_length((MonoArray*)data) : 0;
        result.EnsureCapacity(length);
        MConverter<T> converter;
        converter.ToNativeArray(result, (MonoArray*)data, length);
    }

    void ToManagedArray(MonoArray* result, const Span<Array<T>>& data)
    {
        CRASH; // Not implemented
    }

    template<typename AllocationType = HeapAllocation>
    void ToNativeArray(Array<Array<T>, AllocationType>& result, MonoArray* data, int32 length)
    {
        CRASH; // Not implemented
    }
};

namespace MUtils
{
    // Outputs the full typename for the type of the specified object.
    extern FLAXENGINE_API void GetClassFullname(MonoObject* obj, MString& fullname);

    // Outputs the full typename for the specified type.
    extern FLAXENGINE_API void GetClassFullname(MonoClass* monoClass, MString& fullname);

    // Outputs the full typename for the specified type.
    extern FLAXENGINE_API void GetClassFullname(MonoReflectionType* type, MString& fullname);

    // Outputs the full typename for the type of the specified object.
    inline MString GetClassFullname(MonoObject* obj)
    {
        MString fullname;
        GetClassFullname(obj, fullname);
        return fullname;
    }

    // Outputs the full typename for the type of the specified object.
    inline MString GetClassFullname(MonoClass* monoClass)
    {
        MString fullname;
        GetClassFullname(monoClass, fullname);
        return fullname;
    }

    // Returns the class of the provided object.
    extern FLAXENGINE_API MonoClass* GetClass(MonoObject* object);

    // Returns the class of the provided type.
    extern FLAXENGINE_API MonoClass* GetClass(MonoReflectionType* type);

    // Returns the class of the provided VariantType value.
    extern FLAXENGINE_API MonoClass* GetClass(const VariantType& value);

    // Returns the class of the provided Variant value.
    extern FLAXENGINE_API MonoClass* GetClass(const Variant& value);

    // Returns the type of the provided object.
    extern FLAXENGINE_API MonoReflectionType* GetType(MonoObject* object);

    // Returns the type of the provided class.
    extern FLAXENGINE_API MonoReflectionType* GetType(MonoClass* klass);

    // Returns the type of the provided class.
    extern FLAXENGINE_API MonoReflectionType* GetType(MClass* mclass);

    /// <summary>
    /// Boxes the native value into the MonoObject.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <param name="valueClass">The value type class.</param>
    template<class T>
    MonoObject* Box(const T& value, MonoClass* valueClass)
    {
        MConverter<T> converter;
        return converter.Box(value, valueClass);
    }

    /// <summary>
    /// Unboxes MonoObject to the native value of the given type.
    /// </summary>
    template<class T>
    T Unbox(MonoObject* object)
    {
        MConverter<T> converter;
        T result;
        converter.Unbox(result, object);
        return result;
    }

    /// <summary>
    /// Links managed array data to the unmanaged BytesContainer.
    /// </summary>
    /// <param name="arrayObj">The array object.</param>
    /// <returns>The result data container with linked array data bytes (not copied).</returns>
    extern FLAXENGINE_API BytesContainer LinkArray(MonoArray* arrayObj);

    /// <summary>
    /// Allocates new managed array of data and copies contents from given native array.
    /// </summary>
    /// <param name="data">The array object.</param>
    /// <param name="valueClass">The array values type class.</param>
    /// <returns>The output array.</returns>
    template<typename T>
    MonoArray* ToArray(const Span<T>& data, MonoClass* valueClass)
    {
        if (!valueClass)
            return nullptr;
        // TODO: use shared empty arrays cache
        auto result = mono_array_new(mono_domain_get(), valueClass, data.Length());
        MConverter<T> converter;
        converter.ToManagedArray(result, data);
        return result;
    }

    /// <summary>
    /// Allocates new managed array of data and copies contents from given native array.
    /// </summary>
    /// <param name="data">The array object.</param>
    /// <param name="valueClass">The array values type class.</param>
    /// <returns>The output array.</returns>
    template<typename T, typename AllocationType>
    FORCE_INLINE MonoArray* ToArray(const Array<T, AllocationType>& data, MonoClass* valueClass)
    {
        return MUtils::ToArray(Span<T>(data.Get(), data.Count()), valueClass);
    }

    /// <summary>
    /// Converts the managed array into native array container object.
    /// </summary>
    /// <param name="arrayObj">The managed array object.</param>
    /// <returns>The output array.</returns>
    template<typename T, typename AllocationType = HeapAllocation>
    Array<T, AllocationType> ToArray(MonoArray* arrayObj)
    {
        Array<T, AllocationType> result;
        auto length = arrayObj ? (int32)mono_array_length(arrayObj) : 0;
        result.EnsureCapacity(length);
        MConverter<T> converter;
        converter.ToNativeArray(result, arrayObj, length);
        return result;
    }

    /// <summary>
    /// Converts the managed array into native Span.
    /// </summary>
    /// <param name="arrayObj">The managed array object.</param>
    /// <returns>The output array pointer and size.</returns>
    template<typename T>
    Span<T> ToSpan(MonoArray* arrayObj)
    {
        auto ptr = (T*)(void*)mono_array_addr_with_size(arrayObj, sizeof(T), 0);
        auto length = arrayObj ? (int32)mono_array_length(arrayObj) : 0;
        return Span<T>(ptr, length);
    }

    /// <summary>
    /// Converts the native array into native Span.
    /// </summary>
    /// <param name="data">The native array object.</param>
    /// <returns>The output array pointer and size.</returns>
    template<typename T>
    FORCE_INLINE Span<T> ToSpan(const Array<T>& data)
    {
        return Span<T>(data.Get(), data.Count());
    }

    /// <summary>
    /// Links managed array data to the unmanaged DataContainer (must use simple type like struct or float/int/bool).
    /// </summary>
    /// <param name="arrayObj">The array object.</param>
    /// <param name="result">The result data (linked not copied).</param>
    template<typename T>
    void ToArray(MonoArray* arrayObj, DataContainer<T>& result)
    {
        auto length = arrayObj ? (int32)mono_array_length(arrayObj) : 0;
        if (length == 0)
        {
            result.Release();
            return;
        }

        auto bytesRaw = (T*)(void*)mono_array_addr_with_size(arrayObj, sizeof(T), 0);
        result.Link(bytesRaw, length);
    }

    /// <summary>
    /// Allocates new managed bytes array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MonoArray* ToArray(const Span<byte>& data)
    {
        return ToArray(data, mono_get_byte_class());
    }

    /// <summary>
    /// Allocates new managed bytes array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MonoArray* ToArray(Array<byte>& data)
    {
        return ToArray(Span<byte>(data.Get(), data.Count()), mono_get_byte_class());
    }

    /// <summary>
    /// Allocates new managed strings array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MonoArray* ToArray(const Span<String>& data)
    {
        return ToArray(data, mono_get_string_class());
    }

    /// <summary>
    /// Allocates new managed strings array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MonoArray* ToArray(const Array<String>& data)
    {
        return ToArray(Span<String>(data.Get(), data.Count()), mono_get_string_class());
    }

#if USE_NETCORE
    /// <summary>
    /// Allocates new boolean array and copies data from the given unmanaged data container.
    /// The managed runtime is responsible for releasing the returned array data.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE bool* ToBoolArray(const Array<bool>& data)
    {
        bool* arr = (bool*)CoreCLR::Allocate(data.Count() * sizeof(bool));
        memcpy(arr, data.Get(), data.Count() * sizeof(bool));
        return arr;
    }

    /// <summary>
    /// Allocates new boolean array and copies data from the given unmanaged data container.
    /// The managed runtime is responsible for releasing the returned array data.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    template<typename AllocationType = HeapAllocation>
    FORCE_INLINE bool* ToBoolArray(const BitArray<AllocationType>& data)
    {
        bool* arr = (bool*)CoreCLR::Allocate(data.Count() * sizeof(bool));
        for (int i = 0; i < data.Count(); i++)
            arr[i] = data[i];
        return arr;
    }
#else
    FORCE_INLINE bool* ToBoolArray(const Array<bool>& data)
    {
        return nullptr;
    }

    template<typename AllocationType = HeapAllocation>
    FORCE_INLINE bool* ToBoolArray(const BitArray<AllocationType>& data)
    {
        return nullptr;
    }
#endif

    FORCE_INLINE MGCHandle NewGCHandle(MonoObject* obj, bool pinned)
    {
#if USE_NETCORE
        return CoreCLR::NewGCHandle(obj, pinned);
#else
        return mono_gchandle_new(obj, pinned);
#endif
    }

    FORCE_INLINE MGCHandle NewGCHandleWeakref(MonoObject* obj, bool track_resurrection)
    {
#if USE_NETCORE
        return CoreCLR::NewGCHandleWeakref(obj, track_resurrection);
#else
        return mono_gchandle_new_weakref(obj, track_resurrection);
#endif
    }

    FORCE_INLINE MonoObject* GetGCHandleTarget(const MGCHandle& handle)
    {
#if USE_NETCORE
        return (MonoObject*)CoreCLR::GetGCHandleTarget(handle);
#else
        return mono_gchandle_get_target(handle);
#endif
    }

    FORCE_INLINE void FreeGCHandle(const MGCHandle& handle)
    {
#if USE_NETCORE
        CoreCLR::FreeGCHandle(handle);
#else
        mono_gchandle_free(handle);
#endif
    }

    extern void* VariantToManagedArgPtr(Variant& value, const MType& type, bool& failed);
    extern MonoObject* ToManaged(const Version& value);
    extern Version ToNative(MonoObject* value);
};

#endif
