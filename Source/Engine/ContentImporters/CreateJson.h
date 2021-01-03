// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Serialization/Json.h"

/// <summary>
/// Json resources factory.
/// </summary>
class CreateJson
{
public:

    static bool Create(const StringView& path, rapidjson_flax::StringBuffer& data, const String& dataTypename);
    static bool Create(const StringView& path, rapidjson_flax::StringBuffer& data, const char* dataTypename);
    static bool Create(const StringView& path, DataContainer<char>& data, DataContainer<char>& dataTypename);
};

#endif
