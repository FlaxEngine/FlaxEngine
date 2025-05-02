// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Contains algorithms for data encryption/decryption.
/// </summary>
class FLAXENGINE_API Encryption
{
public:
    /// <summary>
    /// Encrypt bytes with custom data
    /// </summary>
    /// <param name="data">Bytes to encrypt</param>
    /// <param name="size">Amount of bytes to process</param>
    static void EncryptBytes(byte* data, uint64 size);

    /// <summary>
    /// Decrypt bytes with custom data
    /// </summary>
    /// <param name="data">Bytes to decrypt</param>
    /// <param name="size">Amount of bytes to process</param>
    static void DecryptBytes(byte* data, uint64 size);

public:
    static int32 Base64EncodeLength(int32 size);
    static int32 Base64DecodeLength(const char* encoded, int32 length);
    static void Base64Encode(const byte* bytes, int32 size, Array<char>& encoded);
    static void Base64Encode(const byte* bytes, int32 size, char* encoded);
    static void Base64Decode(const char* encoded, int32 length, Array<byte>& output);
    static void Base64Decode(const char* encoded, int32 length, byte* output);
};
