// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Config.h"

// The maximum allowed amount of material slots per model resource
#define MODEL_MAX_MATERIAL_SLOTS 4096
// [Deprecated in v1.10] Use MODEL_MAX_MATERIAL_SLOTS
#define MAX_MATERIAL_SLOTS MODEL_MAX_MATERIAL_SLOTS

// Maximum amount of levels of detail for the model
#define MODEL_MAX_LODS 6

// Maximum amount of meshes per model LOD
#define MODEL_MAX_MESHES 4096

// Maximum amount of texture channels (UVs) per vertex
#define MODEL_MAX_UV 4

// Maximum amount of vertex buffers (VBs) per mesh
#define MODEL_MAX_VB 3

// Enable/disable precise mesh collision testing (with in-build vertex buffer caching, this will increase memory usage)
#define MODEL_USE_PRECISE_MESH_INTERSECTS (USE_EDITOR)
// [Deprecated in v1.10] Use MODEL_USE_PRECISE_MESH_INTERSECTS
#define USE_PRECISE_MESH_INTERSECTS MODEL_USE_PRECISE_MESH_INTERSECTS

// Defines the maximum amount of bones affecting every vertex of the skinned mesh
#define MODEL_MAX_BONES_PER_VERTEX 4
// [Deprecated in v1.10] Use MODEL_MAX_BONES_PER_VERTEX
#define MAX_BONES_PER_VERTEX MODEL_MAX_BONES_PER_VERTEX

// Defines the maximum allowed amount of skeleton bones to be used with skinned model
#define MODEL_MAX_BONES_PER_MODEL 0xffff
// [Deprecated in v1.10] Use MODEL_MAX_BONES_PER_MODEL
#define MAX_BONES_PER_MODEL MODEL_MAX_BONES_PER_MODEL
