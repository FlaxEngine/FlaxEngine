// Copyright (c) Wojciech Figat. All rights reserved.

#include "Encryption.h"
#include "Engine/Platform/StringUtils.h"

namespace
{
    const char* Base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

    // @formatter:off
    const int32 B64index[256] =
    {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,
        0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
        0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };
    // @formatter:on
}

void Encryption::EncryptBytes(byte* data, uint64 size)
{
    byte offset = 71;
    for (uint64 i = 0; i < size; i++)
    {
        data[i] = (data[i] ^ (byte)i) + offset;
        offset += 13;
    }
}

void Encryption::DecryptBytes(byte* data, uint64 size)
{
    byte offset = 71;
    for (uint64 i = 0; i < size; i++)
    {
        data[i] = data[i] - offset ^ (byte)i;
        offset += 13;
    }
}

int32 Encryption::Base64EncodeLength(int32 size)
{
    return (size + 2) / 3 * 4;
}

int32 Encryption::Base64DecodeLength(const char* encoded, int32 length)
{
    if (length == 0)
        return 0;
    const int32 pad1 = length % 4 || encoded[length - 1] == '=';
    const int32 pad2 = pad1 && (length % 4 > 2 || encoded[length - 2] != '=');
    const int32 last = (length - pad1) / 4 << 2;
    return last / 4 * 3 + pad1 + pad2;
}

void Encryption::Base64Encode(const byte* bytes, int32 size, Array<char>& encoded)
{
    encoded.Resize(Base64EncodeLength(size));
    Base64Encode(bytes, size, encoded.Get());
}

void Encryption::Base64Encode(const byte* bytes, int32 size, char* encoded)
{
    int32 i;
    for (i = 0; i < size - 2; i += 3)
    {
        *encoded++ = Base64Chars[bytes[i] >> 2 & 0x3F];
        *encoded++ = Base64Chars[(bytes[i] & 0x3) << 4 | (int32)(bytes[i + 1] & 0xF0) >> 4];
        *encoded++ = Base64Chars[(bytes[i + 1] & 0xF) << 2 | (int32)(bytes[i + 2] & 0xC0) >> 6];
        *encoded++ = Base64Chars[bytes[i + 2] & 0x3F];
    }
    if (i < size)
    {
        *encoded++ = Base64Chars[bytes[i] >> 2 & 0x3F];
        if (i == size - 1)
        {
            *encoded++ = Base64Chars[((bytes[i] & 0x3) << 4)];
            *encoded++ = '=';
        }
        else
        {
            *encoded++ = Base64Chars[(bytes[i] & 0x3) << 4 | (int32)(bytes[i + 1] & 0xF0) >> 4];
            *encoded++ = Base64Chars[((bytes[i + 1] & 0xF) << 2)];
        }
        *encoded = '=';
    }
}

void Encryption::Base64Decode(const char* encoded, int32 length, Array<byte>& output)
{
    output.Clear();
    if (length == 0)
        return;
    output.Resize(Base64DecodeLength(encoded, length));
    Base64Decode(encoded, length, output.Get());
}

void Encryption::Base64Decode(const char* encoded, int32 length, byte* output)
{
    if (length == 0)
        return;
    int32 j = 0;
    const int32 pad1 = length % 4 || encoded[length - 1] == '=', pad2 = pad1 && (length % 4 > 2 || encoded[length - 2] != '=');
    const int32 last = (length - pad1) / 4 << 2;
    byte* str = output;
    for (int32 i = 0; i < last; i += 4)
    {
        int32 n = B64index[encoded[i]] << 18 | B64index[encoded[i + 1]] << 12 | B64index[encoded[i + 2]] << 6 | B64index[encoded[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad1)
    {
        int32 n = B64index[encoded[last]] << 18 | B64index[encoded[last + 1]] << 12;
        str[j++] = n >> 16;
        if (pad2)
        {
            n |= B64index[encoded[last + 2]] << 6;
            str[j] = n >> 8 & 0xFF;
        }
    }
}
