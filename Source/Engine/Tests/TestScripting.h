// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"

// Test structure.
API_STRUCT(NoDefault) struct TestStruct : public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(TestStruct);

    // Var
    API_FIELD() Float3 Vector = Float3::One;
    // Ref
    API_FIELD() ScriptingObject* Object = nullptr;

    friend bool operator==(const TestStruct& lhs, const TestStruct& rhs)
    {
        return lhs.Vector == rhs.Vector && lhs.Object == rhs.Object;
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

    int32 TestInterfaceMethod(const String& str) override
    {
        return str.Length();
    }
};
