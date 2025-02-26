// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/SerializableScriptingObject.h"
#include "Engine/Scripting/SoftTypeReference.h"
#include "Engine/Content/SceneReference.h"

// Test default values init on fields.
API_STRUCT(NoDefault) struct TestDefaultValues
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(TestDefaultValues);

    // Default value case 1
    API_FIELD() float TestFloat1 = {};
    // Default value case 2
    API_FIELD() float TestFloat2 = { };
    // Default value case 3
    API_FIELD() float TestFloat3 = {1.0f};
    // Default value case 4
    API_FIELD() float TestFloat4 = float{};
    // Default value case 5
    API_FIELD() float TestFloat5 = float{ };
    // Default value case 6
    API_FIELD() float TestFloat6 = float{1.0f};
    // Default value case 7
    API_FIELD() float TestFloat7 {};
    // Default value case 8
    API_FIELD() float TestFloat8 {1.0f};
    // Default value case 9
    API_FIELD() float TestFloat9 = 1.0f;
    // Default value case 10
    API_FIELD() float TestFloat10 = 1.f;
    // Default value case 11
    API_FIELD() float TestFloat11 = 1;

    // Test code injection parsing
    API_INJECT_CODE(csharp, ""
"namespace Test"
"{"
"\t/// <summary>\n\t/// Test code injection parsing\n\t/// </summary>"
"    public static class TestClass"
"    {"
"    }"
"}")
};

// Test interface (name conflict with namespace)
API_INTERFACE(Namespace="Foo") class FLAXENGINE_API IFoo
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(IFoo);
};

// Test class (name conflict with namespace)
API_CLASS(Namespace="Foo") class FLAXENGINE_API Foo : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(Foo);

    // Test field.
    API_FIELD() IFoo* FooInterface;
};

// Test compilation with nested types.
API_CLASS() class TestNesting : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE(TestNesting);
    API_AUTO_SERIALIZATION();

    // Structure
    API_STRUCT() struct TestAttribute : public ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(TestAttribute);
        API_AUTO_SERIALIZATION();

        // Enumeration
        API_ENUM() enum TestEnum
        {
            E1, E2,
        };

        // Enum
        API_FIELD() TestEnum Enum = E1;
    };

    // Attributes
    API_FIELD() Array<TestAttribute> Attributes;
    // Enum
    API_FIELD() TestAttribute::TestEnum Enum = TestAttribute::E1;
};

// Test compilation with nested types.
API_CLASS() class TestNesting2 : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE(TestNesting2);
    API_AUTO_SERIALIZATION();

    // Structure
    API_STRUCT() struct TestAttribute
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(TestAttribute);

        // Enumeration
        API_ENUM() enum TestEnum
        {
            E1, E2,
        };
    };

    // Attributes
    API_FIELD() Array<TestNesting::TestAttribute> Attributes;
    // Enum
    API_FIELD() TestNesting::TestAttribute::TestEnum Enum = TestNesting::TestAttribute::E1;
};

// Test structure.
API_STRUCT(NoDefault) struct TestStruct : public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(TestStruct);

    // Var
    API_FIELD() Float3 Vector = Float3::One;
    // Ref
    API_FIELD() ScriptingObject* Object = nullptr;
    // Soft Type Ref
    API_FIELD() SoftTypeReference<ScriptingObject> SoftTypeRef;
    // Scene Ref
    API_FIELD() SceneReference SceneRef;

    friend bool operator==(const TestStruct& lhs, const TestStruct& rhs)
    {
        return lhs.Vector == rhs.Vector && 
            lhs.Object == rhs.Object && 
            lhs.SoftTypeRef == rhs.SoftTypeRef &&
            lhs.SceneRef == rhs.SceneRef;
    }
};

// Test structure.
API_STRUCT(NoDefault) struct TestStructPOD
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(TestStructPOD);

    // Var
    API_FIELD() Float3 Vector = Float3::One;
};

template<>
struct TIsPODType<TestStructPOD>
{
    enum { Value = true };
};

// Test interface.
API_INTERFACE() class ITestInterface
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ITestInterface);
    ~ITestInterface() = default;

    // Test abstract method
    API_FUNCTION() virtual int32 TestInterfaceMethod(const String& str) = 0;
};

// Test class.
API_CLASS() class TestClassNative : public ScriptingObject, public ISerializable, public ITestInterface
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(TestClassNative);

public:
    // Test value
    API_FIELD() int32 SimpleField = 1;

    // Test struct
    API_FIELD() TestStruct SimpleStruct;

    // Test event
    API_EVENT() Delegate<int32, Float3, const String&, String&, TestStruct&, const Array<TestStruct>&, Array<TestStruct>&> SimpleEvent;

    // Test virtual method
    API_FUNCTION() virtual int32 TestMethod(const String& str, API_PARAM(Ref) TestStructPOD& pod, API_PARAM(Ref) TestStruct& nonPod, const Array<TestStruct>& struct1, API_PARAM(Ref) Array<TestStruct>& struct2, API_PARAM(Out) Array<ScriptingObject*>& objects)
    {
        if (nonPod.Vector != Float3::One)
            return -1;
        nonPod.Object = this;
        nonPod.Vector = Float3::UnitY;
        return str.Length();
    }

    // Test trailing return type
    API_FUNCTION() auto TestTrailingReturn(int32 number) -> float
    {
        return static_cast<float>(number);
    }

    // Test nameless arguments
    API_FUNCTION() void TestNamelessArguments(int32, float, bool){}

    int32 TestInterfaceMethod(const String& str) override
    {
        return str.Length();
    }
};
