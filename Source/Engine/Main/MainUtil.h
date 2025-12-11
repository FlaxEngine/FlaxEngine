// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/StringUtils.h"

const Char* GetCommandLine(int argc, char* argv[])
{
    int32 length = 0;
    for (int i = 1; argc > 1 && i < argc; i++)
    {
        length += StringUtils::Length((const char*)argv[i]);
        if (i + 1 != argc)
            length++;
    }
    const Char* cmdLine;
    if (length != 0)
    {
        Char* str = (Char*)malloc(length * sizeof(Char));
        cmdLine = str;
        for (int i = 1; i < argc; i++)
        {
            length = StringUtils::Length((const char*)argv[i]);
            int32 strLen = 0;
            StringUtils::ConvertANSI2UTF16(argv[i], str, length, strLen);
            str += strLen;
            if (i + 1 != argc)
                *str++ = TEXT(' ');
        }
        *str = TEXT('\0');
    }
    else
    {
        cmdLine = TEXT("");
    }
    return cmdLine;
}
