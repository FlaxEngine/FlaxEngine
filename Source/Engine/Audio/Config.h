// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"
#include "Engine/Content/Config.h"

// The maximum amount of listeners used at once
#define AUDIO_MAX_LISTENERS 1

// The maximum amount of audio emitter buffers
#define AUDIO_MAX_SOURCE_BUFFERS (ASSET_FILE_DATA_CHUNKS)
