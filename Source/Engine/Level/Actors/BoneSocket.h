// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"

/// <summary>
/// Actor that links to the animated model skeleton node transformation.
/// </summary>
API_CLASS() class FLAXENGINE_API BoneSocket : public Actor
{
DECLARE_SCENE_OBJECT(BoneSocket);

private:

    String _node;
    int32 _index;
    bool _useScale;

public:

    /// <summary>
    /// Gets the target node name to link to it.
    /// </summary>
    /// <returns>The target node name.</returns>
    API_PROPERTY(Attributes="EditorOrder(10), EditorDisplay(\"Bone Socket\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.SkeletonNodeEditor\")")
    FORCE_INLINE const String& GetNode() const
    {
        return _node;
    }

    /// <summary>
    /// Sets the target node to link to it.
    /// </summary>
    /// <param name="name">The target node name.</param>
    API_PROPERTY()
    void SetNode(const StringView& name);

    /// <summary>
    /// Gets the value indicating whenever use the target node scale. Otherwise won't override the actor scale.
    /// </summary>
    /// <returns>If set to <c>true</c> the node socket will use target node scale, otherwise it will be ignored.</returns>
    API_PROPERTY(Attributes="EditorOrder(20), EditorDisplay(\"Bone Socket\"), DefaultValue(false)")
    FORCE_INLINE bool GetUseScale() const
    {
        return _useScale;
    }

    /// <summary>
    /// Sets the value indicating whenever use the target node scale. Otherwise won't override the actor scale.
    /// </summary>
    /// <param name="value">If set to <c>true</c> the node socket will use target node scale, otherwise it will be ignored.</param>
    API_PROPERTY()
    void SetUseScale(bool value);

public:

    /// <summary>
    /// Updates the actor transformation based on a skeleton node.
    /// </summary>
    API_FUNCTION()
    void UpdateTransformation();

public:

    // [Actor]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:

    // [Actor]
    void OnTransformChanged() override;
    void OnParentChanged() override;
};
