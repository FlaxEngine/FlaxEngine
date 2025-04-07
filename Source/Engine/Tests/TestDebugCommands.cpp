// Copyright (c) Wojciech Figat. All rights reserved.

#include "TestScripting.h"
#include "Engine/Debug/DebugCommands.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    bool PassExec = false;
}

bool TestDebugCommand1::Var = false;
float TestDebugCommand2::Var = 0.0f;

TestDebugCommand2::TestDebugCommand2(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void TestDebugCommand2::Exec()
{
    PassExec = true;
}

TEST_CASE("DebugCommands")
{
    SECTION("Test Commands")
    {
        // Test async cache flow
        DebugCommands::InitAsync();
        Platform::Sleep(1);

        // Test commands invoking
        CHECK(TestDebugCommand1::Var == false);
        DebugCommands::Execute(TEXT("TestDebugCommand1.Var true"));
        CHECK(TestDebugCommand1::Var == true);
        CHECK(TestDebugCommand2::Var == 0.0f);
        DebugCommands::Execute(TEXT("TestDebugCommand2.Var 1.5"));
        DebugCommands::Execute(TEXT("TestDebugCommand2.Var"));
        CHECK(TestDebugCommand2::Var == 1.5f);
        CHECK(!PassExec);
        DebugCommands::Execute(TEXT("TestDebugCommand2.Exec"));
        CHECK(PassExec);
    }
}
