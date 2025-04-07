// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"

// Helper utility to get type name at compile time. Example:
// constexpr StringAnsiView name = Nameof::Get<PlatformType>()

namespace Nameof
{
    // [Reference: https://github.com/Neargye/nameof]
    constexpr StringAnsiView PrettyName(StringAnsiView name) noexcept
    {
        if (name.Length() >= 1 && (name.Get()[0] == '"' || name.Get()[0] == '\''))
            return {};
        if (name.Length() >= 2 && name.Get()[0] == 'R' && (name.Get()[1] == '"' || name.Get()[1] == '\''))
            return {};
        if (name.Length() >= 2 && name.Get()[0] == 'L' && (name.Get()[1] == '"' || name.Get()[1] == '\''))
            return {};
        if (name.Length() >= 2 && name.Get()[0] == 'U' && (name.Get()[1] == '"' || name.Get()[1] == '\''))
            return {};
        if (name.Length() >= 2 && name.Get()[0] == 'u' && (name.Get()[1] == '"' || name.Get()[1] == '\''))
            return {};
        if (name.Length() >= 3 && name.Get()[0] == 'u' && name.Get()[1] == '8' && (name.Get()[2] == '"' || name.Get()[2] == '\''))
            return {};
        if (name.Length() >= 1 && (name.Get()[0] >= '0' && name.Get()[0] <= '9'))
            return {};
        for (int32 i = name.Length(), h = 0, s = 0; i > 0; --i)
        {
            if (name.Get()[i - 1] == ')')
            {
                ++h;
                ++s;
                continue;
            }
            if (name.Get()[i - 1] == '(')
            {
                --h;
                ++s;
                continue;
            }
            if (h == 0)
            {
                name = StringAnsiView(*name, name.Length() - s);
                break;
            }
            ++s;
        }
        int32 s = 0;
        for (int32 i = name.Length(), h = 0; i > 0; --i)
        {
            if (name.Get()[i - 1] == '>')
            {
                ++h;
                ++s;
                continue;
            }
            if (name.Get()[i - 1] == '<')
            {
                --h;
                ++s;
                continue;
            }
            if (h == 0)
                break;
            ++s;
        }
        for (int32 i = name.Length() - s; i > 0; --i)
        {
            if (!((name.Get()[i - 1] >= '0' && name.Get()[i - 1] <= '9') || (name.Get()[i - 1] >= 'a' && name.Get()[i - 1] <= 'z') || (name.Get()[i - 1] >= 'A' && name.Get()[i - 1] <= 'Z') || (name.Get()[i - 1] == '_')))
            {
                name = StringAnsiView(*name + i, name.Length() - i);
                break;
            }
        }
        name = StringAnsiView(*name, name.Length() - s);
        if (name.Length() > 0 && ((name.Get()[0] >= 'a' && name.Get()[0] <= 'z') || (name.Get()[0] >= 'A' && name.Get()[0] <= 'Z') || (name.Get()[0] == '_')))
            return name;
        return {};
    }

    // Gets the type name as constant expression (compile-time string)
    template<typename T>
    constexpr StringAnsiView Get() noexcept
    {
#if defined(__clang__) || defined(__GNUC__)
        return PrettyName(StringAnsiView(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 2));
#elif defined(_MSC_VER)
        return PrettyName(StringAnsiView(__FUNCSIG__, sizeof(__FUNCSIG__) - 17));
#else
#error "Unsupported compiler."
#endif
    }
}
