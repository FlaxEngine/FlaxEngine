// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// The material slot descriptor that specifies how to render geometry using it.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API MaterialSlot : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(MaterialSlot, ScriptingObject);

    /// <summary>
    /// The material to use for rendering.
    /// </summary>
    API_FIELD() AssetReference<MaterialBase> Material;

    /// <summary>
    /// The shadows casting mode by this visual element.
    /// </summary>
    API_FIELD() ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// The slot name.
    /// </summary>
    API_FIELD() String Name;
};
