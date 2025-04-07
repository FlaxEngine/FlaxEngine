// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

// Amount of content loading threads per single physical CPU core
#define LOADING_THREAD_PER_LOGICAL_CORE 0.5f

// Enables additional assets metadata verification
#define ASSETS_LOADING_EXTRA_VERIFICATION (BUILD_DEBUG || USE_EDITOR)

// Maximum amount of data chunks used by the single asset
#define ASSET_FILE_DATA_CHUNKS 16

// Enables searching workspace for missing assets
#define ENABLE_ASSETS_DISCOVERY (USE_EDITOR)

// Default extension for all asset files
#define ASSET_FILES_EXTENSION TEXT("flax")
#define ASSET_FILES_EXTENSION_WITH_DOT TEXT(".flax")

// Default extension for all package files
#define PACKAGE_FILES_EXTENSION TEXT("flaxpac")
#define PACKAGE_FILES_EXTENSION_WITH_DOT TEXT(".flaxpac")
