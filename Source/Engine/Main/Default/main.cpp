// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/String.h"
#if PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_IOS
#if PLATFORM_LINUX || PLATFORM_MAC
#include "Engine/Engine/Engine.h"

int main(int argc, const char** argv)
{
    auto arg = Array<String>(argc);
    for (int i = 1; i < argc; i++)
        arg.Add(String(argv[i]));
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
        args.Append(argv[i]);

        if (i + 1 != argc)
            args.Append(TEXT(' '));
    }
    args.Append(TEXT('\0'));

    return Engine::Main(*args);
}

#endif
#endif
