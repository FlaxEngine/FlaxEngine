#pragma once

#include "Engine/Core/Cache.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Serialization/JsonWriters.h"

/// <summary>
/// Actor script component that represents missing script.
/// </summary>
API_CLASS(Attributes="HideInEditor") class FLAXENGINE_API MissingScript : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(MissingScript);

public:

    /// <summary>
    /// Namespace and type name of missing script.
    /// </summary>
    API_FIELD(Attributes="ReadOnly") String MissingTypeName;

    /// <summary>
    /// Missing script serialized data.
    /// </summary>
    API_FIELD(Hidden, Attributes="HideInEditor") String Data;

    /// <summary>
    /// Field for assigning new script to transfer data to.
    /// </summary>
    API_PROPERTY()
    FORCE_INLINE ScriptingObjectReference<Script> GetReferenceScript() const
    {
        return _referenceScript;
    }

    /// <summary>
    /// Field for assigning new script to transfer data to.
    /// </summary>
    API_PROPERTY()
    void SetReferenceScript(ScriptingObjectReference<Script> value)
    {
        _referenceScript = value;

        if(Data.IsEmpty()) return;
        
        MapToReferenceScript();
    }

private:
    ScriptingObjectReference<Script> _referenceScript;
    
    void MapToReferenceScript()
    {
        rapidjson_flax::Document doc;
        doc.Parse(Data.ToStringAnsi().GetText());
        
        auto modifier = Cache::ISerializeModifier.Get();
        _referenceScript->Deserialize(doc, modifier.Value);

        this->DeleteObject();
    }
};

inline MissingScript::MissingScript(const SpawnParams& params) : Script(params){}
