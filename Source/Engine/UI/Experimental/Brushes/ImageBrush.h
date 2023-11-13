// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once
#include "Engine/UI/Experimental/Common.h"

API_CLASS(Namespace = "FlaxEngine.Experimental.UI") 
class FLAXENGINE_API ImageBrush : public ScriptingObject, public IBrush
{
    DECLARE_SCRIPTING_TYPE(ImageBrush);
public:
    API_FIELD()
        AssetReference<Texture> Image;
    API_FIELD()
        Int2 ImageSize = Int2(32,32);
    API_FIELD()
        Color Tint = Color::White;

    // Inherited via IBrush

    void OnPreCunstruct(bool isInDesigner) override;

    void OnCunstruct() override;

    void OnDraw(const Float2& At) override;

    void OnDestruct() override;

    void OnImageAssetChanged();
};
