//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include <Engine/Scripting/SerializableScriptingObject.h>
#include "ImageBrush.h"
#include "../Types/Margin.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class UIImageBoxBrush : public UIImageBrush
{
    DECLARE_SCRIPTING_TYPE(UIImageBoxBrush);
private:
    UIMargin Margin;
public:
    API_PROPERTY() void SetMargin(const UIMargin& InUIMargin);
    API_PROPERTY() const UIMargin& GetMargin();

    API_FIELD() Span<Float2> Verts;
    API_FIELD() Span<Float2> Uvs;
protected:
    virtual void Draw(const Rectangle& rect) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
