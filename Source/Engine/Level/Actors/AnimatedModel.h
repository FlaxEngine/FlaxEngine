// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Content/Assets/AnimationGraph.h"
#include "Engine/Graphics/Models/SkinnedMeshDrawData.h"

/// <summary>
/// Performs an animation and renders a skinned model.
/// </summary>
API_CLASS() class FLAXENGINE_API AnimatedModel : public ModelInstanceActor
{
DECLARE_SCENE_OBJECT(AnimatedModel);
    friend class AnimationManagerService;
public:

    /// <summary>
    /// Describes the animation graph updates frequency for the animated model.
    /// </summary>
    API_ENUM() enum class AnimationUpdateMode
    {
        /// <summary>
        /// The automatic updates will be used (based on platform capabilities, distance to the player, etc.).
        /// </summary>
        Auto = 0,

        /// <summary>
        /// Animation will be updated every game update.
        /// </summary>
        EveryUpdate = 1,

        /// <summary>
        /// Animation will be updated every second game update.
        /// </summary>
        EverySecondUpdate = 2,

        /// <summary>
        /// Animation will be updated every fourth game update.
        /// </summary>
        EveryFourthUpdate = 3,

        /// <summary>
        /// Animation can be updated manually by the user scripts.
        /// </summary>
        Manual = 4,

        /// <summary>
        /// Animation won't be updated at all.
        /// </summary>
        Never = 5,
    };

private:

    BoundingBox _boxLocal;
    Matrix _world;
    GeometryDrawStateData _drawState;
    SkinnedMeshDrawData _skinningData;
    AnimationUpdateMode _actualMode;
    uint32 _counter;
    float _lastMinDstSqr;
    uint64 _lastUpdateFrame;
    BlendShapesInstance _blendShapes;

public:

    /// <summary>
    /// The skinned model asset used for rendering.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(null), EditorDisplay(\"Skinned Model\")")
    AssetReference<SkinnedModel> SkinnedModel;

    /// <summary>
    /// The animation graph asset used for the skinned mesh skeleton bones evaluation (controls the animation).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(15), DefaultValue(null), EditorDisplay(\"Skinned Model\")")
    AssetReference<AnimationGraph> AnimationGraph;

    /// <summary>
    /// If true, use per-bone motion blur on this skeletal model. It requires additional rendering, can be disabled to save performance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(true), EditorDisplay(\"Skinned Model\")")
    bool PerBoneMotionBlur = true;

    /// <summary>
    /// If true, animation speed will be affected by the global time scale parameter.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(true), EditorDisplay(\"Skinned Model\")")
    bool UseTimeScale = true;

    /// <summary>
    /// If true, the animation will be updated even when an actor cannot be seen by any camera. Otherwise, the animations themselves will also stop running when the actor is off-screen.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(false), EditorDisplay(\"Skinned Model\")")
    bool UpdateWhenOffscreen = false;

    /// <summary>
    /// The animation update mode. Can be used to optimize the performance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(AnimationUpdateMode.Auto), EditorDisplay(\"Skinned Model\")")
    AnimationUpdateMode UpdateMode = AnimationUpdateMode::Auto;

    /// <summary>
    /// The master scale parameter for the actor bounding box. Helps reducing mesh flickering effect on screen edges.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60), DefaultValue(1.5f), Limit(0), EditorDisplay(\"Skinned Model\")")
    float BoundsScale = 1.5f;

    /// <summary>
    /// The custom bounds(in actor local space). If set to empty bounds then source skinned model bind pose bounds will be used.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(70), EditorDisplay(\"Skinned Model\")")
    BoundingBox CustomBounds = BoundingBox::Zero;

    /// <summary>
    /// The model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(80), DefaultValue(0), Limit(-100, 100, 0.1f), EditorDisplay(\"Skinned Model\", \"LOD Bias\")")
    int32 LODBias = 0;

    /// <summary>
    /// Gets the model forced Level Of Detail index. Allows to bind the given model LOD to show. Value -1 disables this feature.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(90), DefaultValue(-1), Limit(-1, 100, 0.1f), EditorDisplay(\"Skinned Model\", \"Forced LOD\")")
    int32 ForcedLOD = -1;

    /// <summary>
    /// The draw passes to use for rendering this object.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), DefaultValue(DrawPass.Default), EditorDisplay(\"Skinned Model\")")
    DrawPass DrawModes = DrawPass::Default;

    /// <summary>
    /// The shadows casting mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110), DefaultValue(ShadowsCastingMode.All), EditorDisplay(\"Skinned Model\")")
    ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// The animation root motion apply target. If not specified the animated model will apply it itself.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(120), DefaultValue(null), EditorDisplay(\"Skinned Model\")")
    ScriptingObjectReference<Actor> RootMotionTarget;

public:

    /// <summary>
    /// The graph instance data container. For dynamic usage only at runtime, not serialized.
    /// </summary>
    AnimGraphInstanceData GraphInstance;

    /// <summary>
    /// Gets the model world matrix transform.
    /// </summary>
    FORCE_INLINE void GetWorld(Matrix* world) const
    {
        *world = _world;
    }

    /// <summary>
    /// Resets the animation state (clears the instance state data but preserves the instance parameters values).
    /// </summary>
    API_FUNCTION() void ResetAnimation();

    /// <summary>
    /// Performs the full animation update.
    /// </summary>
    API_FUNCTION() void UpdateAnimation();

    /// <summary>
    /// Validates and creates a proper skinning data.
    /// </summary>
    API_FUNCTION() void SetupSkinningData();

    /// <summary>
    /// Creates and setups the skinning data (writes the identity bones transformations).
    /// </summary>
    API_FUNCTION() void PreInitSkinningData();

    /// <summary>
    /// Updates the child bone socket actors.
    /// </summary>
    API_FUNCTION() void UpdateSockets();

    /// <summary>
    /// Gets the per-node final transformations.
    /// </summary>
    /// <param name="nodesTransformation">The output per-node final transformation matrices.</param>
    /// <param name="worldSpace">True if convert matrices into world-space, otherwise returned values will be in local-space of the actor.</param>
    API_FUNCTION() void GetCurrentPose(API_PARAM(Out) Array<Matrix>& nodesTransformation, bool worldSpace = false) const;

    /// <summary>
    /// Gets the node final transformation.
    /// </summary>
    /// <param name="nodeIndex">The index of the skinned model skeleton node.</param>
    /// <param name="nodeTransformation">The output final node transformation matrix.</param>
    /// <param name="worldSpace">True if convert matrices into world-space, otherwise returned values will be in local-space of the actor.</param>
    API_FUNCTION() void GetNodeTransformation(int32 nodeIndex, API_PARAM(Out) Matrix& nodeTransformation, bool worldSpace = false) const;

    /// <summary>
    /// Gets the node final transformation.
    /// </summary>
    /// <param name="nodeName">The name of the skinned model skeleton node.</param>
    /// <param name="nodeTransformation">The output final node transformation matrix.</param>
    /// <param name="worldSpace">True if convert matrices into world-space, otherwise returned values will be in local-space of the actor.</param>
    API_FUNCTION() void GetNodeTransformation(const StringView& nodeName, API_PARAM(Out) Matrix& nodeTransformation, bool worldSpace = false) const;

public:

    /// <summary>
    /// Gets the anim graph instance parameters collection.
    /// </summary>
    API_PROPERTY() const Array<AnimGraphParameter>& GetParameters() const
    {
        return GraphInstance.Parameters;
    }

    /// <summary>
    /// Gets the anim graph instance parameter by name.
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <returns>The parameters.</returns>
    API_FUNCTION() AnimGraphParameter* GetParameter(const StringView& name);

    /// <summary>
    /// Gets the anim graph instance parameter value.
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <returns>The value.</returns>
    API_FUNCTION() Variant GetParameterValue(const StringView& name);

    /// <summary>
    /// Sets the anim graph instance parameter value.
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <param name="value">The value to set.</param>
    API_FUNCTION() void SetParameterValue(const StringView& name, const Variant& value);

    /// <summary>
    /// Gets the anim graph instance parameter value.
    /// </summary>
    /// <param name="id">The parameter id.</param>
    /// <returns>The value.</returns>
    API_FUNCTION() Variant GetParameterValue(const Guid& id);

    /// <summary>
    /// Sets the anim graph instance parameter value.
    /// </summary>
    /// <param name="id">The parameter id.</param>
    /// <param name="value">The value to set.</param>
    API_FUNCTION() void SetParameterValue(const Guid& id, const Variant& value);

public:

    /// <summary>
    /// Gets the weight of the blend shape.
    /// </summary>
    /// <param name="name">The blend shape name.</param>
    /// <returns>The normalized weight of the blend shape (in range -1:1).</returns>
    API_FUNCTION() float GetBlendShapeWeight(const StringView& name);

    /// <summary>
    /// Sets the weight of the blend shape.
    /// </summary>
    /// <param name="name">The blend shape name.</param>
    /// <param name="value">The normalized weight of the blend shape (in range -1:1).</param>
    API_FUNCTION() void SetBlendShapeWeight(const StringView& name, float value);

    /// <summary>
    /// Clears the weights of the blend shapes (disabled any used blend shapes).
    /// </summary>
    API_FUNCTION() void ClearBlendShapeWeights();

private:

    /// <summary>
    /// Applies the root motion delta to the target actor.
    /// </summary>
    void ApplyRootMotion(const RootMotionData& rootMotionDelta);

    /// <summary>
    /// Synchronizes the parameters collection (may lost existing params data on collection change detected).
    /// </summary>
    void SyncParameters();

    /// <summary>
    /// Updates the local bounds of the actor.
    /// </summary>
    void UpdateLocalBounds();

    void UpdateBounds();

    /// <summary>
    /// Called after animation graph update.
    /// </summary>
    void OnAnimUpdate();

    void OnSkinnedModelChanged();
    void OnSkinnedModelLoaded();

    void OnGraphChanged();
    void OnGraphLoaded();

    void Update();

public:

    // [ModelInstanceActor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void DrawGeneric(RenderContext& renderContext) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    bool IntersectsItself(const Ray& ray, float& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool IntersectsEntry(int32 entryIndex, const Ray& ray, float& distance, Vector3& normal) override;
    bool IntersectsEntry(const Ray& ray, float& distance, Vector3& normal, int32& entryIndex) override;

protected:

    // [ModelInstanceActor]
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnActiveInTreeChanged() override;
    void OnTransformChanged() override;
};
