// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/SceneObject.h"
#include "ScriptingObject.h"

/// <summary>
/// Base class for all scripts.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API Script : public SceneObject
{
    DECLARE_SCRIPTING_TYPE(Script);
    friend Actor;
    friend SceneTicking;
    friend class PrefabInstanceData;
protected:
    uint16 _enabled : 1;
    uint16 _tickFixedUpdate : 1;
    uint16 _tickUpdate : 1;
    uint16 _tickLateUpdate : 1;
    uint16 _tickLateFixedUpdate : 1;
    uint16 _wasAwakeCalled : 1;
    uint16 _wasStartCalled : 1;
    uint16 _wasEnableCalled : 1;
#if USE_EDITOR
    uint16 _executeInEditor : 1;
#endif

public:
    /// <summary>
    /// Gets value indicating if script is active.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor")
    FORCE_INLINE bool GetEnabled() const
    {
        return _enabled != 0;
    }

    /// <summary>
    /// Sets enabled state if this script.
    /// </summary>
    API_PROPERTY() void SetEnabled(bool value);

    /// <summary>
    /// Gets value indicating if script is enabled and active in the scene graph. It must be active as well as all it's parents.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize") bool IsEnabledInHierarchy() const;

    /// <summary>
    /// Gets the actor owning that script.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoAnimate")
    Actor* GetActor() const;

    /// <summary>
    /// Sets the actor owning that script.
    /// </summary>
    API_PROPERTY() void SetActor(Actor* value);

public:
    /// <summary>
    /// Called after the object is loaded.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnAwake()
    {
    }

    /// <summary>
    /// Called when object becomes enabled and active.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnEnable()
    {
    }

    /// <summary>
    /// Called when object becomes disabled and inactive.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnDisable()
    {
    }

    /// <summary>
    /// Called before the object will be destroyed.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnDestroy()
    {
    }

    /// <summary>
    /// Called when a script is enabled just before any of the Update methods is called for the first time.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnStart()
    {
    }

    /// <summary>
    /// Called every frame if object is enabled.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnUpdate()
    {
    }

    /// <summary>
    /// Called every frame (after gameplay Update) if object is enabled.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnLateUpdate()
    {
    }

    /// <summary>
    /// Called every fixed framerate frame if object is enabled.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnFixedUpdate()
    {
    }

    /// <summary>
    /// Called every fixed framerate frame (after FixedUpdate) if object is enabled.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnLateFixedUpdate()
    {
    }

    /// <summary>
    /// Called during drawing debug shapes in editor. Use <see cref="DebugDraw"/> to draw debug shapes and other visualization.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnDebugDraw()
    {
    }

    /// <summary>
    /// Called during drawing debug shapes in editor when object is selected. Use <see cref="DebugDraw"/> to draw debug shapes and other visualization.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnDebugDrawSelected()
    {
    }

private:
    void SetupType();
    void Start();
    void Enable();
    void Disable();

public:
    // [ScriptingObject]
    String ToString() const override;
    void OnDeleteObject() override;

    // [SceneObject]
    const Guid& GetSceneObjectId() const override;
    void SetParent(Actor* value, bool canBreakPrefabLink = true) override;
    int32 GetOrderInParent() const override;
    void SetOrderInParent(int32 index) override;
    void Initialize() override;
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
