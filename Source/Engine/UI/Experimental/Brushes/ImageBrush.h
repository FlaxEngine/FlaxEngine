//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include <Engine/Scripting/SerializableScriptingObject.h>
#include "Brush.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class UIImageBrush : public UIBrush
{
    DECLARE_SCRIPTING_TYPE(UIImageBrush);
public:
    API_FIELD() AssetReference<Texture> Image;
protected:
    virtual void Draw(const Rectangle& rect) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
