// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Json.h"
#include "JsonWriter.h"

template<typename WriterType>
class JsonWriterBase : public JsonWriter
{
protected:

    WriterType writer;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="JsonWriterBase"/> class.
    /// </summary>
    /// <param name="buffer">The output buffer.</param>
    JsonWriterBase(rapidjson_flax::StringBuffer& buffer)
        : JsonWriter()
        , writer(buffer)
    {
    }

public:

    FORCE_INLINE WriterType& GetWriter()
    {
        return writer;
    }

public:

    // [JsonWriter]
    void Key(const char* str, int32 length) override
    {
        writer.Key(str, static_cast<rapidjson::SizeType>(length));
    }

    void String(const char* str, int32 length) override
    {
        writer.String(str, length);
    }

    void RawValue(const char* json, int32 length) override
    {
        writer.RawValue(json, length);
    }

    void Bool(bool d) override
    {
        writer.Bool(d);
    }

    void Int(int32 d) override
    {
        writer.Int(d);
    }

    void Int64(int64 d) override
    {
        writer.Int64(d);
    }

    void Uint(uint32 d) override
    {
        writer.Uint(d);
    }

    void Uint64(uint64 d) override
    {
        writer.Uint64(d);
    }

    void Float(float d) override
    {
        writer.Float(d);
    }

    void Double(double d) override
    {
        writer.Double(d);
    }

    void StartObject() override
    {
        writer.StartObject();
    }

    void EndObject() override
    {
        writer.EndObject();
    }

    void StartArray() override
    {
        writer.StartArray();
    }

    void EndArray(int32 count = 0) override
    {
        writer.EndArray(count);
    }
};

class FLAXENGINE_API CompactJsonWriterImpl : public rapidjson_flax::Writer<rapidjson_flax::StringBuffer>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="CompactJsonWriterImpl"/> class.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    CompactJsonWriterImpl(rapidjson_flax::StringBuffer& buffer)
        : Writer(buffer)
    {
    }

public:

    void RawValue(const char* json, int32 length)
    {
        Prefix(rapidjson::kObjectType);
        WriteRawValue(json, length);
    }

    void Float(float d)
    {
        Prefix(rapidjson::kNumberType);
        WriteDouble(d);
    }
};

/// <summary>
/// Json writer creating compact and optimized text.
/// </summary>
typedef JsonWriterBase<CompactJsonWriterImpl> CompactJsonWriter;

class FLAXENGINE_API PrettyJsonWriterImpl : public rapidjson_flax::PrettyWriter<rapidjson_flax::StringBuffer>
{
public:

    typedef rapidjson_flax::PrettyWriter<rapidjson_flax::StringBuffer> Writer;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="PrettyJsonWriterImpl"/> class.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    PrettyJsonWriterImpl(rapidjson_flax::StringBuffer& buffer)
        : Writer(buffer)
    {
        SetIndent('\t', 1);
    }

public:

    FORCE_INLINE void RawValue(const char* json, int32 length)
    {
        PrettyPrefix(rapidjson::kObjectType);
        WriteRawValue(json, length);
    }

    FORCE_INLINE void Float(float d)
    {
        PrettyPrefix(rapidjson::kNumberType);
        WriteDouble(d);
    }
};

/// <summary>
/// Json writer creating prettify text.
/// </summary>
typedef JsonWriterBase<PrettyJsonWriterImpl> PrettyJsonWriter;
