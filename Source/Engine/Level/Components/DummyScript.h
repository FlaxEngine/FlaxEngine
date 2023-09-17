#pragma once

#include "Engine/Core/Cache.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Serialization/JsonWriters.h"

API_CLASS(Attributes="HideInEditor") class FLAXENGINE_API DummyScript : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(DummyScript);

public:
    API_PROPERTY(Attributes="ReadOnly")
    FORCE_INLINE String GetMissingTypeName() const
    {
        if(Data.IsEmpty()) return TEXT("");
        
        rapidjson_flax::Document doc;
        doc.Parse(Data.ToStringAnsi().GetText());
        
        String str (doc["TypeName"].GetString());
        
        return str;
    }
    
    API_PROPERTY()
    void SetMissingTypeName(String value)
    {
        _missingTypeName = value;
    }
    
    API_FIELD(Hidden, Attributes="HideInEditor") String Data;
    
    API_PROPERTY()
    FORCE_INLINE ScriptingObjectReference<Script> GetReferenceScript() const
    {
        return _referenceScript;
    }
    
    API_PROPERTY()
    void SetReferenceScript(ScriptingObjectReference<Script> value)
    {
        _referenceScript = value;

        if(Data.IsEmpty()) return;
        
        MapToReferenceScript();
    }

private:
    ScriptingObjectReference<Script> _referenceScript;
    String _missingTypeName;
    
    void MapToReferenceScript()
    {
        rapidjson_flax::Document doc;
        doc.Parse(Data.ToStringAnsi().GetText());
        
        auto modifier = Cache::ISerializeModifier.Get();
        _referenceScript->Deserialize(doc, modifier.Value);

        this->DeleteObject();
    }
};

inline DummyScript::DummyScript(const SpawnParams& params) : Script(params){}
