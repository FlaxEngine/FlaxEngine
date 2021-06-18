// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "StreamableResource.h"
#include "Streaming.h"

StreamableResource::StreamableResource(StreamingGroup* group)
    : _group(group)
    , _isDynamic(true)
    , _isStreaming(false)
    , _streamingQuality(1.0f)
{
    ASSERT(_group != nullptr);
}

StreamableResource::~StreamableResource()
{
    StopStreaming();
}

void StreamableResource::StartStreaming(bool isDynamic)
{
    _isDynamic = isDynamic;

    if (_isStreaming == false)
    {
        Streaming::Resources.Add(this);
        _isStreaming = true;
    }
}

void StreamableResource::StopStreaming()
{
    if (_isStreaming == true)
    {
        Streaming::Resources.Remove(this);
        Streaming = StreamingCache();
        _isStreaming = false;
    }
}
