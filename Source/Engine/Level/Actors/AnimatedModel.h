// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Content/Assets/AnimationGraph.h"
#include "Engine/Graphics/Models/SkinnedMeshDrawData.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Core/Delegate.h"

/// <summary>
/// Performs an animation and renders a skinned model.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Animation/Animated Model\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API AnimatedModel : public ModelInstanceActor
{
    DECLARE_SCENE_OBJECT(AnimatedModel);
    friend class AnimationsSystem;

    /// <summary>
    /// Keeps the data of a Node and its relevant Transform Matrix together when passing it between functions.
    /// </summary>
    API_STRUCT() struct NodeTransformation
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(NodeTransformation);

        /// <summary>
        /// The index of the node in the node hierarchy.
        /// </summary>
        API_FIELD() uint32 NodeIndex;

        /// <summary>
        /// The transformation matrix of the node
        /// </summary>
        API_FIELD() Matrix NodeMatrix;
    };

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
    struct BlendShapeMesh
    {
        uint16 LODIndex;
        uint16 MeshIndex;
        uint32 Usages;
    };

    GeometryDrawStateData _drawState;
    SkinnedMeshDrawData _skinningData;
    AnimationUpdateMode _actualMode;
    uint32 _counter;
    Real _lastMinDstSqr;
    bool _isDuringUpdateEvent = false;
    uint64 _lastUpdateFrame;
    mutable MeshDeformation* _deformation = nullptr;
    ScriptingObjectReference<AnimatedModel> _masterPose;
    Array<Pair<String, float>> _blendShapeWeights;
    Array<BlendShapeMesh> _blendShapeMeshes;

public:
    ~AnimatedModel();

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
    /// If true, animation speed will be affected by the global timescale parameter.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(true), EditorDisplay(\"Skinned Model\")")
    bool UseTimeScale = true;

    /// <summary>
    /// If true, the animation will be updated even when an actor cannot be seen by any camera. Otherwise, the animations themselves will also stop running when the actor is off-screen.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(false), EditorDisplay(\"Skinned Model\")")
    bool UpdateWhenOffscreen = false;

    /// <summary>
    /// The animation update delta timescale. Can be used to speed up animation playback or create slow motion effect.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(45), Limit(0, float.MaxValue, 0.025f), EditorDisplay(\"Skinned Model\")")
    float UpdateSpeed = 1.0f;

    /// <summary>
    /// The animation update mode. Can be used to optimize the performance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(AnimationUpdateMode.Auto), EditorDisplay(\"Skinned Model\")")
    AnimationUpdateMode UpdateMode = AnimationUpdateMode::Auto;

    /// <summary>
    /// The master scale parameter for the actor bounding box. Helps to reduce mesh flickering effect on screen edges.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60), DefaultValue(1.5f), Limit(0, float.MaxValue, 0.025f), EditorDisplay(\"Skinned Model\")")
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
    /// The object sort order key used when sorting drawable objects during rendering. Use lower values to draw object before others, higher values are rendered later (on top). Can be used to control transparency drawing.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Skinned Model\"), EditorOrder(110), DefaultValue(0)")
    int8 SortOrder = 0;

    /// <summary>
    /// The shadows casting mode.
    /// [Deprecated on 26.10.2022, expires on 26.10.2024]
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110), DefaultValue(ShadowsCastingMode.All), EditorDisplay(\"Skinned Model\")")
    DEPRECATED() ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// The animation root motion apply target. If not specified the animated model will apply it itself.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(120), DefaultValue(null), EditorDisplay(\"Skinned Model\")")
    ScriptingObjectReference<Actor> RootMotionTarget;

#if USE_EDITOR
    /// <summary>
    /// If checked, the skeleton pose will be shawn during debug shapes drawing.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(200), EditorDisplay(\"Skinned Model\")") bool ShowDebugDrawSkeleton = false;
#endif

public:
    /// <summary>
    /// The graph instance data container. For dynamic usage only at runtime, not serialized.
    /// </summary>
    AnimGraphInstanceData GraphInstance;

    /// <summary>
    /// Resets the animation state (clears the instance state data but preserves the instance parameters values).
    /// </summary>
    API_FUNCTION() void ResetAnimation();

    /// <summary>
    /// Performs the full animation update. The actual update will be performed during gameplay tick.
    /// </summary>
    API_FUNCTION() void UpdateAnimation();

    /// <summary>
    /// Called after animation gets updated (new skeleton pose).
    /// </summary>
    API_EVENT() Action AnimationUpdated;

    /// <summary>
    /// Validates and creates a proper skinning data.
    /// </summary>
    API_FUNCTION() void SetupSkinningData();

    /// <summary>
    /// Creates and setups the skinning data (writes the identity bones transformations).
    /// </summary>
    API_FUNCTION() void PreInitSkinningData();

    /// <summary>
    /// Gets the per-node final transformations (skeleton pose).
    /// </summary>
    /// <param name="nodesTransformation">The output per-node final transformation matrices.</param>
    /// <param name="worldSpace">True if convert matrices into world-space, otherwise returned values will be in local-space of the actor.</param>
    API_FUNCTION() void GetCurrentPose(API_PARAM(Out) Array<Matrix>& nodesTransformation, bool worldSpace = false) const;

    /// <summary>
    /// Sets the per-node final transformations (skeleton pose).
    /// </summary>
    /// <param name="nodesTransformation">The per-node final transformation matrices.</param>
    /// <param name="worldSpace">True if convert matrices from world-space, otherwise values are in local-space of the actor.</param>
    API_FUNCTION() void SetCurrentPose(const Array<Matrix>& nodesTransformation, bool worldSpace = false);

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

    /// <summary>
    /// Gets the node final transformation for a series of nodes.
    /// </summary>
    /// <param name="nodeTransformations">The series of nodes that will be returned</param>
    /// <param name="worldSpace">True if convert matrices into world-space, otherwise returned values will be in local-space of the actor.</param>
    /// <returns></returns>
    API_FUNCTION() void GetNodeTransformation(API_PARAM(Ref) Array<NodeTransformation>& nodeTransformations, bool worldSpace = false) const;

    /// <summary>
    /// Sets the node final transformation. If multiple nodes are to be set within a frame, do not use set worldSpace to true, and do the conversion yourself to avoid recalculation of inv matrices. 
    /// </summary>
    /// <param name="nodeIndex">The index of the skinned model skeleton node.</param>
    /// <param name="nodeTransformation">The final node transformation matrix.</param>
    /// <param name="worldSpace">True if convert matrices from world-space, otherwise values will be in local-space of the actor.</param>
    API_FUNCTION() void SetNodeTransformation(int32 nodeIndex, const Matrix& nodeTransformation, bool worldSpace = false);

    /// <summary>
    /// Sets the node final transformation. If multiple nodes are to be set within a frame, do not use set worldSpace to true, and do the conversion yourself to avoid recalculation of inv matrices. 
    /// </summary>
    /// <param name="nodeName">The name of the skinned model skeleton node.</param>
    /// <param name="nodeTransformation">The final node transformation matrix.</param>
    /// <param name="worldSpace">True if convert matrices from world-space, otherwise values will be in local-space of the actor.</param>
    API_FUNCTION() void SetNodeTransformation(const StringView& nodeName, const Matrix& nodeTransformation, bool worldSpace = false);

    /// <summary>
    /// Sets a group of nodes final transformation.
    /// </summary>
    /// <param name="nodeTransformations">Array of the final node transformation matrix.</param>
    /// <param name="worldSpace">True if convert matrices from world-space, otherwise values will be in local-space of the actor.</param>
    /// <returns></returns>
    API_FUNCTION() void SetNodeTransformation(const Array<NodeTransformation>& nodeTransformations, bool worldSpace = false);

    /// <summary>
    /// Finds the closest node to a given location.
    /// </summary>
    /// <param name="location">The text location (in local-space of the actor or world-space depending on <paramref name="worldSpace"/>).</param>
    /// <param name="worldSpace">True if convert input location is in world-space, otherwise it's in local-space of the actor.</param>
    /// <returns>The zero-based index of the found node. Returns -1 if skeleton is missing.</returns>
    API_FUNCTION() int32 FindClosestNode(const Vector3& location, bool worldSpace = false) const;

    /// <summary>
    /// Sets the master pose model that will be used to copy the skeleton nodes animation. Useful for modular characters.
    /// </summary>
    /// <param name="masterPose">The master pose actor to use.</param>
    API_FUNCTION() void SetMasterPoseModel(AnimatedModel* masterPose);

    /// <summary>
    /// Enables extracting animation playback insights for debugging or custom scripting.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize") bool GetEnableTracing() const
    {
        return GraphInstance.EnableTracing;
    }

    /// <summary>
    /// Enables extracting animation playback insights for debugging or custom scripting.
    /// </summary>
    API_PROPERTY() void SetEnableTracing(bool value)
    {
        GraphInstance.EnableTracing = value;
    }

    /// <summary>
    /// Gets the trace events from the last animation update. Valid only when EnableTracing is active.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize") const Array<AnimGraphTraceEvent>& GetTraceEvents() const;

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
    API_FUNCTION() const Variant& GetParameterValue(const StringView& name) const;

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
    API_FUNCTION() const Variant& GetParameterValue(const Guid& id) const;

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
    /// Clears the weights of the blend shapes (disables any used blend shapes).
    /// </summary>
    API_FUNCTION() void ClearBlendShapeWeights();

public:
    /// <summary>
    /// Plays the animation on the slot in Anim Graph.
    /// </summary>
    /// <param name="slotName">The name of the slot.</param>
    /// <param name="anim">The animation to play.</param>
    /// <param name="speed">The playback speed.</param>
    /// <param name="blendInTime">The animation blending in time (in seconds). Cam be used to smooth the slot animation playback with the input pose when starting the animation.</param>
    /// <param name="blendOutTime">The animation blending out time (in seconds). Cam be used to smooth the slot animation playback with the input pose when ending animation.</param>
    /// <param name="loopCount">The amount of loops to play the animation: 0 to play once, -1 to play infinite, 1 or higher to loop once or more.</param>
    API_FUNCTION() void PlaySlotAnimation(const StringView& slotName, Animation* anim, float speed = 1.0f, float blendInTime = 0.2f, float blendOutTime = 0.2f, int32 loopCount = 0);

    /// <summary>
    /// Stops all the animations playback on the all slots in Anim Graph.
    /// </summary>
    API_FUNCTION() void StopSlotAnimation();

    /// <summary>
    /// Stops the animation playback on the slot in Anim Graph.
    /// </summary>
    /// <param name="slotName">The name of the slot.</param>
    /// <param name="anim">The animation to stop.</param>
    API_FUNCTION() void StopSlotAnimation(const StringView& slotName, Animation* anim);

    /// <summary>
    /// Pauses all the animations playback on the all slots in Anim Graph.
    /// </summary>
    API_FUNCTION() void PauseSlotAnimation();

    /// <summary>
    /// Pauses the animation playback on the slot in Anim Graph.
    /// </summary>
    /// <param name="slotName">The name of the slot.</param>
    /// <param name="anim">The animation to pause.</param>
    API_FUNCTION() void PauseSlotAnimation(const StringView& slotName, Animation* anim);

    /// <summary>
    /// Checks if any  animation playback is active on any  slot in Anim Graph (not paused).
    /// </summary>
    API_FUNCTION() bool IsPlayingSlotAnimation();

    /// <summary>
    /// Checks if the animation playback is active on the slot in Anim Graph (not paused).
    /// </summary>
    /// <param name="slotName">The name of the slot.</param>
    /// <param name="anim">The animation to check.</param>
    API_FUNCTION() bool IsPlayingSlotAnimation(const StringView& slotName, Animation* anim);

private:
    void ApplyRootMotion(const Transform& rootMotionDelta);
    void SyncParameters();
    void RunBlendShapeDeformer(const MeshBase* mesh, struct MeshDeformationData& deformation);

    void Update();
    void UpdateSockets();
    void OnAnimationUpdated_Async();
    void OnAnimationUpdated_Sync();
    void OnAnimationUpdated();

    void OnSkinnedModelChanged();
    void OnSkinnedModelLoaded();

    void OnGraphChanged();
    void OnGraphLoaded();

public:
    // [ModelInstanceActor]
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void Draw(RenderContextBatch& renderContextBatch) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
    void OnDebugDraw() override;
    BoundingBox GetEditorBox() const override;
#endif
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    const Span<MaterialSlot> GetMaterialSlots() const override;
    MaterialBase* GetMaterial(int32 entryIndex) override;
    bool IntersectsEntry(int32 entryIndex, const Ray& ray, Real& distance, Vector3& normal) override;
    bool IntersectsEntry(const Ray& ray, Real& distance, Vector3& normal, int32& entryIndex) override;
    bool GetMeshData(const MeshReference& ref, MeshBufferType type, BytesContainer& result, int32& count, GPUVertexLayout** layout) const override;
    void UpdateBounds() override;
    MeshDeformation* GetMeshDeformation() const override;
    void OnDeleteObject() override;

protected:
    // [ModelInstanceActor]
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnActiveInTreeChanged() override;
    void WaitForModelLoad() override;
};
