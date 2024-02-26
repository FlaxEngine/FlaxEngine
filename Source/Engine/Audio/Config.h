// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

// The maximum amount of listeners used at once
#define AUDIO_MAX_LISTENERS 8

// The maximum amount of audio emitter buffers
#define AUDIO_MAX_SOURCE_BUFFERS (ASSET_FILE_DATA_CHUNKS)

// The type of the audio source IDs used to identify it (per listener)
#define AUDIO_SOURCE_ID_TYPE uint32

// The type of the audio buffer IDs used to identify it
#define AUDIO_BUFFER_ID_TYPE uint32

// The buffer ID that is invalid (unused)
#define AUDIO_BUFFER_ID_INVALID 0
