// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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
};

// Test class.
API_CLASS() class TestClassNative : public ScriptingObject, public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(TestClassNative);

public:
    // Test value
    API_FIELD() int32 SimpleField = 1;

    // Test struct
    API_FIELD() TestStruct SimpleStruct;

    // Test event
    API_EVENT() Delegate<int32, Float3, const String&, String&, const Array<TestStruct>&, Array<TestStruct>&> SimpleEvent;

    // Test virtual method
    API_FUNCTION() virtual int32 TestMethod(const String& str)
    {
        return str.Length();
    }
};
