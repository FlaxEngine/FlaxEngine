// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Guid.h"

/// <summary>
/// Represents the reference to the scene asset. Stores the unique ID of the scene to reference. Can be used to load the selected scene.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API SceneReference
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(SceneReference);

    /// <summary>
    /// The identifier of the scene asset (and the scene object).
    /// </summary>
    API_FIELD() Guid ID;
};
