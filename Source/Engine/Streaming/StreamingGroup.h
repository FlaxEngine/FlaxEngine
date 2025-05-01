// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IStreamingHandler.h"
#include "Engine/Core/Enums.h"
#include "Engine/Core/Singleton.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Describes streamable resources group object.
/// </summary>
class FLAXENGINE_API StreamingGroup
{
public:

    DECLARE_ENUM_4(Type, Custom, Textures, Models, Audio);

protected:

    Type _type;
    IStreamingHandler* _handler;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="StreamingGroup"/> class.
    /// </summary>
    /// <param name="type">The group type.</param>
    /// <param name="handler">Group dedicated handler.</param>
    StreamingGroup(Type type, IStreamingHandler* handler);

public:

    /// <summary>
    /// Gets the group type.
    /// </summary>
    FORCE_INLINE Type GetType() const
    {
        return _type;
    }

    /// <summary>
    /// Gets the group type name.
    /// </summary>
    FORCE_INLINE const Char* GetTypename() const
    {
        return ToString(_type);
    }

    /// <summary>
    /// Gets the group streaming handler used by this group.
    /// </summary>
    FORCE_INLINE IStreamingHandler* GetHandler() const
    {
        return _handler;
    }
};

/// <summary>
/// Streaming groups manager.
/// </summary>
class FLAXENGINE_API StreamingGroups : public Singleton<StreamingGroups>
{
private:

    StreamingGroup* _textures;
    StreamingGroup* _models;
    StreamingGroup* _skinnedModels;
    StreamingGroup* _audio;

    Array<StreamingGroup*> _groups;
    Array<IStreamingHandler*> _handlers;

public:

    StreamingGroups();
    ~StreamingGroups();

public:

    /// <summary>
    /// Gets textures group.
    /// </summary>
    FORCE_INLINE StreamingGroup* Textures() const
    {
        return _textures;
    }

    /// <summary>
    /// Gets models group.
    /// </summary>
    FORCE_INLINE StreamingGroup* Models() const
    {
        return _models;
    }

    /// <summary>
    /// Gets skinned models group.
    /// </summary>
    FORCE_INLINE StreamingGroup* SkinnedModels() const
    {
        return _skinnedModels;
    }

    /// <summary>
    /// Gets audio group.
    /// </summary>
    FORCE_INLINE StreamingGroup* Audio() const
    {
        return _audio;
    }

public:

    /// <summary>
    /// Gets all the groups.
    /// </summary>
    FORCE_INLINE const Array<StreamingGroup*>& Groups() const
    {
        return _groups;
    }

    /// <summary>
    /// Gets all the handlers.
    /// </summary>
    FORCE_INLINE const Array<IStreamingHandler*>& Handlers() const
    {
        return _handlers;
    }

public:

    /// <summary>
    /// Adds the specified streaming group.
    /// </summary>
    /// <param name="group">The group.</param>
    void Add(StreamingGroup* group);
};
