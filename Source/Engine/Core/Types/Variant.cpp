// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Variant.h"
#include "CommonValue.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/Asset.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Int2.h"
#include "Engine/Core/Math/Int3.h"
#include "Engine/Core/Math/Int4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Utilities/Crc.h"
#include "Engine/Utilities/StringConverter.h"
#if USE_MONO
#include <ThirdParty/mono-2.0/mono/metadata/object.h>
#endif

static_assert(sizeof(VariantType) <= 16, "Invalid VariantType size!");

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
        length++;
        TypeName = static_cast<char*>(Allocator::Allocate(length));
        Platform::MemoryCopy(TypeName, typeName.Get(), length);
    }
}

VariantType::VariantType(Types type, _MonoClass* klass)
{
    Type = type;
    TypeName = nullptr;
    if (klass)
    {
        MString typeName;
        MUtils::GetClassFullname(klass, typeName);
        int32 length = typeName.Length() + 1;
        TypeName = static_cast<char*>(Allocator::Allocate(length));
        Platform::MemoryCopy(TypeName, typeName.Get(), length);
    }
}

VariantType::VariantType(const VariantType& other)
{
    Type = other.Type;
    TypeName = nullptr;
    int32 length = StringUtils::Length(other.TypeName);
    if (length)
    {
        length++;
        TypeName = static_cast<char*>(Allocator::Allocate(length));
        Platform::MemoryCopy(TypeName, other.TypeName, length);
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
    int32 length = StringUtils::Length(other.TypeName);
    if (length)
    {
        length++;
        TypeName = static_cast<char*>(Allocator::Allocate(length));
        Platform::MemoryCopy(TypeName, other.TypeName, length);
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
    switch (Type)
    {
    case Void:
        return "System.Void";
    case Bool:
        return "System.Boolean";
    case Int16:
        return "System.Int16";
    case Uint16:
        return "System.UInt16";
    case Int:
        return "System.Int32";
    case Uint:
        return "System.UInt32";
    case Int64:
        return "System.Int64";
    case Uint64:
        return "System.UInt64";
    case Float:
        return "System.Single";
    case Double:
        return "System.Double";
    case Pointer:
        return "System.IntPtr";
    case String:
        return "System.String";
    case Object:
        return "System.Object";
    case Asset:
        return "FlaxEngine.Asset";
    case Vector2:
        return "FlaxEngine.Vector2";
    case Vector3:
        return "FlaxEngine.Vector3";
    case Vector4:
        return "FlaxEngine.Vector4";
    case Color:
        return "FlaxEngine.Color";
    case Guid:
        return "System.Guid";
    case BoundingBox:
        return "FlaxEngine.BoundingBox";
    case BoundingSphere:
        return "FlaxEngine.BoundingSphere";
    case Quaternion:
        return "FlaxEngine.Quaternion";
    case Transform:
        return "FlaxEngine.Transform";
    case Rectangle:
        return "FlaxEngine.Rectangle";
    case Ray:
        return "FlaxEngine.Ray";
    case Matrix:
        return "FlaxEngine.Matrix";
    case Typename:
        return "System.Type";
    default:
        return "";
    }
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
    case Vector2:
        result = TEXT("Vector2");
        break;
    case Vector3:
        result = TEXT("Vector3");
        break;
    case Vector4:
        result = TEXT("Vector4");
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
    case MAX:
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

static_assert(sizeof(Variant) <= 32, "Invalid Variant size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Vector2), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Vector3), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Vector4), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Color), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Quaternion), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Rectangle), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Guid), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(BoundingSphere), "Invalid Variant data size!");
static_assert(sizeof(Variant::AsData) >= sizeof(Array<Variant, HeapAllocation>), "Invalid Variant data size!");

const Variant Variant::Zero(0.0f);
const Variant Variant::One(1.0f);
const Variant Variant::Null(static_cast<void*>(nullptr));
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
        AsUint = other.AsUint;
        other.AsUint = 0;
        break;
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::MAX:
        break;
    case VariantType::Structure:
    case VariantType::Blob:
    case VariantType::String:
    case VariantType::BoundingBox:
    case VariantType::Transform:
    case VariantType::Ray:
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
        v->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
}

Variant::Variant(Asset* v)
    : Type(VariantType::Asset)
{
    AsAsset = v;
    if (v)
    {
        v->AddReference();
        v->OnUnloaded.Bind<Variant, &Variant::OnAssetUnloaded>(this);
    }
}

Variant::Variant(_MonoObject* v)
    : Type(VariantType::ManagedObject, v ? mono_object_get_class(v) : nullptr)
{
    AsUint = v ? mono_gchandle_new(v, true) : 0;
}

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
        StringUtils::ConvertANSI2UTF16(v.Get(), (Char*)AsBlob.Data, v.Length());
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

Variant::Variant(const Vector2& v)
    : Type(VariantType::Vector2)
{
    *(Vector2*)AsData = v;
}

Variant::Variant(const Vector3& v)
    : Type(VariantType::Vector3)
{
    *(Vector3*)AsData = v;
}

Variant::Variant(const Vector4& v)
    : Type(VariantType::Vector4)
{
    *(Vector4*)AsData = v;
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
    *(BoundingSphere*)AsData = v;
}

Variant::Variant(const Rectangle& v)
    : Type(VariantType::Rectangle)
{
    *(Rectangle*)AsData = v;
}

Variant::Variant(const BoundingBox& v)
    : Type(VariantType::BoundingBox)
{
    AsBlob.Length = sizeof(BoundingBox);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(BoundingBox*)AsBlob.Data = v;
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
    AsBlob.Length = sizeof(Ray);
    AsBlob.Data = Allocator::Allocate(AsBlob.Length);
    *(Ray*)AsBlob.Data = v;
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

Variant::Variant(const Dictionary<Variant, Variant>& v)
    : Type(VariantType::Dictionary)
{
    AsDictionary = New<Dictionary<Variant, Variant>>(v);
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
    case VariantType::BoundingBox:
    case VariantType::Transform:
    case VariantType::Ray:
    case VariantType::Matrix:
    case VariantType::Typename:
        Allocator::Free(AsBlob.Data);
        break;
    case VariantType::Array:
        reinterpret_cast<Array<Variant, HeapAllocation>*>(AsData)->~Array<Variant, HeapAllocation>();
        break;
    case VariantType::Dictionary:
        Delete(AsDictionary);
        break;
#if USE_MONO
    case VariantType::ManagedObject:
        if (AsUint)
            mono_gchandle_free(AsUint);
        break;
#endif
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
    case VariantType::BoundingBox:
    case VariantType::Transform:
    case VariantType::Ray:
    case VariantType::Matrix:
    case VariantType::Typename:
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
        AsUint = other.AsUint;
        other.AsUint = 0;
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
    case VariantType::BoundingBox:
    case VariantType::Transform:
    case VariantType::Ray:
    case VariantType::Matrix:
    case VariantType::Typename:
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
        {
            AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
        }
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
        AsUint = other.AsUint ? mono_gchandle_new(mono_gchandle_get_target(other.AsUint), true) : 0;
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
        case VariantType::BoundingBox:
        case VariantType::Transform:
        case VariantType::Ray:
        case VariantType::Matrix:
            return AsBlob.Length == other.AsBlob.Length && Platform::MemoryCompare(AsBlob.Data, other.AsBlob.Data, AsBlob.Length) == 0;
        case VariantType::Asset:
            return AsAsset == other.AsAsset;
        case VariantType::Vector2:
            return *(Vector2*)AsData == *(Vector2*)other.AsData;
        case VariantType::Vector3:
            return *(Vector3*)AsData == *(Vector3*)other.AsData;
        case VariantType::Vector4:
            return *(Vector4*)AsData == *(Vector4*)other.AsData;
        case VariantType::Color:
            return *(Color*)AsData == *(Color*)other.AsData;
        case VariantType::Quaternion:
            return *(Quaternion*)AsData == *(Quaternion*)other.AsData;
        case VariantType::Rectangle:
            return *(Rectangle*)AsData == *(Rectangle*)other.AsData;
        case VariantType::BoundingSphere:
            return *(BoundingSphere*)AsData == *(BoundingSphere*)other.AsData;
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
        case VariantType::ManagedObject:
            // TODO: invoke C# Equality logic?
            return AsUint == other.AsUint || mono_gchandle_get_target(AsUint) == mono_gchandle_get_target(other.AsUint);
        case VariantType::Typename:
            if (AsBlob.Data == nullptr && other.AsBlob.Data == nullptr)
                return true;
            if (AsBlob.Data == nullptr)
                return false;
            if (other.AsBlob.Data == nullptr)
                return false;
            return AsBlob.Length == other.AsBlob.Length && StringUtils::Compare(static_cast<const char*>(AsBlob.Data), static_cast<const char*>(other.AsBlob.Data), AsBlob.Length - 1) == 0;
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
        return AsUint != 0 && mono_gchandle_get_target(AsUint) != nullptr;
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
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (int8)AsInt16;
    case VariantType::Uint16:
        return (int8)AsUint16;
    case VariantType::Int:
        return (int8)AsInt;
    case VariantType::Uint:
        return (int8)AsUint;
    case VariantType::Int64:
        return (int8)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (int8)AsUint64;
    case VariantType::Float:
        return (int8)AsFloat;
    case VariantType::Double:
        return (int8)AsDouble;
    case VariantType::Pointer:
        return (int8)(intptr)AsPointer;
    default:
        return 0;
    }
}

Variant::operator int16() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return AsInt16;
    case VariantType::Uint16:
        return (int16)AsUint16;
    case VariantType::Int:
        return (int16)AsInt;
    case VariantType::Uint:
        return (int16)AsUint;
    case VariantType::Int64:
        return (int16)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (int16)AsUint64;
    case VariantType::Float:
        return (int16)AsFloat;
    case VariantType::Double:
        return (int16)AsDouble;
    case VariantType::Pointer:
        return (int16)(intptr)AsPointer;
    default:
        return 0;
    }
}

Variant::operator int32() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (int32)AsInt16;
    case VariantType::Uint16:
        return (int32)AsUint16;
    case VariantType::Int:
        return AsInt;
    case VariantType::Uint:
        return (int32)AsUint;
    case VariantType::Int64:
        return (int32)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (int32)AsUint64;
    case VariantType::Float:
        return (int32)AsFloat;
    case VariantType::Double:
        return (int32)AsDouble;
    case VariantType::Pointer:
        return (int32)(intptr)AsPointer;
    default:
        return 0;
    }
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
    default:
        return 0;
    }
}

Variant::operator uint8() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (uint8)AsInt16;
    case VariantType::Uint16:
        return (uint8)AsUint16;
    case VariantType::Int:
        return (uint8)AsInt;
    case VariantType::Uint:
        return (uint8)AsUint;
    case VariantType::Int64:
        return (uint8)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (uint8)AsUint64;
    case VariantType::Float:
        return (uint8)AsFloat;
    case VariantType::Double:
        return (uint8)AsDouble;
    case VariantType::Pointer:
        return (uint8)(uintptr)AsPointer;
    default:
        return 0;
    }
}

Variant::operator uint16() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (uint16)AsInt16;
    case VariantType::Uint16:
        return AsUint16;
    case VariantType::Int:
        return (uint16)AsInt;
    case VariantType::Uint:
        return (uint16)AsUint;
    case VariantType::Int64:
        return (uint16)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (uint16)AsUint64;
    case VariantType::Float:
        return (uint16)AsFloat;
    case VariantType::Double:
        return (uint16)AsDouble;
    case VariantType::Pointer:
        return (uint16)(uintptr)AsPointer;
    default:
        return 0;
    }
}

Variant::operator uint32() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return AsBool ? 1 : 0;
    case VariantType::Int16:
        return (uint32)AsInt16;
    case VariantType::Uint16:
        return (uint32)AsUint16;
    case VariantType::Int:
        return (uint32)AsInt;
    case VariantType::Uint:
        return AsUint;
    case VariantType::Int64:
        return (uint32)AsInt64;
    case VariantType::Uint64:
    case VariantType::Enum:
        return (uint32)AsUint64;
    case VariantType::Float:
        return (uint32)AsFloat;
    case VariantType::Double:
        return (uint32)AsDouble;
    case VariantType::Pointer:
        return (uint32)(uintptr)AsPointer;
    default:
        return 0;
    }
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
        return AsUint ? mono_gchandle_get_target(AsUint) : nullptr;
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

Variant::operator _MonoObject*() const
{
    return Type.Type == VariantType::ManagedObject && AsUint ? mono_gchandle_get_target(AsUint) : nullptr;
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

Variant::operator Vector2() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Vector2(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Vector2((float)AsInt16);
    case VariantType::Uint16:
        return Vector2((float)AsUint16);
    case VariantType::Int:
        return Vector2((float)AsInt);
    case VariantType::Uint:
        return Vector2((float)AsUint);
    case VariantType::Int64:
        return Vector2((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Vector2((float)AsUint64);
    case VariantType::Float:
        return Vector2(AsFloat);
    case VariantType::Double:
        return Vector2((float)AsDouble);
    case VariantType::Pointer:
        return Vector2((float)(intptr)AsPointer);
    case VariantType::Vector2:
        return *(Vector2*)AsData;
    case VariantType::Vector3:
        return Vector2(*(Vector3*)AsData);
    case VariantType::Vector4:
    case VariantType::Color:
        return Vector2(*(Vector4*)AsData);
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Vector2::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Vector2*)AsBlob.Data;
    default:
        return Vector2::Zero;
    }
}

Variant::operator Vector3() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Vector3(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Vector3((float)AsInt16);
    case VariantType::Uint16:
        return Vector3((float)AsUint16);
    case VariantType::Int:
        return Vector3((float)AsInt);
    case VariantType::Uint:
        return Vector3((float)AsUint);
    case VariantType::Int64:
        return Vector3((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Vector3((float)AsUint64);
    case VariantType::Float:
        return Vector3(AsFloat);
    case VariantType::Double:
        return Vector3((float)AsDouble);
    case VariantType::Pointer:
        return Vector3((float)(intptr)AsPointer);
    case VariantType::Vector2:
        return Vector3(*(Vector2*)AsData, 0.0f);
    case VariantType::Vector3:
        return *(Vector3*)AsData;
    case VariantType::Vector4:
    case VariantType::Color:
        return Vector3(*(Vector4*)AsData);
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Vector3::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Vector3*)AsBlob.Data;
    default:
        return Vector3::Zero;
    }
}

Variant::operator Vector4() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Vector4(AsBool ? 1.0f : 0.0f);
    case VariantType::Int16:
        return Vector4((float)AsInt16);
    case VariantType::Uint16:
        return Vector4((float)AsUint16);
    case VariantType::Int:
        return Vector4((float)AsInt);
    case VariantType::Uint:
        return Vector4((float)AsUint);
    case VariantType::Int64:
        return Vector4((float)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Vector4((float)AsUint64);
    case VariantType::Float:
        return Vector4(AsFloat);
    case VariantType::Double:
        return Vector4((float)AsDouble);
    case VariantType::Pointer:
        return Vector4((float)(intptr)AsPointer);
    case VariantType::Vector2:
        return Vector4(*(Vector2*)AsData, 0.0f, 0.0f);
    case VariantType::Vector3:
        return Vector4(*(Vector3*)AsData, 0.0f);
    case VariantType::Vector4:
    case VariantType::Color:
        return *(Vector4*)AsData;
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Vector4::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Vector4*)AsBlob.Data;
    default:
        return Vector4::Zero;
    }
}

Variant::operator Int2() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Int2((int32)(AsBool ? 1.0f : 0.0f));
    case VariantType::Int16:
        return Int2((int32)AsInt16);
    case VariantType::Uint16:
        return Int2((int32)AsUint16);
    case VariantType::Int:
        return Int2((int32)AsInt);
    case VariantType::Uint:
        return Int2((int32)AsUint);
    case VariantType::Int64:
        return Int2((int32)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Int2((int32)AsUint64);
    case VariantType::Float:
        return Int2((int32)AsFloat);
    case VariantType::Double:
        return Int2((int32)AsDouble);
    case VariantType::Pointer:
        return Int2((int32)(intptr)AsPointer);
    case VariantType::Vector2:
        return Int2(*(Vector2*)AsData);
    case VariantType::Vector3:
        return Int2(*(Vector3*)AsData);
    case VariantType::Vector4:
        return Int2(*(Vector4*)AsData);
    case VariantType::Int2:
        return Int2(*(Int2*)AsData);
    case VariantType::Int3:
        return Int2(*(Int3*)AsData);
    case VariantType::Int4:
    case VariantType::Color:
        return Int2(*(Int4*)AsData);
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Int2::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Int2*)AsBlob.Data;
    default:
        return Int3::Zero;
    }
}

Variant::operator Int3() const
{
    switch (Type.Type)
    {
    case VariantType::Bool:
        return Int3((int32)(AsBool ? 1 : 0));
    case VariantType::Int16:
        return Int3((int32)AsInt16);
    case VariantType::Uint16:
        return Int3((int32)AsUint16);
    case VariantType::Int:
        return Int3((int32)AsInt);
    case VariantType::Uint:
        return Int3((int32)AsUint);
    case VariantType::Int64:
        return Int3((int32)AsInt64);
    case VariantType::Uint64:
    case VariantType::Enum:
        return Int3((int32)AsUint64);
    case VariantType::Float:
        return Int3((int32)AsFloat);
    case VariantType::Double:
        return Int3((int32)AsDouble);
    case VariantType::Pointer:
        return Int3((int32)(intptr)AsPointer);
    case VariantType::Vector2:
        return Int3(*(Vector2*)AsData, 0);
    case VariantType::Vector3:
        return Int3(*(Vector3*)AsData);
    case VariantType::Vector4:
        return Int3(*(Vector4*)AsData);
    case VariantType::Int2:
        return Int3(*(Int2*)AsData, 0);
    case VariantType::Int3:
        return Int3(*(Int3*)AsData);
    case VariantType::Int4:
    case VariantType::Color:
        return Int3(*(Int4*)AsData);
    case VariantType::Structure:
        if (StringUtils::Compare(Type.TypeName, Int3::TypeInitializer.GetType().Fullname.Get()) == 0)
            return *(Int3*)AsBlob.Data;
    default:
        return Int3::Zero;
    }
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
    case VariantType::Vector2:
        return Int4(*(Vector2*)AsData, 0, 0);
    case VariantType::Vector3:
        return Int4(*(Vector3*)AsData, 0);
    case VariantType::Vector4:
        return Int4(*(Vector4*)AsData);
    case VariantType::Int2:
        return Int4(*(Int2*)AsData, 0, 0);
    case VariantType::Int3:
        return Int4(*(Int3*)AsData, 0);
    case VariantType::Int4:
    case VariantType::Color:
        return *(Int4*)AsData;
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
    case VariantType::Vector2:
        return Color((*(Vector2*)AsData).X, (*(Vector2*)AsData).Y, 0.0f, 1.0f);
    case VariantType::Vector3:
        return Color(*(Vector3*)AsData, 1.0f);
    case VariantType::Vector4:
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
    case VariantType::Vector3:
        return Quaternion::Euler(*(Vector3*)AsData);
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
        return *(BoundingSphere*)AsData;
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
        return *(BoundingBox*)AsBlob.Data;
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
        return *(Ray*)AsBlob.Data;
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
    return *(const Vector4*)AsData;
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
    case VariantType::BoundingBox:
    case VariantType::Transform:
    case VariantType::Ray:
    case VariantType::Matrix:
    case VariantType::Typename:
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
        if (AsUint)
            mono_gchandle_free(AsUint);
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
    case VariantType::BoundingBox:
        AsBlob.Data = Allocator::Allocate(sizeof(BoundingBox));
        AsBlob.Length = sizeof(BoundingBox);
        break;
    case VariantType::Transform:
        AsBlob.Data = Allocator::Allocate(sizeof(Transform));
        AsBlob.Length = sizeof(Transform);
        break;
    case VariantType::Ray:
        AsBlob.Data = Allocator::Allocate(sizeof(Ray));
        AsBlob.Length = sizeof(Ray);
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
        AsUint = 0;
        break;
    case VariantType::Structure:
        AllocStructure();
        break;
    default: ;
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
    case VariantType::BoundingBox:
    case VariantType::Transform:
    case VariantType::Ray:
    case VariantType::Matrix:
    case VariantType::Typename:
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
        if (AsUint)
            mono_gchandle_free(AsUint);
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
    case VariantType::BoundingBox:
        AsBlob.Data = Allocator::Allocate(sizeof(BoundingBox));
        AsBlob.Length = sizeof(BoundingBox);
        break;
    case VariantType::Transform:
        AsBlob.Data = Allocator::Allocate(sizeof(Transform));
        AsBlob.Length = sizeof(Transform);
        break;
    case VariantType::Ray:
        AsBlob.Data = Allocator::Allocate(sizeof(Ray));
        AsBlob.Length = sizeof(Ray);
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
        AsUint = 0;
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
    const int32 length = str.Length() * sizeof(Char) + 2;
    if (AsBlob.Length != length)
    {
        Allocator::Free(AsBlob.Data);
        AsBlob.Data = Allocator::Allocate(length);
        AsBlob.Length = length;
    }
    StringUtils::ConvertANSI2UTF16(str.Get(), (Char*)AsBlob.Data, str.Length());
    ((Char*)AsBlob.Data)[str.Length()] = 0;
}

void Variant::SetTypename(const StringView& typeName)
{
    SetType(VariantType(VariantType::Typename));
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

void Variant::SetManagedObject(_MonoObject* object)
{
    if (object)
    {
        if (Type.Type != VariantType::ManagedObject)
            SetType(VariantType(VariantType::ManagedObject, mono_object_get_class(object)));
        AsUint = mono_gchandle_new(object, true);
    }
    else
    {
        if (Type.Type != VariantType::ManagedObject || Type.TypeName)
            SetType(VariantType(VariantType::ManagedObject));
        AsUint = 0;
    }
}

void Variant::SetAsset(Asset* asset)
{
    if (Type.Type != VariantType::Asset)
        SetType(VariantType(VariantType::Asset));
    if (AsAsset)
    {
        asset->OnUnloaded.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
        asset->RemoveReference();
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
    case VariantType::Enum:
        return StringUtils::ToString(AsUint);
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
    case VariantType::Vector2:
        return (*(Vector2*)AsData).ToString();
    case VariantType::Vector3:
        return (*(Vector3*)AsData).ToString();
    case VariantType::Vector4:
        return (*(Vector4*)AsData).ToString();
    case VariantType::Color:
        return (*(Color*)AsData).ToString();
    case VariantType::Guid:
        return (*(Guid*)AsData).ToString();
    case VariantType::BoundingSphere:
        return (*(BoundingSphere*)AsData).ToString();
    case VariantType::Quaternion:
        return (*(Quaternion*)AsData).ToString();
    case VariantType::Rectangle:
        return (*(Rectangle*)AsData).ToString();
    case VariantType::BoundingBox:
        return (*(BoundingBox*)AsBlob.Data).ToString();
    case VariantType::Transform:
        return (*(Transform*)AsBlob.Data).ToString();
    case VariantType::Ray:
        return (*(Ray*)AsBlob.Data).ToString();
    case VariantType::Matrix:
        return (*(Matrix*)AsBlob.Data).ToString();
    case VariantType::ManagedObject:
        return AsUint ? String(MUtils::ToString(mono_object_to_string(mono_gchandle_get_target(AsUint), nullptr))) : TEXT("null");
    case VariantType::Typename:
        return String((const char*)AsBlob.Data, AsBlob.Length ? AsBlob.Length - 1 : 0);
    default:
        return String::Empty;
    }
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
            return true;
        default:
            return false;
        }
    case VariantType::Vector2:
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
        case VariantType::Vector3:
        case VariantType::Vector4:
        case VariantType::Color:
            return true;
        default:
            return false;
        }
    case VariantType::Vector3:
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
        case VariantType::Vector2:
        case VariantType::Vector4:
        case VariantType::Color:
            return true;
        default:
            return false;
        }
    case VariantType::Vector4:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Color:
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
        case VariantType::Vector2:
        case VariantType::Vector3:
        case VariantType::Vector4:
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
        case VariantType::Vector2:
            return Variant(Vector2(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Vector3:
            return Variant(Vector3(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Vector4:
            return Variant(Vector4(v.AsBool ? 1.0f : 0.0f));
        case VariantType::Color:
            return Variant(Color(v.AsBool ? 1.0f : 0.0f));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsInt16));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsInt16));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsInt16));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt16));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsInt));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsInt));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsInt));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsUint16));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsUint16));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsUint16));
        case VariantType::Color:
            return Variant(Color((float)v.AsUint16));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsUint));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsUint));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsUint));
        case VariantType::Color:
            return Variant(Color((float)v.AsUint));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsInt64));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsInt64));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsInt64));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt64));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsInt));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsInt));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsInt));
        case VariantType::Color:
            return Variant(Color((float)v.AsInt));
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
        case VariantType::Vector2:
            return Variant(Vector2(v.AsFloat));
        case VariantType::Vector3:
            return Variant(Vector3(v.AsFloat));
        case VariantType::Vector4:
            return Variant(Vector4(v.AsFloat));
        case VariantType::Color:
            return Variant(Color(v.AsFloat));
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
        case VariantType::Vector2:
            return Variant(Vector2((float)v.AsDouble));
        case VariantType::Vector3:
            return Variant(Vector3((float)v.AsDouble));
        case VariantType::Vector4:
            return Variant(Vector4((float)v.AsDouble));
        case VariantType::Color:
            return Variant(Color((float)v.AsDouble));
        default: ;
        }
        break;
    case VariantType::Vector2:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Vector2*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Vector2*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Vector2*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Vector2*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Vector2*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Vector2*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Vector2*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Vector2*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Vector2*)v.AsData)->X);
        case VariantType::Vector3:
            return Variant(Vector3(*(Vector2*)v.AsData, 0.0f));
        case VariantType::Vector4:
            return Variant(Vector4(*(Vector2*)v.AsData, 0.0f, 0.0f));
        case VariantType::Color:
            return Variant(Color(((Vector2*)v.AsData)->X, ((Vector2*)v.AsData)->Y, 0.0f, 0.0f));
        default: ;
        }
        break;
    case VariantType::Vector3:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Vector3*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Vector3*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Vector3*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Vector3*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Vector3*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Vector3*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Vector3*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Vector3*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Vector3*)v.AsData)->X);
        case VariantType::Vector2:
            return Variant(Vector2(*(Vector3*)v.AsData));
        case VariantType::Vector4:
            return Variant(Vector4(*(Vector3*)v.AsData, 0.0f));
        case VariantType::Color:
            return Variant(Color(((Vector3*)v.AsData)->X, ((Vector3*)v.AsData)->Y, ((Vector3*)v.AsData)->Z, 0.0f));
        default: ;
        }
        break;
    case VariantType::Vector4:
        switch (to.Type)
        {
        case VariantType::Bool:
            return Variant(Math::Abs(((Vector4*)v.AsData)->X) > ZeroTolerance);
        case VariantType::Int16:
            return Variant((int16)((Vector4*)v.AsData)->X);
        case VariantType::Int:
            return Variant((int32)((Vector4*)v.AsData)->X);
        case VariantType::Uint16:
            return Variant((uint16)((Vector4*)v.AsData)->X);
        case VariantType::Uint:
            return Variant((uint32)((Vector4*)v.AsData)->X);
        case VariantType::Int64:
            return Variant((int64)((Vector4*)v.AsData)->X);
        case VariantType::Uint64:
            return Variant((uint64)((Vector4*)v.AsData)->X);
        case VariantType::Float:
            return Variant((float)((Vector4*)v.AsData)->X);
        case VariantType::Double:
            return Variant((double)((Vector4*)v.AsData)->X);
        case VariantType::Vector2:
            return Variant(Vector2(*(Vector4*)v.AsData));
        case VariantType::Vector3:
            return Variant(Vector3(*(Vector4*)v.AsData));
        case VariantType::Color:
            return Variant(*(Vector4*)v.AsData);
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
        case VariantType::Vector2:
            return Variant(Vector2(*(Color*)v.AsData));
        case VariantType::Vector3:
            return Variant(Vector3(*(Color*)v.AsData));
        case VariantType::Vector4:
            return Variant(*(Color*)v.AsData);
        default: ;
        }
        break;
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
    case VariantType::Vector2:
        return Vector2::NearEqual(*(Vector2*)a.AsData, *(Vector2*)b.AsData, epsilon);
    case VariantType::Vector3:
        return Vector3::NearEqual(*(Vector3*)a.AsData, *(Vector3*)b.AsData, epsilon);
    case VariantType::Vector4:
        return Vector4::NearEqual(*(Vector4*)a.AsData, *(Vector4*)b.AsData, epsilon);
    case VariantType::Color:
        return Color::NearEqual(*(Color*)a.AsData, *(Color*)b.AsData, epsilon);
    case VariantType::BoundingSphere:
        return BoundingSphere::NearEqual(*(BoundingSphere*)a.AsData, *(BoundingSphere*)b.AsData, epsilon);
    case VariantType::Quaternion:
        return Quaternion::NearEqual(*(Quaternion*)a.AsData, *(Quaternion*)b.AsData, epsilon);
    case VariantType::Rectangle:
        return Rectangle::NearEqual(*(Rectangle*)a.AsData, *(Rectangle*)b.AsData, epsilon);
    case VariantType::BoundingBox:
        return BoundingBox::NearEqual(*(BoundingBox*)a.AsBlob.Data, *(BoundingBox*)b.AsBlob.Data, epsilon);
    case VariantType::Transform:
        return Transform::NearEqual(*(Transform*)a.AsBlob.Data, *(Transform*)b.AsBlob.Data, epsilon);
    case VariantType::Ray:
        return Ray::NearEqual(*(Ray*)a.AsBlob.Data, *(Ray*)b.AsBlob.Data, epsilon);
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
    case VariantType::Vector2:
        return Vector2::Lerp(*(Vector2*)a.AsData, *(Vector2*)b.AsData, alpha);
    case VariantType::Vector3:
        return Vector3::Lerp(*(Vector3*)a.AsData, *(Vector3*)b.AsData, alpha);
    case VariantType::Vector4:
        return Vector4::Lerp(*(Vector4*)a.AsData, *(Vector4*)b.AsData, alpha);
    case VariantType::Color:
        return Color::Lerp(*(Color*)a.AsData, *(Color*)b.AsData, alpha);
    case VariantType::Quaternion:
        return Quaternion::Lerp(*(Quaternion*)a.AsData, *(Quaternion*)b.AsData, alpha);
    case VariantType::BoundingSphere:
        return BoundingSphere(Vector3::Lerp((*(BoundingSphere*)a.AsData).Center, (*(BoundingSphere*)b.AsData).Center, alpha), Math::Lerp((*(BoundingSphere*)a.AsData).Radius, (*(BoundingSphere*)b.AsData).Radius, alpha));
    case VariantType::Rectangle:
        return Rectangle(Vector2::Lerp((*(Rectangle*)a.AsData).Location, (*(Rectangle*)b.AsData).Location, alpha), Vector2::Lerp((*(Rectangle*)a.AsData).Size, (*(Rectangle*)b.AsData).Size, alpha));
    case VariantType::Transform:
        return Variant(Transform::Lerp(*(Transform*)a.AsBlob.Data, *(Transform*)b.AsBlob.Data, alpha));
    case VariantType::BoundingBox:
        return Variant(BoundingBox(Vector3::Lerp((*(BoundingBox*)a.AsBlob.Data).Minimum, (*(BoundingBox*)b.AsBlob.Data).Minimum, alpha), Vector3::Lerp((*(BoundingBox*)a.AsBlob.Data).Maximum, (*(BoundingBox*)b.AsBlob.Data).Maximum, alpha)));
    case VariantType::Ray:
        return Variant(Ray(Vector3::Lerp((*(Ray*)a.AsBlob.Data).Position, (*(Ray*)b.AsBlob.Data).Position, alpha), Vector3::Normalize(Vector3::Lerp((*(Ray*)a.AsBlob.Data).Direction, (*(Ray*)b.AsBlob.Data).Direction, alpha))));
    default:
        return a;
    }
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
    else
    {
        if (typeName.Length() != 0)
        {
            LOG(Warning, "Missing scripting type \'{0}\'", String(typeName.Get()));
        }
        AsBlob.Data = nullptr;
        AsBlob.Length = 0;
    }
}

void Variant::CopyStructure(void* src)
{
    if (AsBlob.Data && src)
    {
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(Type.TypeName));
        if (typeHandle)
        {
            auto& type = typeHandle.GetType();
            type.Struct.Copy(AsBlob.Data, src);
        }
        else
        {
            Platform::MemoryCopy(AsBlob.Data, src, AsBlob.Length);
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
    case VariantType::ManagedObject:
        return key.AsUint ? (uint32)mono_object_hash(mono_gchandle_get_target(key.AsUint)) : 0;
    case VariantType::Typename:
        return GetHash((const char*)key.AsBlob.Data);
    default:
        return 0;
    }
}
