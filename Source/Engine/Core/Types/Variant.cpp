// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Variant.h"
#include "CommonValue.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Mathd.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Utilities/Crc.h"
#include "Engine/Utilities/StringConverter.h"

#if USE_NETCORE
#define MANAGED_GC_HANDLE AsUint64
#else
#define MANAGED_GC_HANDLE AsUint
#endif

namespace
{
    const char* InBuiltTypesTypeNames[40] =
    {
        // @formatter:off
        "",// Null
        "System.Void",// Void
        "System.Boolean",// Bool
        "System.Int32",// Int
        "System.UInt32",// Uint
        "System.Int64",// Int64
        "System.UInt64",// Uint64
        "System.Single",// Float
        "System.Double",// Double
        "System.IntPtr",// Pointer
        "System.String",// String
        "System.Object",// Object
        "",// Structure
        "FlaxEngine.Asset",// Asset
        "System.Byte[]",// Blob
        "",// Enum
        "FlaxEngine.Float2",// Float2
        "FlaxEngine.Float3",// Float3
        "FlaxEngine.Float4",// Float4
        "FlaxEngine.Color",// Color
        "System.Guid",// Guid
        "FlaxEngine.BoundingBox",// BoundingBox
        "FlaxEngine.BoundingSphere",// BoundingSphere
        "FlaxEngine.Quaternion",// Quaternion
        "FlaxEngine.Transform",// Transform
        "FlaxEngine.Rectangle",// Rectangle
        "FlaxEngine.Ray",// Ray
        "FlaxEngine.Matrix",// Matrix
        "System.Object[]",// Array
        "System.Collections.Generic.Dictionary`2[System.Object,System.Object]",// Dictionary
        "System.Object",// ManagedObject
        "System.Type",// Typename
        "FlaxEngine.Int2",// Int2
        "FlaxEngine.Int3",// Int3
        "FlaxEngine.Int4",// Int4
        "System.Int16",// Int16
        "System.UInt16",// Uint16
        "FlaxEngine.Double2",// Double2
        "FlaxEngine.Double3",// Double3
        "FlaxEngine.Double4",// Double4
        // @formatter:on
    };
}

static_assert(sizeof(VariantType) <= 16, "Invalid VariantType size!");
static_assert((int32)VariantType::Types::MAX == ARRAY_COUNT(InBuiltTypesTypeNames), "Invalid amount of in-built types infos!");

VariantType::VariantType(Types type, const StringView& typeName)
{
    Type = type;
    TypeName = nullptr;
    const int32 length = typeName.Length();
    if (length)
    {
        TypeName = static_cast<char*>(Allocator::Allocate(length + 1));
        StringUtils::ConvertUTF162ANSI(typeName.Get(), TypeName, length);
        TypeName[length] = 0;
    }
}

VariantType::VariantType(Types type, const StringAnsiView& typeName)
{
    Type = type;
    TypeName = nullptr;
    int32 length = typeName.Length();
    if (length)
    {
        TypeName = static_cast<char*>(Allocator::Allocate(length + 1));
        Platform::MemoryCopy(TypeName, typeName.Get(), length);
        TypeName[length] = 0;
    }
}

VariantType::VariantType(Types type, const MClass* klass)
{
    Type = type;
    TypeName = nullptr;
#if USE_CSHARP
    if (klass)
    {
        const StringAnsi& typeName = klass->GetFullName();
        const int32 length = typeName.Length();
        TypeName = static_cast<char*>(Allocator::Allocate(length + 1));
        Platform::MemoryCopy(TypeName, typeName.Get(), length);
        TypeName[length] = 0;
    }
#endif
}

VariantType::VariantType(const StringAnsiView& typeName)
{
    // Try using in-built type
    for (uint32 i = 0; i < ARRAY_COUNT(InBuiltTypesTypeNames); i++)
    {
        if (typeName == InBuiltTypesTypeNames[i])
        {
            new(this) VariantType((Types)i);
            return;
        }
    }
    {
        // Aliases
        if (typeName == "FlaxEngine.Vector2")
        {
            new(this) VariantType(Vector2);
            return;
        }
        if (typeName == "FlaxEngine.Vector3")
        {
            new(this) VariantType(Vector3);
            return;
        }
        if (typeName == "FlaxEngine.Vector4")
        {
            new(this) VariantType(Vector4);
            return;
        }
    }

    // Check case for array
    if (typeName.EndsWith(StringAnsiView("[]"), StringSearchCase::CaseSensitive))
    {
        new(this) VariantType(Array, StringAnsiView(typeName.Get(), typeName.Length() - 2));
        return;
    }

    // Try using scripting type
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (typeHandle)
    {
        const ScriptingType& type = typeHandle.GetType();
        switch (type.Type)
        {
        case ScriptingTypes::Script:
        case ScriptingTypes::Class:
        case ScriptingTypes::Interface:
            new(this) VariantType(Object, typeName);
            return;
        case ScriptingTypes::Structure:
            new(this) VariantType(Structure, typeName);
            return;
        case ScriptingTypes::Enum:
            new(this) VariantType(Enum, typeName);
            return;
        }
    }

    // Try using managed class
#if USE_CSHARP
    if (const auto mclass = Scripting::FindClass(typeName))
    {
        if (mclass->IsEnum())
            new(this) VariantType(Enum, typeName);
        else
            new(this) VariantType(ManagedObject, typeName);
        return;
    }
#endif

    new(this) VariantType();
    LOG(Warning, "Missing scripting type \'{0}\'", ::String(typeName));
}

VariantType::VariantType(const VariantType& other)
{
    Type = other.Type;
    TypeName = nullptr;
    const int32 length = StringUtils::Length(other.TypeName);
    if (length)
    {
        TypeName = static_cast<char*>(Allocator::Allocate(length + 1));
        Platform::MemoryCopy(TypeName, other.TypeName, length);
        TypeName[length] = 0;
    }
}

VariantType::VariantType(VariantType&& other) noexcept
{
    Type = other.Type;
    TypeName = other.TypeName;
    other.Type = Null;
    other.TypeName = nullptr;
}

VariantType& VariantType::operator=(const Types& type)
{
    Type = type;
    Allocator::Free(TypeName);
    TypeName = nullptr;
    return *this;
}

VariantType& VariantType::operator=(VariantType&& other)
{
    ASSERT(this != &other);
    Swap(Type, other.Type);
    Swap(TypeName, other.TypeName);
    return *this;
}

VariantType& VariantType::operator=(const VariantType& other)
{
    ASSERT(this != &other);
    Type = other.Type;
    Allocator::Free(TypeName);
    TypeName = nullptr;
    const int32 length = StringUtils::Length(other.TypeName);
    if (length)
    {
        TypeName = static_cast<char*>(Allocator::Allocate(length + 1));
        Platform::MemoryCopy(TypeName, other.TypeName, length);
        TypeName[length] = 0;
    }
    return *this;
}

bool VariantType::operator==(const Types& type) const
{
    return Type == type && TypeName == nullptr;
}

bool VariantType::operator==(const VariantType& other) const
{
    if (Type == other.Type)
    {
        if (TypeName && other.TypeName)
        {
            return StringUtils::Compare(TypeName, other.TypeName) == 0;
        }
        return true;
    }
    return false;
}

bool VariantType::operator==(const ScriptingTypeHandle& type) const
{
    if (Type == Null)
        return !type;
    return type && type.GetType().Fullname == GetTypeName();
}

void VariantType::SetTypeName(const StringView& typeName)
{
    if (StringUtils::Length(TypeName) != typeName.Length())
    {
        Allocator::Free(TypeName);
        TypeName = static_cast<char*>(Allocator::Allocate(typeName.Length() + 1));
        TypeName[typeName.Length()] = 0;
    }
    StringUtils::ConvertUTF162ANSI(typeName.Get(), TypeName, typeName.Length());
}

void VariantType::SetTypeName(const StringAnsiView& typeName)
{
    if (StringUtils::Length(TypeName) != typeName.Length())
    {
        Allocator::Free(TypeName);
        TypeName = static_cast<char*>(Allocator::Allocate(typeName.Length() + 1));
        TypeName[typeName.Length()] = 0;
    }
    Platform::MemoryCopy(TypeName, typeName.Get(), typeName.Length());
}

const char* VariantType::GetTypeName() const
{
    if (TypeName)
        return TypeName;
    return InBuiltTypesTypeNames[Type];
}

VariantType VariantType::GetElementType() const
{
    if (Type == Array)
    {
        if (TypeName)
        {
            const StringAnsiView elementTypename(TypeName, StringUtils::Length(TypeName) - 2);
            return VariantType(elementTypename);
        }
        return VariantType(Object);
    }
    return VariantType();
}

::String VariantType::ToString() const
{
    ::String result;
    switch (Type)
    {
    case Null:
        result = TEXT("Null");
        break;
    case Void:
        result = TEXT("Void");
        break;
    case Bool:
        result = TEXT("Bool");
        break;
    case Int16:
        result = TEXT("Int16");
        break;
    case Uint16:
        result = TEXT("Uint16");
        break;
    case Int:
        result = TEXT("Int");
        break;
    case Uint:
        result = TEXT("Uint");
        break;
    case Int64:
        result = TEXT("Int64");
        break;
    case Uint64:
        result = TEXT("Uint64");
        break;
    case Float:
        result = TEXT("Float");
        break;
    case Double:
        result = TEXT("Double");
        break;
    case Pointer:
        result = TEXT("Pointer");
        break;
    case String:
        result = TEXT("String");
        break;
    case Object:
        result = TEXT("Object");
        break;
    case Structure:
        result = TEXT("Structure");
        break;
    case Asset:
        result = TEXT("Asset");
        break;
    case Blob:
        result = TEXT("Blob");
        break;
    case Enum:
        result = TEXT("Enum");
        break;
    case Float2:
        result = TEXT("Float2");
        break;
    case Float3:
        result = TEXT("Float3");
        break;
    case Float4:
        result = TEXT("Float4");
        break;
    case Color:
        result = TEXT("Color");
        break;
    case Guid:
        result = TEXT("Guid");
        break;
    case BoundingBox:
        result = TEXT("BoundingBox");
        break;
    case BoundingSphere:
        result = TEXT("BoundingSphere");
        break;
    case Quaternion:
        result = TEXT("Quaternion");
        break;
    case Transform:
        result = TEXT("Transform");
        break;
    case Rectangle:
        result = TEXT("Rectangle");
        break;
    case Ray:
        result = TEXT("Ray");
        break;
    case Matrix:
        result = TEXT("Matrix");
        break;
    case Array:
        result = TEXT("Array");
        break;
    case Dictionary:
        result = TEXT("Dictionary");
        break;
    case ManagedObject:
        result = TEXT("ManagedObject");
        break;
    case Typename:
        result = TEXT("Type");
        break;
    case Int2:
        result = TEXT("Int2");
        break;
    case Int3:
        result = TEXT("Int3");
        break;
    case Int4:
        result = TEXT("Int4");
        break;
    case Double2:
        result = TEXT("Double2");
        break;
    case Double3:
        result = TEXT("Double3");
        break;
    case Double4:
        result = TEXT("Double4");
        break;
    default: ;
    }
    if (TypeName)
    {
        result += TEXT(" ");
        result += TypeName;
    }
    return result;
}

FLAXENGINE_API uint32 GetHash(const VariantType& key)
{
    auto hash = static_cast<uint32>(key.Type);
    CombineHash(hash, GetHash(key.TypeName));
    return hash;
}

static_assert(sizeof(Variant) <= 40, "Invalid Variant size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Float2), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Float3), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Float4), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Double2), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Double3), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Int2), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Int3), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Int4), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Color), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Quaternion), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Rectangle), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Guid), "Invalid Variant data size!");
#if !USE_LARGE_WORLDS
static_assert(sizeof(Variant::AsData) >= sizeof(BoundingSphere), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(BoundingBox), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Ray), "Invalid Variant data size!");
#endif
static_assert(sizeof(Variant::AsData) >= sizeof(Array<Variant, HeapAllocation>), "Invalid Variant data size!");

const Variant Variant::Zero(0.0f);
const Variant Variant::One(1.0f);
const Variant Variant::Null(nullptr);
const Variant Variant::False(false);
const Variant Variant::True(true);

Variant::Variant(const Variant& other)
{
    Type = VariantType();
    *this = other;
}

Variant::Variant(Variant&& other) noexcept
    : Type(MoveTemp(other.Type))
{
    switch (Type.Type)
    {
    case VariantType::Object:
        AsObject = other.AsObject;
        if (AsObject)
        {
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(&other);
            AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
            other.AsObject = nullptr;
        }
        break;
    case VariantType::Asset:
        AsAsset = other.AsAsset;
        if (AsAsset)
        {
            AsAsset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(&other);
            AsAsset->OnUnloaded.Bind<Variant, &Variant::OnAssetUnloaded>(this);
            other.AsAsset = nullptr;
        }
        break;
    case VariantType::Array:
        new(reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData))Array<Variant, HeapAllocation>(MoveTemp(*reinterpret_cast<Array<Variant, HeapAllocation>*>(other.AsData)));
        reinterpret_cast<Array<Variant, HeapAllocation>*>(other.AsData)->~Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        AsDictionary = other.AsDictionary;
        other.AsDictionary = nullptr;
        break;
    case VariantType::ManagedObject:
        MANAGED_GC_HANDLE = other.MANAGED_GC_HANDLE;
        other.MANAGED_GC_HANDLE = 0;
        break;
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::MAX:
        break;
    case VariantType::Structure:
    case VariantType::Blob:
    case VariantType::String:
    case VariantType::Transform:
    case VariantType::Matrix:
        AsBlob.Data = other.AsBlob.Data;
        AsBlob.Length = other.AsBlob.Length;
        other.AsBlob.Data = nullptr;
        other.AsBlob.Length = 0;
        break;
    default:
        Platform::MemoryCopy(AsData, other.AsData, sizeof(AsData));
        break;
    }
}

Variant::Variant(decltype(nullptr))
    : Type(VariantType::Null)
{
}

Variant::Variant(const VariantType& type)
    : Type(type)
{
}

Variant::Variant(VariantType&& type)
    : Type(MoveTemp(type))
{
}

Variant::Variant(bool v)
    : Type(VariantType::Bool)
{
    AsBool = v;
}

Variant::Variant(int16 v)
    : Type(VariantType::Int16)
{
    AsInt16 = v;
}

Variant::Variant(uint16 v)
    : Type(VariantType::Uint16)
{
    AsUint16 = v;
}

Variant::Variant(int32 v)
    : Type(VariantType::Int)
{
    AsInt = v;
}

Variant::Variant(uint32 v)
    : Type(VariantType::Uint)
{
    AsUint = v;
}

Variant::Variant(int64 v)
    : Type(VariantType::Int64)
{
    AsInt64 = v;
}

Variant::Variant(uint64 v)
    : Type(VariantType::Uint64)
{
    AsUint64 = v;
}

Variant::Variant(float v)
    : Type(VariantType::Float)
{
    AsFloat = v;
}

Variant::Variant(double v)
    : Type(VariantType::Double)
{
    AsDouble = v;
}

Variant::Variant(void* v)
    : Type(VariantType::Pointer)
{
    AsPointer = v;
}

Variant::Variant(ScriptingObject* v)
    : Type(VariantType::Object)
{
    AsObject = v;
    if (v)
    {
        Type.SetTypeName(v->GetType().Fullname);
        v->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
    }
}

Variant::Variant(Asset* v)
    : Type(VariantType::Asset)
{
    AsAsset = v;
    if (v)
    {
        Type.SetTypeName(v->GetType().Fullname);
        v->AddReference();
        v->OnUnloaded.Bind<Variant, &Variant::OnAssetUnloaded>(this);
    }
}

#if USE_CSHARP

Variant::Variant(MObject* v)
    : Type(VariantType::ManagedObject, v ? MCore::Object::GetClass(v) : nullptr)
{
    MANAGED_GC_HANDLE = v ? MCore::GCHandle::New(v) : 0;
}

#else

Variant::Variant(MObject* v)
    : Type(VariantType::ManagedObject, nullptr)
{
    AsUint = 0;
}

#endif

Variant::Variant(const StringView& v)
    : Type(VariantType::String)
{
    if (v.Length() > 0)
    {
        const int32 length = v.Length() * sizeof(Char) + 2;
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
        Platform::MemoryCopy(AsBlob.Data, v.Get(), length);
        ((Char*)AsBlob.Data)[v.Length()] = 0;
    }
    else
    {
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
    }
}

Variant::Variant(const StringAnsiView& v)
    : Type(VariantType::String)
{
    if (v.Length() > 0)
    {
        const int32 length = v.Length() * sizeof(Char) + 2;
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
        int32 tmp;
        StringUtils::ConvertANSI2UTF16(v.Get(), (Char*)AsBlob.Data, v.Length(), tmp);
        ((Char*)AsBlob.Data)[v.Length()] = 0;
    }
    else
    {
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
    }
}

Variant::Variant(const Char* v)
    : Variant(StringView(v))
{
}

Variant::Variant(const char* v)
    : Variant(StringAnsiView(v))
{
}

Variant::Variant(const Guid& v)
    : Type(VariantType::Guid)
{
    *(Guid*)AsData = v;
}

Variant::Variant(const Float2& v)
    : Type(VariantType::Float2)
{
    *(Float2*)AsData = v;
}

Variant::Variant(const Float3& v)
    : Type(VariantType::Float3)
{
    *(Float3*)AsData = v;
}

Variant::Variant(const Float4& v)
    : Type(VariantType::Float4)
{
    *(Float4*)AsData = v;
}

Variant::Variant(const Double2& v)
    : Type(VariantType::Double2)
{
    *(Double2*)AsData = v;
}

Variant::Variant(const Double3& v)
    : Type(VariantType::Double3)
{
    *(Double3*)AsData = v;
}

Variant::Variant(const Double4& v)
    : Type(VariantType::Double4)
{
    AsBlob.Length = sizeof(Double4);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(Double4*)AsBlob.Data = v;
}

Variant::Variant(const Int2& v)
    : Type(VariantType::Int2)
{
    *(Int2*)AsData = v;
}

Variant::Variant(const Int3& v)
    : Type(VariantType::Int3)
{
    *(Int3*)AsData = v;
}

Variant::Variant(const Int4& v)
    : Type(VariantType::Int4)
{
    *(Int4*)AsData = v;
}

Variant::Variant(const Color& v)
    : Type(VariantType::Color)
{
    *(Color*)AsData = v;
}

Variant::Variant(const Quaternion& v)
    : Type(VariantType::Quaternion)
{
    *(Quaternion*)AsData = v;
}

Variant::Variant(const BoundingSphere& v)
    : Type(VariantType::BoundingSphere)
{
#if USE_LARGE_WORLDS
    AsBlob.Length = sizeof(BoundingSphere);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(BoundingSphere*)AsBlob.Data = v;
#else
    *(BoundingSphere*)AsData = v;
#endif
}

Variant::Variant(const Rectangle& v)
    : Type(VariantType::Rectangle)
{
    *(Rectangle*)AsData = v;
}

Variant::Variant(const BoundingBox& v)
    : Type(VariantType::BoundingBox)
{
#if USE_LARGE_WORLDS
    AsBlob.Length = sizeof(BoundingBox);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(BoundingBox*)AsBlob.Data = v;
#else
    *(BoundingBox*)AsData = v;
#endif
}

Variant::Variant(const Transform& v)
    : Type(VariantType::Transform)
{
    AsBlob.Length = sizeof(Transform);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(Transform*)AsBlob.Data = v;
}

Variant::Variant(const Ray& v)
    : Type(VariantType::Ray)
{
#if USE_LARGE_WORLDS
    AsBlob.Length = sizeof(Ray);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(Ray*)AsBlob.Data = v;
#else
    *(Ray*)AsData = v;
#endif
}

Variant::Variant(const Matrix& v)
    : Type(VariantType::Matrix)
{
    AsBlob.Length = sizeof(Matrix);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(Matrix*)AsBlob.Data = v;
}

Variant::Variant(Array<Variant>&& v)
    : Type(VariantType::Array)
{
    auto* array = reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData);
    new(array)Array<Variant, HeapAllocation>(MoveTemp(v));
}

Variant::Variant(const Array<Variant, HeapAllocation>& v)
    : Type(VariantType::Array)
{
    auto* array = reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData);
    new(array)Array<Variant, HeapAllocation>(v);
}

Variant::Variant(Dictionary<Variant, Variant>&& v)
    : Type(VariantType::Dictionary)
{
    AsDictionary = New<Dictionary<Variant, Variant>>(MoveTemp(v));
}

Variant::Variant(const Dictionary<Variant, Variant>& v)
    : Type(VariantType::Dictionary)
{
    AsDictionary = New<Dictionary<Variant, Variant>>(v);
}

Variant::Variant(const Span<byte>& v)
    : Type(VariantType::Blob)
{
    AsBlob.Length = v.Length();
    if (AsBlob.Length > 0)
    {
        AsBlob.Data = Allocator::Allocate(AsBlob.Length);
        Platform::MemoryCopy(AsBlob.Data, v.Get(), AsBlob.Length);
    }
    else
    {
        AsBlob.Data = nullptr;
    }
}

Variant::Variant(const CommonValue& value)
    : Variant()
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    switch (value.Type)
    {
    case CommonType::Bool:
        *this = value.AsBool;
        break;
    case CommonType::Integer:
        *this = value.AsInteger;
        break;
    case CommonType::Float:
        *this = value.AsFloat;
        break;
    case CommonType::Vector2:
        *this = value.AsVector2;
        break;
    case CommonType::Vector3:
        *this = value.AsVector3;
        break;
    case CommonType::Vector4:
        *this = value.AsVector4;
        break;
    case CommonType::Color:
        *this = value.AsColor;
        break;
    case CommonType::Guid:
        *this = value.AsGuid;
        break;
    case CommonType::String:
        SetString(StringView(value.AsString));
        break;
    case CommonType::Box:
        *this = Variant(value.AsBox);
        break;
    case CommonType::Rotation:
        *this = value.AsRotation;
        break;
    case CommonType::Transform:
        *this = Variant(value.AsTransform);
        break;
    case CommonType::Sphere:
        *this = value.AsSphere;
        break;
    case CommonType::Rectangle:
        *this = value.AsRectangle;
        break;
    case CommonType::Pointer:
        *this = value.AsPointer;
        break;
    case CommonType::Matrix:
        *this = Variant(value.AsMatrix);
        break;
    case CommonType::Blob:
        SetBlob(value.AsBlob.Data, value.AsBlob.Length);
        break;
    case CommonType::Object:
        SetObject(value.AsObject);
        break;
    case CommonType::Ray:
        *this = Variant(value.AsRay);
        break;
    default:
        CRASH;
    }
}

Variant::~Variant()
{
    switch (Type.Type)
    {
    case VariantType::Object:
        if (AsObject)
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
        break;
    case VariantType::Asset:
        if (AsAsset)
        {
            AsAsset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
            AsAsset->RemoveReference();
        }
        break;
    case VariantType::Structure:
        FreeStructure();
        break;
    case VariantType::String:
    case VariantType::Blob:
    case VariantType::Transform:
    case VariantType::Matrix:
    case VariantType::Typename:
    case VariantType::Double4:
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
        Allocator::Free(AsBlob.Data);
        break;
    case VariantType::Array:
        reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData)->~Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        Delete(AsDictionary);
        break;
    case VariantType::ManagedObject:
        if (MANAGED_GC_HANDLE)
            MCore::GCHandle::Free(MANAGED_GC_HANDLE);
        break;
    default: ;
    }
}

Variant& Variant::operator=(Variant&& other)
{
    ASSERT(this != &other);
    SetType(VariantType());
    Type = MoveTemp(other.Type);
    switch (Type.Type)
    {
    case VariantType::String:
    case VariantType::Structure:
    case VariantType::Blob:
    case VariantType::Transform:
    case VariantType::Matrix:
    case VariantType::Typename:
    case VariantType::Double4:
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
        AsBlob.Data = other.AsBlob.Data;
        AsBlob.Length = other.AsBlob.Length;
        break;
    case VariantType::Object:
        AsObject = other.AsObject;
        if (AsObject)
        {
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(&other);
            AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
        }
        break;
    case VariantType::Asset:
        AsAsset = other.AsAsset;
        if (AsAsset)
        {
            AsAsset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(&other);
            AsAsset->OnUnloaded.Bind<Variant, &Variant::OnAssetUnloaded>(this);
        }
        break;
    case VariantType::Array:
        new(reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData))Array<Variant, HeapAllocation>(MoveTemp(*reinterpret_cast<Array<Variant, HeapAllocation>*>(other.AsData)));
        reinterpret_cast<Array<Variant, HeapAllocation>*>(other.AsData)->~Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        AsDictionary = other.AsDictionary;
        other.AsDictionary = nullptr;
        break;
    case VariantType::ManagedObject:
        MANAGED_GC_HANDLE = other.MANAGED_GC_HANDLE;
        other.MANAGED_GC_HANDLE = 0;
        break;
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::MAX:
        break;
    default:
        Platform::MemoryCopy(AsData, other.AsData, sizeof(AsData));
        break;
    }
    return *this;
}

Variant& Variant::operator=(const Variant& other)
{
    ASSERT(this != &other);
    SetType(other.Type);
    switch (Type.Type)
    {
    case VariantType::Structure:
        CopyStructure(other.AsBlob.Data);
        break;
    case VariantType::String:
    case VariantType::Blob:
    case VariantType::Transform:
    case VariantType::Matrix:
    case VariantType::Typename:
    case VariantType::Double4:
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
        if (other.AsBlob.Data)
        {
            if (!AsBlob.Data || AsBlob.Length != other.AsBlob.Length)
            {
                Allocator::Free(AsBlob.Data);
                AsBlob.Data = Allocator::Allocate(other.AsBlob.Length);
            }
            Platform::MemoryCopy(AsBlob.Data, other.AsBlob.Data, other.AsBlob.Length);
        }
        else if (AsBlob.Data)
        {
            Allocator::Free(AsBlob.Data);
            AsBlob.Data = nullptr;
        }
        AsBlob.Length = other.AsBlob.Length;
        break;
    case VariantType::Object:
        AsObject = other.AsObject;
        if (other.AsObject)
            AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
        break;
    case VariantType::Asset:
        AsAsset = other.AsAsset;
        if (other.AsAsset)
        {
            AsAsset->AddReference();
            AsAsset->OnUnloaded.Bind<Variant, &Variant::OnAssetUnloaded>(this);
        }
        break;
    case VariantType::Array:
        *reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData) = *reinterpret_cast<const Array<Variant, HeapAllocation>*>(other.AsData);
        break;
    case VariantType::Dictionary:
        if (AsDictionary)
            *AsDictionary = *other.AsDictionary;
        else if (other.AsDictionary)
            AsDictionary = New<Dictionary<Variant, Variant>>(*other.AsDictionary);
        break;
    case VariantType::ManagedObject:
        MANAGED_GC_HANDLE = other.MANAGED_GC_HANDLE ? MCore::GCHandle::New(MCore::GCHandle::GetTarget(other.MANAGED_GC_HANDLE)) : 0;
        break;
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::MAX:
        break;
    default:
        Platform::MemoryCopy(AsData, other.AsData, sizeof(AsData));
        break;
    }
    return *this;
}

bool Variant::operator==(const Variant& other) const
{
    if (Type == other.Type)
    {
        switch (Type.Type)
        {
        case VariantType::Null:
            return true;
        case VariantType::Bool:
            return AsBool == other.AsBool;
        case VariantType::Int16:
            return AsInt16 == other.AsInt16;
        case VariantType::Uint16:
            return AsUint16 == other.AsUint16;
        case VariantType::Int:
            return AsInt == other.AsInt;
        case VariantType::Uint:
            return AsUint == other.AsUint;
        case VariantType::Int64:
            return AsInt64 == other.AsInt64;
        case VariantType::Uint64:
        case VariantType::Enum:
            return AsUint64 == other.AsUint64;
        case VariantType::Float:
            return Math::NearEqual(AsFloat, other.AsFloat);
        case VariantType::Double:
            return Math::Abs(AsDouble - other.AsDouble) < ZeroTolerance;
        case VariantType::Pointer:
            return AsPointer == other.AsPointer;
        case VariantType::String:
            if (AsBlob.Data == nullptr && other.AsBlob.Data == nullptr)
                return true;
            if (AsBlob.Data == nullptr)
                return false;
            if (other.AsBlob.Data == nullptr)
                return false;
            return AsBlob.Length == other.AsBlob.Length && StringUtils::Compare(static_cast<const Char*>(AsBlob.Data), static_cast<const Char*>(other.AsBlob.Data), AsBlob.Length - 1) == 0;
        case VariantType::Object:
            return AsObject == other.AsObject;
        case VariantType::Structure:
        case VariantType::Blob:
        case VariantType::Transform:
        case VariantType::Matrix:
        case VariantType::Double4:
            return AsBlob.Length == other.AsBlob.Length && Platform::MemoryCompare(AsBlob.Data, other.AsBlob.Data, AsBlob.Length) == 0;
        case VariantType::Asset:
            return AsAsset == other.AsAsset;
        case VariantType::Float2:
            return *(Float2*)AsData == *(Float2*)other.AsData;
        case VariantType::Float3:
            return *(Float3*)AsData == *(Float3*)other.AsData;
        case VariantType::Float4:
            return *(Float4*)AsData == *(Float4*)other.AsData;
        case VariantType::Int2:
            return *(Int2*)AsData == *(Int2*)other.AsData;
        case VariantType::Int3:
            return *(Int3*)AsData == *(Int3*)other.AsData;
        case VariantType::Int4:
            return *(Int4*)AsData == *(Int4*)other.AsData;
        case VariantType::Double2:
            return *(Double2*)AsData == *(Double2*)other.AsData;
        case VariantType::Double3:
            return *(Double3*)AsData == *(Double3*)other.AsData;
        case VariantType::Color:
            return *(Color*)AsData == *(Color*)other.AsData;
        case VariantType::Quaternion:
            return *(Quaternion*)AsData == *(Quaternion*)other.AsData;
        case VariantType::Rectangle:
            return *(Rectangle*)AsData == *(Rectangle*)other.AsData;
        case VariantType::BoundingSphere:
            return AsBoundingSphere() == other.AsBoundingSphere();
        case VariantType::BoundingBox:
            return AsBoundingBox() == other.AsBoundingBox();
        case VariantType::Ray:
            return AsRay() == other.AsRay();
        case VariantType::Guid:
            return *(Guid*)AsData == *(Guid*)other.AsData;
        case VariantType::Array:
        {
            const auto* array = reinterpret_cast<const Array<Variant, HeapAllocation>*>(AsData);
            const Array<Variant, HeapAllocation>* otherArray = reinterpret_cast<const Array<Variant, HeapAllocation>*>(other.AsData);
            if (array->Count() != otherArray->Count())
                return false;
            for (int32 i = 0; i < array->Count(); i++)
            {
                if (array->At(i) != otherArray->At(i))
                    return false;
            }
            return true;
        }
        case VariantType::Dictionary:
            if (AsDictionary == nullptr && other.AsDictionary == nullptr)
                return true;
            if (!AsDictionary || !other.AsDictionary)
                return false;
            if (AsDictionary->Count() != other.AsDictionary->Count())
                return false;
            for (auto& i : *AsDictionary)
            {
                if (!other.AsDictionary->ContainsKey(i.Key) || other.AsDictionary->At(i.Key) != i.Value)
                    return false;
            }
            return true;
        case VariantType::Typename:
            if (AsBlob.Data == nullptr && other.AsBlob.Data == nullptr)
                return true;
            if (AsBlob.Data == nullptr)
                return false;
            if (other.AsBlob.Data == nullptr)
                return false;
            return AsBlob.Length == other.AsBlob.Length && StringUtils::Compare(static_cast<const char*>(AsBlob.Data), static_cast<const char*>(other.AsBlob.Data), AsBlob.Length - 1) == 0;
        case VariantType::ManagedObject:
#if USE_CSHARP
            // TODO: invoke C# Equality logic?
            return MANAGED_GC_HANDLE == other.MANAGED_GC_HANDLE || MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE) == MCore::GCHandle::GetTarget(other.MANAGED_GC_HANDLE);
#else
            return false;
#endif
        default:
            return false;
        }
    }
    if (CanCast(*this, other.Type))
    {
        return Cast(*this, other.Type) == other;
    }
    return false;
}

bool Variant::operator<(const Variant& other) const
{
    if (Type == other.Type)
    {
        switch (Type.Type)
        {
        case VariantType::Null:
        case VariantType::Void:
            return true;
        case VariantType::Bool:
            return AsBool < other.AsBool;
        case VariantType::Int16:
            return AsInt16 < other.AsInt16;
        case VariantType::Uint16:
            return AsUint16 < other.AsUint16;
        case VariantType::Int:
            return AsInt < other.AsInt;
        case VariantType::Uint:
            return AsUint < other.AsUint;
        case VariantType::Int64:
            return AsInt64 < other.AsInt64;
        case VariantType::Uint64:
        case VariantType::Enum:
            return AsUint64 < other.AsUint64;
        case VariantType::Float:
            return AsFloat < other.AsFloat;
        case VariantType::Double:
            return AsDouble < other.AsDouble;
        case VariantType::Pointer:
            return AsPointer < other.AsPointer;
        case VariantType::String:
            return StringUtils::Compare(AsBlob.Data ? (const Char*)AsBlob.Data : TEXT(""), other.AsBlob.Data ? (const Char*)other.AsBlob.Data : TEXT("")) < 0;
        case VariantType::Typename:
            return StringUtils::Compare(AsBlob.Data ? (const char*)AsBlob.Data : "", other.AsBlob.Data ? (const char*)other.AsBlob.Data : "") < 0;
        default:
            return false;
        }
    }
    if (CanCast(*this, other.Type))
    {
        return Cast(*this, other.Type) < other;
    }
    return false;
}

Variant::operator bool() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool;
    case VariantType::Int16:
        return AsInt16 != 0;
    case VariantType::Uint16:
        return AsUint16 != 0;
    case VariantType::Int:
        return AsInt != 0;
    case VariantType::Uint:
        return AsUint != 0;
    case VariantType::Int64:
        return AsInt64 != 0;
    case VariantType::Uint64:
    case VariantType::Enum:
        return AsUint64 != 0;
    case VariantType::Float:
        return !Math::IsZero(AsFloat);
    case VariantType::Double:
        return !Math::IsZero(AsDouble);
    case VariantType::Pointer:
        return AsPointer != nullptr;
    case VariantType::String:
    case VariantType::Typename:
        return AsBlob.Length > 1;
    case VariantType::Object:
        return AsObject != nullptr;
    case VariantType::Asset:
        return AsAsset != nullptr;
    case VariantType::ManagedObject:
#if USE_CSHARP
        return MANAGED_GC_HANDLE != 0 && MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE) != nullptr;
#else
        return false;
#endif
    default:
        return false;
    }
}

Variant::operator Char() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (Char)AsInt16;
    case VariantType::Uint16:
        return (Char)AsUint16;
    case VariantType::Int:
        return (Char)AsInt;
    case VariantType::Uint:
        return (Char)AsUint;
    case VariantType::Int64:
        return (Char)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (Char)AsUint64;
    case VariantType::Float:
        return (Char)AsFloat;
    case VariantType::Double:
        return (Char)AsDouble;
    case VariantType::Pointer:
        return (Char)(intptr)AsPointer;
    default:
        return 0;
    }
}

Variant::operator int8() const
{
    return (int8)operator int64();
}

Variant::operator int16() const
{
    return (int16)operator int64();
}

Variant::operator int32() const
{
    return (int32)operator int64();
}

Variant::operator int64() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (int64)AsInt16;
    case VariantType::Uint16:
        return (int64)AsUint16;
    case VariantType::Int:
        return (int64)AsInt;
    case VariantType::Uint:
        return (int64)AsUint;
    case VariantType::Int64:
        return AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (int64)AsUint64;
    case VariantType::Float:
        return (int64)AsFloat;
    case VariantType::Double:
        return (int64)AsDouble;
    case VariantType::Pointer:
        return (int64)AsPointer;
    case VariantType::Float2:
        return (int64)AsFloat2().X;
    case VariantType::Float3:
        return (int64)AsFloat3().X;
    case VariantType::Float4:
        return (int64)AsFloat4().X;
    case VariantType::Double2:
        return (int64)AsDouble2().X;
    case VariantType::Double3:
        return (int64)AsDouble3().X;
    case VariantType::Double4:
        return (int64)AsDouble4().X;
    case VariantType::Int2:
        return (int64)AsInt2().X;
    case VariantType::Int3:
        return (int64)AsInt3().X;
    case VariantType::Int4:
        return (int64)AsInt4().X;
    default:
        return 0;
    }
}

Variant::operator uint8() const
{
    return (uint8)operator uint64();
}

Variant::operator uint16() const
{
    return (uint16)operator uint64();
}

Variant::operator uint32() const
{
    return (uint32)operator uint64();
}

Variant::operator uint64() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (uint64)AsInt16;
    case VariantType::Uint16:
        return (uint64)AsUint16;
    case VariantType::Int:
        return (uint64)AsInt;
    case VariantType::Uint:
        return (uint64)AsUint;
    case VariantType::Int64:
        return (uint64)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return AsUint64;
    case VariantType::Float:
        return (uint64)AsFloat;
    case VariantType::Double:
        return (uint64)AsDouble;
    case VariantType::Pointer:
        return (uint64)AsPointer;
    case VariantType::Float2:
        return (uint64)AsFloat2().X;
    case VariantType::Float3:
        return (uint64)AsFloat3().X;
    case VariantType::Float4:
        return (uint64)AsFloat4().X;
    case VariantType::Double2:
        return (uint64)AsDouble2().X;
    case VariantType::Double3:
        return (uint64)AsDouble3().X;
    case VariantType::Double4:
        return (uint64)AsDouble4().X;
    case VariantType::Int2:
        return (uint64)AsInt2().X;
    case VariantType::Int3:
        return (uint64)AsInt3().X;
    case VariantType::Int4:
        return (uint64)AsInt4().X;
    default:
        return 0;
    }
}

Variant::operator float() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1.0f : 0.0f;
    case VariantType::Int16:
        return (float)AsInt16;
    case VariantType::Uint16:
        return (float)AsUint16;
    case VariantType::Int:
        return (float)AsInt;
    case VariantType::Uint:
        return (float)AsUint;
    case VariantType::Int64:
        return (float)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (float)AsUint64;
    case VariantType::Float:
        return AsFloat;
    case VariantType::Double:
        return (float)AsDouble;
    case VariantType::Float2:
        return AsFloat2().X;
    case VariantType::Float3:
        return AsFloat3().X;
    case VariantType::Float4:
    case VariantType::Color:
        return AsFloat4().X;
    case VariantType::Double2:
        return (float)AsDouble2().X;
    case VariantType::Double3:
        return (float)AsDouble3().X;
    case VariantType::Double4:
        return (float)AsDouble4().X;
    case VariantType::Int2:
        return (float)AsInt2().X;
    case VariantType::Int3:
        return (float)AsInt3().X;
    case VariantType::Int4:
        return (float)AsInt4().X;
    case VariantType::Pointer:
        return AsPointer ? 1.0f : 0.0f;
    case VariantType::Object:
        return AsObject ? 1.0f : 0.0f;
    case VariantType::Asset:
        return AsAsset ? 1.0f : 0.0f;
    case VariantType::Blob:
        return AsBlob.Length > 0 ? 1.0f : 0.0f;
    case VariantType::ManagedObject:
        return MANAGED_GC_HANDLE ? 1.0f : 0.0f;
    default:
        return 0;
    }
}

Variant::operator double() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1.0 : 0.0;
    case VariantType::Int16:
        return (double)AsInt16;
    case VariantType::Uint16:
        return (double)AsUint16;
    case VariantType::Int:
        return (double)AsInt;
    case VariantType::Uint:
        return (double)AsUint;
    case VariantType::Int64:
        return (double)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (double)AsUint64;
    case VariantType::Float:
        return (double)AsFloat;
    case VariantType::Double:
        return AsDouble;
    case VariantType::Float2:
        return (double)AsFloat2().X;
    case VariantType::Float3:
        return (double)AsFloat3().X;
    case VariantType::Float4:
        return (double)AsFloat4().X;
    case VariantType::Double2:
        return (double)AsDouble2().X;
    case VariantType::Double3:
        return (double)AsDouble3().X;
    case VariantType::Double4:
        return (double)AsDouble4().X;
    case VariantType::Int2:
        return (double)AsInt2().X;
    case VariantType::Int3:
        return (double)AsInt3().X;
    case VariantType::Int4:
        return (double)AsInt4().X;
    default:
        return 0;
    }
}

Variant::operator void*() const
{
    switch (Type.Type)
    {
    case VariantType::Pointer:
        return AsPointer;
    case VariantType::Object:
        return AsObject;
    case VariantType::Asset:
        return AsAsset;
    case VariantType::Structure:
    case VariantType::Blob:
        return AsBlob.Data;
    case VariantType::ManagedObject:
#if USE_CSHARP
        return MANAGED_GC_HANDLE ? MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE) : nullptr;
#else
        return nullptr;
#endif
    default:
        return nullptr;
    }
}

Variant::operator StringView() const
{
    switch (Type.Type)
    {
    case VariantType::String:
        return StringView((const Char*)AsBlob.Data, AsBlob.Length != 0 ? AsBlob.Length / sizeof(Char) - 1 : 0);
    default:
        return StringView::Empty;
    }
}

Variant::operator StringAnsiView() const
{
    switch (Type.Type)
    {
    case VariantType::Typename:
        return StringAnsiView((const char*)AsBlob.Data, AsBlob.Length != 0 ? AsBlob.Length - 1 : 0);
    default:
        return StringAnsiView::Empty;
    }
}

Variant::operator ScriptingObject*() const
{
    switch (Type.Type)
    {
    case VariantType::Object:
        return AsObject;
    case VariantType::Asset:
        return AsAsset;
    default:
        return nullptr;
    }
}

Variant::operator MObject*() const
{
#if USE_CSHARP
    return Type.Type == VariantType::ManagedObject && MANAGED_GC_HANDLE ? MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE) : nullptr;
#else
    return nullptr;
#endif
}

Variant::operator Asset*() const
{
    switch (Type.Type)
    {
    case VariantType::Asset:
        return AsAsset;
    default:
        return nullptr;
    }
}

Variant::operator Float2() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Float2(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Float2((float)AsInt16);
    case VariantType::Uint16:
        return Float2((float)AsUint16);
    case VariantType::Int:
        return Float2((float)AsInt);
    case VariantType::Uint:
        return Float2((float)AsUint);
    case VariantType::Int64:
        return Float2((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Float2((float)AsUint64);
    case VariantType::Float:
        return Float2(AsFloat);
    case VariantType::Double:
        return Float2((float)AsDouble);
    case VariantType::Pointer:
        return Float2((float)(intptr)AsPointer);
    case VariantType::Float2:
        return *(Float2*)AsData;
    case VariantType::Float3:
        return Float2(*(Float3*)AsData);
    case VariantType::Float4:
    case VariantType::Color:
        return Float2(*(Float4*)AsData);
    case VariantType::Double2:
        return Float2(AsDouble2());
    case VariantType::Double3:
        return Float2(AsDouble3());
    case VariantType::Double4:
        return Float2(AsDouble4());
    case VariantType::Int2:
        return Float2(AsInt2());
    case VariantType::Int3:
        return Float2(AsInt3());
    case VariantType::Int4:
        return Float2(AsInt4());
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Float2::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Float2*)AsBlob.Data;
    default:
        return Float2::Zero;
    }
}

Variant::operator Float3() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Float3(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Float3((float)AsInt16);
    case VariantType::Uint16:
        return Float3((float)AsUint16);
    case VariantType::Int:
        return Float3((float)AsInt);
    case VariantType::Uint:
        return Float3((float)AsUint);
    case VariantType::Int64:
        return Float3((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Float3((float)AsUint64);
    case VariantType::Float:
        return Float3(AsFloat);
    case VariantType::Double:
        return Float3((float)AsDouble);
    case VariantType::Pointer:
        return Float3((float)(intptr)AsPointer);
    case VariantType::Float2:
        return Float3(*(Float2*)AsData, 0.0f);
    case VariantType::Float3:
        return *(Float3*)AsData;
    case VariantType::Float4:
    case VariantType::Color:
        return Float3(*(Float4*)AsData);
    case VariantType::Double2:
        return Float3(AsDouble2());
    case VariantType::Double3:
        return Float3(AsDouble3());
    case VariantType::Double4:
        return Float3(AsDouble4());
    case VariantType::Int2:
        return Float3(AsInt2(), 0.0f);
    case VariantType::Int3:
        return Float3(AsInt3());
    case VariantType::Int4:
        return Float3(AsInt4());
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Float3::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Float3*)AsBlob.Data;
    default:
        return Float3::Zero;
    }
}

Variant::operator Float4() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Float4(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Float4((float)AsInt16);
    case VariantType::Uint16:
        return Float4((float)AsUint16);
    case VariantType::Int:
        return Float4((float)AsInt);
    case VariantType::Uint:
        return Float4((float)AsUint);
    case VariantType::Int64:
        return Float4((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Float4((float)AsUint64);
    case VariantType::Float:
        return Float4(AsFloat);
    case VariantType::Double:
        return Float4((float)AsDouble);
    case VariantType::Pointer:
        return Float4((float)(intptr)AsPointer);
    case VariantType::Float2:
        return Float4(*(Float2*)AsData, 0.0f, 0.0f);
    case VariantType::Float3:
        return Float4(*(Float3*)AsData, 0.0f);
    case VariantType::Float4:
    case VariantType::Color:
    case VariantType::Quaternion:
        return *(Float4*)AsData;
    case VariantType::Double2:
        return Float4(AsDouble2(), 0.0f, 0.0f);
    case VariantType::Double3:
        return Float4(AsDouble3(), 0.0f);
    case VariantType::Double4:
        return Float4(AsDouble4());
    case VariantType::Int2:
        return Float4(AsInt2(), 0.0f, 0.0f);
    case VariantType::Int3:
        return Float4(AsInt3(), 0.0f);
    case VariantType::Int4:
        return Float4(AsInt4());
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Float4::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Float4*)AsBlob.Data;
    default:
        return Float4::Zero;
    }
}

Variant::operator Double2() const
{
    return Double2(operator Double3());
}

Variant::operator Double3() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Double3(AsBool ? 1.0 : 0.0);
    case VariantType::Int16:
        return Double3((double)AsInt16);
    case VariantType::Uint16:
        return Double3((double)AsUint16);
    case VariantType::Int:
        return Double3((double)AsInt);
    case VariantType::Uint:
        return Double3((double)AsUint);
    case VariantType::Int64:
        return Double3((double)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Double3((double)AsUint64);
    case VariantType::Float:
        return Double3(AsFloat);
    case VariantType::Double:
        return Double3(AsDouble);
    case VariantType::Pointer:
        return Double3((double)(intptr)AsPointer);
    case VariantType::Float2:
        return Double3(*(Float2*)AsData, 0.0);
    case VariantType::Float3:
        return Double3(*(Float3*)AsData);
    case VariantType::Float4:
    case VariantType::Color:
        return Double3(*(Float4*)AsData);
    case VariantType::Double2:
        return Double3(AsDouble2());
    case VariantType::Double3:
        return AsDouble3();
    case VariantType::Double4:
        return Double3(AsDouble4());
    case VariantType::Int2:
        return Double3(AsInt2(), 0.0);
    case VariantType::Int3:
        return Double3(AsInt3());
    case VariantType::Int4:
        return Double3(AsInt4());
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Double3::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Double3*)AsBlob.Data;
    default:
        return Double3::Zero;
    }
}

Variant::operator Double4() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Double4(AsBool ? 1.0 : 0.0);
    case VariantType::Int16:
        return Double4((double)AsInt16);
    case VariantType::Uint16:
        return Double4((double)AsUint16);
    case VariantType::Int:
        return Double4((double)AsInt);
    case VariantType::Uint:
        return Double4((double)AsUint);
    case VariantType::Int64:
        return Double4((double)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Double4((double)AsUint64);
    case VariantType::Float:
        return Double4(AsFloat);
    case VariantType::Double:
        return Double4(AsDouble);
    case VariantType::Pointer:
        return Double4((double)(intptr)AsPointer);
    case VariantType::Float2:
        return Double4(*(Float2*)AsData, 0.0, 0.0);
    case VariantType::Float3:
        return Double4(*(Float3*)AsData, 0.0);
    case VariantType::Float4:
    case VariantType::Color:
        return Double4(*(const Float4*)AsData);
    case VariantType::Double2:
        return Double4(AsDouble2(), 0.0, 0.0);
    case VariantType::Double3:
        return Double4(AsDouble3(), 0.0);
    case VariantType::Double4:
        return AsDouble4();
    case VariantType::Int2:
        return Double4(AsInt2(), 0.0, 0.0);
    case VariantType::Int3:
        return Double4(AsInt3(), 0.0);
    case VariantType::Int4:
        return Double4(AsInt4());
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Double4::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Double4*)AsBlob.Data;
    default:
        return Double4::Zero;
    }
}

Variant::operator Int2() const
{
    return Int2(operator Int4());
}

Variant::operator Int3() const
{
    return Int3(operator Int4());
}

Variant::operator Int4() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Int4((int32)(AsBool ? 1 : 0));
    case VariantType::Int16:
        return Int4(AsInt16);
    case VariantType::Uint16:
        return Int4((int32)AsUint16);
    case VariantType::Int:
        return Int4(AsInt);
    case VariantType::Uint:
        return Int4((int32)AsUint);
    case VariantType::Int64:
        return Int4((int32)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Int4((int32)AsUint64);
    case VariantType::Float:
        return Int4((int32)AsFloat);
    case VariantType::Double:
        return Int4((int32)AsDouble);
    case VariantType::Pointer:
        return Int4((int32)(intptr)AsPointer);
    case VariantType::Float2:
        return Int4(*(Float2*)AsData, 0, 0);
    case VariantType::Float3:
        return Int4(*(Float3*)AsData, 0);
    case VariantType::Float4:
    case VariantType::Color:
        return Int4(*(Float4*)AsData);
    case VariantType::Int2:
        return Int4(*(Int2*)AsData, 0, 0);
    case VariantType::Int3:
        return Int4(*(Int3*)AsData, 0);
    case VariantType::Int4:
        return *(Int4*)AsData;
    case VariantType::Double2:
        return Int4(AsDouble2(), 0, 0);
    case VariantType::Double3:
        return Int4(AsDouble3(), 0);
    case VariantType::Double4:
        return Int4(AsDouble4());
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Int4::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Int4*)AsBlob.Data;
    default:
        return Int4::Zero;
    }
}

Variant::operator Color() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Color(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Color((float)AsInt16);
    case VariantType::Uint16:
        return Color((float)AsUint16);
    case VariantType::Int:
        return Color((float)AsInt);
    case VariantType::Uint:
        return Color((float)AsUint);
    case VariantType::Int64:
        return Color((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Color((float)AsUint64);
    case VariantType::Float:
        return Color(AsFloat);
    case VariantType::Double:
        return Color((float)AsDouble);
    case VariantType::Pointer:
        return Color((float)(intptr)AsPointer);
    case VariantType::Float2:
        return Color((*(Float2*)AsData).X, (*(Float2*)AsData).Y, 0.0f, 1.0f);
    case VariantType::Float3:
        return Color(*(Float3*)AsData, 1.0f);
    case VariantType::Float4:
    case VariantType::Color:
        return *(Color*)AsData;
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Color::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Color*)AsBlob.Data;
    default:
        return Color::Black;
    }
}

Variant::operator Quaternion() const
{
    switch (Type.Type)
    {
    case VariantType::Float3:
        return Quaternion::Euler(*(Float3*)AsData);
    case VariantType::Double3:
        return Quaternion::Euler((Float3)*(Double3*)AsData);
    case VariantType::Quaternion:
        return *(Quaternion*)AsData;
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Quaternion::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Quaternion*)AsBlob.Data;
    default:
        return Quaternion::Identity;
    }
}

Variant::operator Guid() const
{
    switch (Type.Type)
    {
    case VariantType::Guid:
        return *(Guid*)AsData;
    case VariantType::Object:
        return AsObject ? AsObject->GetID() : Guid::Empty;
    case VariantType::Asset:
        return AsAsset ? AsAsset->GetID() : Guid::Empty;
    default:
        return Guid::Empty;
    }
}

Variant::operator BoundingSphere() const
{
    switch (Type.Type)
    {
    case VariantType::BoundingSphere:
        return AsBoundingSphere();
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, BoundingSphere::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(BoundingSphere*)AsBlob.Data;
    default:
        return BoundingSphere::Empty;
    }
}

Variant::operator BoundingBox() const
{
    switch (Type.Type)
    {
    case VariantType::BoundingBox:
        return AsBoundingBox();
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, BoundingBox::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(BoundingBox*)AsBlob.Data;
    default:
        return BoundingBox::Empty;
    }
}

Variant::operator Transform() const
{
    switch (Type.Type)
    {
    case VariantType::Transform:
        return *(Transform*)AsBlob.Data;
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Transform::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Transform*)AsBlob.Data;
    default:
        return Transform::Identity;
    }
}

Variant::operator Matrix() const
{
    switch (Type.Type)
    {
    case VariantType::Matrix:
        return *(Matrix*)AsBlob.Data;
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Matrix::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Matrix*)AsBlob.Data;
    default:
        return Matrix::Identity;
    }
}

Variant::operator Ray() const
{
    switch (Type.Type)
    {
    case VariantType::Ray:
        return AsRay();
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Ray::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Ray*)AsBlob.Data;
    default:
        return Ray::Identity;
    }
}

Variant::operator Rectangle() const
{
    switch (Type.Type)
    {
    case VariantType::Rectangle:
        return *(Rectangle*)AsData;
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Rectangle::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Rectangle*)AsBlob.Data;
    default:
        return Rectangle::Empty;
    }
}

const Vector2& Variant::AsVector2() const
{
    return *(const Vector2*)AsData;
}

const Vector3& Variant::AsVector3() const
{
    return *(const Vector3*)AsData;
}

const Vector4& Variant::AsVector4() const
{
#if USE_LARGE_WORLDS
    return *(const Vector4*)AsBlob.Data;
#else
    return *(const Vector4*)AsData;
#endif
}

const Float2& Variant::AsFloat2() const
{
    return *(const Float2*)AsData;
}

Float3& Variant::AsFloat3()
{
    return *(Float3*)AsData;
}

const Float3& Variant::AsFloat3() const
{
    return *(const Float3*)AsData;
}

const Float4& Variant::AsFloat4() const
{
    return *(const Float4*)AsData;
}

const Double2& Variant::AsDouble2() const
{
    return *(const Double2*)AsData;
}

const Double3& Variant::AsDouble3() const
{
    return *(const Double3*)AsData;
}

const Double4& Variant::AsDouble4() const
{
    return *(const Double4*)AsBlob.Data;
}

const Int2& Variant::AsInt2() const
{
    return *(const Int2*)AsData;
}

const Int3& Variant::AsInt3() const
{
    return *(const Int3*)AsData;
}

const Int4& Variant::AsInt4() const
{
    return *(const Int4*)AsData;
}

const Color& Variant::AsColor() const
{
    return *(const Color*)AsData;
}

const Quaternion& Variant::AsQuaternion() const
{
    return *(const Quaternion*)AsData;
}

const Rectangle& Variant::AsRectangle() const
{
    return *(const Rectangle*)AsData;
}

const Guid& Variant::AsGuid() const
{
    return *(const Guid*)AsData;
}

BoundingSphere& Variant::AsBoundingSphere()
{
#if USE_LARGE_WORLDS
    return *(BoundingSphere*)AsBlob.Data;
#else
    return *(BoundingSphere*)AsData;
#endif
}

const BoundingSphere& Variant::AsBoundingSphere() const
{
#if USE_LARGE_WORLDS
    return *(const BoundingSphere*)AsBlob.Data;
#else
    return *(const BoundingSphere*)AsData;
#endif
}

BoundingBox& Variant::AsBoundingBox()
{
#if USE_LARGE_WORLDS
    return *(BoundingBox*)AsBlob.Data;
#else
    return *(BoundingBox*)AsData;
#endif
}

const BoundingBox& Variant::AsBoundingBox() const
{
#if USE_LARGE_WORLDS
    return *(const BoundingBox*)AsBlob.Data;
#else
    return *(const BoundingBox*)AsData;
#endif
}

Ray& Variant::AsRay()
{
#if USE_LARGE_WORLDS
    return *(Ray*)AsBlob.Data;
#else
    return *(Ray*)AsData;
#endif
}

const Ray& Variant::AsRay() const
{
#if USE_LARGE_WORLDS
    return *(const Ray*)AsBlob.Data;
#else
    return *(const Ray*)AsData;
#endif
}

Transform& Variant::AsTransform()
{
    return *(Transform*)AsBlob.Data;
}

const Transform& Variant::AsTransform() const
{
    return *(const Transform*)AsBlob.Data;
}

const Matrix& Variant::AsMatrix() const
{
    return *(const Matrix*)AsBlob.Data;
}

Array<Variant>& Variant::AsArray()
{
    return *reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData);
}

const Array<Variant>& Variant::AsArray() const
{
    return *reinterpret_cast<const Array<Variant, HeapAllocation>*>(AsData);
}

void Variant::SetType(const VariantType& type)
{
    if (Type == type)
        return;

    switch (Type.Type)
    {
    case VariantType::Object:
        if (AsObject)
        {
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
        }
        break;
    case VariantType::Asset:
        if (AsAsset)
        {
            AsAsset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
            AsAsset->RemoveReference();
        }
        break;
    case VariantType::Structure:
        FreeStructure();
        break;
    case VariantType::String:
    case VariantType::Blob:
    case VariantType::Transform:
    case VariantType::Matrix:
    case VariantType::Typename:
    case VariantType::Double4:
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
        Allocator::Free(AsBlob.Data);
        break;
    case VariantType::Array:
        reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData)->~Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        if (AsDictionary)
            Delete(AsDictionary);
        break;
    case VariantType::ManagedObject:
#if USE_CSHARP
        if (MANAGED_GC_HANDLE)
            MCore::GCHandle::Free(MANAGED_GC_HANDLE);
#endif
        break;
    default: ;
    }

    Type = type;

    switch (Type.Type)
    {
    case VariantType::String:
    case VariantType::Blob:
    case VariantType::Typename:
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
        break;
    case VariantType::Object:
        AsObject = nullptr;
        break;
    case VariantType::Asset:
        AsAsset = nullptr;
        break;
    case VariantType::Double4:
        AsBlob.Data = Allocator::Allocate(sizeof(Double4));
        AsBlob.Length = sizeof(Double4);
        break;
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
        AsBlob.Data = Allocator::Allocate(sizeof(BoundingSphere));
        AsBlob.Length = sizeof(BoundingSphere);
        break;
    case VariantType::BoundingBox:
        AsBlob.Data = Allocator::Allocate(sizeof(BoundingBox));
        AsBlob.Length = sizeof(BoundingBox);
        break;
    case VariantType::Ray:
        AsBlob.Data = Allocator::Allocate(sizeof(Ray));
        AsBlob.Length = sizeof(Ray);
        break;
#endif
    case VariantType::Transform:
        AsBlob.Data = Allocator::Allocate(sizeof(Transform));
        AsBlob.Length = sizeof(Transform);
        break;
    case VariantType::Matrix:
        AsBlob.Data = Allocator::Allocate(sizeof(Matrix));
        AsBlob.Length = sizeof(Matrix);
        break;
    case VariantType::Array:
        new(reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData))Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        AsDictionary = New<Dictionary<Variant, Variant>>();
        break;
    case VariantType::ManagedObject:
#if USE_CSHARP
        MANAGED_GC_HANDLE = 0;
#endif
        break;
    case VariantType::Structure:
        AllocStructure();
        break;
    default:
        AsUint64 = 0;
        break;
    }
}

void Variant::SetType(VariantType&& type)
{
    if (Type == type)
        return;

    switch (Type.Type)
    {
    case VariantType::Object:
        if (AsObject)
        {
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
        }
        break;
    case VariantType::Asset:
        if (AsAsset)
        {
            AsAsset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
            AsAsset->RemoveReference();
        }
        break;
    case VariantType::Structure:
        FreeStructure();
        break;
    case VariantType::String:
    case VariantType::Blob:
    case VariantType::Transform:
    case VariantType::Matrix:
    case VariantType::Typename:
    case VariantType::Double4:
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
        Allocator::Free(AsBlob.Data);
        break;
    case VariantType::Array:
        reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData)->~Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        if (AsDictionary)
            Delete(AsDictionary);
        break;
    case VariantType::ManagedObject:
#if USE_CSHARP
        if (MANAGED_GC_HANDLE)
            MCore::GCHandle::Free(MANAGED_GC_HANDLE);
#endif
        break;
    default: ;
    }

    Type = MoveTemp(type);

    switch (Type.Type)
    {
    case VariantType::String:
    case VariantType::Blob:
    case VariantType::Typename:
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
        break;
    case VariantType::Object:
        AsObject = nullptr;
        break;
    case VariantType::Asset:
        AsAsset = nullptr;
        break;
    case VariantType::Double4:
        AsBlob.Data = Allocator::Allocate(sizeof(Double4));
        AsBlob.Length = sizeof(Double4);
        break;
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
        AsBlob.Data = Allocator::Allocate(sizeof(BoundingSphere));
        AsBlob.Length = sizeof(BoundingSphere);
        break;
    case VariantType::BoundingBox:
        AsBlob.Data = Allocator::Allocate(sizeof(BoundingBox));
        AsBlob.Length = sizeof(BoundingBox);
        break;
    case VariantType::Ray:
        AsBlob.Data = Allocator::Allocate(sizeof(Ray));
        AsBlob.Length = sizeof(Ray);
        break;
#endif
    case VariantType::Transform:
        AsBlob.Data = Allocator::Allocate(sizeof(Transform));
        AsBlob.Length = sizeof(Transform);
        break;
    case VariantType::Matrix:
        AsBlob.Data = Allocator::Allocate(sizeof(Matrix));
        AsBlob.Length = sizeof(Matrix);
        break;
    case VariantType::Array:
        new(reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData))Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        AsDictionary = New<Dictionary<Variant, Variant>>();
        break;
    case VariantType::ManagedObject:
#if USE_CSHARP
        MANAGED_GC_HANDLE = 0;
#endif
        break;
    case VariantType::Structure:
        AllocStructure();
        break;
    default: ;
    }
}

void Variant::SetString(const StringView& str)
{
    SetType(VariantType(VariantType::String));
    if (str.Length() <= 0)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
        return;
    }
    const int32 length = str.Length() * sizeof(Char) + 2;
    if (AsBlob.Length != length)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
    }
    Platform::MemoryCopy(AsBlob.Data, str.Get(), length);
    ((Char*)AsBlob.Data)[str.Length()] = 0;
}

void Variant::SetString(const StringAnsiView& str)
{
    SetType(VariantType(VariantType::String));
    if (str.Length() <= 0)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
        return;
    }
    const int32 length = str.Length() * sizeof(Char) + 2;
    if (AsBlob.Length != length)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
    }
    int32 tmp;
    StringUtils::ConvertANSI2UTF16(str.Get(), (Char*)AsBlob.Data, str.Length(), tmp);
    ((Char*)AsBlob.Data)[str.Length()] = 0;
}

void Variant::SetTypename(const StringView& typeName)
{
    SetType(VariantType(VariantType::Typename));
    if (typeName.Length() <= 0)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
        return;
    }
    const int32 length = typeName.Length() + 1;
    if (AsBlob.Length != length)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
    }
    StringUtils::ConvertUTF162ANSI(typeName.Get(), (char*)AsBlob.Data, typeName.Length());
    ((char*)AsBlob.Data)[typeName.Length()] = 0;
}

void Variant::SetTypename(const StringAnsiView& typeName)
{
    SetType(VariantType(VariantType::Typename));
    if (typeName.Length() <= 0)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
        return;
    }
    const int32 length = typeName.Length() + 1;
    if (AsBlob.Length != length)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
    }
    Platform::MemoryCopy(AsBlob.Data, typeName.Get(), length);
    ((char*)AsBlob.Data)[typeName.Length()] = 0;
}

void Variant::SetBlob(int32 length)
{
    SetType(VariantType(VariantType::Blob));
    if (AsBlob.Length != length)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = length > 0 ? Allocator::Allocate(length) : nullptr;
        AsBlob.Length = length;
    }
}

void Variant::SetBlob(const void* data, int32 length)
{
    SetBlob(length);
    Platform::MemoryCopy(AsBlob.Data, data, length);
}

void Variant::SetObject(ScriptingObject* object)
{
    if (Type.Type != VariantType::Object)
        SetType(VariantType(VariantType::Object));
    if (AsObject)
        AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
    AsObject = object;
    if (object)
        object->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
}

void Variant::SetManagedObject(MObject* object)
{
#if USE_CSHARP
    if (object)
    {
        if (Type.Type != VariantType::ManagedObject)
            SetType(VariantType(VariantType::ManagedObject, MCore::Object::GetClass(object)));
        MANAGED_GC_HANDLE = MCore::GCHandle::New(object);
    }
    else
    {
        if (Type.Type != VariantType::ManagedObject || Type.TypeName)
            SetType(VariantType(VariantType::ManagedObject));
        MANAGED_GC_HANDLE = 0;
    }
#endif
}

void Variant::SetAsset(Asset* asset)
{
    if (Type.Type != VariantType::Asset)
        SetType(VariantType(VariantType::Asset));
    if (AsAsset)
    {
        AsAsset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
        AsAsset->RemoveReference();
    }
    AsAsset = asset;
    if (asset)
    {
        asset->AddReference();
        asset->OnUnloaded.Bind<Variant, &Variant::OnAssetUnloaded>(this);
    }
}

String Variant::ToString() const
{
    switch (Type.Type)
    {
    case VariantType::Null:
        return TEXT("null");
    case VariantType::Bool:
        return AsBool ? TEXT("true") : TEXT("false");
    case VariantType::Int16:
        return StringUtils::ToString(AsInt16);
    case VariantType::Uint16:
        return StringUtils::ToString(AsUint16);
    case VariantType::Int:
        return StringUtils::ToString(AsInt);
    case VariantType::Uint:
        return StringUtils::ToString(AsUint);
    case VariantType::Enum:
        if (Type.TypeName)
        {
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(Type.TypeName));
            if (typeHandle && typeHandle.GetType().Type == ScriptingTypes::Enum)
            {
                const auto items = typeHandle.GetType().Enum.Items;
                for (int32 i = 0; items[i].Name; i++)
                {
                    if (items[i].Value == AsUint64)
                        return String(items[i].Name);
                }
            }
        }
        return StringUtils::ToString(AsUint64);
    case VariantType::Int64:
        return StringUtils::ToString(AsInt64);
    case VariantType::Uint64:
        return StringUtils::ToString(AsUint64);
    case VariantType::Float:
        return StringUtils::ToString(AsFloat);
    case VariantType::Double:
        return StringUtils::ToString(AsDouble);
    case VariantType::Pointer:
        return String::Format(TEXT("{}"), AsPointer);
    case VariantType::String:
        return String((const Char*)AsBlob.Data, AsBlob.Length ? AsBlob.Length / sizeof(Char) - 1 : 0);
    case VariantType::Object:
        return AsObject ? AsObject->ToString() : TEXT("null");
    case VariantType::Asset:
        return AsAsset ? AsAsset->ToString() : TEXT("null");
    case VariantType::Structure:
    case VariantType::Blob:
    case VariantType::Dictionary:
    case VariantType::Array:
        return Type.ToString();
    case VariantType::Float2:
        return (*(Float2*)AsData).ToString();
    case VariantType::Float3:
        return (*(Float3*)AsData).ToString();
    case VariantType::Float4:
        return (*(Float4*)AsData).ToString();
    case VariantType::Double2:
        return AsDouble2().ToString();
    case VariantType::Double3:
        return AsDouble3().ToString();
    case VariantType::Double4:
        return AsDouble4().ToString();
    case VariantType::Int2:
        return AsInt2().ToString();
    case VariantType::Int3:
        return AsInt3().ToString();
    case VariantType::Int4:
        return AsInt4().ToString();
    case VariantType::Color:
        return (*(Color*)AsData).ToString();
    case VariantType::Guid:
        return (*(Guid*)AsData).ToString();
    case VariantType::BoundingSphere:
        return AsBoundingSphere().ToString();
    case VariantType::Quaternion:
        return (*(Quaternion*)AsData).ToString();
    case VariantType::Rectangle:
        return (*(Rectangle*)AsData).ToString();
    case VariantType::BoundingBox:
        return AsBoundingBox().ToString();
    case VariantType::Transform:
        return (*(Transform*)AsBlob.Data).ToString();
    case VariantType::Ray:
        return AsRay().ToString();
    case VariantType::Matrix:
        return (*(Matrix*)AsBlob.Data).ToString();
    case VariantType::Typename:
        return String((const char*)AsBlob.Data, AsBlob.Length ? AsBlob.Length - 1 : 0);
    case VariantType::ManagedObject:
#if USE_CSHARP
        return MANAGED_GC_HANDLE ? String(MUtils::ToString(MCore::Object::ToString(MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE)))) : TEXT("null");
#else
        return String::Empty;
#endif
    default:
        return String::Empty;
    }
}

void Variant::Inline()
{
    VariantType::Types type = VariantType::Null;
    byte data[sizeof(Matrix)];
    if (Type.Type == VariantType::Structure && AsBlob.Data && AsBlob.Length <= sizeof(Matrix))
    {
        for (int32 i = 2; i < VariantType::MAX; i++)
        {
            if (StringUtils::Compare(Type.TypeName, InBuiltTypesTypeNames[i]) == 0)
            {
                type = (VariantType::Types)i;
                break;
            }
        }
        if (type == VariantType::Null)
        {
            // Aliases
            if (StringUtils::Compare(Type.TypeName, "FlaxEngine.Vector2") == 0)
                type = VariantType::Types::Vector2;
            else if (StringUtils::Compare(Type.TypeName, "FlaxEngine.Vector3") == 0)
                type = VariantType::Types::Vector3;
            else if (StringUtils::Compare(Type.TypeName, "FlaxEngine.Vector4") == 0)
                type = VariantType::Types::Vector4;
        }
        if (type != VariantType::Null)
        {
            ASSERT(sizeof(data) >= AsBlob.Length);
            Platform::MemoryCopy(data, AsBlob.Data, AsBlob.Length);
        }
    }
    if (type != VariantType::Null)
    {
        switch (type)
        {
        case VariantType::Bool:
            *this = *(bool*)data;
            break;
        case VariantType::Int:
            *this = *(int32*)data;
            break;
        case VariantType::Uint:
            *this = *(uint32*)data;
            break;
        case VariantType::Int64:
            *this = *(int64*)data;
            break;
        case VariantType::Uint64:
            *this = *(uint64*)data;
            break;
        case VariantType::Float:
            *this = *(float*)data;
            break;
        case VariantType::Double:
            *this = *(double*)data;
            break;
        case VariantType::Float2:
            *this = *(Float2*)data;
            break;
        case VariantType::Float3:
            *this = *(Float3*)data;
            break;
        case VariantType::Float4:
            *this = *(Float4*)data;
            break;
        case VariantType::Color:
            *this = *(Color*)data;
            break;
        case VariantType::Guid:
            *this = *(Guid*)data;
            break;
        case VariantType::BoundingBox:
            *this = Variant(*(BoundingBox*)data);
            break;
        case VariantType::BoundingSphere:
            *this = *(BoundingSphere*)data;
            break;
        case VariantType::Quaternion:
            *this = *(Quaternion*)data;
            break;
        case VariantType::Transform:
            *this = Variant(*(Transform*)data);
            break;
        case VariantType::Rectangle:
            *this = *(Rectangle*)data;
            break;
        case VariantType::Ray:
            *this = Variant(*(Ray*)data);
            break;
        case VariantType::Matrix:
            *this = Variant(*(Matrix*)data);
            break;
        case VariantType::Int2:
            *this = *(Int2*)data;
            break;
        case VariantType::Int3:
            *this = *(Int3*)data;
            break;
        case VariantType::Int4:
            *this = *(Int4*)data;
            break;
        case VariantType::Int16:
            *this = *(int16*)data;
            break;
        case VariantType::Uint16:
            *this = *(uint16*)data;
            break;
        case VariantType::Double2:
            *this = *(Double2*)data;
            break;
        case VariantType::Double3:
            *this = *(Double3*)data;
            break;
        case VariantType::Double4:
            *this = *(Double4*)data;
            break;
        }
    }
}

void Variant::InvertInline()
{
    byte data[sizeof(Matrix)];
    switch (Type.Type)
    {
    case VariantType::Bool:
    case VariantType::Int:
    case VariantType::Uint:
    case VariantType::Int64:
    case VariantType::Uint64:
    case VariantType::Float:
    case VariantType::Double:
    case VariantType::Pointer:
    case VariantType::String:
    case VariantType::Float2:
    case VariantType::Float3:
    case VariantType::Float4:
    case VariantType::Color:
#if !USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
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
    case VariantType::Double4:
        static_assert(sizeof(data) >= sizeof(AsData), "Invalid memory size.");
        Platform::MemoryCopy(data, AsData, sizeof(AsData));
        break;
#if USE_LARGE_WORLDS
    case VariantType::BoundingSphere:
    case VariantType::BoundingBox:
    case VariantType::Ray:
#endif
    case VariantType::Transform:
    case VariantType::Matrix:
        ASSERT(sizeof(data) >= AsBlob.Length);
        Platform::MemoryCopy(data, AsBlob.Data, AsBlob.Length);
        break;
    default:
        return; // Not used
    }
    SetType(VariantType(VariantType::Structure, InBuiltTypesTypeNames[Type.Type]));
    CopyStructure(data);
}

Variant Variant::NewValue(const StringAnsiView& typeName)
{
    Variant v;
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (typeHandle)
    {
        const ScriptingType& type = typeHandle.GetType();
        switch (type.Type)
        {
        case ScriptingTypes::Script:
            v.SetType(VariantType(VariantType::Object, typeName));
            v.AsObject = type.Script.Spawn(ScriptingObjectSpawnParams(Guid::New(), typeHandle));
            if (v.AsObject)
                v.AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(&v);
            break;
        case ScriptingTypes::Structure:
            v.SetType(VariantType(VariantType::Structure, typeName));
            break;
        case ScriptingTypes::Enum:
            v.SetType(VariantType(VariantType::Enum, typeName));
            v.AsUint64 = 0;
            break;
        default:
            LOG(Error, "Unsupported scripting type '{}' for Variant", typeName.ToString());
            break;
        }
    }
#if USE_CSHARP
    else if (const auto mclass = Scripting::FindClass(typeName))
    {
        // Fallback to C#-only types
        if (mclass->IsEnum())
        {
            v.SetType(VariantType(VariantType::Enum, typeName));
            v.AsUint64 = 0;
        }
        else if (mclass->IsValueType())
        {
            v.SetType(VariantType(VariantType::Structure, typeName));
        }
        else
        {
            v.SetType(VariantType(VariantType::ManagedObject, typeName));
            MObject* instance = mclass->CreateInstance();
            if (instance)
            {
                v.MANAGED_GC_HANDLE = MCore::GCHandle::New(instance);
            }
        }
    }
#endif
    else if (typeName.HasChars())
    {
        LOG(Warning, "Missing scripting type \'{0}\'", String(typeName));
    }
    v.Inline(); // Wrap in-built types
    return MoveTemp(v);
}

void Variant::DeleteValue()
{
    // Delete any object owned by the Variant
    switch (Type.Type)
    {
    case VariantType::Object:
        if (AsObject)
        {
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
            AsObject->DeleteObject();
            AsObject = nullptr;
        }
        break;
    }

    // Go back to null
    SetType(VariantType(VariantType::Null));
}

Variant Variant::Parse(const StringView& text, const VariantType& type)
{
    Variant result;
    result.SetType(type);
    if (text.IsEmpty())
        return result;
    if (type != VariantType())
    {
        switch (type.Type)
        {
        case VariantType::Bool:
            if (text == TEXT("1") || text.Compare(StringView(TEXT("true"), 4), StringSearchCase::IgnoreCase) == 0)
                result.AsBool = true;
            break;
        case VariantType::Int16:
            StringUtils::Parse(text.Get(), text.Length(), &result.AsInt16);
            break;
        case VariantType::Uint16:
            StringUtils::Parse(text.Get(), text.Length(), &result.AsUint16);
            break;
        case VariantType::Int:
            StringUtils::Parse(text.Get(), text.Length(), &result.AsInt);
            break;
        case VariantType::Uint:
            StringUtils::Parse(text.Get(), text.Length(), &result.AsUint);
            break;
        case VariantType::Int64:
            StringUtils::Parse(text.Get(), text.Length(), &result.AsInt64);
            break;
        case VariantType::Uint64:
        case VariantType::Enum:
            if (!StringUtils::Parse(text.Get(), text.Length(), &result.AsUint64))
            {
            }
            else if (type.TypeName)
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(type.TypeName));
                if (typeHandle && typeHandle.GetType().Type == ScriptingTypes::Enum)
                {
                    const auto items = typeHandle.GetType().Enum.Items;
                    StringAsANSI<32> textAnsi(text.Get(), text.Length());
                    StringAnsiView textAnsiView(textAnsi.Get());
                    for (int32 i = 0; items[i].Name; i++)
                    {
                        if (textAnsiView == items[i].Name)
                        {
                            result.AsUint64 = items[i].Value;
                            break;
                        }
                    }
                }
            }
            break;
        case VariantType::Float:
            StringUtils::Parse(text.Get(), &result.AsFloat);
            break;
        case VariantType::Double:
            StringUtils::Parse(text.Get(), &result.AsFloat);
            result.AsDouble = (float)result.AsFloat;
            break;
        case VariantType::String:
            result.SetString(text);
        default:
            break;
        }
    }
    else
    {
        // Parse as number
        int32 valueInt;
        if (!StringUtils::Parse(text.Get(), text.Length(), &valueInt))
        {
            result = valueInt;
        }
        else
        {
            // Fallback to string
            result.SetString(text);
        }
    }
    return result;
}

Variant Variant::Typename(const StringAnsiView& value)
{
    Variant result;
    result.SetTypename(value);
    return result;
}

bool Variant::CanCast(const Variant& v, const VariantType& to)
{
    if (v.Type == to)
        return true;
    switch (v.Type.Type)
    {
    case VariantType::Bool:
        switch (to.Type)
        {
        case VariantType::Int16:
        case VariantType::Uint16:
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
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Int16:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Uint64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Int:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Int16:
        case VariantType::Int64:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Uint64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Uint16:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Uint:
        case VariantType::Uint64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Uint:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Uint16:
        case VariantType::Uint64:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Int64:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Uint64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Uint64:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Float:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Int64:
        case VariantType::Uint64:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Double:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Uint:
        case VariantType::Uint16:
        case VariantType::Int64:
        case VariantType::Uint64:
        case VariantType::Float:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Float2:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Float3:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float4:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Float4:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Color:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Color:
        switch (to.Type)
        {
        case VariantType::Bool:
        case VariantType::Uint16:
        case VariantType::Uint:
        case VariantType::Int16:
        case VariantType::Int:
        case VariantType::Int64:
        case VariantType::Float:
        case VariantType::Double:
        case VariantType::Float2:
        case VariantType::Float3:
        case VariantType::Float4:
        case VariantType::Double2:
        case VariantType::Double3:
        case VariantType::Double4:
            return true;
        default:
            return false;
        }
    case VariantType::Null:
        switch (to.Type)
        {
    case VariantType::Asset:
    case VariantType::ManagedObject:
    case VariantType::Object:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

Variant Variant::Cast(const Variant& v, const VariantType& to)
{
    if (v.Type == to)
        return v;
    switch (v.Type.Type)
    {
    case VariantType::Bool:
        switch (to.Type)
        {
        case VariantType::Int16: // No portable literal suffix for short ( Available in MSVC but Undocumented : i16 )
            return Variant((int16)(v.AsBool ? 1 : 0));
        case VariantType::Uint16: // No portable literal suffix for short ( Available in MSVC but Undocumented : ui16 )
            return Variant((uint16)(v.AsBool ? 1 : 0));
        case VariantType::Int:
            return Variant(v.AsBool ? 1 : 0);
        case VariantType::Uint:
            return Variant(v.AsBool ? 1u : 0u);
        case VariantType::Int64:
            return Variant(v.AsBool ? 1ll : 0ll);
        case VariantType::Uint64:
            return Variant(v.AsBool ? 1ull : 0ull);
        case VariantType::Float:
            return Variant(v.AsBool ? 1.0f : 0.0f);
        case VariantType::Double:
            return Variant(v.AsBool ? 1.0 : 0.0);
        case VariantType::Float2:
            return Variant(Float2(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Float3:
            return Variant(Float3(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Float4:
            return Variant(Float2(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Color:
            return Variant(Color(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Double2:
            return Variant(Double2(v.AsBool ? 1.0 : 0.0));
        case VariantType::Double3:
            return Variant(Double3(v.AsBool ? 1.0 : 0.0));
        case VariantType::Double4:
            return Variant(Double4(v.AsBool ? 1.0 : 0.0));
        default: ;
        }
        break;
    case VariantType::Int16:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(v.AsInt != 0);
        case VariantType::Int:
            return Variant((int32)v.AsInt16);
        case VariantType::Int64:
            return Variant((int64)v.AsInt16);
        case VariantType::Uint16:
            return Variant((uint16)v.AsInt16);
        case VariantType::Uint:
            return Variant((uint32)v.AsInt16);
        case VariantType::Uint64:
            return Variant((uint64)v.AsInt16);
        case VariantType::Float:
            return Variant((float)v.AsInt16);
        case VariantType::Double:
            return Variant((double)v.AsInt16);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsInt16));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsInt16));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsInt16));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt16));
        case VariantType::Double2:
            return Variant(Double2((double)v.AsInt16));
        case VariantType::Double3:
            return Variant(Double3((double)v.AsInt16));
        case VariantType::Double4:
            return Variant(Double4((double)v.AsInt16));
        default: ;
        }
        break;
    case VariantType::Int:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(v.AsInt != 0);
        case VariantType::Int16:
            return Variant((int16)v.AsInt);
        case VariantType::Int64:
            return Variant((int64)v.AsInt);
        case VariantType::Uint16:
            return Variant((uint16)v.AsInt);
        case VariantType::Uint:
            return Variant((uint32)v.AsInt);
        case VariantType::Uint64:
            return Variant((uint64)v.AsInt);
        case VariantType::Float:
            return Variant((float)v.AsInt);
        case VariantType::Double:
            return Variant((double)v.AsInt);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsInt));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsInt));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsInt));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt));
        default: ;
        }
        break;
    case VariantType::Uint16:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(v.AsUint16 != 0);
        case VariantType::Int16:
            return Variant((int16)v.AsUint16);
        case VariantType::Int:
            return Variant((int32)v.AsUint16);
        case VariantType::Int64:
            return Variant((int64)v.AsUint16);
        case VariantType::Uint16:
            return Variant((uint16)v.AsUint16);
        case VariantType::Uint64:
            return Variant((uint64)v.AsUint16);
        case VariantType::Float:
            return Variant((float)v.AsUint16);
        case VariantType::Double:
            return Variant((double)v.AsUint16);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsUint16));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsUint16));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsUint16));
        case VariantType::Color:
            return Variant(Color((float)v.AsUint16));
        case VariantType::Double2:
            return Variant(Double2((double)v.AsUint16));
        case VariantType::Double3:
            return Variant(Double3((double)v.AsUint16));
        case VariantType::Double4:
            return Variant(Double4((double)v.AsUint16));
        default: ;
        }
        break;
    case VariantType::Uint:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(v.AsUint != 0);
        case VariantType::Int16:
            return Variant((int16)v.AsUint);
        case VariantType::Int:
            return Variant((int32)v.AsUint);
        case VariantType::Int64:
            return Variant((int64)v.AsUint);
        case VariantType::Uint16:
            return Variant((uint16)v.AsUint);
        case VariantType::Uint64:
            return Variant((uint64)v.AsUint);
        case VariantType::Float:
            return Variant((float)v.AsUint);
        case VariantType::Double:
            return Variant((double)v.AsUint);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsUint));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsUint));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsUint));
        case VariantType::Color:
            return Variant(Color((float)v.AsUint));
        case VariantType::Double2:
            return Variant(Double2((double)v.AsUint));
        case VariantType::Double3:
            return Variant(Double3((double)v.AsUint));
        case VariantType::Double4:
            return Variant(Double4((double)v.AsUint));
        default: ;
        }
        break;
    case VariantType::Int64:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(v.AsInt64 != 0);
        case VariantType::Int16:
            return Variant((int16)v.AsInt64);
        case VariantType::Int:
            return Variant((int32)v.AsInt64);
        case VariantType::Uint16:
            return Variant((uint16)v.AsInt64);
        case VariantType::Uint:
            return Variant((uint32)v.AsInt64);
        case VariantType::Uint64:
            return Variant((uint64)v.AsInt64);
        case VariantType::Float:
            return Variant((float)v.AsInt64);
        case VariantType::Double:
            return Variant((double)v.AsInt64);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsInt64));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsInt64));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsInt64));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt64));
        case VariantType::Double2:
            return Variant(Double2((double)v.AsInt64));
        case VariantType::Double3:
            return Variant(Double3((double)v.AsInt64));
        case VariantType::Double4:
            return Variant(Double4((double)v.AsInt64));
        default: ;
        }
        break;
    case VariantType::Uint64:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(v.AsUint64 != 0);
        case VariantType::Int16:
            return Variant((int16)v.AsUint64);
        case VariantType::Int:
            return Variant((int32)v.AsUint64);
        case VariantType::Int64:
            return Variant((int64)v.AsUint64);
        case VariantType::Uint16:
            return Variant((uint16)v.AsUint16);
        case VariantType::Uint:
            return Variant((uint32)v.AsUint);
        case VariantType::Float:
            return Variant((float)v.AsUint64);
        case VariantType::Double:
            return Variant((double)v.AsUint64);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsInt));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsInt));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsInt));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt));
        case VariantType::Double2:
            return Variant(Double2((double)v.AsInt));
        case VariantType::Double3:
            return Variant(Double3((double)v.AsInt));
        case VariantType::Double4:
            return Variant(Double4((double)v.AsInt));
        default: ;
        }
        break;
    case VariantType::Float:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(v.AsFloat) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)v.AsFloat);
        case VariantType::Int:
            return Variant((int32)v.AsFloat);
        case VariantType::Uint16:
            return Variant((uint16)v.AsFloat);
        case VariantType::Uint:
            return Variant((uint32)v.AsFloat);
        case VariantType::Int64:
            return Variant((int64)v.AsFloat);
        case VariantType::Uint64:
            return Variant((uint64)v.AsFloat);
        case VariantType::Double:
            return Variant((double)v.AsFloat);
        case VariantType::Float2:
            return Variant(Float2(v.AsFloat));
        case VariantType::Float3:
            return Variant(Float3(v.AsFloat));
        case VariantType::Float4:
            return Variant(Float4(v.AsFloat));
        case VariantType::Color:
            return Variant(Color(v.AsFloat));
        case VariantType::Double2:
            return Variant(Double2(v.AsFloat));
        case VariantType::Double3:
            return Variant(Double3(v.AsFloat));
        case VariantType::Double4:
            return Variant(Double4(v.AsFloat));
        default: ;
        }
        break;
    case VariantType::Double:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(v.AsDouble) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)v.AsDouble);
        case VariantType::Int:
            return Variant((int32)v.AsDouble);
        case VariantType::Uint16:
            return Variant((uint16)v.AsDouble);
        case VariantType::Uint:
            return Variant((uint32)v.AsDouble);
        case VariantType::Int64:
            return Variant((int64)v.AsDouble);
        case VariantType::Uint64:
            return Variant((uint64)v.AsDouble);
        case VariantType::Float:
            return Variant((float)v.AsDouble);
        case VariantType::Float2:
            return Variant(Float2((float)v.AsDouble));
        case VariantType::Float3:
            return Variant(Float3((float)v.AsDouble));
        case VariantType::Float4:
            return Variant(Float4((float)v.AsDouble));
        case VariantType::Color:
            return Variant(Color((float)v.AsDouble));
        case VariantType::Double2:
            return Variant(Double2(v.AsDouble));
        case VariantType::Double3:
            return Variant(Double3(v.AsDouble));
        case VariantType::Double4:
            return Variant(Double4(v.AsDouble));
        default: ;
        }
        break;
    case VariantType::Float2:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Float2*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Float2*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Float2*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Float2*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Float2*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Float2*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Float2*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Float2*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Float2*)v.AsData)->X);
        case VariantType::Float3:
            return Variant(Float3(*(Float2*)v.AsData, 0.0f));
        case VariantType::Float4:
            return Variant(Float4(*(Float2*)v.AsData, 0.0f, 0.0f));
        case VariantType::Color:
            return Variant(Color(((Float2*)v.AsData)->X, ((Float2*)v.AsData)->Y, 0.0f, 0.0f));
        case VariantType::Double2:
            return Variant(Double2(*(Float2*)v.AsData));
        case VariantType::Double3:
            return Variant(Double3(*(Float2*)v.AsData, 0.0));
        case VariantType::Double4:
            return Variant(Double4(*(Float2*)v.AsData, 0.0, 0.0));
        default: ;
        }
        break;
    case VariantType::Float3:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Float3*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Float3*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Float3*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Float3*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Float3*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Float3*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Float3*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Float3*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Float3*)v.AsData)->X);
        case VariantType::Float2:
            return Variant(Float2(*(Float3*)v.AsData));
        case VariantType::Float4:
            return Variant(Float4(*(Float3*)v.AsData, 0.0f));
        case VariantType::Color:
            return Variant(Color(((Float3*)v.AsData)->X, ((Float3*)v.AsData)->Y, ((Float3*)v.AsData)->Z, 0.0f));
        case VariantType::Double2:
            return Variant(Double2(*(Float3*)v.AsData));
        case VariantType::Double3:
            return Variant(Double3(*(Float3*)v.AsData));
        case VariantType::Double4:
            return Variant(Double4(*(Float3*)v.AsData, 0.0));
        default: ;
        }
        break;
    case VariantType::Float4:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Float4*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Float4*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Float4*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Float4*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Float4*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Float4*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Float4*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Float4*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Float4*)v.AsData)->X);
        case VariantType::Float2:
            return Variant(Float2(*(Float4*)v.AsData));
        case VariantType::Float3:
            return Variant(Float3(*(Float4*)v.AsData));
        case VariantType::Color:
            return Variant(*(Float4*)v.AsData);
        case VariantType::Double2:
            return Variant(Double2(*(Float4*)v.AsData));
        case VariantType::Double3:
            return Variant(Double3(*(Float4*)v.AsData));
        case VariantType::Double4:
            return Variant(Double4(*(Float4*)v.AsData));
        default: ;
        }
        break;
    case VariantType::Double2:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Double2*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Double2*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Double2*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Double2*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Double2*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Double2*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Double2*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Double2*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Double2*)v.AsData)->X);
        case VariantType::Float2:
            return Variant(Float2(*(Double2*)v.AsData));
        case VariantType::Float3:
            return Variant(Float3(*(Double2*)v.AsData, 0.0f));
        case VariantType::Float4:
            return Variant(Float4(*(Double2*)v.AsData, 0.0f, 0.0f));
        case VariantType::Color:
            return Variant(Color(((Double2*)v.AsData)->X, ((Double2*)v.AsData)->Y, 0.0f, 0.0f));
        case VariantType::Double3:
            return Variant(Double3(*(Double2*)v.AsData, 0.0));
        case VariantType::Double4:
            return Variant(Double4(*(Double2*)v.AsData, 0.0, 0.0));
        default: ;
        }
        break;
    case VariantType::Double3:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Double3*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Double3*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Double3*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Double3*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Double3*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Double3*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Double3*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Double3*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Double3*)v.AsData)->X);
        case VariantType::Float2:
            return Variant(Float2(*(Double3*)v.AsData));
        case VariantType::Float3:
            return Variant(Float3(*(Double3*)v.AsData));
        case VariantType::Float4:
            return Variant(Float4(*(Double3*)v.AsData, 0.0f));
        case VariantType::Color:
            return Variant(Color(((Double3*)v.AsData)->X, ((Double3*)v.AsData)->Y, ((Double3*)v.AsData)->Z, 0.0f));
        case VariantType::Double2:
            return Variant(Double2(*(Double3*)v.AsData));
        case VariantType::Double4:
            return Variant(Double4(*(Double3*)v.AsData, 0.0));
        default: ;
        }
        break;
    case VariantType::Double4:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Double4*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Double4*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Double4*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Double4*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Double4*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Double4*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Double4*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Double4*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Double4*)v.AsData)->X);
        case VariantType::Float2:
            return Variant(Float2(*(Double4*)v.AsData));
        case VariantType::Float3:
            return Variant(Float3(*(Double4*)v.AsData));
        case VariantType::Float4:
            return Variant(Float4(*(Double4*)v.AsData));
        case VariantType::Color:
            return Variant(*(Double4*)v.AsData);
        case VariantType::Double2:
            return Variant(Double2(*(Double4*)v.AsData));
        case VariantType::Double3:
            return Variant(Double3(*(Double4*)v.AsData));
        default: ;
        }
        break;
    case VariantType::Color:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Color*)v.AsData)->R) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Color*)v.AsData)->R);
        case VariantType::Int:
            return Variant((int32)((Color*)v.AsData)->R);
        case VariantType::Uint16:
            return Variant((uint16)((Color*)v.AsData)->R);
        case VariantType::Uint:
            return Variant((uint32)((Color*)v.AsData)->R);
        case VariantType::Int64:
            return Variant((int64)((Color*)v.AsData)->R);
        case VariantType::Uint64:
            return Variant((uint64)((Color*)v.AsData)->R);
        case VariantType::Float:
            return Variant((float)((Color*)v.AsData)->R);
        case VariantType::Double:
            return Variant((double)((Color*)v.AsData)->R);
        case VariantType::Float2:
            return Variant(Float2(*(Color*)v.AsData));
        case VariantType::Float3:
            return Variant(Float3(*(Color*)v.AsData));
        case VariantType::Float4:
            return Variant(*(Color*)v.AsData);
        case VariantType::Double2:
            return Variant(Double2(*(Color*)v.AsData));
        case VariantType::Double3:
            return Variant(Double3(*(Color*)v.AsData));
        case VariantType::Double4:
            return Variant(Double4(*(Color*)v.AsData));
        default: ;
        }
        break;
    case VariantType::Null:
        switch (to.Type)
        {
        case VariantType::Asset:
            return Variant((Asset*)nullptr);
        case VariantType::Object:
            return Variant((ScriptingObject*)nullptr);
        case VariantType::ManagedObject:
        {
            Variant result;
            result.SetType(VariantType(VariantType::ManagedObject));
            result.MANAGED_GC_HANDLE = 0;
            return result;
        }
        default:
            return false;
        }
    default: ;
    }
    LOG(Error, "Cannot cast Variant from {0} to {1}", v.Type, to);
    return Null;
}

bool Variant::NearEqual(const Variant& a, const Variant& b, float epsilon)
{
    if (a.Type != b.Type)
        return false;
    switch (a.Type.Type)
    {
    case VariantType::Int16:
        return Math::Abs(a.AsInt16 - b.AsInt16) <= (int32)epsilon;
    case VariantType::Int:
        return Math::Abs(a.AsInt - b.AsInt) <= (int32)epsilon;
    case VariantType::Int64:
        return Math::Abs(a.AsInt64 - b.AsInt64) <= (int64)epsilon;
    case VariantType::Float:
        return Math::NearEqual(a.AsFloat, b.AsFloat, epsilon);
    case VariantType::Double:
        return Math::NearEqual((float)a.AsDouble, (float)b.AsDouble, epsilon);
    case VariantType::Float2:
        return Float2::NearEqual(*(Float2*)a.AsData, *(Float2*)b.AsData, epsilon);
    case VariantType::Float3:
        return Float3::NearEqual(*(Float3*)a.AsData, *(Float3*)b.AsData, epsilon);
    case VariantType::Float4:
        return Float4::NearEqual(*(Float4*)a.AsData, *(Float4*)b.AsData, epsilon);
    case VariantType::Double2:
        return Double2::NearEqual(*(Double2*)a.AsData, *(Double2*)b.AsData, epsilon);
    case VariantType::Double3:
        return Double3::NearEqual(*(Double3*)a.AsData, *(Double3*)b.AsData, epsilon);
    case VariantType::Double4:
        return Double4::NearEqual(*(Double4*)a.AsBlob.Data, *(Double4*)b.AsBlob.Data, epsilon);
    case VariantType::Color:
        return Color::NearEqual(*(Color*)a.AsData, *(Color*)b.AsData, epsilon);
    case VariantType::BoundingSphere:
        return BoundingSphere::NearEqual(a.AsBoundingSphere(), b.AsBoundingSphere(), epsilon);
    case VariantType::Quaternion:
        return Quaternion::NearEqual(*(Quaternion*)a.AsData, *(Quaternion*)b.AsData, epsilon);
    case VariantType::Rectangle:
        return Rectangle::NearEqual(*(Rectangle*)a.AsData, *(Rectangle*)b.AsData, epsilon);
    case VariantType::BoundingBox:
        return BoundingBox::NearEqual(a.AsBoundingBox(), b.AsBoundingBox(), epsilon);
    case VariantType::Transform:
        return Transform::NearEqual(*(Transform*)a.AsBlob.Data, *(Transform*)b.AsBlob.Data, epsilon);
    case VariantType::Ray:
        return Ray::NearEqual(a.AsRay(), b.AsRay(), epsilon);
    default:
        return a == b;
    }
}

Variant Variant::Lerp(const Variant& a, const Variant& b, float alpha)
{
    if (a.Type != b.Type)
        return a;
    switch (a.Type.Type)
    {
    case VariantType::Bool:
        return alpha < 0.5f ? a : b;
    case VariantType::Int16:
        return Math::Lerp(a.AsInt16, b.AsInt16, alpha);
    case VariantType::Int:
        return Math::Lerp(a.AsInt, b.AsInt, alpha);
    case VariantType::Uint16:
        return Math::Lerp(a.AsUint16, b.AsUint16, alpha);
    case VariantType::Uint:
        return Math::Lerp(a.AsUint, b.AsUint, alpha);
    case VariantType::Int64:
        return Math::Lerp(a.AsInt64, b.AsInt64, alpha);
    case VariantType::Uint64:
        return Math::Lerp(a.AsUint64, b.AsUint64, alpha);
    case VariantType::Float:
        return Math::Lerp(a.AsFloat, b.AsFloat, alpha);
    case VariantType::Float2:
        return Float2::Lerp(*(Float2*)a.AsData, *(Float2*)b.AsData, alpha);
    case VariantType::Float3:
        return Float3::Lerp(*(Float3*)a.AsData, *(Float3*)b.AsData, alpha);
    case VariantType::Float4:
        return Float4::Lerp(*(Float4*)a.AsData, *(Float4*)b.AsData, alpha);
    case VariantType::Double2:
        return Double2::Lerp(*(Double2*)a.AsData, *(Double2*)b.AsData, alpha);
    case VariantType::Double3:
        return Double3::Lerp(*(Double3*)a.AsData, *(Double3*)b.AsData, alpha);
    case VariantType::Double4:
        return Double4::Lerp(*(Double4*)a.AsBlob.Data, *(Double4*)b.AsBlob.Data, alpha);
    case VariantType::Color:
        return Color::Lerp(*(Color*)a.AsData, *(Color*)b.AsData, alpha);
    case VariantType::Quaternion:
        return Quaternion::Lerp(*(Quaternion*)a.AsData, *(Quaternion*)b.AsData, alpha);
    case VariantType::BoundingSphere:
        return BoundingSphere(Vector3::Lerp(a.AsBoundingSphere().Center, b.AsBoundingSphere().Center, alpha), Math::Lerp(a.AsBoundingSphere().Radius, b.AsBoundingSphere().Radius, alpha));
    case VariantType::Rectangle:
        return Rectangle(Float2::Lerp((*(Rectangle*)a.AsData).Location, (*(Rectangle*)b.AsData).Location, alpha), Float2::Lerp((*(Rectangle*)a.AsData).Size, (*(Rectangle*)b.AsData).Size, alpha));
    case VariantType::Transform:
        return Variant(Transform::Lerp(*(Transform*)a.AsBlob.Data, *(Transform*)b.AsBlob.Data, alpha));
    case VariantType::BoundingBox:
        return Variant(BoundingBox(Vector3::Lerp(a.AsBoundingBox().Minimum, b.AsBoundingBox().Minimum, alpha), Vector3::Lerp(a.AsBoundingBox().Maximum, b.AsBoundingBox().Maximum, alpha)));
    case VariantType::Ray:
        return Variant(Ray(Vector3::Lerp(a.AsRay().Position, b.AsRay().Position, alpha), Vector3::Normalize(Vector3::Lerp(a.AsRay().Direction, b.AsRay().Direction, alpha))));
    default:
        return a;
    }
}

void Variant::OnObjectDeleted(ScriptingObject* obj)
{
    AsObject = nullptr;
}

void Variant::OnAssetUnloaded(Asset* obj)
{
    AsAsset = nullptr;
}

void Variant::AllocStructure()
{
    const StringAnsiView typeName(Type.TypeName);
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (typeHandle)
    {
        const ScriptingType& type = typeHandle.GetType();
        AsBlob.Length = type.Size;
        AsBlob.Data = Allocator::Allocate(AsBlob.Length);
        Platform::MemoryClear(AsBlob.Data, AsBlob.Length);
        type.Struct.Ctor(AsBlob.Data);
    }
    else if (typeName == "System.Int16" || typeName == "System.UInt16")
    {
        // [Deprecated on 10.05.2021, expires on 10.05.2023]
        // Hack for 16bit int
        AsBlob.Length = 2;
        AsBlob.Data = Allocator::Allocate(AsBlob.Length);
        *((int16*)AsBlob.Data) = 0;
    }
#if USE_CSHARP
    else if (const auto mclass = Scripting::FindClass(typeName))
    {
        // Fallback to C#-only types
        MCore::Thread::Attach();
        MObject* instance = mclass->CreateInstance();
        if (instance)
        {
#if 0
            void* data = MCore::Object::Unbox(instance);
            int32 instanceSize = mclass->GetInstanceSize();
            AsBlob.Length = instanceSize - (int32)((uintptr)data - (uintptr)instance);
            AsBlob.Data = Allocator::Allocate(AsBlob.Length);
            Platform::MemoryCopy(AsBlob.Data, data, AsBlob.Length);
#else
            Type.Type = VariantType::ManagedObject;
            MANAGED_GC_HANDLE = MCore::GCHandle::New(instance);
#endif
        }
        else
        {
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
        }
    }
#endif
    else
    {
        if (typeName.Length() != 0)
        {
            LOG(Warning, "Missing scripting type \'{0}\'", String(typeName));
        }
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
    }
}

void Variant::CopyStructure(void* src)
{
    if (AsBlob.Data && src)
    {
        const StringAnsiView typeName(Type.TypeName);
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
        if (typeHandle)
        {
            auto& type = typeHandle.GetType();
            type.Struct.Copy(AsBlob.Data, src);
        }
#if USE_CSHARP
        else if (const auto mclass = Scripting::FindClass(typeName))
        {
            // Fallback to C#-only types
            MCore::Thread::Attach();
            if (MANAGED_GC_HANDLE && mclass->IsValueType())
            {
                MObject* instance = MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE);
                void* data = MCore::Object::Unbox(instance);
                Platform::MemoryCopy(data, src, mclass->GetInstanceSize());
            }
        }
#endif
        else
        {
            if (typeName.Length() != 0)
            {
                LOG(Warning, "Missing scripting type \'{0}\'", String(typeName));
            }
        }
    }
}

void Variant::FreeStructure()
{
    if (!AsBlob.Data)
        return;
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(Type.TypeName));
    if (typeHandle)
    {
        const ScriptingType& type = typeHandle.GetType();
        type.Struct.Dtor(AsBlob.Data);
    }
    Allocator::Free(AsBlob.Data);
}

uint32 GetHash(const Variant& key)
{
    switch (key.Type.Type)
    {
    case VariantType::Bool:
        return GetHash(key.AsBool);
    case VariantType::Int16:
        return GetHash(key.AsInt16);
    case VariantType::Int:
        return GetHash(key.AsInt);
    case VariantType::Uint16:
        return GetHash(key.AsUint16);
    case VariantType::Uint:
        return GetHash(key.AsUint);
    case VariantType::Int64:
        return GetHash(key.AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return GetHash(key.AsUint64);
    case VariantType::Float:
        return GetHash(key.AsFloat);
    case VariantType::Double:
        return GetHash(key.AsDouble);
    case VariantType::Pointer:
        return GetHash(key.AsPointer);
    case VariantType::String:
        return GetHash((const Char*)key.AsBlob.Data);
    case VariantType::Object:
        return GetHash((void*)key.AsObject);
    case VariantType::Structure:
    case VariantType::Blob:
        return Crc::MemCrc32(key.AsBlob.Data, key.AsBlob.Length);
    case VariantType::Asset:
        return GetHash((void*)key.AsAsset);
    case VariantType::Color:
        return GetHash(*(Color*)key.AsData);
    case VariantType::Guid:
        return GetHash(*(Guid*)key.AsData);
    case VariantType::Typename:
        return GetHash((const char*)key.AsBlob.Data);
    case VariantType::ManagedObject:
#if USE_CSHARP
        return key.MANAGED_GC_HANDLE ? (uint32)MCore::Object::GetHashCode(MCore::GCHandle::GetTarget(key.MANAGED_GC_HANDLE)) : 0;
#else
        return 0;
#endif
    default:
        return 0;
    }
}
