// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "StreamableResource.h"
#include "StreamingManager.h"

StreamableResource::StreamableResource(StreamingGroup* group)
    : _group(group)
    , _isDynamic(true)
    , _isStreaming(false)
    , _streamingQuality(MAX_STREAMING_QUALITY)
{
    ASSERT(_group != nullptr);
}

StreamableResource::~StreamableResource()
{
    stopStreaming();
}

void StreamableResource::startStreaming(bool isDynamic)
{
    _isDynamic = isDynamic;

    if (_isStreaming == false)
    {
        StreamingManager::Resources.Add(this);
        _isStreaming = true;
    }
}

void StreamableResource::stopStreaming()
{
    if (_isStreaming == true)
    {
        StreamingManager::Resources.Remove(this);
        Streaming = StreamingCache();
        _isStreaming = false;
    }
}
