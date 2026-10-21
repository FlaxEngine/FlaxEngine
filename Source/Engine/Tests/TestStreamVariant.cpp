// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Types/Variant.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Reads a Variant from the data and returns true if the stream reports an error
    bool ReadVariantFails(const MemoryWriteStream& data, Variant& value)
    {
        MemoryReadStream stream(data.GetHandle(), (uint32)data.GetPosition());
        stream.Read(value);
        return stream.HasError();
    }
}

TEST_CASE("Stream")
{
    SECTION("Test Variant with an invalid length")
    {
        // Corrupted or untrusted data: the length is negative or above STREAM_MAX_STRING_LENGTH
        const int32 lengths[] = { -1, -100, STREAM_MAX_STRING_LENGTH + 1, MAX_int32 - 1 };
        for (const int32 length : lengths)
        {
            // String value
            {
                MemoryWriteStream data;
                data.WriteByte((byte)VariantType::String);
                data.WriteInt32(0); // No type name
                data.WriteInt32(length);
                Variant value;
                CHECK(ReadVariantFails(data, value));
            }

            // Typename value
            {
                MemoryWriteStream data;
                data.WriteByte((byte)VariantType::Typename);
                data.WriteInt32(0); // No type name
                data.WriteInt32(length);
                Variant value;
                CHECK(ReadVariantFails(data, value));
            }

            // Type name of the value type
            {
                MemoryWriteStream data;
                data.WriteByte((byte)VariantType::Object);
                data.WriteInt32(MAX_int32); // Type name in the current format
                data.WriteInt32(length);
                Variant value;
                CHECK(ReadVariantFails(data, value));
            }
        }
    }

    SECTION("Test Variant with a valid length")
    {
        MemoryWriteStream data;
        data.Write(Variant(TEXT("Text")));
        Variant value;
        CHECK(!ReadVariantFails(data, value));
        CHECK(value == Variant(TEXT("Text")));
    }
}
