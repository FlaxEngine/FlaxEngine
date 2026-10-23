// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ScriptingType.h"
#include "Engine/Core/Types/String.h"

/// <summary>
/// Utilities for using enums types (declared in scripting with API_ENUM).
/// </summary>
class ScriptingEnum
{
public:
    // Gets the list with enum items (the last item Name is null)
    template<class EnumType>
    static const ScriptingType::EnumItem* GetItems()
    {
        if (const ScriptingTypeHandle typeHandle = StaticType<EnumType>())
        {
            const ScriptingType& type = typeHandle.GetType();
            if (type.Type == ScriptingTypes::Enum)
                return type.Enum.Items;
        }
        return nullptr;
    }

    // Gets the name of the enum value or null if invalid
    template<class EnumType>
    static const char* GetName(EnumType value)
    {
        if (const auto items = GetItems<EnumType>())
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
    template<class EnumType>
    static String ToString(EnumType value)
    {
        String result;
        const char* name = GetName<EnumType>(value);
        if (name)
            result = String(name);
        else if (const ScriptingTypeHandle typeHandle = StaticType<EnumType>())
        {
            // Flags
            const ScriptingType& type = typeHandle.GetType();
            if (type.Type == ScriptingTypes::Enum && type.Enum.Flags && type.Enum.Items)
            {
                auto items = type.Enum.Items;
                for (int32 i = 0; items[i].Name; i++)
                {
                    if (((uint64)value & items[i].Value) == items[i].Value && items[i].Value)
                    {
                        if (result.Length())
                            result += TEXT("|");
                        result += items[i].Name;
                    }
                }
            }
        }
        return result;
    }

    // Gets the value of the enum based on the name.
    template<class EnumType>
    static EnumType FromString(const StringAnsiView& name)
    {
        if (const auto items = GetItems<EnumType>())
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
    template<class EnumType>
    static EnumType FromString(const StringView& name)
    {
        // TODO: add parsing flags (with '|' separator)
        return FromString<EnumType>(StringAnsi(name));
    }

    // Gets the name of the enum value as separated flags
    template<class EnumType>
    static String ToStringFlags(EnumType value, Char separator = '|')
    {
        String result;
        if (const auto items = GetItems<EnumType>())
        {
            for (int32 i = 0; items[i].Name; i++)
            {
                const uint64 itemValue = items[i].Value;
                if ((uint64)value == 0 && itemValue == 0)
                {
                    result = items[i].Name;
                    break;
                }
                if (itemValue != 0 && EnumHasAllFlags<EnumType>(value, (EnumType)itemValue))
                {
                    if (result.HasChars())
                        result += separator;
                    result += items[i].Name;
                }
            }
        }
        return result;
    }
};
