#pragma once
#include "Engine/UI/Experimental/Types/IBrush.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI") 
class FLAXENGINE_API ImageBrush : public ScriptingObject, public IBrush
{
    DECLARE_SCRIPTING_TYPE(ImageBrush);
public:
    API_FIELD()
        AssetReference<Texture> Image;
    API_FIELD()
        Int2 ImageSize;
    API_FIELD()
        Color Tint = Color::White;

    // Inherited via IBrush

    void OnPreCunstruct(bool isInDesigner) override;

    void OnCunstruct() override;

    void OnDraw(const Float2& At) override;

    void OnDestruct() override;

    Float2 GetDesiredSize() override;

    void OnImageAssetChanged();
};
