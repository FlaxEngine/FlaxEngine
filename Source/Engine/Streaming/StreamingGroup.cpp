// Copyright (c) Wojciech Figat. All rights reserved.

#include "StreamingGroup.h"
#include "StreamingHandlers.h"

StreamingGroup::StreamingGroup(Type type, IStreamingHandler* handler)
    : _type(type)
    , _handler(handler)
{
    ASSERT(_handler != nullptr);
}

StreamingGroups::StreamingGroups()
    : _textures(nullptr)
    , _models(nullptr)
    , _skinnedModels(nullptr)
    , _audio(nullptr)
    , _groups(8)
{
    // Register in-build streaming groups
    Add(_textures = New<StreamingGroup>(StreamingGroup::Type::Textures, New<TexturesStreamingHandler>()));
    Add(_models = New<StreamingGroup>(StreamingGroup::Type::Models, New<ModelsStreamingHandler>()));
    Add(_skinnedModels = New<StreamingGroup>(StreamingGroup::Type::Models, New<SkinnedModelsStreamingHandler>()));
    Add(_audio = New<StreamingGroup>(StreamingGroup::Type::Audio, New<AudioStreamingHandler>()));
}

StreamingGroups::~StreamingGroups()
{
    // Cleanup
    _groups.ClearDelete();
    _handlers.ClearDelete();
}

void StreamingGroups::Add(StreamingGroup* group)
{
    ASSERT(group && !_groups.Contains(group));

    // Register group and it's handler
    _groups.Add(group);
    if (!_handlers.Contains(group->GetHandler()))
        _handlers.Add(group->GetHandler());
}
