// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/Script.h"
#if USE_EDITOR
#include "Engine/Tools/ModelTool/ModelTool.h"
#endif

/// <summary>
/// Actor script component that handled model prefabs importing and setup.
/// </summary>
API_CLASS(Attributes="HideInEditor") class FLAXENGINE_API ModelPrefab : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(ModelPrefab);

#if USE_EDITOR
    /// <summary>
    /// Source model file path (absolute or relative to the project).
    /// </summary>
    API_FIELD(Attributes="ReadOnly, HideInEditor") String ImportPath;

    /// <summary>
    /// Model file import settings.
    /// </summary>
    API_FIELD() ModelTool::Options ImportOptions;
#endif
};
