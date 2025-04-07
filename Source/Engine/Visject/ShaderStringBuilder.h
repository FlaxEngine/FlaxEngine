// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"

// Helper utility for shader source code formatting.
class ShaderStringBuilder
{
private:
    String _code;
    Array<Pair<String, String>> _replacements;

public:
    ShaderStringBuilder& Code(const Char* shaderCode);
    ShaderStringBuilder& Replace(const String& key, const String& value);
    String Build() const;
};
