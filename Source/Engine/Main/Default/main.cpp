// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_IOS
#if PLATFORM_LINUX || PLATFORM_MAC
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Engine/Engine.h"

int main(int argc, const char** argv)
{
    auto arg = Array<String>(argc);
    for (int i = 1; i < argc; i++)
    {
        arg.Add(String(argv[i]));
    }
    return Engine::Main(arg);
}

#else
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
#endif
