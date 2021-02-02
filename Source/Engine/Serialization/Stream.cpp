// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ReadStream.h"
#include "WriteStream.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/Asset.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

void ReadStream::ReadStringAnsi(StringAnsi* data)
{
    int32 length;
    ReadInt32(&length);
    if (length < 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        _hasError = true;
        *data = "";
        return;
    }

    data->ReserveSpace(length);
    char* ptr = data->Get();
    ASSERT(ptr != nullptr);
    Read(ptr, length);
}

void ReadStream::ReadStringAnsi(StringAnsi* data, int8 lock)
{
    int32 length;
    ReadInt32(&length);
    if (length < 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        _hasError = true;
        *data = "";
        return;
    }

    data->ReserveSpace(length);
    char* ptr = data->Get();
    ASSERT(ptr != nullptr);
    Read(ptr, length);

    for (int32 i = 0; i < length; i++)
    {
        *ptr = *ptr ^ lock;
        ptr++;
    }
}

void ReadStream::ReadString(String* data)
{
    int32 length;
    ReadInt32(&length);
    if (length <= 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        if (length != 0)
            _hasError = true;
        data->Clear();
        return;
    }

    data->ReserveSpace(length);
    Char* ptr = data->Get();
    ASSERT(ptr != nullptr);
    Read(ptr, length);
}

void ReadStream::ReadString(String* data, int16 lock)
{
    int32 length;
    ReadInt32(&length);
    if (length <= 0 || length > STREAM_MAX_STRING_LENGTH)
    {
        if (length != 0)
            _hasError = true;
        data->Clear();
        return;
    }

    data->ReserveSpace(length);
    Char* ptr = data->Get();
    ASSERT(ptr != nullptr);
    Read(ptr, length);

    for (int32 i = 0; i < length; i++)
    {
        *ptr = *ptr ^ lock;
        ptr++;
    }
}

void ReadStream::ReadCommonValue(CommonValue* data)
{
    byte type;
    ReadByte(&type);
    switch (static_cast<CommonType>(type))
    {
    case CommonType::Bool:
        data->Set(ReadBool());
        break;
    case CommonType::Integer:
    {
        int32 v;
        ReadInt32(&v);
        data->Set(v);
    }
    break;
    case CommonType::Float:
    {
        float v;
        ReadFloat(&v);
        data->Set(v);
    }
    break;
    case CommonType::Vector2:
    {
        Vector2 v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Vector3:
    {
        Vector3 v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Vector4:
    {
        Vector4 v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Color:
    {
        Color v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Guid:
    {
        Guid v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::String:
    {
        String v;
        ReadString(&v, 953);
        data->Set(v);
    }
    break;
    case CommonType::Box:
    {
        BoundingBox v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Rotation:
    {
        Quaternion v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Transform:
    {
        Transform v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Sphere:
    {
        BoundingSphere v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Rectangle:
    {
        Rectangle v;
        Read(&v);
        data->Set(v);
    }
    case CommonType::Ray:
    {
        Ray v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Matrix:
    {
        Matrix v;
        Read(&v);
        data->Set(v);
    }
    break;
    case CommonType::Blob:
    {
        int32 length;
        Read(&length);
        data->SetBlob(length);
        if (length > 0)
        {
            ReadBytes(data->AsBlob.Data, length);
        }
    }
    break;
    default: CRASH;
    }
}

void ReadStream::ReadVariantType(VariantType* data)
{
    *data = VariantType((VariantType::Types)ReadByte());
    int32 typeNameLength;
    ReadInt32(&typeNameLength);
    if (typeNameLength == MAX_int32)
    {
        ReadInt32(&typeNameLength);
        if (typeNameLength == 0)
            return;
        data->TypeName = static_cast<char*>(Allocator::Allocate(typeNameLength + 1));
        char* ptr = data->TypeName;
        Read(ptr, typeNameLength);
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
        Read(ptr, typeNameLength);
        for (int32 i = 0; i < typeNameLength; i++)
        {
            *ptr = *ptr ^ 77;
            ptr++;
        }
        *ptr = 0;
        data->TypeName = static_cast<char*>(Allocator::Allocate(typeNameLength + 1));
        StringUtils::ConvertUTF162ANSI(chars.Get(), data->TypeName, typeNameLength);
        data->TypeName[typeNameLength] = 0;
    }
}

void ReadStream::ReadVariant(Variant* data)
{
    VariantType type;
    ReadVariantType(&type);
    data->SetType(MoveTemp(type));
    switch (data->Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::ManagedObject:
        break;
    case VariantType::Bool:
        data->AsBool = ReadBool();
        break;
    case VariantType::Int:
        ReadInt32(&data->AsInt);
        break;
    case VariantType::Uint:
        ReadUint32(&data->AsUint);
        break;
    case VariantType::Int64:
        ReadInt64(&data->AsInt64);
        break;
    case VariantType::Uint64:
    case VariantType::Enum:
        ReadUint64(&data->AsUint64);
        break;
    case VariantType::Float:
        ReadFloat(&data->AsFloat);
        break;
    case VariantType::Double:
        ReadDouble(&data->AsDouble);
        break;
    case VariantType::Pointer:
    {
        uint64 asUint64;
        ReadUint64(&asUint64);
        data->AsPointer = (void*)(uintptr)asUint64;
        break;
    }
    case VariantType::String:
    {
        int32 length;
        ReadInt32(&length);
        ASSERT(length < STREAM_MAX_STRING_LENGTH);
        const int32 dataLength = length * sizeof(Char) + 2;
        if (data->AsBlob.Length != dataLength)
        {
            Allocator::Free(data->AsBlob.Data);
            data->AsBlob.Data = dataLength > 0 ? Allocator::Allocate(dataLength) : nullptr;
            data->AsBlob.Length = dataLength;
        }
        Char* ptr = (Char*)data->AsBlob.Data;
        Read(ptr, length);
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
        Read(&id);
        data->SetObject(FindObject(id, ScriptingObject::GetStaticClass()));
        break;
    }
    case VariantType::Structure:
    {
        int32 length;
        ReadInt32(&length);
        if (data->AsBlob.Length == length)
        {
            ReadBytes(data->AsBlob.Data, length);
        }
        else
        {
            LOG(Error, "Invalid Variant {2} data length {0}. Expected {1} bytes from stream.", data->AsBlob.Length, length, data->Type.ToString());

            // Skip those bytes
            void* ptr = Allocator::Allocate(length);
            ReadBytes(ptr, length);
            Allocator::Free(ptr);
        }
        break;
    }
    case VariantType::Blob:
    {
        int32 length;
        ReadInt32(&length);
        data->SetBlob(length);
        ReadBytes(data->AsBlob.Data, length);
        break;
    }
    case VariantType::Asset:
    {
        Guid id;
        Read(&id);
        data->SetAsset(LoadAsset(id, Asset::TypeInitializer));
        break;
    }
    case VariantType::Vector2:
        ReadBytes(&data->AsData, sizeof(Vector2));
        break;
    case VariantType::Vector3:
        ReadBytes(&data->AsData, sizeof(Vector3));
        break;
    case VariantType::Vector4:
        ReadBytes(&data->AsData, sizeof(Vector4));
        break;
    case VariantType::Color:
        ReadBytes(&data->AsData, sizeof(Color));
        break;
    case VariantType::Guid:
        ReadBytes(&data->AsData, sizeof(Guid));
        break;
    case VariantType::BoundingBox:
        ReadBytes(data->AsBlob.Data, sizeof(BoundingBox));
        break;
    case VariantType::BoundingSphere:
        ReadBytes(&data->AsData, sizeof(BoundingSphere));
        break;
    case VariantType::Quaternion:
        ReadBytes(&data->AsData, sizeof(Quaternion));
        break;
    case VariantType::Transform:
        ReadBytes(data->AsBlob.Data, sizeof(Transform));
        break;
    case VariantType::Rectangle:
        ReadBytes(&data->AsData, sizeof(Rectangle));
        break;
    case VariantType::Ray:
        ReadBytes(data->AsBlob.Data, sizeof(Ray));
        break;
    case VariantType::Matrix:
        ReadBytes(data->AsBlob.Data, sizeof(Matrix));
        break;
    case VariantType::Array:
    {
        int32 count;
        ReadInt32(&count);
        auto& array = *(Array<Variant>*)data->AsData;
        array.Resize(count);
        for (int32 i = 0; i < count; i++)
            ReadVariant(&array[i]);
        break;
    }
    case VariantType::Dictionary:
    {
        int32 count;
        ReadInt32(&count);
        auto& dictionary = *data->AsDictionary;
        dictionary.Clear();
        dictionary.EnsureCapacity(count);
        Variant key, value;
        for (int32 i = 0; i < count; i++)
        {
            ReadVariant(&key);
            ReadVariant(&value);
            dictionary.Add(key, value);
        }
        break;
    }
    case VariantType::Typename:
    {
        int32 length;
        ReadInt32(&length);
        ASSERT(length < STREAM_MAX_STRING_LENGTH);
        const int32 dataLength = length + 1;
        if (data->AsBlob.Length != dataLength)
        {
            Allocator::Free(data->AsBlob.Data);
            data->AsBlob.Data = dataLength > 0 ? Allocator::Allocate(dataLength) : nullptr;
            data->AsBlob.Length = dataLength;
        }
        char* ptr = (char*)data->AsBlob.Data;
        Read(ptr, length);
        for (int32 i = 0; i < length; i++)
        {
            *ptr = *ptr ^ -14;
            ptr++;
        }
        *ptr = 0;
        break;
    }
    default:
    CRASH;
    }
}

void WriteStream::WriteText(const StringView& text)
{
    for (int32 i = 0; i < text.Length(); i++)
        WriteChar(text[i]);
}

void WriteStream::WriteString(const StringView& data)
{
    const int32 length = data.Length();
    ASSERT(length < STREAM_MAX_STRING_LENGTH);
    WriteInt32(length);
    Write(*data, length);
}

void WriteStream::WriteString(const StringView& data, int16 lock)
{
    ASSERT(data.Length() < STREAM_MAX_STRING_LENGTH);
    WriteInt32(data.Length());
    for (int32 i = 0; i < data.Length(); i++)
        WriteUint16((uint16)((uint16)data[i] ^ lock));
}

void WriteStream::WriteStringAnsi(const StringAnsiView& data)
{
    const int32 length = data.Length();
    ASSERT(length < STREAM_MAX_STRING_LENGTH);
    WriteInt32(length);
    Write(data.Get(), length);
}

void WriteStream::WriteStringAnsi(const StringAnsiView& data, int16 lock)
{
    const int32 length = data.Length();
    ASSERT(length < STREAM_MAX_STRING_LENGTH);
    WriteInt32(length);
    for (int32 i = 0; i < length; i++)
        WriteUint8((uint8)((uint8)data[i] ^ lock));
}

void WriteStream::WriteCommonValue(const CommonValue& data)
{
    WriteByte(static_cast<byte>(data.Type));
    switch (data.Type)
    {
    case CommonType::Bool:
        WriteBool(data.AsBool);
        break;
    case CommonType::Integer:
        WriteInt32(data.AsInteger);
        break;
    case CommonType::Float:
        WriteFloat(data.AsFloat);
        break;
    case CommonType::Vector2:
        Write(&data.AsVector2);
        break;
    case CommonType::Vector3:
        Write(&data.AsVector3);
        break;
    case CommonType::Vector4:
        Write(&data.AsVector4);
        break;
    case CommonType::Color:
        Write(&data.AsColor);
        break;
    case CommonType::Guid:
        Write(&data.AsGuid);
        break;
    case CommonType::String:
        WriteString(data.AsString, 953);
        break;
    case CommonType::Box:
        Write(&data.AsBox);
        break;
    case CommonType::Rotation:
        Write(&data.AsRotation);
        break;
    case CommonType::Transform:
        Write(&data.AsTransform);
        break;
    case CommonType::Sphere:
        Write(&data.AsSphere);
        break;
    case CommonType::Rectangle:
        Write(&data.AsRectangle);
        break;
    case CommonType::Ray:
        Write(&data.AsRay);
        break;
    case CommonType::Matrix:
        Write(&data.AsMatrix);
        break;
    case CommonType::Blob:
        WriteInt32(data.AsBlob.Length);
        if (data.AsBlob.Length > 0)
            WriteBytes(data.AsBlob.Data, data.AsBlob.Length);
        break;
    default: CRASH;
    }
}

void WriteStream::WriteVariantType(const VariantType& data)
{
    WriteByte((byte)data.Type);
    WriteInt32(MAX_int32);
    WriteStringAnsi(StringAnsiView(data.TypeName), 77);
}

void WriteStream::WriteVariant(const Variant& data)
{
    WriteVariantType(data.Type);
    Guid id;
    switch (data.Type.Type)
    {
    case VariantType::Null:
    case VariantType::Void:
    case VariantType::ManagedObject:
        break;
    case VariantType::Bool:
        WriteBool(data.AsBool);
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
        WriteString((StringView)data, -14);
        break;
    case VariantType::Object:
        id = data.AsObject ? data.AsObject->GetID() : Guid::Empty;
        Write(&id);
        break;
    case VariantType::Structure:
    case VariantType::Blob:
        WriteInt32(data.AsBlob.Length);
        WriteBytes(data.AsBlob.Data, data.AsBlob.Length);
        break;
    case VariantType::BoundingBox:
        WriteBytes(data.AsBlob.Data, sizeof(BoundingBox));
        break;
    case VariantType::Transform:
        WriteBytes(data.AsBlob.Data, sizeof(Transform));
        break;
    case VariantType::Ray:
        WriteBytes(data.AsBlob.Data, sizeof(Ray));
        break;
    case VariantType::Matrix:
        WriteBytes(data.AsBlob.Data, sizeof(Matrix));
        break;
    case VariantType::Asset:
        id = data.AsAsset ? data.AsAsset->GetID() : Guid::Empty;
        Write(&id);
        break;
    case VariantType::Vector2:
        Write(data.AsData, sizeof(Vector2));
        break;
    case VariantType::Vector3:
        Write(data.AsData, sizeof(Vector3));
        break;
    case VariantType::Vector4:
        Write(data.AsData, sizeof(Vector4));
        break;
    case VariantType::Color:
        Write(data.AsData, sizeof(Color));
        break;
    case VariantType::Guid:
        Write(data.AsData, sizeof(Guid));
        break;
    case VariantType::Quaternion:
        Write(data.AsData, sizeof(Quaternion));
        break;
    case VariantType::Rectangle:
        Write(data.AsData, sizeof(Rectangle));
        break;
    case VariantType::BoundingSphere:
        Write(data.AsData, sizeof(BoundingSphere));
        break;
    case VariantType::Array:
        id.A = ((Array<Variant>*)data.AsData)->Count();
        WriteInt32(id.A);
        for (uint32 i = 0; i < id.A; i++)
            WriteVariant(((Array<Variant>*)data.AsData)->At(i));
        break;
    case VariantType::Dictionary:
        WriteInt32(data.AsDictionary->Count());
        for (auto i = data.AsDictionary->Begin(); i.IsNotEnd(); ++i)
        {
            WriteVariant(i->Key);
            WriteVariant(i->Value);
        }
        break;
    case VariantType::Typename:
        WriteStringAnsi((StringAnsiView)data, -14);
        break;
    default:
    CRASH;
    }
}
