// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ScriptingType.h"
#include "Engine/Core/Types/String.h"

// Utilities for using enums types (declared in scripting with API_ENUM).
template<class EnumType>
class ScriptingEnum
{
public:
    // Gets the list with enum items (the last item Name is null)
    static const ScriptingType::EnumItem* GetItems()
    {
        const ScriptingTypeHandle typeHandle = StaticType<EnumType>();
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            if (type.Type == ScriptingTypes::Enum)
                return type.Enum.Items;
        }
        return nullptr;
    }

    // Gets the name of the enum value or null if invalid
    static const char* GetName(EnumType value)
    {
        if (const auto items = GetItems())
        {
            for (int32 i = 0; items[i].Name; i++)
            {
                if (items[i].Value == (uint64)value)
                    return items[i].Name;
            }
        }
        return nullptr;
    }

    // Gets the name of the enum value or empty string if invalid
    static String ToString(EnumType value)
    {
        return String(GetName(value));
    }

    // Gets the value of the enum based on the name.
    static EnumType FromString(const StringAnsiView& name)
    {
        if (const auto items = GetItems())
        {
            for (int32 i = 0; items[i].Name; i++)
            {
                if (name == items[i].Name)
                    return (EnumType)items[i].Value;
            }
        }
        return (EnumType)0;
    }

    // Gets the value of the enum based on the name.
    static EnumType FromString(const StringView& name)
    {
        return FromString(StringAnsi(name));
    }
};
