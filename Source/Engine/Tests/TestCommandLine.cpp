// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Engine/CommandLine.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("CommandLine")
{
    SECTION("Test Argument Parser")
    {
        SECTION("Single quoted word")
        {
            String input("\"word\"");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 1);
            CHECK(arguments[0].Compare(StringAnsi("word")) == 0);
        }
        SECTION("Quotes at the beginning of the word")
        {
            String input("start\"word\"");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 1);
            CHECK(arguments[0].Compare(StringAnsi("start\"word\"")) == 0);
        }
        SECTION("Quotes in the middle of the word")
        {
            String input("start\"word\"end");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 1);
            CHECK(arguments[0].Compare(StringAnsi("start\"word\"end")) == 0);
        }
        SECTION("Quotes at the end of the word")
        {
            String input("\"word\"end");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 1);
            CHECK(arguments[0].Compare(StringAnsi("\"word\"end")) == 0);
        }
        SECTION("Multiple words")
        {
            String input("The quick brown fox");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 4);
            CHECK(arguments[0].Compare(StringAnsi("The")) == 0);
            CHECK(arguments[1].Compare(StringAnsi("quick")) == 0);
            CHECK(arguments[2].Compare(StringAnsi("brown")) == 0);
            CHECK(arguments[3].Compare(StringAnsi("fox")) == 0);
        }
        SECTION("Multiple words with quotes")
        {
            String input("The \"quick brown fox\" jumps over the \"lazy\" dog");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 7);
            CHECK(arguments[0].Compare(StringAnsi("The")) == 0);
            CHECK(arguments[1].Compare(StringAnsi("quick brown fox")) == 0);
            CHECK(arguments[2].Compare(StringAnsi("jumps")) == 0);
            CHECK(arguments[3].Compare(StringAnsi("over")) == 0);
            CHECK(arguments[4].Compare(StringAnsi("the")) == 0);
            CHECK(arguments[5].Compare(StringAnsi("lazy")) == 0);
            CHECK(arguments[6].Compare(StringAnsi("dog")) == 0);
        }
        SECTION("Flax.Build sample parameters")
        {
            String input("-log -mutex -workspace=\"C:\\path with spaces/to/FlaxEngine/\" -configuration=Debug -hotreload=\".HotReload.1\"");
            Array<StringAnsi> arguments;
            CHECK(!CommandLine::ParseArguments(input, arguments));
            CHECK(arguments.Count() == 5);
            CHECK(arguments[0].Compare(StringAnsi("-log")) == 0);
            CHECK(arguments[1].Compare(StringAnsi("-mutex")) == 0);
            CHECK(arguments[2].Compare(StringAnsi("-workspace=\"C:\\path with spaces/to/FlaxEngine/\"")) == 0);
            CHECK(arguments[3].Compare(StringAnsi("-configuration=Debug")) == 0);
            CHECK(arguments[4].Compare(StringAnsi("-hotreload=\".HotReload.1\"")) == 0);
        }
    }
}
