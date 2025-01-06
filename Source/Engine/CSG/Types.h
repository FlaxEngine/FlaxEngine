// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"
#include "Engine/Level/Actors/BrushMode.h"

namespace CSG
{
    typedef ::BrushMode Mode;

    struct MeshVertex
    {
        Float3 Position;
        Float2 TexCoord;
        Float3 Normal;
        Float3 Tangent;
        Float3 Bitangent;
        Float2 LightmapUVs;
    };
};
