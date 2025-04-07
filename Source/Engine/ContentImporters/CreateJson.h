// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Serialization/Json.h"

/// <summary>
/// Json resources factory. Ensure to keep data encoded in UTF-8.
/// </summary>
class CreateJson
{
public:
    static bool Create(const StringView& path, rapidjson_flax::StringBuffer& data, const String& dataTypename);
    static bool Create(const StringView& path, rapidjson_flax::StringBuffer& data, const char* dataTypename);
    static bool Create(const StringView& path, const StringAnsiView& data, const StringAnsiView& dataTypename);
    static CreateAssetResult ImportPo(CreateAssetContext& context);
};

#endif
