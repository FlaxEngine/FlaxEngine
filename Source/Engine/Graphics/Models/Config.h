// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../Config.h"

// The maximum allowed amount of material slots per model resource
#define MAX_MATERIAL_SLOTS 4096

// Maximum amount of levels of detail for the model
#define MODEL_MAX_LODS 6

// Maximum amount of meshes per model LOD
#define MODEL_MAX_MESHES 4096

// Enable/disable precise mesh collision testing (with in-build vertex buffer caching, this will increase memory usage)
#define USE_PRECISE_MESH_INTERSECTS (USE_EDITOR)

// Defines the maximum amount of bones affecting every vertex of the skinned mesh
#define MAX_BONES_PER_VERTEX 4

// Defines the maximum allowed amount of skeleton bones to be used with skinned model
#define MAX_BONES_PER_MODEL 256
