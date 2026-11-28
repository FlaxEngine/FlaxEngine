// Copyright (c) Wojciech Figat. All rights reserved.

#include "TestScripting.h"
#include "Engine/Level/Actors/EmptyActor.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include <ThirdParty/catch2/catch.hpp>

Foo::Foo(const SpawnParams& params)
    : ScriptingObject(params)
    , FooInterface(nullptr)
{
}

TestNesting::TestNesting(const SpawnParams& params)
    : SerializableScriptingObject(params)
{
}

TestNesting2::TestNesting2(const SpawnParams& params)
    : SerializableScriptingObject(params)
{
}

int32 TestScriptLifecycle::AwakeCount = 0;
int32 TestScriptLifecycle::StartCount = 0;
int32 TestScriptLifecycle::EnableCount = 0;
int32 TestScriptLifecycle::DestroyCount = 0;

TestScriptLifecycle::TestScriptLifecycle(const SpawnParams& params)
    : Script(params)
{
}

TestScriptAddingScript::TestScriptAddingScript(const SpawnParams& params)
    : Script(params)
{
}

void TestScriptAddingScript::OnAwake()
{
    // The actor is not in play yet (the scene loads), so the new script is not initialized here
    auto script = ScriptingObject::NewObject<TestScriptLifecycle>();
    script->SetParent(GetActor());
}

TestScriptSpawningActor::TestScriptSpawningActor(const SpawnParams& params)
    : Script(params)
{
}

void TestScriptSpawningActor::OnAwake()
{
    // The new actor is enabled when it is parented to the (loading) scene, before the script is added to it
    auto actor = ScriptingObject::NewObject<EmptyActor>();
    actor->SetParent(GetActor(), false, true);
    auto script = ScriptingObject::NewObject<TestScriptLifecycle>();
    script->SetParent(actor);
}

TestClassNative::TestClassNative(const SpawnParams& params)
    : ScriptingObject(params)
{
}

TEST_CASE("Scripting")
{
    SECTION("Test Library Imports")
    {
        MClass* klass = Scripting::FindClass("FlaxEngine.Tests.TestScripting");
        CHECK(klass);
        MMethod* method = klass->GetMethod("TestLibraryImports");
        CHECK(method);
        MObject* result = method->Invoke(nullptr, nullptr, nullptr);
        CHECK(MUtils::Unbox<int32>(result) == 0);
    }

    SECTION("Test Class")
    {
        // Test native class
        ScriptingTypeHandle type = Scripting::FindScriptingType("FlaxEngine.TestClassNative");
        CHECK(type == TestClassNative::TypeInitializer);
        ScriptingObject* object = Scripting::NewObject(type);
        CHECK(object);
        CHECK(object->Is<TestClassNative>());
        TestClassNative* testClass = (TestClassNative*)object;
        CHECK(testClass->SimpleField == 1);
        CHECK(testClass->SimpleStruct.Object == nullptr);
        CHECK(testClass->SimpleStruct.Vector == Float3::One);
        TestStruct nonPod;
        Array<TestStruct> struct1 = { testClass->SimpleStruct };
        Array<TestStruct> struct2 = { testClass->SimpleStruct };
        Array<ScriptingObject*> objects;
        TestStructPOD pod;
        int32 methodResult = testClass->TestMethod(TEXT("123"), pod, nonPod, struct1, struct2, objects);
        CHECK(methodResult == 3);
        CHECK(nonPod.Object == testClass);
        CHECK(nonPod.Vector == Float3::UnitY);
        CHECK(objects.Count() == 0);

        // Test managed class
        type = Scripting::FindScriptingType("FlaxEngine.TestClassManaged");
        CHECK(type);
        object = Scripting::NewObject(type);
        CHECK(object);
        CHECK(object->Is<TestClassNative>());
        testClass = (TestClassNative*)object;
        MObject* managed = testClass->GetOrCreateManagedInstance(); // Ensure to create C# object and run it's ctor
        CHECK(managed);
        CHECK(testClass->SimpleField == 2);
        CHECK(testClass->SimpleStruct.Object == testClass);
        CHECK(testClass->SimpleStruct.Vector == Float3::UnitX);
        nonPod = TestStruct();
        struct1 = { testClass->SimpleStruct };
        struct2 = { testClass->SimpleStruct };
        objects.Clear();
        pod.Vector = Float3::One;
        methodResult = testClass->TestMethod(TEXT("123"), pod, nonPod, struct1, struct2, objects);
        CHECK(methodResult == 6);
        CHECK(pod.Vector == Float3::Half);
        CHECK(nonPod.Object == testClass);
        CHECK(nonPod.Vector == Float3::UnitY);
        CHECK(struct2.Count() == 2);
        CHECK(struct2[0] == testClass->SimpleStruct);
        CHECK(struct2[1] == testClass->SimpleStruct);
        CHECK(objects.Count() == 3);
    }

    SECTION("Test Event")
    {
        ScriptingTypeHandle type = Scripting::FindScriptingType("FlaxEngine.TestClassManaged");
        CHECK(type);
        ScriptingObject* object = Scripting::NewObject(type);
        CHECK(object);
        MObject* managed = object->GetOrCreateManagedInstance(); // Ensure to create C# object and run it's ctor
        CHECK(managed);
        TestClassNative* testClass = (TestClassNative*)object;
        CHECK(testClass->SimpleField == 2);
        String str1 = TEXT("1");
        String str2 = TEXT("2");
        TestStruct nonPod;
        Array<TestStruct> arr1 = { testClass->SimpleStruct };
        Array<TestStruct> arr2 = { testClass->SimpleStruct };
        testClass->SimpleEvent(1, Float3::One, str1, str2, nonPod, arr1, arr2);
        CHECK(testClass->SimpleField == 4);
        CHECK(str2 == TEXT("4"));
        CHECK(nonPod.Object == testClass);
        CHECK(nonPod.Vector == Float3::UnitY);
        CHECK(arr2.Count() == 2);
        CHECK(arr2[0].Vector == Float3::Half);
        CHECK(arr2[0].Object == nullptr);
        CHECK(arr2[1].Vector == testClass->SimpleStruct.Vector);
        CHECK(arr2[1].Object == testClass);
    }

    SECTION("Test Interface")
    {
        // Test native interface implementation
        ScriptingTypeHandle type = Scripting::FindScriptingType("FlaxEngine.TestClassNative");
        CHECK(type);
        ScriptingObject* object = Scripting::NewObject(type);
        CHECK(object);
        TestClassNative* testClass = (TestClassNative*)object;
        int32 methodResult = testClass->TestInterfaceMethod(TEXT("123"));
        CHECK(methodResult == 3);
        ITestInterface* interface = ScriptingObject::ToInterface<ITestInterface>(object);
        CHECK(interface);
        methodResult = interface->TestInterfaceMethod(TEXT("1234"));
        CHECK(methodResult == 4);
        ScriptingObject* interfaceObject = ScriptingObject::FromInterface<ITestInterface>(interface);
        CHECK(interfaceObject);
        CHECK(interfaceObject == object);

        // Test managed interface override
        type = Scripting::FindScriptingType("FlaxEngine.TestClassManaged");
        CHECK(type);
        object = Scripting::NewObject(type);
        CHECK(object);
        testClass = (TestClassNative*)object;
        methodResult = testClass->TestInterfaceMethod(TEXT("123"));
        CHECK(methodResult == 6);
        interface = ScriptingObject::ToInterface<ITestInterface>(object);
        CHECK(interface);
        methodResult = interface->TestInterfaceMethod(TEXT("1234"));
        CHECK(methodResult == 8);
        interfaceObject = ScriptingObject::FromInterface<ITestInterface>(interface);
        CHECK(interfaceObject);
        CHECK(interfaceObject == object);

        // Test managed interface implementation
        type = Scripting::FindScriptingType("FlaxEngine.TestInterfaceManaged");
        CHECK(type);
        object = Scripting::NewObject(type);
        CHECK(object);
        interface = ScriptingObject::ToInterface<ITestInterface>(object);
        CHECK(interface);
        methodResult = interface->TestInterfaceMethod(TEXT("1234"));
        CHECK(methodResult == 4);
        interfaceObject = ScriptingObject::FromInterface<ITestInterface>(interface);
        CHECK(interfaceObject);
        CHECK(interfaceObject == object);
    }

    SECTION("Test Interface Reference")
    {
        // Test native interface implementation
        MClass* klass = Scripting::FindClass("FlaxEngine.Tests.TestScripting");
        CHECK(klass);
        MMethod* method = klass->GetMethod("TestInterfaceReference");
        CHECK(method);
        MObject* result = method->Invoke(nullptr, nullptr, nullptr);
        CHECK(MUtils::Unbox<int32>(result) == 0);
    }
}
