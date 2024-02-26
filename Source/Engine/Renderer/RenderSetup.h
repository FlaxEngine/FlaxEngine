﻿// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/RenderTask.h"

/// <summary>
/// The utility container for rendering setup configuration. Allows to properly decide about using certain render features (eg. motion vectors, TAA jitter) before rendering happens.
/// </summary>
struct FLAXENGINE_API RenderSetup
{
    RenderingUpscaleLocation UpscaleLocation = RenderingUpscaleLocation::AfterAntiAliasingPass;
    bool UseMotionVectors = false;
    bool UseTemporalAAJitter = false;
};
