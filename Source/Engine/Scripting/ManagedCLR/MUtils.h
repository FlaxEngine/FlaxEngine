// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"
#include "MClass.h"
#include "MCore.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"

#if USE_CSHARP

struct Version;
class CultureInfo;
template<typename AllocationType>
class BitArray;

namespace MUtils
{
    extern FLAXENGINE_API StringView ToString(MString* str);
    extern FLAXENGINE_API StringAnsi ToStringAnsi(MString* str);
    extern FLAXENGINE_API void ToString(MString* str, String& result);
    extern FLAXENGINE_API void ToString(MString* str, StringView& result);
    extern FLAXENGINE_API void ToString(MString* str, Variant& result);
    extern FLAXENGINE_API void ToString(MString* str, StringAnsi& result);

    extern FLAXENGINE_API MString* ToString(const char* str);
    extern FLAXENGINE_API MString* ToString(const StringAnsi& str);
    extern FLAXENGINE_API MString* ToString(const String& str);
    extern FLAXENGINE_API MString* ToString(const String& str, MDomain* domain);
    extern FLAXENGINE_API MString* ToString(const StringAnsiView& str);
    extern FLAXENGINE_API MString* ToString(const StringView& str);
    extern FLAXENGINE_API MString* ToString(const StringView& str, MDomain* domain);

    extern FLAXENGINE_API ScriptingTypeHandle UnboxScriptingTypeHandle(MTypeObject* value);
    extern FLAXENGINE_API MTypeObject* BoxScriptingTypeHandle(const ScriptingTypeHandle& value);
    extern FLAXENGINE_API VariantType UnboxVariantType(MType* type);
    extern FLAXENGINE_API MTypeObject* BoxVariantType(const VariantType& value);
    extern FLAXENGINE_API Variant UnboxVariant(MObject* value);
    extern FLAXENGINE_API MObject* BoxVariant(const Variant& value);
}

// Converter for data of type T between managed and unmanaged world
template<typename T, typename Enable = void>
struct MConverter
{
    MObject* Box(const T& data, const MClass* klass);
    void Unbox(T& result, MObject* data);
    void ToManagedArray(MArray* result, const Span<T>& data);
    void ToNativeArray(Span<T>& result, const MArray* data);
};

// Converter for POD types (that can use raw memory copy).
template<typename T>
struct MConverter<T, typename TEnableIf<TAnd<TIsPODType<T>, TNot<TIsBaseOf<class ScriptingObject, typename TRemovePointer<T>::Type>>>::Value>::Type>
{
    MObject* Box(const T& data, const MClass* klass)
    {
        return MCore::Object::Box((void*)&data, klass);
    }

    void Unbox(T& result, MObject* data)
    {
        if (data)
            Platform::MemoryCopy(&result, MCore::Object::Unbox(data), sizeof(T));
    }

    void ToManagedArray(MArray* result, const Span<T>& data)
    {
        Platform::MemoryCopy(MCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(T));
    }

    void ToNativeArray(Span<T>& result, const MArray* data)
    {
        Platform::MemoryCopy(result.Get(), MCore::Array::GetAddress(data), result.Length() * sizeof(T));
    }
};

// Converter for String.
template<>
struct MConverter<String>
{
    MObject* Box(const String& data, const MClass* klass)
    {
#if USE_NETCORE
        MString* str = MUtils::ToString(data);
        return MCore::Object::Box(str, klass);
#else
        return (MObject*)MUtils::ToString(data);
#endif
    }

    void Unbox(String& result, MObject* data)
    {
#if USE_NETCORE
        MString* str = (MString*)MCore::Object::Unbox(data);
        result = MUtils::ToString(str);
#else
        result = MUtils::ToString((MString*)data);
#endif
    }

    void ToManagedArray(MArray* result, const Span<String>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = (MObject*)MUtils::ToString(data.Get()[i]);
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<String>& result, const MArray* data)
    {
        MString** dataPtr = MCore::Array::GetAddress<MString*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            MUtils::ToString(dataPtr[i], result.Get()[i]);
    }
};

// Converter for StringAnsi.
template<>
struct MConverter<StringAnsi>
{
    MObject* Box(const StringAnsi& data, const MClass* klass)
    {
        return (MObject*)MUtils::ToString(data);
    }

    void Unbox(StringAnsi& result, MObject* data)
    {
        result = MUtils::ToStringAnsi((MString*)data);
    }

    void ToManagedArray(MArray* result, const Span<StringAnsi>& data)
    {
        if (data.Length() == 0)
            return;
        auto* objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = (MObject*)MUtils::ToString(data.Get()[i]);
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<StringAnsi>& result, const MArray* data)
    {
        MString** dataPtr = MCore::Array::GetAddress<MString*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            MUtils::ToString(dataPtr[i], result.Get()[i]);
    }
};

// Converter for StringView.
template<>
struct MConverter<StringView>
{
    MObject* Box(const StringView& data, const MClass* klass)
    {
        return (MObject*)MUtils::ToString(data);
    }

    void Unbox(StringView& result, MObject* data)
    {
        result = MUtils::ToString((MString*)data);
    }

    void ToManagedArray(MArray* result, const Span<StringView>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = (MObject*)MUtils::ToString(data.Get()[i]);
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<StringView>& result, const MArray* data)
    {
        MString** dataPtr = MCore::Array::GetAddress<MString*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            MUtils::ToString(dataPtr[i], result.Get()[i]);
    }
};

// Converter for Variant.
template<>
struct MConverter<Variant>
{
    MObject* Box(const Variant& data, const MClass* klass)
    {
        return MUtils::BoxVariant(data);
    }

    void Unbox(Variant& result, MObject* data)
    {
        result = MUtils::UnboxVariant(data);
    }

    void ToManagedArray(MArray* result, const Span<Variant>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = MUtils::BoxVariant(data[i]);
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<Variant>& result, const MArray* data)
    {
        MObject** dataPtr = MCore::Array::GetAddress<MObject*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            result.Get()[i] = MUtils::UnboxVariant(dataPtr[i]);
    }
};

// Converter for Scripting Objects (collection of pointers).
template<typename T>
struct MConverter<T*, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
{
    MObject* Box(T* data, const MClass* klass)
    {
        return data ? data->GetOrCreateManagedInstance() : nullptr;
    }

    void Unbox(T*& result, MObject* data)
    {
        result = (T*)ScriptingObject::ToNative(data);
    }

    void ToManagedArray(MArray* result, const Span<T*>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = data.Get()[i] ? data.Get()[i]->GetOrCreateManagedInstance() : nullptr;
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<T*>& result, const MArray* data)
    {
        MObject** dataPtr = MCore::Array::GetAddress<MObject*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
    }
};

// Converter for Scripting Objects (collection of values).
template<typename T>
struct MConverter<T, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
{
    MObject* Box(const T& data, const MClass* klass)
    {
        return data.GetOrCreateManagedInstance();
    }

    void ToManagedArray(MArray* result, const Span<T>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = data.Get()[i].GetOrCreateManagedInstance();
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }
};

// Converter for ScriptingObject References.
template<typename T>
class ScriptingObjectReference;

template<typename T>
struct MConverter<ScriptingObjectReference<T>>
{
    MObject* Box(const ScriptingObjectReference<T>& data, const MClass* klass)
    {
        return data.GetManagedInstance();
    }

    void Unbox(ScriptingObjectReference<T>& result, MObject* data)
    {
        result = (T*)ScriptingObject::ToNative(data);
    }

    void ToManagedArray(MArray* result, const Span<ScriptingObjectReference<T>>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = data[i].GetManagedInstance();
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<ScriptingObjectReference<T>>& result, const MArray* data)
    {
        MObject** dataPtr = MCore::Array::GetAddress<MObject*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
    }
};

// Converter for Asset References.
template<typename T>
class AssetReference;

template<typename T>
struct MConverter<AssetReference<T>>
{
    MObject* Box(const AssetReference<T>& data, const MClass* klass)
    {
        return data.GetManagedInstance();
    }

    void Unbox(AssetReference<T>& result, MObject* data)
    {
        result = (T*)ScriptingObject::ToNative(data);
    }

    void ToManagedArray(MArray* result, const Span<AssetReference<T>>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = data[i].GetManagedInstance();
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<AssetReference<T>>& result, const MArray* data)
    {
        MObject** dataPtr = MCore::Array::GetAddress<MObject*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
    }
};

// TODO: use MarshalAs=Guid on SoftAssetReference to pass guid over bindings and not load asset in glue code
template<typename T>
class SoftAssetReference;
template<typename T>
struct MConverter<SoftAssetReference<T>>
{
    void ToManagedArray(MArray* result, const Span<SoftAssetReference<T>>& data)
    {
        if (data.Length() == 0)
            return;
        MObject** objects = (MObject**)Allocator::Allocate(data.Length() * sizeof(MObject*));
        for (int32 i = 0; i < data.Length(); i++)
            objects[i] = data[i].GetManagedInstance();
        MCore::GC::WriteArrayRef(result, Span<MObject*>(objects, data.Length()));
        Allocator::Free(objects);
    }

    void ToNativeArray(Span<SoftAssetReference<T>>& result, const MArray* data)
    {
        MObject** dataPtr = MCore::Array::GetAddress<MObject*>(data);
        for (int32 i = 0; i < result.Length(); i++)
            result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
    }
};

// Converter for Array.
template<typename T>
struct MConverter<Array<T>>
{
    MObject* Box(const Array<T>& data, const MClass* klass)
    {
        if (!klass)
            return nullptr;
        MArray* result = MCore::Array::New(klass->GetElementClass(), data.Count());
        MConverter<T> converter;
        converter.ToManagedArray(result, Span<T>(data.Get(), data.Count()));
        return (MObject*)result;
    }

    void Unbox(Array<T>& result, MObject* data)
    {
        MArray* array = MCore::Array::Unbox(data);
        const int32 length = array ? MCore::Array::GetLength(array) : 0;
        result.Resize(length);
        MConverter<T> converter;
        Span<T> resultSpan(result.Get(), length);
        converter.ToNativeArray(resultSpan, array);
    }
};

namespace MUtils
{
    // Outputs the full typename for the type of the specified object.
    extern FLAXENGINE_API const StringAnsi& GetClassFullname(MObject* obj);

    // Returns the class of the provided object.
    extern FLAXENGINE_API MClass* GetClass(MObject* object);

    // Returns the class of the provided type.
    extern FLAXENGINE_API MClass* GetClass(MTypeObject* type);

    // Returns the class of the provided VariantType value.
    extern FLAXENGINE_API MClass* GetClass(const VariantType& value);

    // Returns the class of the provided Variant value.
    extern FLAXENGINE_API MClass* GetClass(const Variant& value);

    // Returns the type of the provided object.
    extern FLAXENGINE_API MTypeObject* GetType(MObject* object);

    // Returns the type of the provided class.
    extern FLAXENGINE_API MTypeObject* GetType(MClass* klass);

    /// <summary>
    /// Boxes the native value into the managed object.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <param name="valueClass">The value type class.</param>
    template<class T>
    MObject* Box(const T& value, const MClass* valueClass)
    {
        MConverter<T> converter;
        return converter.Box(value, valueClass);
    }

    /// <summary>
    /// Unboxes MObject to the native value of the given type.
    /// </summary>
    template<class T>
    T Unbox(MObject* object)
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
    extern FLAXENGINE_API BytesContainer LinkArray(MArray* arrayObj);

    /// <summary>
    /// Allocates new managed array of data and copies contents from given native array.
    /// </summary>
    /// <param name="data">The array object.</param>
    /// <param name="valueClass">The array values type class.</param>
    /// <returns>The output array.</returns>
    template<typename T>
    MArray* ToArray(const Span<T>& data, const MClass* valueClass)
    {
        if (!valueClass)
            return nullptr;
        MArray* result = MCore::Array::New(valueClass, data.Length());
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
    FORCE_INLINE MArray* ToArray(const Array<T, AllocationType>& data, const MClass* valueClass)
    {
        return MUtils::ToArray(Span<T>(data.Get(), data.Count()), valueClass);
    }

    /// <summary>
    /// Converts the managed array into native array container object.
    /// </summary>
    /// <param name="arrayObj">The managed array object.</param>
    /// <returns>The output array.</returns>
    template<typename T, typename AllocationType = HeapAllocation>
    Array<T, AllocationType> ToArray(MArray* arrayObj)
    {
        Array<T, AllocationType> result;
        const int32 length = arrayObj ? MCore::Array::GetLength(arrayObj) : 0;
        result.Resize(length);
        MConverter<T> converter;
        Span<T> resultSpan(result.Get(), length);
        converter.ToNativeArray(resultSpan, arrayObj);
        return result;
    }

    /// <summary>
    /// Converts the managed array into native Span.
    /// </summary>
    /// <param name="arrayObj">The managed array object.</param>
    /// <returns>The output array pointer and size.</returns>
    template<typename T>
    Span<T> ToSpan(MArray* arrayObj)
    {
        T* ptr = (T*)MCore::Array::GetAddress(arrayObj);
        const int32 length = arrayObj ? MCore::Array::GetLength(arrayObj) : 0;
        return Span<T>(ptr, length);
    }

    /// <summary>
    /// Converts the native array into native Span.
    /// </summary>
    /// <param name="data">The native array object.</param>
    /// <returns>The output array pointer and size.</returns>
    template<typename T, typename AllocationType = HeapAllocation>
    FORCE_INLINE Span<T> ToSpan(const Array<T, AllocationType>& data)
    {
        return Span<T>(data.Get(), data.Count());
    }

    /// <summary>
    /// Links managed array data to the unmanaged DataContainer (must use simple type like struct or float/int/bool).
    /// </summary>
    /// <param name="arrayObj">The array object.</param>
    /// <param name="result">The result data (linked not copied).</param>
    template<typename T>
    void ToArray(MArray* arrayObj, DataContainer<T>& result)
    {
        const int32 length = arrayObj ? MCore::Array::GetLength(arrayObj) : 0;
        if (length == 0)
        {
            result.Release();
            return;
        }
        T* bytesRaw = (T*)MCore::Array::GetAddress(arrayObj);
        result.Link(bytesRaw, length);
    }

    /// <summary>
    /// Allocates new managed bytes array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MArray* ToArray(const Span<byte>& data)
    {
        return ToArray(data, MCore::TypeCache::Byte);
    }

    /// <summary>
    /// Allocates new managed bytes array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MArray* ToArray(Array<byte>& data)
    {
        return ToArray(Span<byte>(data.Get(), data.Count()), MCore::TypeCache::Byte);
    }

    /// <summary>
    /// Allocates new managed strings array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MArray* ToArray(const Span<String>& data)
    {
        return ToArray(data, MCore::TypeCache::String);
    }

    /// <summary>
    /// Allocates new managed strings array and copies data from the given unmanaged data container.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE MArray* ToArray(const Array<String>& data)
    {
        return ToArray(Span<String>(data.Get(), data.Count()), MCore::TypeCache::String);
    }

#if USE_NETCORE
    /// <summary>
    /// Allocates new boolean array and copies data from the given unmanaged data container. The managed runtime is responsible for releasing the returned array data.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    FORCE_INLINE bool* ToBoolArray(const Array<bool>& data)
    {
        // System.Runtime.InteropServices.Marshalling.ArrayMarshaller uses CoTask memory alloc to native data pointer
        bool* arr = (bool*)MCore::GC::AllocateMemory(data.Count() * sizeof(bool), true);
        memcpy(arr, data.Get(), data.Count() * sizeof(bool));
        return arr;
    }

    /// <summary>
    /// Allocates new boolean array and copies data from the given unmanaged data container. The managed runtime is responsible for releasing the returned array data.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output array.</returns>
    template<typename AllocationType = HeapAllocation>
    FORCE_INLINE bool* ToBoolArray(const BitArray<AllocationType>& data)
    {
        // System.Runtime.InteropServices.Marshalling.ArrayMarshaller uses CoTask memory alloc to native data pointer
        bool* arr = (bool*)MCore::GC::AllocateMemory(data.Count() * sizeof(bool), true);
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

    extern void* VariantToManagedArgPtr(Variant& value, MType* type, bool& failed);

    extern MObject* ToManaged(const Version& value);
    extern Version ToNative(MObject* value);
};

#endif
