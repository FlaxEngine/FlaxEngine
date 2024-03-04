//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../Brush.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"

UIBrush::UIBrush(const SpawnParams& params) : SerializableScriptingObject(params)
{
    Tint = Color::White;
}
void UIBrush::Draw(const Rectangle& rect)
{
    Render2D::FillRectangle(rect, Tint);
}

void UIBrush::Serialize(SerializeStream& stream, const void* otherObj) 
{
    if (Tint != Color::White)
    {
        SERIALIZE_GET_OTHER_OBJ(UIBrush);
        stream.JKEY("Tint");
        stream.String(Tint.ToHexString().Get());
    }
}

void UIBrush::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    const auto e = stream.FindMember(rapidjson_flax::Value("Tint", (sizeof("Tint") / sizeof("Tint"[0])) - 1));
    if (e != stream.MemberEnd())
        Tint.FromHex(String(e->value.GetString()));
}
