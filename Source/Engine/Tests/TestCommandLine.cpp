// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Engine/CommandLine.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Parses the command line and returns the value of the given argument (empty if not set), as text that Catch can print.
    StringAnsi ParseValue(const String& commandLine, const StringView& name, int32* argsCount = nullptr)
    {
        Array<CommandLine::Arg> args;
        CHECK(!CommandLine::Parse(commandLine, args));
        if (argsCount)
            *argsCount = args.Count();
        for (const CommandLine::Arg& arg : args)
        {
            if (arg.Name == name)
                return arg.Value.HasChars() ? StringAnsi(arg.Value) : StringAnsi::Empty;
        }
        return StringAnsi::Empty;
    }
}

TEST_CASE("CommandLine")
{
    SECTION("Test value with '=' (Flax.Build style)")
    {
        int32 count = 0;
        const String commandLine(TEXT("-log -workspace=\"C:\\path with spaces/to/FlaxEngine/\" -configuration=Debug"));
        CHECK(ParseValue(commandLine, TEXT("workspace"), &count) == "C:\\path with spaces/to/FlaxEngine/");
        CHECK(ParseValue(commandLine, TEXT("configuration")) == "Debug");
        CHECK(count == 3);
    }
    SECTION("Test switches without a value")
    {
        int32 count = 0;
        CHECK(ParseValue(TEXT("-std -nolog -d3d12"), TEXT("nolog"), &count) == "");
        CHECK(count == 3);
    }
    SECTION("Test value after a space")
    {
        // The editor gets the project path as a separate, quoted value (eg. '-project "C:\path"')
        CHECK(ParseValue(TEXT("-project \"C:\\Projects\\Game\""), TEXT("project")) == "C:\\Projects\\Game");
        CHECK(ParseValue(TEXT("-project C:\\Projects\\Game"), TEXT("project")) == "C:\\Projects\\Game");

        // A quoted value keeps its '-', space and apostrophe characters
        CHECK(ParseValue(TEXT("-project \"C:\\Projects\\My-Game\""), TEXT("project")) == "C:\\Projects\\My-Game");
        CHECK(ParseValue(TEXT("-project \"C:\\Flax Projects\\Game\""), TEXT("project")) == "C:\\Flax Projects\\Game");
        CHECK(ParseValue(TEXT("-project \"C:\\Bob's Game\""), TEXT("project")) == "C:\\Bob's Game");

        // The arguments after the value
        int32 count = 0;
        const String commandLine(TEXT("-project \"C:\\Flax Projects\\My-Game\" -std -play=Main"));
        CHECK(ParseValue(commandLine, TEXT("project"), &count) == "C:\\Flax Projects\\My-Game");
        CHECK(ParseValue(commandLine, TEXT("play")) == "Main");
        CHECK(count == 3);
    }
}
