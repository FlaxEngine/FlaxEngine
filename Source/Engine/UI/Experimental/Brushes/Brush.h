//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/SerializableScriptingObject.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Texture.h"
#include <Engine/Core/Math/Vector2.h>
#include <Engine/Core/Math/Color.h>
#include "../Asset/UIBrushAsset.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.UIBrushEditor))")
class UIBrush : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UIBrush);
public:
    API_FIELD() bool Override = false;
    API_FIELD(Attributes = "VisibleIf(\"Override\",true)")
        AssetReference<UIBrushAsset> Asset;
    API_FIELD(Attributes = "VisibleIf(\"Override\",false)")
        Color Tint;
protected:
    virtual void Draw(const Rectangle& rect);
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
protected:

    friend class UIButton;
    friend class UIImage;
};
