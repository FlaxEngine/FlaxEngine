// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Scripting/ScriptingObject.h"

// Test class.
API_CLASS() class TestClassNative : public ScriptingObject, public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(TestClassNative);

public:
    // Test value
    API_FIELD() int32 SimpleField = 1;

    // Test virtual method
    API_FUNCTION() virtual int32 Test(const String& str)
    {
        return str.Length();
    }
};
