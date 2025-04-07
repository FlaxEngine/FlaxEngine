// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

class Foliage;
class FoliageCluster;
class FoliageType;
struct FoliageInstance;

// Enable/disable foliage editing and changing at runtime. If your game need to use procedural foliage then enable this option.
#define FOLIAGE_EDITING (USE_EDITOR)

// Enables using single quad-tree acceleration structure per foliage actor, otherwise will use quad-tree per foliage type to optimize drawing performance at a cost of higher memory usage.
#define FOLIAGE_USE_SINGLE_QUAD_TREE 0

// Enables using manual draw calls batching instead of using automated generic solution in RenderList. Boosts performance for large foliage.
#define FOLIAGE_USE_DRAW_CALLS_BATCHING 1

// Size of the instance allocation chunks (number of instances per allocated page)
#define FOLIAGE_INSTANCE_CHUNKS_SIZE (4096*4)

// Size of the cluster allocation chunks (number of clusters per allocated page)
#define FOLIAGE_CLUSTER_CHUNKS_SIZE (2048)

// Size of the cluster container for instances
#define FOLIAGE_CLUSTER_CAPACITY (64)
