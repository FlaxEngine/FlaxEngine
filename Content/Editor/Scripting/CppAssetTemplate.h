%copyright%#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// %class% Json Asset. 
/// </summary>
API_CLASS() class %module%%class% : public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(%class%);
public:
    // Custom float value.
    API_FIELD(Attributes = "Range(0, 20), EditorOrder(0), EditorDisplay(\"Data\")") 
    float FloatValue = 20.0f;
    // Custom vector data.
    API_FIELD(Attributes = "EditorOrder(1), EditorDisplay(\"Data\")")
    Vector3 Vector3Value = Vector3(0.1f);
};
