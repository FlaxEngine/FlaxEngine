// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

// Forward declarations
class Foliage;
class FoliageCluster;
class FoliageType;
struct FoliageInstance;

// Enable/disable foliage editing and changing at runtime. If your game need to use procedural foliage then enable this option.
#define FOLIAGE_EDITING (USE_EDITOR)

// Size of the instance allocation chunks (number of instances per allocated page)
#define FOLIAGE_INSTANCE_CHUNKS_SIZE (4096*4)

// Size of the cluster allocation chunks (number of clusters per allocated page)
#define FOLIAGE_CLUSTER_CHUNKS_SIZE (2048)

// Size of the cluster container for instances
#define FOLIAGE_CLUSTER_CAPACITY (64)
