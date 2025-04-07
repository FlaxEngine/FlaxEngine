// Copyright (c) Wojciech Figat. All rights reserved.

#include "ReadStream.h"
#include "WriteStream.h"
#include "JsonWriters.h"
#include "JsonSerializer.h"
#include "MemoryReadStream.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/Asset.h"
#include "Engine/Core/Cache.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Internal/ManagedSerialization.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"

void ReadStream::Read(StringAnsi& data)
{
    int32 length;
    ReadInt32(&length);
    if (length < 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        _hasError = true;
        data = "";
        return;
    }

    data.ReserveSpace(length);
    if (length == 0)
        return;
    char* ptr = data.Get();
    ASSERT(ptr != nullptr);
    ReadBytes(ptr, length);
}

void ReadStream::Read(StringAnsi& data, int8 lock)
{
    int32 length;
    ReadInt32(&length);
    if (length < 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        _hasError = true;
        data = "";
        return;
    }

    data.ReserveSpace(length);
    if (length == 0)
        return;
    char* ptr = data.Get();
    ASSERT(ptr != nullptr);
    ReadBytes(ptr, length);

    for (int32 i = 0; i < length; i++)
    {
        *ptr = *ptr ^ lock;
        ptr++;
    }
}

void ReadStream::Read(String& data)
{
    int32 length;
    ReadInt32(&length);
    if (length <= 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        if (length != 0)
            _hasError = true;
        data.Clear();
        return;
    }

    data.ReserveSpace(length);
    Char* ptr = data.Get();
    ASSERT(ptr != nullptr);
    ReadBytes(ptr, length * sizeof(Char));
}

void ReadStream::Read(String& data, int16 lock)
{
    int32 length;
    ReadInt32(&length);
    if (length <= 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        if (length != 0)
            _hasError = true;
        data.Clear();
        return;
    }

    data.ReserveSpace(length);
    Char* ptr = data.Get();
    ASSERT(ptr != nullptr);
    ReadBytes(ptr, length * sizeof(Char));

    for (int32 i = 0; i < length; i++)
    {
        *ptr = *ptr ^ lock;
        ptr++;
    }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void ReadStream::Read(CommonValue& data)
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    byte type;
    ReadByte(&type);
    switch (static_cast<CommonType>(type))
    {
    case CommonType::Bool:
        data.Set(ReadBool());
        break;
    case CommonType::Integer:
    {
        int32 v;
        ReadInt32(&v);
        data.Set(v);
    }
    break;
    case CommonType::Float:
    {
        float v;
        ReadFloat(&v);
        data.Set(v);
    }
    break;
    case CommonType::Vector2:
    {
        Float2 v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Vector3:
    {
        Float3 v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Vector4:
    {
        Float4 v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Color:
    {
        Color v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Guid:
    {
        Guid v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::String:
    {
        String v;
        Read(v, 953);
        data.Set(v);
    }
    break;
    case CommonType::Box:
    {
        BoundingBox v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Rotation:
    {
        Quaternion v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Transform:
    {
        Transform v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Sphere:
    {
        BoundingSphere v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Rectangle:
    {
        Rectangle v;
        Read(v);
        data.Set(v);
    }
    case CommonType::Ray:
    {
        Ray v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Matrix:
    {
        Matrix v;
        Read(v);
        data.Set(v);
    }
    break;
    case CommonType::Blob:
    {
        int32 length;
        Read(length);
        data.SetBlob(length);
        if (length > 0)
        {
            ReadBytes(data.AsBlob.Data, length);
        }
    }
    break;
    default: CRASH;
    }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void ReadStream::Read(VariantType& data)
{
    data = VariantType((VariantType::Types)ReadByte());
    int32 typeNameLength;
    ReadInt32(&typeNameLength);
    if (typeNameLength == MAX_int32)
    {
        ReadInt32(&typeNameLength);
        if (typeNameLength == 0)
            return;
        data.TypeName = static_cast<char*>(Allocator::Allocate(typeNameLength + 1));
        char* ptr = data.TypeName;
        ReadBytes(ptr, typeNameLength);
        for (int32 i = 0; i < typeNameLength; i++)
        {
            *ptr = *ptr ^ 77;
            ptr++;
        }
        *ptr = 0;
    }
    else if (typeNameLength > 0)
    {
        // [Deprecated on 27.08.2020, expires on 27.08.2021]
        ASSERT(typeNameLength < STREAM_MAX_STRING_LENGTH);
        Array<Char> chars;
        chars.Resize(typeNameLength + 1);
        Char* ptr = chars.Get();
        ReadBytes(ptr, typeNameLength * sizeof(Char));
        for (int32 i = 0; i < typeNameLength; i++)
        {
            *ptr = *ptr ^ 77;
            ptr++;
        }
        *ptr = 0;
        data.TypeName = static_cast<char*>(Allocator::Allocate(typeNameLength + 1));
        StringUtils::ConvertUTF162ANSI(chars.Get(), data.TypeName, typeNameLength);
        data.TypeName[typeNameLength] = 0;
    }
}

void ReadStream::Read(Variant& data)
{
    VariantType type;
    Read(type);
    data.SetType(MoveTemp(type));
    switch (data.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
        break;
    case VariantType::Bool:
        data.AsBool = ReadBool();
        break;
    case VariantType::Int16:
        ReadInt16(&data.AsInt16);
        break;
    case VariantType::Uint16:
        ReadUint16(&data.AsUint16);
        break;
    case VariantType::Int:
        ReadInt32(&data.AsInt);
        break;
    case VariantType::Uint:
        ReadUint32(&data.AsUint);
        break;
    case VariantType::Int64:
        ReadInt64(&data.AsInt64);
        break;
    case VariantType::Uint64:
    case VariantType::Enum:
        ReadUint64(&data.AsUint64);
        break;
    case VariantType::Float:
        ReadFloat(&data.AsFloat);
        break;
    case VariantType::Double:
        ReadDouble(&data.AsDouble);
        break;
    case VariantType::Pointer:
    {
        uint64 asUint64;
        ReadUint64(&asUint64);
        data.AsPointer = (void*)(uintptr)asUint64;
        break;
    }
    case VariantType::String:
    {
        int32 length;
        ReadInt32(&length);
        ASSERT(length < STREAM_MAX_STRING_LENGTH);
        const int32 dataLength = length * sizeof(Char) + 2;
        if (data.AsBlob.Length != dataLength)
        {
            Allocator::Free(data.AsBlob.Data);
            data.AsBlob.Data = dataLength > 0 ? Allocator::Allocate(dataLength) : nullptr;
            data.AsBlob.Length = dataLength;
        }
        Char* ptr = (Char*)data.AsBlob.Data;
        ReadBytes(ptr, length * sizeof(Char));
        for (int32 i = 0; i < length; i++)
        {
            *ptr = *ptr ^ -14;
            ptr++;
        }
        *ptr = 0;
        break;
    }
    case VariantType::Object:
    {
        Guid id;
        Read(id);
        data.SetObject(FindObject(id, ScriptingObject::GetStaticClass()));
        break;
    }
    case VariantType::ManagedObject:
    case VariantType::Structure:
    {
        const byte format = ReadByte();
        if (format == 0)
        {
            // No data
        }
        else if (format == 1)
        {
            // Json
            StringAnsi json;
            Read(json, -71);
#if USE_CSHARP
            MCore::Thread::Attach();
            MClass* klass = MUtils::GetClass(data.Type);
            if (!klass)
            {
                LOG(Error, "Invalid variant type {0}", data.Type);
                return;
            }
            MObject* obj = MCore::Object::New(klass);
            if (!obj)
            {
                LOG(Error, "Failed to managed instance of the variant type {0}", data.Type);
                return;
            }
            if (!klass->IsValueType())
                MCore::Object::Init(obj);
            ManagedSerialization::Deserialize(json, obj);
            if (data.Type.Type == VariantType::ManagedObject)
                data.SetManagedObject(obj);
            else
                data = MUtils::UnboxVariant(obj);
#endif
        }
        else
        {
            LOG(Error, "Invalid Variant {0) format {1}", data.Type.ToString(), format);
        }
        break;
    }
    case VariantType::Blob:
    {
        int32 length;
        ReadInt32(&length);
        data.SetBlob(length);
        ReadBytes(data.AsBlob.Data, length);
        break;
    }
    case VariantType::Asset:
    {
        Guid id;
        Read(id);
        data.SetAsset(LoadAsset(id, Asset::TypeInitializer));
        break;
    }
    case VariantType::Float2:
        ReadBytes(&data.AsData, sizeof(Float2));
        break;
    case VariantType::Float3:
        ReadBytes(&data.AsData, sizeof(Float3));
        break;
    case VariantType::Float4:
        ReadBytes(&data.AsData, sizeof(Float4));
        break;
    case VariantType::Double2:
        ReadBytes(&data.AsData, sizeof(Double2));
        break;
    case VariantType::Double3:
        ReadBytes(&data.AsData, sizeof(Double3));
        break;
    case VariantType::Double4:
        ReadBytes(data.AsBlob.Data, sizeof(Double4));
        break;
    case VariantType::Color:
        ReadBytes(&data.AsData, sizeof(Color));
        break;
    case VariantType::Guid:
        ReadBytes(&data.AsData, sizeof(Guid));
        break;
    case VariantType::BoundingBox:
        Read(data.AsBoundingBox());
        break;
    case VariantType::BoundingSphere:
        Read(data.AsBoundingSphere());
        break;
    case VariantType::Quaternion:
        ReadBytes(&data.AsData, sizeof(Quaternion));
        break;
    case VariantType::Transform:
        Read(data.AsTransform());
        break;
    case VariantType::Rectangle:
        ReadBytes(&data.AsData, sizeof(Rectangle));
        break;
    case VariantType::Ray:
        Read(data.AsRay());
        break;
    case VariantType::Matrix:
        ReadBytes(data.AsBlob.Data, sizeof(Matrix));
        break;
    case VariantType::Array:
    {
        int32 count;
        Read(count);
        auto& array = *(Array<Variant>*)data.AsData;
        array.Resize(count);
        for (int32 i = 0; i < count; i++)
            Read(array[i]);
        break;
    }
    case VariantType::Dictionary:
    {
        int32 count;
        Read(count);
        auto& dictionary = *data.AsDictionary;
        dictionary.Clear();
        dictionary.EnsureCapacity(count);
        for (int32 i = 0; i < count; i++)
        {
            Variant key;
            Read(key);
            Read(dictionary[MoveTemp(key)]);
        }
        break;
    }
    case VariantType::Typename:
    {
        int32 length;
        ReadInt32(&length);
        ASSERT(length < STREAM_MAX_STRING_LENGTH);
        const int32 dataLength = length + 1;
        if (data.AsBlob.Length != dataLength)
        {
            Allocator::Free(data.AsBlob.Data);
            data.AsBlob.Data = dataLength > 0 ? Allocator::Allocate(dataLength) : nullptr;
            data.AsBlob.Length = dataLength;
        }
        char* ptr = (char*)data.AsBlob.Data;
        ReadBytes(ptr, length);
        for (int32 i = 0; i < length; i++)
        {
            *ptr = *ptr ^ -14;
            ptr++;
        }
        *ptr = 0;
        break;
    }
    default:
        _hasError = true;
        LOG(Error, "Invalid Variant type. Corrupted data.");
        break;
    }
}

void ReadStream::ReadJson(ISerializable* obj)
{
    int32 engineBuild, size;
    ReadInt32(&engineBuild);
    ReadInt32(&size);
    if (obj)
    {
        if (const auto memoryStream = dynamic_cast<MemoryReadStream*>(this))
        {
            JsonSerializer::LoadFromBytes(obj, Span<byte>((byte*)memoryStream->Move(size), size), engineBuild);
        }
        else
        {
            void* data = Allocator::Allocate(size);
            ReadBytes(data, size);
            JsonSerializer::LoadFromBytes(obj, Span<byte>((byte*)data, size), engineBuild);
            Allocator::Free(data);
        }
    }
    else
        SetPosition(GetPosition() + size);
}

void ReadStream::ReadStringAnsi(StringAnsi* data)
{
    Read(*data);
}

void ReadStream::ReadStringAnsi(StringAnsi* data, int8 lock)
{
    Read(*data, lock);
}

void ReadStream::ReadString(String* data)
{
    Read(*data);
}

void ReadStream::ReadString(String* data, int16 lock)
{
    Read(*data, lock);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void ReadStream::ReadCommonValue(CommonValue* data)
{
    Read(*data);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void ReadStream::ReadVariantType(VariantType* data)
{
    Read(*data);
}

void ReadStream::ReadVariant(Variant* data)
{
    Read(*data);
}

void ReadStream::Read(BoundingBox& box, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        ReadBytes(&box, sizeof(BoundingBox));
    else
    {
        Float3 min, max;
        Read(min);
        Read(max);
        box.Minimum = min;
        box.Maximum = max;
    }
#else
    if (useDouble)
    {
        Double3 min, max;
        Read(min);
        Read(max);
        box.Minimum = min;
        box.Maximum = max;
    }
    else
        ReadBytes(&box, sizeof(BoundingBox));
#endif
}

void ReadStream::Read(BoundingSphere& sphere, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        ReadBytes(&sphere, sizeof(BoundingSphere));
    else
    {
        Float3 center;
        float radius;
        Read(center);
        Read(radius);
        sphere.Center = center;
        sphere.Radius = radius;
    }
#else
    if (useDouble)
    {
        Double3 center;
        double radius;
        Read(center);
        Read(radius);
        sphere.Center = center;
        sphere.Radius = (float)radius;
    }
    else
        ReadBytes(&sphere, sizeof(BoundingSphere));
#endif
}

void ReadStream::Read(Transform& transform, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        ReadBytes(&transform, sizeof(Transform));
    else
    {
        Float3 translation;
        Read(translation);
        Read(transform.Orientation);
        Read(transform.Scale);
        transform.Translation = translation;
    }
#else
    if (useDouble)
    {
        Double3 translation;
        Read(translation);
        Read(transform.Orientation);
        Read(transform.Scale);
        transform.Translation = translation;
    }
    else
        ReadBytes(&transform, sizeof(Transform));
#endif
}

void ReadStream::Read(Ray& ray, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        ReadBytes(&ray, sizeof(Ray));
    else
    {
        Float3 position, direction;
        Read(position);
        Read(direction);
        ray.Position = position;
        ray.Direction = direction;
    }
#else
    if (useDouble)
    {
        Double3 position, direction;
        Read(position);
        Read(direction);
        ray.Position = position;
        ray.Direction = direction;
    }
    else
        ReadBytes(&ray, sizeof(Ray));
#endif
}

void ReadStream::ReadBoundingBox(BoundingBox* box, bool useDouble)
{
    Read(*box);
}

void ReadStream::ReadBoundingSphere(BoundingSphere* sphere, bool useDouble)
{
    Read(*sphere);
}

void ReadStream::ReadTransform(Transform* transform, bool useDouble)
{
    Read(*transform);
}

void ReadStream::ReadRay(Ray* ray, bool useDouble)
{
    Read(*ray);
}

void WriteStream::WriteText(const StringView& text)
{
    WriteBytes(text.Get(), sizeof(Char) * text.Length());
}

void WriteStream::WriteText(const StringAnsiView& text)
{
    WriteBytes(text.Get(), sizeof(char) * text.Length());
}

void WriteStream::Write(const StringView& data)
{
    const int32 length = data.Length();
    ASSERT(length < STREAM_MAX_STRING_LENGTH);
    WriteInt32(length);
    WriteBytes(*data, length * sizeof(Char));
}

void WriteStream::Write(const StringView& data, int16 lock)
{
    ASSERT(data.Length() < STREAM_MAX_STRING_LENGTH);
    WriteInt32(data.Length());
    for (int32 i = 0; i < data.Length(); i++)
        WriteUint16((uint16)((uint16)data[i] ^ lock));
}

void WriteStream::Write(const StringAnsiView& data)
{
    const int32 length = data.Length();
    ASSERT(length < STREAM_MAX_STRING_LENGTH);
    WriteInt32(length);
    WriteBytes(data.Get(), length);
}

void WriteStream::Write(const StringAnsiView& data, int8 lock)
{
    const int32 length = data.Length();
    ASSERT(length < STREAM_MAX_STRING_LENGTH);
    WriteInt32(length);
    for (int32 i = 0; i < length; i++)
        WriteUint8((uint8)((uint8)data[i] ^ lock));
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void WriteStream::Write(const CommonValue& data)
{
    // [Deprecated on 31.07.2020, expires on 31.07.2022]
    WriteByte(static_cast<byte>(data.Type));
    switch (data.Type)
    {
    case CommonType::Bool:
        Write(data.AsBool);
        break;
    case CommonType::Integer:
        Write(data.AsInteger);
        break;
    case CommonType::Float:
        Write(data.AsFloat);
        break;
    case CommonType::Vector2:
        Write(data.AsVector2);
        break;
    case CommonType::Vector3:
        Write(data.AsVector3);
        break;
    case CommonType::Vector4:
        Write(data.AsVector4);
        break;
    case CommonType::Color:
        Write(data.AsColor);
        break;
    case CommonType::Guid:
        Write(data.AsGuid);
        break;
    case CommonType::String:
        Write(data.AsString, 953);
        break;
    case CommonType::Box:
        Write(data.AsBox);
        break;
    case CommonType::Rotation:
        Write(data.AsRotation);
        break;
    case CommonType::Transform:
        Write(data.AsTransform);
        break;
    case CommonType::Sphere:
        Write(data.AsSphere);
        break;
    case CommonType::Rectangle:
        Write(data.AsRectangle);
        break;
    case CommonType::Ray:
        Write(data.AsRay);
        break;
    case CommonType::Matrix:
        Write(data.AsMatrix);
        break;
    case CommonType::Blob:
        WriteInt32(data.AsBlob.Length);
        if (data.AsBlob.Length > 0)
            WriteBytes(data.AsBlob.Data, data.AsBlob.Length);
        break;
    default: CRASH;
    }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void WriteStream::Write(const VariantType& data)
{
    WriteByte((byte)data.Type);
    WriteInt32(MAX_int32);
    Write(StringAnsiView(data.TypeName), 77);
}

void WriteStream::Write(const Variant& data)
{
    Write(data.Type);
    Guid id;
    switch (data.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
        break;
    case VariantType::Bool:
        WriteBool(data.AsBool);
        break;
    case VariantType::Int16:
        WriteInt16(data.AsInt16);
        break;
    case VariantType::Uint16:
        WriteUint16(data.AsUint16);
        break;
    case VariantType::Int:
        WriteInt32(data.AsInt);
        break;
    case VariantType::Uint:
        WriteUint32(data.AsUint);
        break;
    case VariantType::Int64:
        WriteInt64(data.AsInt64);
        break;
    case VariantType::Uint64:
    case VariantType::Enum:
        WriteUint64(data.AsUint64);
        break;
    case VariantType::Float:
        WriteFloat(data.AsFloat);
        break;
    case VariantType::Double:
        WriteDouble(data.AsDouble);
        break;
    case VariantType::Pointer:
        WriteUint64((uint64)(uintptr)data.AsPointer);
        break;
    case VariantType::String:
        Write((StringView)data, -14);
        break;
    case VariantType::Object:
        id = data.AsObject ? data.AsObject->GetID() : Guid::Empty;
        Write(id);
        break;
    case VariantType::Blob:
        WriteInt32(data.AsBlob.Length);
        WriteBytes(data.AsBlob.Data, data.AsBlob.Length);
        break;
    case VariantType::BoundingBox:
        Write(data.AsBoundingBox());
        break;
    case VariantType::Transform:
        Write(data.AsTransform());
        break;
    case VariantType::Ray:
        Write(data.AsRay());
        break;
    case VariantType::Matrix:
        WriteBytes(data.AsBlob.Data, sizeof(Matrix));
        break;
    case VariantType::Asset:
        id = data.AsAsset ? data.AsAsset->GetID() : Guid::Empty;
        Write(id);
        break;
    case VariantType::Float2:
        WriteBytes(data.AsData, sizeof(Float2));
        break;
    case VariantType::Float3:
        WriteBytes(data.AsData, sizeof(Float3));
        break;
    case VariantType::Float4:
        WriteBytes(data.AsData, sizeof(Float4));
        break;
    case VariantType::Double2:
        WriteBytes(data.AsData, sizeof(Double2));
        break;
    case VariantType::Double3:
        WriteBytes(data.AsData, sizeof(Double3));
        break;
    case VariantType::Double4:
        WriteBytes(data.AsBlob.Data, sizeof(Double4));
        break;
    case VariantType::Color:
        WriteBytes(data.AsData, sizeof(Color));
        break;
    case VariantType::Guid:
        WriteBytes(data.AsData, sizeof(Guid));
        break;
    case VariantType::Quaternion:
        WriteBytes(data.AsData, sizeof(Quaternion));
        break;
    case VariantType::Rectangle:
        WriteBytes(data.AsData, sizeof(Rectangle));
        break;
    case VariantType::BoundingSphere:
        Write(data.AsBoundingSphere());
        break;
    case VariantType::Array:
        id.A = ((Array<Variant>*)data.AsData)->Count();
        WriteInt32(id.A);
        for (uint32 i = 0; i < id.A; i++)
            Write(((Array<Variant>*)data.AsData)->At(i));
        break;
    case VariantType::Dictionary:
        WriteInt32(data.AsDictionary->Count());
        for (auto i = data.AsDictionary->Begin(); i.IsNotEnd(); ++i)
        {
            Write(i->Key);
            Write(i->Value);
        }
        break;
    case VariantType::Typename:
        Write((StringAnsiView)data, -14);
        break;
    case VariantType::ManagedObject:
    case VariantType::Structure:
    {
#if USE_CSHARP
        MObject* obj;
        if (data.Type.Type == VariantType::Structure)
            obj = MUtils::BoxVariant(data);
        else
            obj = (MObject*)data;
        if (obj)
        {
            WriteByte(1);
            rapidjson_flax::StringBuffer json;
            CompactJsonWriter writerObj(json);
            MCore::Thread::Attach();
            ManagedSerialization::Serialize(writerObj, obj);
            Write(StringAnsiView(json.GetString(), (int32)json.GetSize()), -71);
        }
        else
#endif
        {
            WriteByte(0);
        }
        break;
    }
    default:
        CRASH;
    }
}

void WriteStream::WriteJson(ISerializable* obj, const void* otherObj)
{
    WriteInt32(FLAXENGINE_VERSION_BUILD);
    if (obj)
    {
        rapidjson_flax::StringBuffer buffer;
        CompactJsonWriter writer(buffer);
        writer.StartObject();
        obj->Serialize(writer, otherObj);
        writer.EndObject();

        WriteInt32((int32)buffer.GetSize());
        WriteBytes((byte*)buffer.GetString(), (int32)buffer.GetSize());
    }
    else
        WriteInt32(0);
}

void WriteStream::WriteJson(const StringAnsiView& json)
{
    WriteInt32(FLAXENGINE_VERSION_BUILD);
    WriteInt32((int32)json.Length());
    WriteBytes((byte*)json.Get(), (int32)json.Length());
}

void WriteStream::WriteString(const StringView& data)
{
    Write(data);
}

void WriteStream::WriteString(const StringView& data, int16 lock)
{
    Write(data, lock);
}

void WriteStream::WriteStringAnsi(const StringAnsiView& data)
{
    Write(data);
}

void WriteStream::WriteStringAnsi(const StringAnsiView& data, int8 lock)
{
    Write(data, lock);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void WriteStream::WriteCommonValue(const CommonValue& data)
{
    Write(data);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void WriteStream::WriteVariantType(const VariantType& data)
{
    Write(data);
}

void WriteStream::WriteVariant(const Variant& data)
{
    Write(data);
}

void WriteStream::Write(const BoundingBox& box, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        WriteBytes(&box, sizeof(BoundingBox));
    else
    {
        Float3 min = box.Minimum, max = box.Maximum;
        Write(min);
        Write(max);
    }
#else
    if (useDouble)
    {
        Double3 min = box.Minimum, max = box.Maximum;
        Write(min);
        Write(max);
    }
    else
        WriteBytes(&box, sizeof(BoundingBox));
#endif
}

void WriteStream::Write(const BoundingSphere& sphere, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        WriteBytes(&sphere, sizeof(BoundingSphere));
    else
    {
        Float3 center = sphere.Center;
        float radius = (float)sphere.Radius;
        Write(center);
        Write(radius);
    }
#else
    if (useDouble)
    {
        Double3 center = sphere.Center;
        float radius = (float)sphere.Radius;
        Write(center);
        Write(radius);
    }
    else
        WriteBytes(&sphere, sizeof(BoundingSphere));
#endif
}

void WriteStream::Write(const Transform& transform, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        WriteBytes(&transform, sizeof(Transform));
    else
    {
        Float3 translation = transform.Translation;
        Write(translation);
        Write(transform.Orientation);
        Write(transform.Scale);
    }
#else
    if (useDouble)
    {
        Double3 translation = transform.Translation;
        Write(translation);
        Write(transform.Orientation);
        Write(transform.Scale);
    }
    else
        WriteBytes(&transform, sizeof(Transform));
#endif
}

void WriteStream::Write(const Ray& ray, bool useDouble)
{
#if USE_LARGE_WORLDS
    if (useDouble)
        WriteBytes(&ray, sizeof(Ray));
    else
    {
        Float3 position = ray.Position, direction = ray.Direction;
        Write(position);
        Write(direction);
    }
#else
    if (useDouble)
    {
        Double3 position = ray.Position, direction = ray.Direction;
        Write(position);
        Write(direction);
    }
    else
        WriteBytes(&ray, sizeof(Ray));
#endif
}

void WriteStream::WriteBoundingBox(const BoundingBox& box, bool useDouble)
{
    Write(box, useDouble);
}

void WriteStream::WriteBoundingSphere(const BoundingSphere& sphere, bool useDouble)
{
    Write(sphere, useDouble);
}

void WriteStream::WriteTransform(const Transform& transform, bool useDouble)
{
    Write(transform, useDouble);
}

void WriteStream::WriteRay(const Ray& ray, bool useDouble)
{
    Write(ray, useDouble);
}

Array<byte> JsonSerializer::SaveToBytes(ISerializable* obj)
{
    Array<byte> result;
    if (obj)
    {
        rapidjson_flax::StringBuffer buffer;
        CompactJsonWriter writer(buffer);
        writer.StartObject();
        obj->Serialize(writer, nullptr);
        writer.EndObject();
        result.Set((byte*)buffer.GetString(), (int32)buffer.GetSize());
    }
    return result;
}

void JsonSerializer::LoadFromBytes(ISerializable* obj, const Span<byte>& data, int32 engineBuild)
{
    if (!obj || data.Length() == 0)
        return;

    ISerializable::SerializeDocument document;
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse((const char*)data.Get(), data.Length());
    }
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
        return;
    }

    auto modifier = Cache::ISerializeModifier.Get();
    modifier->EngineBuild = engineBuild;
    obj->Deserialize(document, modifier.Value);
}
