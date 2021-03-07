// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ManagedSerialization.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/Json.h"
#include "Engine/Serialization/JsonWriter.h"
#include "StdTypesContainer.h"
#include "MException.h"
#include "ManagedCLR/MMethod.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>

void ManagedSerialization::Serialize(ISerializable::SerializeStream& stream, MonoObject* object)
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
    MonoObject* exception = nullptr;
    // TODO: use method thunk
    auto invokeResultStr = (MonoString*)StdTypesContainer::Instance()->Json_Serialize->Invoke(nullptr, params, &exception);
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
    const auto invokeResultChars = mono_string_to_utf8(invokeResultStr);
    stream.RawValue(invokeResultChars);
    mono_free(invokeResultChars);
}

void ManagedSerialization::SerializeDiff(ISerializable::SerializeStream& stream, MonoObject* object, MonoObject* other)
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
    MonoObject* exception = nullptr;
    // TODO: use method thunk
    auto invokeResultStr = (MonoString*)StdTypesContainer::Instance()->Json_SerializeDiff->Invoke(nullptr, params, &exception);
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
    auto invokeResultChars = mono_string_to_utf8(invokeResultStr);
    stream.RawValue(invokeResultChars);
    mono_free(invokeResultChars);
}

void ManagedSerialization::Deserialize(ISerializable::DeserializeStream& stream, MonoObject* object)
{
    ASSERT(stream.IsObject());

    // Get serialized data
    rapidjson_flax::StringBuffer buffer;
    rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
    stream.Accept(writer);

    Deserialize(buffer, object);
}

void ManagedSerialization::Deserialize(const rapidjson_flax::StringBuffer& data, MonoObject* object)
{
    if (!object)
        return;
    const char* str = data.GetString();
    const int32 len = (int32)data.GetSize();

    // Skip case {} to improve performance
    if (StringUtils::Compare(str, "{}") == 0)
        return;

    // Prepare arguments
    void* args[3];
    args[0] = object;
    args[1] = (void*)str;
    args[2] = (void*)&len;

    // Call serialization tool
    MonoObject* exception = nullptr;
    // TODO: use method thunk
    StdTypesContainer::Instance()->Json_Deserialize->Invoke(nullptr, args, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("ManagedSerialization::Deserialize"));
    }
}
