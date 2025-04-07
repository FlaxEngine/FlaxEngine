// Copyright (c) Wojciech Figat. All rights reserved.

#include "ShaderStringBuilder.h"

ShaderStringBuilder& ShaderStringBuilder::Code(const Char* shaderCode)
{
    _code = shaderCode;
    return *this;
}

ShaderStringBuilder& ShaderStringBuilder::Replace(const String& key, const String& value)
{
    _replacements.Add(Pair<String, String>(key, value));
    return *this;
}

String ShaderStringBuilder::Build() const
{
    String result = _code;
    for (const auto& replacement : _replacements)
    {
        const auto& key = replacement.First;
        const auto& value = replacement.Second;
        int32 position = 0;
        while ((position = result.Find(key)) != -1)
        {
            result = String::Format(TEXT("{0}{1}{2}"),
                StringView(result.Get(), position),
                value,
                StringView(result.Get() + position + key.Length()));
        }
    }
    return result;
}
