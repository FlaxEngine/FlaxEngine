// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MATERIAL_GRAPH

#include "Engine/Core/Enums.h"
#include "Engine/Visject/ShaderGraph.h"

class MaterialGraph : public ShaderGraph<>
{
};

typedef ShaderGraphBox MaterialGraphBox;
typedef ShaderGraphParameter MaterialGraphParameter;
typedef ShaderGraphValue MaterialValue;

/// <summary>
/// Material function generate tree type
/// </summary>
DECLARE_ENUM_3(MaterialTreeType, VertexShader, DomainShader, PixelShader);

/// <summary>
/// Vector transformation coordinate systems.
/// </summary>
enum class TransformCoordinateSystem
{
    /// <summary>
    /// The world space. It's absolute world space coordinate system.
    /// </summary>
    World = 0,

    /// <summary>
    /// The tangent space. It's relative to the surface (tangent frame defined by normal, tangent and bitangent vectors).
    /// </summary>
    Tangent = 1,

    /// <summary>
    /// The view space. It's relative to the current rendered viewport orientation.
    /// </summary>
    View = 2,

    /// <summary>
    /// The local space. It's relative to the rendered object (aka object space).
    /// </summary>
    Local = 3,

    /// <summary>
    /// The count of the items in the enum.
    /// </summary>
    MAX
};

#define ROOT_NODE_TYPE GRAPH_NODE_MAKE_TYPE(1, 1)

class MaterialLayer;

inline bool CanUseSample(MaterialTreeType treeType)
{
    return treeType == MaterialTreeType::PixelShader;
}

#endif
