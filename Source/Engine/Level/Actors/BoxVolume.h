// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"

/// <summary>
/// A base class for actors that define 3D bounding box volume.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BoxVolume : public Actor
{
    DECLARE_SCENE_OBJECT(BoxVolume);
protected:
    Vector3 _size;
    OrientedBoundingBox _bounds;

public:
    /// <summary>
    /// Gets the size of the volume (in local space).
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"Box Volume\"), DefaultValue(typeof(Vector3), \"1000,1000,1000\"), EditorOrder(0)")
    FORCE_INLINE Vector3 GetSize() const
    {
        return _size;
    }

    /// <summary>
    /// Sets the size of the volume (in local space).
    /// </summary>
    API_PROPERTY() void SetSize(const Vector3& value);

    /// <summary>
    /// Gets the volume bounding box (oriented in world space).
    /// </summary>
    API_PROPERTY() FORCE_INLINE OrientedBoundingBox GetOrientedBox() const
    {
        return _bounds;
    }

protected:
    virtual void OnBoundsChanged(const BoundingBox& prevBounds)
    {
    }

#if USE_EDITOR
    virtual Color GetWiresColor();
#endif

public:
    // [Actor]
#if USE_EDITOR
    void OnDebugDraw() override;
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
    void OnTransformChanged() override;
};
