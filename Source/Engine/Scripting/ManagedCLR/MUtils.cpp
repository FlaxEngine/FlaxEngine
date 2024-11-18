// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "MUtils.h"
#include "MClass.h"
#include "MCore.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/Internal/StdTypesContainer.h"
#include "Engine/Scripting/Internal/ManagedDictionary.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Content/Asset.h"

#if USE_CSHARP

namespace
{
    // typeName in format System.Collections.Generic.Dictionary`2[KeyType,ValueType]
    void GetDictionaryKeyValueTypes(const StringAnsiView& typeName, MClass*& keyClass, MClass*& valueClass)
    {
        const int32 keyStart = typeName.Find('[');
        const int32 keyEnd = typeName.Find(',');
        const int32 valueEnd = typeName.Find(']');
        const StringAnsiView keyTypename(*typeName + keyStart + 1, keyEnd - keyStart - 1);
        const StringAnsiView valueTypename(*typeName + keyEnd + 1, valueEnd - keyEnd - 1);
        keyClass = Scripting::FindClass(keyTypename);
        valueClass = Scripting::FindClass(valueTypename);
    }
}

StringView MUtils::ToString(MString* str)
{
    if (str == nullptr)
        return StringView::Empty;
    return MCore::String::GetChars(str);
}

StringAnsi MUtils::ToStringAnsi(MString* str)
{
    if (str == nullptr)
        return StringAnsi::Empty;
    return StringAnsi(MCore::String::GetChars(str));
}

void MUtils::ToString(MString* str, String& result)
{
    if (str)
    {
        const StringView chars = MCore::String::GetChars(str);
        result.Set(chars.Get(), chars.Length());
    }
    else
        result.Clear();
}

void MUtils::ToString(MString* str, StringView& result)
{
    if (str)
        result = MCore::String::GetChars(str);
    else
        result = StringView();
}

void MUtils::ToString(MString* str, Variant& result)
{
    result.SetString(str ? MCore::String::GetChars(str) : StringView::Empty);
}

void MUtils::ToString(MString* str, StringAnsi& result)
{
    if (str)
    {
        const StringView chars = MCore::String::GetChars(str);
        result.Set(chars.Get(), chars.Length());
    }
    else
        result.Clear();
}

MString* MUtils::ToString(const char* str)
{
    if (str == nullptr || *str == 0)
        return MCore::String::GetEmpty();
    return MCore::String::New(str, StringUtils::Length(str));
}

MString* MUtils::ToString(const StringAnsi& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return MCore::String::GetEmpty();
    return MCore::String::New(str.Get(), len);
}

MString* MUtils::ToString(const String& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return MCore::String::GetEmpty();
    return MCore::String::New(str.Get(), len);
}

MString* MUtils::ToString(const String& str, MDomain* domain)
{
    const int32 len = str.Length();
    if (len <= 0)
        return MCore::String::GetEmpty(domain);
    return MCore::String::New(str.Get(), len, domain);
}

MString* MUtils::ToString(const StringAnsiView& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return MCore::String::GetEmpty();
    return MCore::String::New(str.Get(), str.Length());
}

MString* MUtils::ToString(const StringView& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return MCore::String::GetEmpty();
    return MCore::String::New(str.Get(), len);
}

MString* MUtils::ToString(const StringView& str, MDomain* domain)
{
    const int32 len = str.Length();
    if (len <= 0)
        return MCore::String::GetEmpty(domain);
    return MCore::String::New(str.Get(), len, domain);
}

ScriptingTypeHandle MUtils::UnboxScriptingTypeHandle(MTypeObject* value)
{
    MClass* klass = GetClass(value);
    if (!klass)
        return ScriptingTypeHandle();
    const StringAnsi& typeName = klass->GetFullName();
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (!typeHandle)
        LOG(Warning, "Unknown scripting type {}", String(typeName));
    return typeHandle;
}

MTypeObject* MUtils::BoxScriptingTypeHandle(const ScriptingTypeHandle& value)
{
    MTypeObject* result = nullptr;
    if (value)
    {
        MType* mType = value.GetType().ManagedClass->GetType();
        result = INTERNAL_TYPE_GET_OBJECT(mType);
    }
    return result;
}

VariantType MUtils::UnboxVariantType(MType* type)
{
    if (!type)
        return VariantType(VariantType::Null);
    const auto& stdTypes = *StdTypesContainer::Instance();
    MClass* klass = MCore::Type::GetClass(type);
    MTypes types = MCore::Type::GetType(type);

    // Fast type detection for in-built types
    switch (types)
    {
    case MTypes::Void:
        return VariantType(VariantType::Void);
    case MTypes::Boolean:
        return VariantType(VariantType::Bool);
    case MTypes::I1:
    case MTypes::I2:
        return VariantType(VariantType::Int16);
    case MTypes::U1:
    case MTypes::U2:
        return VariantType(VariantType::Uint16);
    case MTypes::I4:
    case MTypes::Char:
        return VariantType(VariantType::Int);
    case MTypes::U4:
        return VariantType(VariantType::Uint);
    case MTypes::I8:
        return VariantType(VariantType::Int64);
    case MTypes::U8:
        return VariantType(VariantType::Uint64);
    case MTypes::R4:
        return VariantType(VariantType::Float);
    case MTypes::R8:
        return VariantType(VariantType::Double);
    case MTypes::String:
        return VariantType(VariantType::String);
    case MTypes::Ptr:
        return VariantType(VariantType::Pointer);
    case MTypes::ValueType:
        if (klass == stdTypes.GuidClass)
            return VariantType(VariantType::Guid);
        if (klass == stdTypes.Vector2Class)
            return VariantType(VariantType::Vector2);
        if (klass == stdTypes.Vector3Class)
            return VariantType(VariantType::Vector3);
        if (klass == stdTypes.Vector4Class)
            return VariantType(VariantType::Vector4);
        if (klass == Int2::TypeInitializer.GetClass())
            return VariantType(VariantType::Int2);
        if (klass == Int3::TypeInitializer.GetClass())
            return VariantType(VariantType::Int3);
        if (klass == Int4::TypeInitializer.GetClass())
            return VariantType(VariantType::Int4);
        if (klass == Float2::TypeInitializer.GetClass())
            return VariantType(VariantType::Float2);
        if (klass == Float3::TypeInitializer.GetClass())
            return VariantType(VariantType::Float3);
        if (klass == Float4::TypeInitializer.GetClass())
            return VariantType(VariantType::Float4);
        if (klass == Double2::TypeInitializer.GetClass())
            return VariantType(VariantType::Double2);
        if (klass == Double3::TypeInitializer.GetClass())
            return VariantType(VariantType::Double3);
        if (klass == Double4::TypeInitializer.GetClass())
            return VariantType(VariantType::Double4);
        if (klass == stdTypes.ColorClass)
            return VariantType(VariantType::Color);
        if (klass == stdTypes.BoundingBoxClass)
            return VariantType(VariantType::BoundingBox);
        if (klass == stdTypes.QuaternionClass)
            return VariantType(VariantType::Quaternion);
        if (klass == stdTypes.TransformClass)
            return VariantType(VariantType::Transform);
        if (klass == stdTypes.BoundingSphereClass)
            return VariantType(VariantType::BoundingSphere);
        if (klass == stdTypes.RectangleClass)
            return VariantType(VariantType::Rectangle);
        if (klass == stdTypes.MatrixClass)
            return VariantType(VariantType::Matrix);
        break;
    case MTypes::Object:
        return VariantType(VariantType::ManagedObject);
    case MTypes::SzArray:
        if (klass == MCore::Array::GetClass(MCore::TypeCache::Byte))
            return VariantType(VariantType::Blob);
        break;
    }

    // Get actual typename for full type info
    if (!klass)
        return VariantType(VariantType::Null);
    const StringAnsiView fullname = klass->GetFullName();
    switch (types)
    {
    case MTypes::SzArray:
    case MTypes::Array:
        return VariantType(VariantType::Array, fullname);
    case MTypes::Enum:
        return VariantType(VariantType::Enum, fullname);
    case MTypes::ValueType:
        return VariantType(VariantType::Structure, fullname);
    }
    if (klass == stdTypes.TypeClass)
        return VariantType(VariantType::Typename);
    if (klass->IsSubClassOf(Asset::GetStaticClass()))
    {
        if (klass == Asset::GetStaticClass())
            return VariantType(VariantType::Asset);
        return VariantType(VariantType::Asset, fullname);
    }
    if (klass->IsSubClassOf(ScriptingObject::GetStaticClass()))
    {
        if (klass == ScriptingObject::GetStaticClass())
            return VariantType(VariantType::Object);
        return VariantType(VariantType::Object, fullname);
    }
    // TODO: support any dictionary unboxing

    LOG(Error, "Invalid managed type to unbox {0}", String(fullname));
    return VariantType();
}

MTypeObject* MUtils::BoxVariantType(const VariantType& value)
{
    if (value.Type == VariantType::Null)
        return nullptr;
    MClass* klass = GetClass(value);
    if (!klass)
    {
        LOG(Error, "Invalid native type to box {0}", value);
        return nullptr;
    }
    MType* mType = klass->GetType();
    return INTERNAL_TYPE_GET_OBJECT(mType);
}

Variant MUtils::UnboxVariant(MObject* value)
{
    if (value == nullptr)
        return Variant::Null;
    const auto& stdTypes = *StdTypesContainer::Instance();
    MClass* klass = MCore::Object::GetClass(value);

    MType* mType = klass->GetType();
    const MTypes mTypes = MCore::Type::GetType(mType);
    void* unboxed = MCore::Object::Unbox(value);

    // Fast type detection for in-built types
    switch (mTypes)
    {
    case MTypes::Void:
        return Variant(VariantType(VariantType::Void));
    case MTypes::Boolean:
        return *static_cast<bool*>(unboxed);
    case MTypes::I1:
        return *static_cast<int8*>(unboxed);
    case MTypes::U1:
        return *static_cast<uint8*>(unboxed);
    case MTypes::I2:
        return *static_cast<int16*>(unboxed);
    case MTypes::U2:
        return *static_cast<uint16*>(unboxed);
    case MTypes::Char:
        return *static_cast<Char*>(unboxed);
    case MTypes::I4:
        return *static_cast<int32*>(unboxed);
    case MTypes::U4:
        return *static_cast<uint32*>(unboxed);
    case MTypes::I8:
        return *static_cast<int64*>(unboxed);
    case MTypes::U8:
        return *static_cast<uint64*>(unboxed);
    case MTypes::R4:
        return *static_cast<float*>(unboxed);
    case MTypes::R8:
        return *static_cast<double*>(unboxed);
    case MTypes::String:
        return Variant(MUtils::ToString((MString*)value));
    case MTypes::Ptr:
        return *static_cast<void**>(unboxed);
    case MTypes::ValueType:
        if (klass == stdTypes.GuidClass)
            return Variant(*static_cast<Guid*>(unboxed));
        if (klass == stdTypes.Vector2Class)
            return *static_cast<Vector2*>(unboxed);
        if (klass == stdTypes.Vector3Class)
            return *static_cast<Vector3*>(unboxed);
        if (klass == stdTypes.Vector4Class)
            return *static_cast<Vector4*>(unboxed);
        if (klass == Int2::TypeInitializer.GetClass())
            return *static_cast<Int2*>(unboxed);
        if (klass == Int3::TypeInitializer.GetClass())
            return *static_cast<Int3*>(unboxed);
        if (klass == Int4::TypeInitializer.GetClass())
            return *static_cast<Int4*>(unboxed);
        if (klass == Float2::TypeInitializer.GetClass())
            return *static_cast<Float2*>(unboxed);
        if (klass == Float3::TypeInitializer.GetClass())
            return *static_cast<Float3*>(unboxed);
        if (klass == Float4::TypeInitializer.GetClass())
            return *static_cast<Float4*>(unboxed);
        if (klass == Double2::TypeInitializer.GetClass())
            return *static_cast<Double2*>(unboxed);
        if (klass == Double3::TypeInitializer.GetClass())
            return *static_cast<Double3*>(unboxed);
        if (klass == Double4::TypeInitializer.GetClass())
            return *static_cast<Double4*>(unboxed);
        if (klass == stdTypes.ColorClass)
            return *static_cast<Color*>(unboxed);
        if (klass == stdTypes.BoundingBoxClass)
            return Variant(*static_cast<BoundingBox*>(unboxed));
        if (klass == stdTypes.QuaternionClass)
            return *static_cast<Quaternion*>(unboxed);
        if (klass == stdTypes.TransformClass)
            return Variant(*static_cast<Transform*>(unboxed));
        if (klass == stdTypes.BoundingSphereClass)
            return *static_cast<BoundingSphere*>(unboxed);
        if (klass == stdTypes.RectangleClass)
            return *static_cast<Rectangle*>(unboxed);
        if (klass == stdTypes.MatrixClass)
            return Variant(*reinterpret_cast<Matrix*>(unboxed));
        break;
    case MTypes::SzArray:
    case MTypes::Array:
    {
        void* ptr = MCore::Array::GetAddress((MArray*)value);
        const MClass* arrayClass = klass == stdTypes.ManagedArrayClass ? MCore::Array::GetArrayClass((MArray*)value) : klass;
        const MClass* elementClass = arrayClass->GetElementClass();
        if (elementClass == MCore::TypeCache::Byte)
        {
            Variant v;
            v.SetBlob(ptr, MCore::Array::GetLength((MArray*)value));
            return v;
        }
        const StringAnsiView fullname = arrayClass->GetFullName();
        Variant v;
        v.SetType(MoveTemp(VariantType(VariantType::Array, fullname)));
        auto& array = v.AsArray();
        array.Resize(MCore::Array::GetLength((MArray*)value));
        const StringAnsiView elementTypename(*fullname, fullname.Length() - 2);
        const int32 elementSize = elementClass->GetInstanceSize();
        if (elementClass->IsEnum())
        {
            // Array of Enums
            for (int32 i = 0; i < array.Count(); i++)
            {
                array[i].SetType(VariantType(VariantType::Enum, elementTypename));
                Platform::MemoryCopy(&array[i].AsUint64, (byte*)ptr + elementSize * i, elementSize);
            }
        }
        else if (elementClass->IsValueType())
        {
            // Array of Structures
            VariantType elementType = UnboxVariantType(elementClass->GetType());
            switch (elementType.Type)
            {
            case VariantType::Bool:
            case VariantType::Int:
            case VariantType::Uint:
            case VariantType::Int64:
            case VariantType::Uint64:
            case VariantType::Float:
            case VariantType::Double:
            case VariantType::Float2:
            case VariantType::Float3:
            case VariantType::Float4:
            case VariantType::Color:
            case VariantType::Guid:
            case VariantType::Quaternion:
            case VariantType::Rectangle:
            case VariantType::Int2:
            case VariantType::Int3:
            case VariantType::Int4:
            case VariantType::Int16:
            case VariantType::Uint16:
            case VariantType::Double2:
            case VariantType::Double3:
#if !USE_LARGE_WORLDS
            case VariantType::BoundingSphere:
            case VariantType::BoundingBox:
            case VariantType::Ray:
#endif
                // Optimized unboxing of raw data type
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto& a = array[i];
                    a.SetType(elementType);
                    Platform::MemoryCopy(&a.AsData, (byte*)ptr + elementSize * i, elementSize);
                }
                break;
            case VariantType::Transform:
            case VariantType::Matrix:
            case VariantType::Double4:
#if USE_LARGE_WORLDS
            case VariantType::BoundingSphere:
            case VariantType::BoundingBox:
            case VariantType::Ray:
#endif
                // Optimized unboxing of raw data type
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto& a = array[i];
                    a.SetType(elementType);
                    Platform::MemoryCopy(a.AsBlob.Data, (byte*)ptr + elementSize * i, elementSize);
                }
                break;
            case VariantType::Structure:
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(elementType.TypeName);
                if (typeHandle)
                {
                    // Unbox array of structures
                    const ScriptingType& type = typeHandle.GetType();
                    ASSERT(type.Type == ScriptingTypes::Structure);
                    // TODO: optimize this for large arrays to prevent multiple AllocStructure calls in Variant::SetType by using computed struct type
                    for (int32 i = 0; i < array.Count(); i++)
                    {
                        auto& a = array[i];
                        a.SetType(elementType);
                        void* managed = (byte*)ptr + elementSize * i;
                        // TODO: optimize structures unboxing to not require MObject* but raw managed value data to prevent additional boxing here
                        MObject* boxed = MCore::Object::New(elementClass);
                        Platform::MemoryCopy(MCore::Object::Unbox(boxed), managed, elementSize);
                        type.Struct.Unbox(a.AsBlob.Data, boxed);
                    }
                    break;
                }
                LOG(Error, "Invalid type to unbox {0}", v.Type);
                break;
            }
            default:
                LOG(Error, "Invalid type to unbox {0}", v.Type);
                break;
            }
        }
        else
        {
            // Array of Objects
            for (int32 i = 0; i < array.Count(); i++)
                array[i] = UnboxVariant(((MObject**)ptr)[i]);
        }
        return v;
    }
    case MTypes::GenericInst:
    {
        if (klass->GetName() == "Dictionary`2" && klass->GetNamespace() == "System.Collections.Generic")
        {
            // Dictionary
            ManagedDictionary managed(value);
            MArray* managedKeys = managed.GetKeys();
            int32 length = managedKeys ? MCore::Array::GetLength(managedKeys) : 0;
            Dictionary<Variant, Variant> native;
            native.EnsureCapacity(length);
            MObject** managedKeysPtr = MCore::Array::GetAddress<MObject*>(managedKeys);
            for (int32 i = 0; i < length; i++)
            {
                MObject* keyManaged = managedKeysPtr[i];
                MObject* valueManaged = managed.GetValue(keyManaged);
                native.Add(UnboxVariant(keyManaged), UnboxVariant(valueManaged));
            }
            Variant v(MoveTemp(native));
            v.Type.SetTypeName(klass->GetFullName());
            return v;
        }
        break;
    }
    }

    if (klass->IsSubClassOf(Asset::GetStaticClass()))
        return static_cast<Asset*>(ScriptingObject::ToNative(value));
    if (klass->IsSubClassOf(ScriptingObject::GetStaticClass()))
        return ScriptingObject::ToNative(value);
    if (klass->IsEnum())
    {
        const StringAnsiView fullname = klass->GetFullName();
        Variant v;
        v.Type = MoveTemp(VariantType(VariantType::Enum, fullname));
        // TODO: what about 64-bit enum? use enum size with memcpy
        v.AsUint64 = *static_cast<uint32*>(MCore::Object::Unbox(value));
        return v;
    }
    if (klass->IsValueType())
    {
        const StringAnsiView fullname = klass->GetFullName();
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(fullname);
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            Variant v;
            v.Type = MoveTemp(VariantType(VariantType::Structure, fullname));
            v.AsBlob.Data = Allocator::Allocate(type.Size);
            v.AsBlob.Length = type.Size;
            type.Struct.Ctor(v.AsBlob.Data);
            type.Struct.Unbox(v.AsBlob.Data, value);
            return v;
        }
        return Variant(value);
    }

    return Variant(value);
}

MObject* MUtils::BoxVariant(const Variant& value)
{
    const auto& stdTypes = *StdTypesContainer::Instance();
    switch (value.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
        return nullptr;
    case VariantType::Bool:
        return MCore::Object::Box((void*)&value.AsBool, MCore::TypeCache::Boolean);
    case VariantType::Int16:
        return MCore::Object::Box((void*)&value.AsInt16, MCore::TypeCache::Int16);
    case VariantType::Uint16:
        return MCore::Object::Box((void*)&value.AsUint16, MCore::TypeCache::UInt16);
    case VariantType::Int:
        return MCore::Object::Box((void*)&value.AsInt, MCore::TypeCache::Int32);
    case VariantType::Uint:
        return MCore::Object::Box((void*)&value.AsUint, MCore::TypeCache::UInt32);
    case VariantType::Int64:
        return MCore::Object::Box((void*)&value.AsInt64, MCore::TypeCache::Int64);
    case VariantType::Uint64:
        return MCore::Object::Box((void*)&value.AsUint64, MCore::TypeCache::UInt64);
    case VariantType::Float:
        return MCore::Object::Box((void*)&value.AsFloat, MCore::TypeCache::Single);
    case VariantType::Double:
        return MCore::Object::Box((void*)&value.AsDouble, MCore::TypeCache::Double);
    case VariantType::Float2:
        return MCore::Object::Box((void*)&value.AsData, Float2::TypeInitializer.GetClass());
    case VariantType::Float3:
        return MCore::Object::Box((void*)&value.AsData, Float3::TypeInitializer.GetClass());
    case VariantType::Float4:
        return MCore::Object::Box((void*)&value.AsData, Float4::TypeInitializer.GetClass());
    case VariantType::Double2:
        return MCore::Object::Box((void*)&value.AsData, Double2::TypeInitializer.GetClass());
    case VariantType::Double3:
        return MCore::Object::Box((void*)&value.AsData, Double3::TypeInitializer.GetClass());
    case VariantType::Double4:
        return MCore::Object::Box((void*)&value.AsData, Double4::TypeInitializer.GetClass());
    case VariantType::Color:
        return MCore::Object::Box((void*)&value.AsData, stdTypes.ColorClass);
    case VariantType::Guid:
        return MCore::Object::Box((void*)&value.AsData, stdTypes.GuidClass);
    case VariantType::String:
#if USE_NETCORE
        return (MObject*)MUtils::ToString((StringView)value);
#else
        return (MObject*)MUtils::ToString((StringView)value);
#endif
    case VariantType::Quaternion:
        return MCore::Object::Box((void*)&value.AsData, stdTypes.QuaternionClass);
    case VariantType::BoundingSphere:
        return MCore::Object::Box((void*)&value.AsBoundingSphere(), stdTypes.BoundingSphereClass);
    case VariantType::Rectangle:
        return MCore::Object::Box((void*)&value.AsData, stdTypes.RectangleClass);
    case VariantType::Pointer:
        return MCore::Object::Box((void*)&value.AsPointer, MCore::TypeCache::IntPtr);
    case VariantType::Ray:
        return MCore::Object::Box((void*)&value.AsRay(), stdTypes.RayClass);
    case VariantType::BoundingBox:
        return MCore::Object::Box((void*)&value.AsBoundingBox(), stdTypes.BoundingBoxClass);
    case VariantType::Transform:
        return MCore::Object::Box(value.AsBlob.Data, stdTypes.TransformClass);
    case VariantType::Matrix:
        return MCore::Object::Box(value.AsBlob.Data, stdTypes.MatrixClass);
    case VariantType::Blob:
        return (MObject*)ToArray(Span<byte>((const byte*)value.AsBlob.Data, value.AsBlob.Length));
    case VariantType::Object:
        return value.AsObject ? value.AsObject->GetOrCreateManagedInstance() : nullptr;
    case VariantType::Asset:
        return value.AsAsset ? value.AsAsset->GetOrCreateManagedInstance() : nullptr;
    case VariantType::Array:
    {
        MArray* managed;
        const auto& array = value.AsArray();
        if (value.Type.TypeName)
        {
            const StringAnsiView elementTypename(value.Type.TypeName, StringUtils::Length(value.Type.TypeName) - 2);
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(elementTypename);
            MClass* elementClass;
            if (typeHandle && typeHandle.GetType().ManagedClass)
                elementClass = typeHandle.GetType().ManagedClass;
            else
                elementClass = Scripting::FindClass(elementTypename);
            if (!elementClass)
            {
                LOG(Error, "Invalid type to box {0}", value.Type);
                return nullptr;
            }
            const int32 elementSize = elementClass->GetInstanceSize();
            managed = MCore::Array::New(elementClass, array.Count());
            if (elementClass->IsEnum())
            {
                // Array of Enums
                byte* managedPtr = (byte*)MCore::Array::GetAddress(managed);
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto data = (uint64)array[i];
                    Platform::MemoryCopy(managedPtr + elementSize * i, &data, elementSize);
                }
            }
            else if (elementClass->IsValueType())
            {
                // Array of Structures
                const VariantType elementType = UnboxVariantType(elementClass->GetType());
                byte* managedPtr = (byte*)MCore::Array::GetAddress(managed);
                switch (elementType.Type)
                {
                case VariantType::Bool:
                case VariantType::Int:
                case VariantType::Uint:
                case VariantType::Int64:
                case VariantType::Uint64:
                case VariantType::Float:
                case VariantType::Double:
                case VariantType::Float2:
                case VariantType::Float3:
                case VariantType::Float4:
                case VariantType::Color:
                case VariantType::Guid:
                case VariantType::Quaternion:
                case VariantType::Rectangle:
                case VariantType::Int2:
                case VariantType::Int3:
                case VariantType::Int4:
                case VariantType::Int16:
                case VariantType::Uint16:
                case VariantType::Double2:
                case VariantType::Double3:
#if !USE_LARGE_WORLDS
                case VariantType::BoundingSphere:
                case VariantType::BoundingBox:
                case VariantType::Ray:
#endif
                    // Optimized boxing of raw data type
                    for (int32 i = 0; i < array.Count(); i++)
                        Platform::MemoryCopy(managedPtr + elementSize * i, &array[i].AsData, elementSize);
                    break;
                case VariantType::Transform:
                case VariantType::Matrix:
                case VariantType::Double4:
#if USE_LARGE_WORLDS
                case VariantType::BoundingSphere:
                case VariantType::BoundingBox:
                case VariantType::Ray:
#endif
                    // Optimized boxing of raw data type
                    for (int32 i = 0; i < array.Count(); i++)
                        Platform::MemoryCopy(managedPtr + elementSize * i, array[i].AsBlob.Data, elementSize);
                    break;
                case VariantType::Structure:
                    if (typeHandle)
                    {
                        const ScriptingType& type = typeHandle.GetType();
                        ASSERT(type.Type == ScriptingTypes::Structure);
                        for (int32 i = 0; i < array.Count(); i++)
                        {
                            // TODO: optimize structures boxing to not return MObject* but use raw managed object to prevent additional boxing here
                            MObject* boxed = type.Struct.Box(array[i].AsBlob.Data);
                            Platform::MemoryCopy(managedPtr + elementSize * i, MCore::Object::Unbox(boxed), elementSize);
                        }
                        break;
                    }
                    LOG(Error, "Invalid type to box {0}", value.Type);
                    break;
                default:
                    LOG(Error, "Invalid type to box {0}", value.Type);
                    break;
                }
            }
            else
            {
                // Array of Objects
                for (int32 i = 0; i < array.Count(); i++)
                    MCore::GC::WriteArrayRef(managed, BoxVariant(array[i]), i);
            }
        }
        else
        {
            // object[]
            managed = MCore::Array::New(MCore::TypeCache::Object, array.Count());
            for (int32 i = 0; i < array.Count(); i++)
                MCore::GC::WriteArrayRef(managed, BoxVariant(array[i]), i);
        }
        return (MObject*)managed;
    }
    case VariantType::Dictionary:
    {
        // Get dictionary key and value types
        MClass *keyClass, *valueClass;
        GetDictionaryKeyValueTypes(value.Type.GetTypeName(), keyClass, valueClass);
        if (!keyClass || !valueClass)
        {
            LOG(Error, "Invalid type to box {0}", value.Type);
            return nullptr;
        }

        // Allocate managed dictionary
        ManagedDictionary managed = ManagedDictionary::New(keyClass->GetType(), valueClass->GetType());
        if (!managed.Instance)
            return nullptr;

        // Add native keys and values
        const auto& dictionary = *value.AsDictionary;
        for (const auto& e : dictionary)
        {
            managed.Add(BoxVariant(e.Key), BoxVariant(e.Value));
        }

        return managed.Instance;
    }
    case VariantType::Structure:
    {
        if (value.AsBlob.Data == nullptr)
            return nullptr;
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(value.Type.TypeName));
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            return type.Struct.Box(value.AsBlob.Data);
        }
        LOG(Error, "Invalid type to box {0}", value.Type);
        return nullptr;
    }
    case VariantType::Enum:
    {
        const auto klass = Scripting::FindClass(StringAnsiView(value.Type.TypeName));
        if (klass)
            return MCore::Object::Box((void*)&value.AsUint64, klass);
        LOG(Error, "Invalid type to box {0}", value.Type);
        return nullptr;
    }
    case VariantType::ManagedObject:
#if USE_NETCORE
        return value.AsUint64 ? MCore::GCHandle::GetTarget(value.AsUint64) : nullptr;
#else
        return value.AsUint ? MCore::GCHandle::GetTarget(value.AsUint) : nullptr;
#endif
    case VariantType::Typename:
    {
        const auto klass = Scripting::FindClass((StringAnsiView)value);
        if (klass)
            return (MObject*)GetType(klass);
        LOG(Error, "Invalid type to box {0}", value);
        return nullptr;
    }
    default:
        LOG(Error, "Invalid type to box {0}", value.Type);
        return nullptr;
    }
}

const StringAnsi& MUtils::GetClassFullname(MObject* obj)
{
    if (obj)
    {
        MClass* mClass = MCore::Object::GetClass(obj);
        return mClass->GetFullName();
    }
    return StringAnsi::Empty;
}

MClass* MUtils::GetClass(MTypeObject* type)
{
    if (type == nullptr)
        return nullptr;
    MType* mType = INTERNAL_TYPE_OBJECT_GET(type);
    return MCore::Type::GetClass(mType);
}

MClass* MUtils::GetClass(const VariantType& value)
{
    auto mclass = Scripting::FindClass(StringAnsiView(value.TypeName));
    if (mclass)
        return mclass;
    const auto& stdTypes = *StdTypesContainer::Instance();
    switch (value.Type)
    {
    case VariantType::Void:
        return MCore::TypeCache::Void;
    case VariantType::Bool:
        return MCore::TypeCache::Boolean;
    case VariantType::Int16:
        return MCore::TypeCache::Int16;
    case VariantType::Uint16:
        return MCore::TypeCache::UInt16;
    case VariantType::Int:
        return MCore::TypeCache::Int32;
    case VariantType::Uint:
        return MCore::TypeCache::UInt32;
    case VariantType::Int64:
        return MCore::TypeCache::Int64;
    case VariantType::Uint64:
        return MCore::TypeCache::UInt64;
    case VariantType::Float:
        return MCore::TypeCache::Single;
    case VariantType::Double:
        return MCore::TypeCache::Double;
    case VariantType::Pointer:
        return MCore::TypeCache::IntPtr;
    case VariantType::String:
        return MCore::TypeCache::String;
    case VariantType::Object:
        return ScriptingObject::GetStaticClass();
    case VariantType::Asset:
        return Asset::GetStaticClass();
    case VariantType::Blob:
        return MCore::Array::GetClass(MCore::TypeCache::Byte);
    case VariantType::Float2:
        return Float2::TypeInitializer.GetClass();
    case VariantType::Float3:
        return Float3::TypeInitializer.GetClass();
    case VariantType::Float4:
        return Float4::TypeInitializer.GetClass();
    case VariantType::Double2:
        return Double2::TypeInitializer.GetClass();
    case VariantType::Double3:
        return Double3::TypeInitializer.GetClass();
    case VariantType::Double4:
        return Double4::TypeInitializer.GetClass();
    case VariantType::Color:
        return stdTypes.ColorClass;
    case VariantType::Guid:
        return stdTypes.GuidClass;
    case VariantType::Typename:
        return stdTypes.TypeClass;
    case VariantType::BoundingBox:
        return stdTypes.BoundingBoxClass;
    case VariantType::BoundingSphere:
        return stdTypes.BoundingSphereClass;
    case VariantType::Quaternion:
        return stdTypes.QuaternionClass;
    case VariantType::Transform:
        return stdTypes.TransformClass;
    case VariantType::Rectangle:
        return stdTypes.RectangleClass;
    case VariantType::Ray:
        return stdTypes.RayClass;
    case VariantType::Matrix:
        return stdTypes.MatrixClass;
    case VariantType::Array:
        if (value.TypeName)
        {
            const StringAnsiView elementTypename(value.TypeName, StringUtils::Length(value.TypeName) - 2);
            mclass = Scripting::FindClass(elementTypename);
            if (mclass)
                return MCore::Array::GetClass(mclass);
        }
        return MCore::Array::GetClass(MCore::TypeCache::Object);
    case VariantType::Dictionary:
    {
        MClass *keyClass, *valueClass;
        GetDictionaryKeyValueTypes(value.GetTypeName(), keyClass, valueClass);
        if (!keyClass || !valueClass)
        {
            LOG(Error, "Invalid type to box {0}", value.ToString());
            return nullptr;
        }
        return GetClass(ManagedDictionary::GetClass(keyClass->GetType(), valueClass->GetType()));
    }
    case VariantType::ManagedObject:
        return MCore::TypeCache::Object;
    default: ;
    }
    return nullptr;
}

MClass* MUtils::GetClass(const Variant& value)
{
    const auto& stdTypes = *StdTypesContainer::Instance();
    switch (value.Type.Type)
    {
    case VariantType::Void:
        return MCore::TypeCache::Void;
    case VariantType::Bool:
        return MCore::TypeCache::Boolean;
    case VariantType::Int16:
        return MCore::TypeCache::Int16;
    case VariantType::Uint16:
        return MCore::TypeCache::UInt16;
    case VariantType::Int:
        return MCore::TypeCache::Int32;
    case VariantType::Uint:
        return MCore::TypeCache::UInt32;
    case VariantType::Int64:
        return MCore::TypeCache::Int64;
    case VariantType::Uint64:
        return MCore::TypeCache::UInt64;
    case VariantType::Float:
        return MCore::TypeCache::Single;
    case VariantType::Double:
        return MCore::TypeCache::Double;
    case VariantType::Pointer:
        return MCore::TypeCache::IntPtr;
    case VariantType::String:
        return MCore::TypeCache::String;
    case VariantType::Blob:
        return MCore::Array::GetClass(MCore::TypeCache::Byte);
    case VariantType::Float2:
        return Float2::TypeInitializer.GetClass();
    case VariantType::Float3:
        return Float3::TypeInitializer.GetClass();
    case VariantType::Float4:
        return Float4::TypeInitializer.GetClass();
    case VariantType::Double2:
        return Double2::TypeInitializer.GetClass();
    case VariantType::Double3:
        return Double3::TypeInitializer.GetClass();
    case VariantType::Double4:
        return Double4::TypeInitializer.GetClass();
    case VariantType::Color:
        return stdTypes.ColorClass;
    case VariantType::Guid:
        return stdTypes.GuidClass;
    case VariantType::Typename:
        return stdTypes.TypeClass;
    case VariantType::BoundingBox:
        return stdTypes.BoundingBoxClass;
    case VariantType::BoundingSphere:
        return stdTypes.BoundingSphereClass;
    case VariantType::Quaternion:
        return stdTypes.QuaternionClass;
    case VariantType::Transform:
        return stdTypes.TransformClass;
    case VariantType::Rectangle:
        return stdTypes.RectangleClass;
    case VariantType::Ray:
        return stdTypes.RayClass;
    case VariantType::Matrix:
        return stdTypes.MatrixClass;
    case VariantType::Array:
    case VariantType::Dictionary:
        break;
    case VariantType::Object:
        return value.AsObject ? value.AsObject->GetClass() : nullptr;
    case VariantType::Asset:
        return value.AsAsset ? value.AsAsset->GetClass() : nullptr;
    case VariantType::Structure:
    case VariantType::Enum:
        return Scripting::FindClass(StringAnsiView(value.Type.TypeName));
    case VariantType::ManagedObject:
    {
        MObject* obj = (MObject*)value;
        if (obj)
            return MCore::Object::GetClass(obj);
    }
    default: ;
    }
    return GetClass(value.Type);
}

MTypeObject* MUtils::GetType(MObject* object)
{
    if (!object)
        return nullptr;
    MClass* klass = MCore::Object::GetClass(object);
    return GetType(klass);
}

MTypeObject* MUtils::GetType(MClass* klass)
{
    if (!klass)
        return nullptr;
    MType* type = klass->GetType();
    return INTERNAL_TYPE_GET_OBJECT(type);
}

BytesContainer MUtils::LinkArray(MArray* arrayObj)
{
    BytesContainer result;
    const int32 length = arrayObj ? MCore::Array::GetLength(arrayObj) : 0;
    if (length != 0)
    {
        result.Link((byte*)MCore::Array::GetAddress(arrayObj), length);
    }
    return result;
}

void* MUtils::VariantToManagedArgPtr(Variant& value, MType* type, bool& failed)
{
    // Convert Variant into matching managed type and return pointer to data for the method invocation
    MTypes mType = MCore::Type::GetType(type);
    switch (mType)
    {
    case MTypes::Boolean:
        if (value.Type.Type != VariantType::Bool)
            value = (bool)value;
        return &value.AsBool;
    case MTypes::Char:
    case MTypes::I1:
    case MTypes::I2:
        if (value.Type.Type != VariantType::Int16)
            value = (int16)value;
        return &value.AsInt16;
    case MTypes::I4:
        if (value.Type.Type != VariantType::Int)
            value = (int32)value;
        return &value.AsInt;
    case MTypes::U1:
    case MTypes::U2:
        if (value.Type.Type != VariantType::Uint16)
            value = (uint16)value;
        return &value.AsUint16;
    case MTypes::U4:
        if (value.Type.Type != VariantType::Uint)
            value = (uint32)value;
        return &value.AsUint;
    case MTypes::I8:
        if (value.Type.Type != VariantType::Int64)
            value = (int64)value;
        return &value.AsInt64;
    case MTypes::U8:
        if (value.Type.Type != VariantType::Uint64)
            value = (uint64)value;
        return &value.AsUint64;
    case MTypes::R4:
        if (value.Type.Type != VariantType::Float)
            value = (float)value;
        return &value.AsFloat;
    case MTypes::R8:
        if (value.Type.Type != VariantType::Double)
            value = (double)value;
        return &value.AsDouble;
    case MTypes::String:
        return MUtils::ToString((StringView)value);
    case MTypes::ValueType:
    {
        MClass* klass = MCore::Type::GetClass(type);
        if (klass->IsEnum())
        {
            if (value.Type.Type != VariantType::Enum)
            {
                value.SetType(VariantType(VariantType::Enum, klass));
                value.AsUint64 = 0;
            }
            return &value.AsUint64;
        }
        const auto stdTypes = StdTypesContainer::Instance();
#define CASE_IN_BUILD_TYPE(type, access) \
    if (klass == stdTypes->type##Class) \
    { \
        if (value.Type.Type != VariantType::type) \
            value = Variant((type)value); \
        return value.access; \
    }
        CASE_IN_BUILD_TYPE(Color, AsData);
        CASE_IN_BUILD_TYPE(Quaternion, AsData);
        CASE_IN_BUILD_TYPE(Guid, AsData);
        CASE_IN_BUILD_TYPE(Rectangle, AsData);
        CASE_IN_BUILD_TYPE(Matrix, AsBlob.Data);
        CASE_IN_BUILD_TYPE(Transform, AsBlob.Data);
#undef CASE_IN_BUILD_TYPE
#define CASE_IN_BUILD_TYPE(type, access) \
    if (klass == stdTypes->type##Class) \
    { \
        if (value.Type.Type != VariantType::type) \
            value = Variant((type)value); \
        return (void*)&value.access(); \
    }
        CASE_IN_BUILD_TYPE(Vector2, AsVector2);
        CASE_IN_BUILD_TYPE(Vector3, AsVector3);
        CASE_IN_BUILD_TYPE(Vector4, AsVector4);
        CASE_IN_BUILD_TYPE(BoundingSphere, AsBoundingSphere);
        CASE_IN_BUILD_TYPE(BoundingBox, AsBoundingBox);
        CASE_IN_BUILD_TYPE(Ray, AsRay);
#undef CASE_IN_BUILD_TYPE
#define CASE_IN_BUILD_TYPE(type, access) \
    if (klass == type::TypeInitializer.GetClass()) \
    { \
        if (value.Type.Type != VariantType::type) \
            value = Variant((type)value); \
        return value.access; \
    }
        CASE_IN_BUILD_TYPE(Float2, AsData);
        CASE_IN_BUILD_TYPE(Float3, AsData);
        CASE_IN_BUILD_TYPE(Float4, AsData);
        CASE_IN_BUILD_TYPE(Double2, AsData);
        CASE_IN_BUILD_TYPE(Double3, AsData);
        CASE_IN_BUILD_TYPE(Double4, AsBlob.Data);
#undef CASE_IN_BUILD_TYPE
        if (klass->IsValueType())
        {
            if (value.Type.Type == VariantType::Structure)
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(value.Type.TypeName));
                if (typeHandle && value.AsBlob.Data)
                {
                    auto& valueType = typeHandle.GetType();
                    if (valueType.ManagedClass == MCore::Type::GetClass(type))
                    {
                        return MCore::Object::Unbox(valueType.Struct.Box(value.AsBlob.Data));
                    }
                    LOG(Error, "Cannot marshal argument of type {0} as {1}", String(valueType.Fullname), MCore::Type::ToString(type));
                }
            }
            else
            {
                const StringAnsiView fullname = klass->GetFullName();
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(fullname);
                if (typeHandle)
                {
                    auto& valueType = typeHandle.GetType();
                    value.SetType(VariantType(VariantType::Structure, fullname));
                    return MCore::Object::Unbox(valueType.Struct.Box(value.AsBlob.Data));
                }
            }
        }
    }
    break;
    case MTypes::Enum:
    {
        if (value.Type.Type != VariantType::Enum)
            return nullptr;
        return &value.AsUint64;
    }
    case MTypes::Class:
    {
        if (value.Type.Type == VariantType::Null)
            return nullptr;
        MObject* object = BoxVariant(value);
        if (object && !MCore::Object::GetClass(object)->IsSubClassOf(MCore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case MTypes::Object:
        return BoxVariant(value);
    case MTypes::SzArray:
    case MTypes::Array:
    {
        if (value.Type.Type != VariantType::Array)
            return nullptr;
        MObject* object = BoxVariant(value);
        auto typeStr = MCore::Type::ToString(type);
        if (object && !MCore::Object::GetClass(object)->IsSubClassOf(MCore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case MTypes::GenericInst:
    {
        if (value.Type.Type == VariantType::Null)
            return nullptr;
        MObject* object = BoxVariant(value);
        if (object && !MCore::Object::GetClass(object)->IsSubClassOf(MCore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case MTypes::Ptr:
        switch (value.Type.Type)
        {
        case VariantType::Pointer:
            return &value.AsPointer;
        case VariantType::Object:
            return &value.AsObject;
        case VariantType::Asset:
            return &value.AsAsset;
        case VariantType::Structure:
        case VariantType::Blob:
            return &value.AsBlob.Data;
        default:
            return nullptr;
        }
    default:
        break;
    }
    failed = true;
    return nullptr;
}

MObject* MUtils::ToManaged(const Version& value)
{
#if USE_NETCORE
    auto scriptingClass = Scripting::GetStaticClass();
    CHECK_RETURN(scriptingClass, nullptr);
    auto versionToManaged = scriptingClass->GetMethod("VersionToManaged", 4);
    CHECK_RETURN(versionToManaged, nullptr);

    int32 major = value.Major();
    int32 minor = value.Minor();
    int32 build = value.Build();
    int32 revision = value.Revision();

    void* params[4];
    params[0] = &major;
    params[1] = &minor;
    params[2] = &build;
    params[3] = &revision;
    auto obj = versionToManaged->Invoke(nullptr, params, nullptr);
#else
    auto obj = MCore::Object::New(Scripting::FindClass("System.Version"));
    Platform::MemoryCopy(MCore::Object::Unbox(obj), &value, sizeof(Version));
#endif
    return obj;
}

Version MUtils::ToNative(MObject* value)
{
    Version result;
    if (value)
#if USE_NETCORE
    {
        auto scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, result);
        auto versionToNative = scriptingClass->GetMethod("VersionToNative", 5);
        CHECK_RETURN(versionToNative, result);

        void* params[5];
        params[0] = value;
        params[1] = (byte*)&result;
        params[2] = (byte*)&result + sizeof(int32);
        params[3] = (byte*)&result + sizeof(int32) * 2;
        params[4] = (byte*)&result + sizeof(int32) * 3;
        versionToNative->Invoke(nullptr, params, nullptr);

        return result;
    }
#else
        return *(Version*)MCore::Object::Unbox(value);
#endif
    return result;
}

#endif
