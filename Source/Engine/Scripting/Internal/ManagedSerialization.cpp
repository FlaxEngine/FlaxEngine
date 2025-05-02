// Copyright (c) Wojciech Figat. All rights reserved.

#include "ManagedSerialization.h"
#if USE_CSHARP
#include "Engine/Core/Log.h"
#include "Engine/Serialization/Json.h"
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/Internal/StdTypesContainer.h"

void ManagedSerialization::Serialize(ISerializable::SerializeStream& stream, MObject* object)
{
    if (!object)
    {
        // Empty object
        stream.StartObject();
        stream.EndObject();
        return;
    }

    // Prepare arguments
    bool isManagedOnly = true;
    void* params[2];
    params[0] = object;
    params[1] = &isManagedOnly;

    // Call serialization tool
    MObject* exception = nullptr;
    // TODO: use method thunk
    auto invokeResultStr = (MString*)StdTypesContainer::Instance()->Json_Serialize->Invoke(nullptr, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("ManagedSerialization::Serialize"));

        // Empty object
        stream.StartObject();
        stream.EndObject();
        return;
    }

    // Write result data
    stream.RawValue(MCore::String::GetChars(invokeResultStr));
}

void ManagedSerialization::SerializeDiff(ISerializable::SerializeStream& stream, MObject* object, MObject* other)
{
    if (!object || !other)
    {
        // Empty object
        stream.StartObject();
        stream.EndObject();
        return;
    }

    // Prepare arguments
    bool isManagedOnly = true;
    void* params[3];
    params[0] = object;
    params[1] = other;
    params[2] = &isManagedOnly;

    // Call serialization tool
    MObject* exception = nullptr;
    // TODO: use method thunk
    auto invokeResultStr = (MString*)StdTypesContainer::Instance()->Json_SerializeDiff->Invoke(nullptr, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("ManagedSerialization::SerializeDiff"));

        // Empty object
        stream.StartObject();
        stream.EndObject();
        return;
    }

    // Write result data
    stream.RawValue(MCore::String::GetChars(invokeResultStr));
}

void ManagedSerialization::Deserialize(ISerializable::DeserializeStream& stream, MObject* object)
{
    if (!object)
        return;

    // Get serialized data
    rapidjson_flax::StringBuffer buffer;
    rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
    stream.Accept(writer);

    Deserialize(StringAnsiView(buffer.GetString(), (int32)buffer.GetSize()), object);
}

void ManagedSerialization::Deserialize(const StringAnsiView& data, MObject* object)
{
    const char* str = data.Get();
    const int32 len = data.Length();
    if (!object || len == 0)
        return;

    // Skip case {} to improve performance
    if (StringUtils::Compare(str, "{}") == 0)
        return;

    // Prepare arguments
    void* args[3];
    args[0] = object;
    args[1] = (void*)&str;
    args[2] = (void*)&len;

    // Call serialization tool
    MObject* exception = nullptr;
    // TODO: use method thunk
    StdTypesContainer::Instance()->Json_Deserialize->Invoke(nullptr, args, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("ManagedSerialization::Deserialize"));
    }
}

#endif
