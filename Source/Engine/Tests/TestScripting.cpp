// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "TestScripting.h"
#include "Engine/Scripting/Scripting.h"
#include <ThirdParty/catch2/catch.hpp>

TestClassNative::TestClassNative(const SpawnParams& params)
    : ScriptingObject(params)
{
}

TEST_CASE("Scripting")
{
    SECTION("Test Class")
    {
        // Test native class
        ScriptingTypeHandle type = Scripting::FindScriptingType("FlaxEngine.TestClassNative");
        CHECK(type == TestClassNative::TypeInitializer);
        ScriptingObject* object = Scripting::NewObject(type.GetType().ManagedClass);
        CHECK(object);
        CHECK(object->Is<TestClassNative>());
        TestClassNative* testClass = (TestClassNative*)object;
        CHECK(testClass->SimpleField == 1);
        int32 methodResult = testClass->Test(TEXT("123"));
        CHECK(methodResult == 3);

        // Test managed class
        type = Scripting::FindScriptingType("FlaxEngine.TestClassManaged");
        CHECK(type);
        object = Scripting::NewObject(type.GetType().ManagedClass);
        CHECK(object);
        CHECK(object->Is<TestClassNative>());
        testClass = (TestClassNative*)object;
        MObject* managed = testClass->GetOrCreateManagedInstance(); // Ensure to create C# object and run it's ctor
        CHECK(managed);
        CHECK(testClass->SimpleField == 2);
        methodResult = testClass->Test(TEXT("123"));
        CHECK(methodResult == 6);
    }
}
