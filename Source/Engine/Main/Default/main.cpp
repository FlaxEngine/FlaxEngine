// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_IOS

#include "Engine/Engine/Engine.h"
#include "Engine/Core/Types/StringBuilder.h"

int main(int argc, char* argv[])
{
    // Join the arguments
    StringBuilder args;
    for (int i = 1; i < argc; i++)
    {
        String arg;
        arg.SetUTF8(argv[i], StringUtils::Length(argv[i]));
        args.Append(arg);

        if (i + 1 != argc)
            args.Append(TEXT(' '));
    }
    args.Append(TEXT('\0'));

    return Engine::Main(*args);
}

#endif
