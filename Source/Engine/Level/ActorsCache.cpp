// Copyright (c) Wojciech Figat. All rights reserved.

#include "ActorsCache.h"

CollectionPoolCache<ActorsCache::ActorsLookupType> ActorsCache::ActorsLookupCache;
CollectionPoolCache<ActorsCache::ActorsListType> ActorsCache::ActorsListCache;
CollectionPoolCache<ActorsCache::SceneObjectsListType> ActorsCache::SceneObjectsListCache;
