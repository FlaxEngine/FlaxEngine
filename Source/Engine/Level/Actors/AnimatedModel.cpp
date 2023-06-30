// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "AnimatedModel.h"
#include "BoneSocket.h"
#include "Engine/Core/Math/Matrix3x4.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Animations/Animations.h"
#include "Engine/Engine/Engine.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Serialization/Serialization.h"

AnimatedModel::AnimatedModel(const SpawnParams& params)
    : ModelInstanceActor(params)
    , _actualMode(AnimationUpdateMode::Never)
    , _counter(0)
    , _lastMinDstSqr(MAX_Real)
    , _lastUpdateFrame(0)
{
    _drawCategory = SceneRendering::SceneDrawAsync;
    GraphInstance.Object = this;
    _box = BoundingBox(Vector3::Zero);
    _sphere = BoundingSphere(Vector3::Zero, 0.0f);

    SkinnedModel.Changed.Bind<AnimatedModel, &AnimatedModel::OnSkinnedModelChanged>(this);
    SkinnedModel.Loaded.Bind<AnimatedModel, &AnimatedModel::OnSkinnedModelLoaded>(this);
    AnimationGraph.Changed.Bind<AnimatedModel, &AnimatedModel::OnGraphChanged>(this);
    AnimationGraph.Loaded.Bind<AnimatedModel, &AnimatedModel::OnGraphLoaded>(this);
}

void AnimatedModel::ResetAnimation()
{
    GraphInstance.ClearState();
}

void AnimatedModel::UpdateAnimation()
{
    // Skip if need to
    if (UpdateMode == AnimationUpdateMode::Never
        || !IsActiveInHierarchy()
        || SkinnedModel == nullptr
        || !SkinnedModel->IsLoaded()
        || _lastUpdateFrame == Engine::FrameCount
        || _masterPose)
        return;
    _lastUpdateFrame = Engine::FrameCount;

    if (AnimationGraph && AnimationGraph->IsLoaded() && AnimationGraph->Graph.IsReady())
    {
        // Request an animation update
        Animations::AddToUpdate(this);
    }
    else
    {
        // Allow to use blend shapes without animation graph assigned
        _blendShapes.Update(SkinnedModel.Get());
    }
}

void AnimatedModel::SetupSkinningData()
{
    ASSERT(SkinnedModel && SkinnedModel->IsLoaded());

    const int32 targetBonesCount = SkinnedModel->Skeleton.Bones.Count();
    const int32 currentBonesCount = _skinningData.BonesCount;

    if (targetBonesCount != currentBonesCount)
    {
        _skinningData.Setup(targetBonesCount);
    }
}

void AnimatedModel::PreInitSkinningData()
{
    if (!SkinnedModel || !SkinnedModel->IsLoaded())
        return;

    ScopeLock lock(SkinnedModel->Locker);

    SetupSkinningData();
    auto& skeleton = SkinnedModel->Skeleton;
    const int32 bonesCount = skeleton.Bones.Count();
    const int32 nodesCount = skeleton.Nodes.Count();

    // Get nodes global transformations for the initial pose
    GraphInstance.NodesPose.Resize(nodesCount, false);
    for (int32 nodeIndex = 0; nodeIndex < nodesCount; nodeIndex++)
    {
        Matrix localTransform;
        skeleton.Nodes[nodeIndex].LocalTransform.GetWorld(localTransform);
        const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
        if (parentIndex != -1)
            GraphInstance.NodesPose[nodeIndex] = localTransform * GraphInstance.NodesPose[parentIndex];
        else
            GraphInstance.NodesPose[nodeIndex] = localTransform;
    }
    GraphInstance.Invalidate();
    GraphInstance.RootTransform = skeleton.Nodes[0].LocalTransform;

    // Setup bones transformations including bone offset matrix
    Array<Matrix> identityMatrices; // TODO: use shared memory?
    identityMatrices.Resize(bonesCount, false);
    for (int32 boneIndex = 0; boneIndex < bonesCount; boneIndex++)
    {
        auto& bone = skeleton.Bones[boneIndex];
        identityMatrices[boneIndex] = bone.OffsetMatrix * GraphInstance.NodesPose[bone.NodeIndex];
    }
    _skinningData.SetData(identityMatrices.Get(), true);

    UpdateBounds();
    UpdateSockets();
}

void AnimatedModel::GetCurrentPose(Array<Matrix>& nodesTransformation, bool worldSpace) const
{
    if (GraphInstance.NodesPose.IsEmpty())
        const_cast<AnimatedModel*>(this)->PreInitSkinningData(); // Ensure to have valid nodes pose to return
    nodesTransformation = GraphInstance.NodesPose;
    if (worldSpace)
    {
        Matrix world;
        _transform.GetWorld(world);
        for (auto& m : nodesTransformation)
            m = world * m;
    }
}

void AnimatedModel::SetCurrentPose(const Array<Matrix>& nodesTransformation, bool worldSpace)
{
    if (GraphInstance.NodesPose.IsEmpty())
        const_cast<AnimatedModel*>(this)->PreInitSkinningData(); // Ensure to have valid nodes pose to return
    CHECK(nodesTransformation.Count() == GraphInstance.NodesPose.Count());
    GraphInstance.NodesPose = nodesTransformation;
    if (worldSpace)
    {
        Matrix world;
        _transform.GetWorld(world);
        Matrix invWorld;
        Matrix::Invert(world, invWorld);
        for (auto& m : GraphInstance.NodesPose)
            m = invWorld * m;
    }
    OnAnimationUpdated();
}

void AnimatedModel::GetNodeTransformation(int32 nodeIndex, Matrix& nodeTransformation, bool worldSpace) const
{
    if (GraphInstance.NodesPose.IsEmpty())
        const_cast<AnimatedModel*>(this)->PreInitSkinningData(); // Ensure to have valid nodes pose to return
    if (nodeIndex >= 0 && nodeIndex < GraphInstance.NodesPose.Count())
        nodeTransformation = GraphInstance.NodesPose[nodeIndex];
    else
        nodeTransformation = Matrix::Identity;
    if (worldSpace)
    {
        Matrix world;
        _transform.GetWorld(world);
        nodeTransformation = nodeTransformation * world;
    }
}

void AnimatedModel::GetNodeTransformation(const StringView& nodeName, Matrix& nodeTransformation, bool worldSpace) const
{
    GetNodeTransformation(SkinnedModel ? SkinnedModel->FindNode(nodeName) : -1, nodeTransformation, worldSpace);
}

int32 AnimatedModel::FindClosestNode(const Vector3& location, bool worldSpace) const
{
    if (GraphInstance.NodesPose.IsEmpty())
        const_cast<AnimatedModel*>(this)->PreInitSkinningData(); // Ensure to have valid nodes pose to return
    const Vector3 pos = worldSpace ? _transform.WorldToLocal(location) : location;
    int32 result = -1;
    Real closest = MAX_Real;
    for (int32 nodeIndex = 0; nodeIndex < GraphInstance.NodesPose.Count(); nodeIndex++)
    {
        const Vector3 node = GraphInstance.NodesPose[nodeIndex].GetTranslation();
        const Real dst = Vector3::DistanceSquared(node, pos);
        if (dst < closest)
        {
            closest = dst;
            result = nodeIndex;
        }
    }
    return result;
}

void AnimatedModel::SetMasterPoseModel(AnimatedModel* masterPose)
{
    if (masterPose == _masterPose)
        return;
    if (_masterPose)
        _masterPose->AnimationUpdated.Unbind<AnimatedModel, &AnimatedModel::OnAnimationUpdated>(this);
    _masterPose = masterPose;
    if (_masterPose)
        _masterPose->AnimationUpdated.Bind<AnimatedModel, &AnimatedModel::OnAnimationUpdated>(this);
}

#define CHECK_ANIM_GRAPH_PARAM_ACCESS() \
    if (!AnimationGraph) \
    { \
        LOG(Warning, "Missing animation graph for animated model '{0}'", ToString()); \
        return; \
    } \
    if (AnimationGraph->WaitForLoaded()) \
    { \
        LOG(Warning, "Failed to load animation graph for animated model '{0}'", ToString()); \
        return; \
    }
#define CHECK_ANIM_GRAPH_PARAM_ACCESS_RESULT(result) \
    if (!AnimationGraph) \
    { \
        LOG(Warning, "Missing animation graph for animated model '{0}'", ToString()); \
        return result; \
    } \
    if (AnimationGraph->WaitForLoaded()) \
    { \
        LOG(Warning, "Failed to load animation graph for animated model '{0}'", ToString()); \
        return result; \
    }

AnimGraphParameter* AnimatedModel::GetParameter(const StringView& name)
{
    CHECK_ANIM_GRAPH_PARAM_ACCESS_RESULT(nullptr);
    for (auto& param : GraphInstance.Parameters)
    {
        if (param.Name == name)
            return &param;
    }
    LOG(Warning, "Failed to get animated model '{0}' missing parameter '{1}'", ToString(), name);
    return nullptr;
}

Variant AnimatedModel::GetParameterValue(const StringView& name)
{
    CHECK_ANIM_GRAPH_PARAM_ACCESS_RESULT(Variant::Null);
    for (auto& param : GraphInstance.Parameters)
    {
        if (param.Name == name)
            return param.Value;
    }
    LOG(Warning, "Failed to get animated model '{0}' missing parameter '{1}'", ToString(), name);
    return Variant::Null;
}

void AnimatedModel::SetParameterValue(const StringView& name, const Variant& value)
{
    CHECK_ANIM_GRAPH_PARAM_ACCESS();
    for (auto& param : GraphInstance.Parameters)
    {
        if (param.Name == name)
        {
            param.Value = value;
            return;
        }
    }
    LOG(Warning, "Failed to set animated model '{0}' missing parameter '{1}'", ToString(), name);
}

Variant AnimatedModel::GetParameterValue(const Guid& id)
{
    CHECK_ANIM_GRAPH_PARAM_ACCESS_RESULT(Variant::Null);
    for (auto& param : GraphInstance.Parameters)
    {
        if (param.Identifier == id)
            return param.Value;
    }
    LOG(Warning, "Failed to get animated model '{0}' missing parameter '{1}'", ToString(), id.ToString());
    return Variant::Null;
}

void AnimatedModel::SetParameterValue(const Guid& id, const Variant& value)
{
    CHECK_ANIM_GRAPH_PARAM_ACCESS();
    for (auto& param : GraphInstance.Parameters)
    {
        if (param.Identifier == id)
        {
            param.Value = value;
            return;
        }
    }
    LOG(Warning, "Failed to set animated model '{0}' missing parameter '{1}'", ToString(), id.ToString());
}

#undef CHECK_ANIM_GRAPH_PARAM_ACCESS

float AnimatedModel::GetBlendShapeWeight(const StringView& name)
{
    for (auto& e : _blendShapes.Weights)
    {
        if (e.First == name)
            return e.Second;
    }
    return 0.0f;
}

void AnimatedModel::SetBlendShapeWeight(const StringView& name, float value)
{
    value = Math::Clamp(value, -1.0f, 1.0f);
    for (int32 i = 0; i < _blendShapes.Weights.Count(); i++)
    {
        auto& e = _blendShapes.Weights[i];
        if (e.First == name)
        {
            if (Math::IsZero(value))
            {
                _blendShapes.WeightsDirty = true;
                _blendShapes.Weights.RemoveAt(i);
            }
            else if (Math::NotNearEqual(e.Second, value))
            {
                _blendShapes.WeightsDirty = true;
                e.Second = value;
            }
            return;
        }
    }
    {
        auto& e = _blendShapes.Weights.AddOne();
        e.First = name;
        e.Second = value;
        _blendShapes.WeightsDirty = true;
    }
}

void AnimatedModel::ClearBlendShapeWeights()
{
    _blendShapes.Clear();
}

void AnimatedModel::PlaySlotAnimation(const StringView& slotName, Animation* anim, float speed, float blendInTime, float blendOutTime, int32 loopCount)
{
    CHECK(anim);
    for (auto& slot : GraphInstance.Slots)
    {
        if (slot.Animation == anim && slot.Name == slotName)
        {
            slot.Pause = false;
            slot.BlendInTime = blendInTime;
            slot.LoopCount = loopCount;
            return;
        }
    }
    int32 index = 0;
    for (; index < GraphInstance.Slots.Count(); index++)
    {
        if (GraphInstance.Slots[index].Animation == nullptr)
            break;
    }
    if (index == GraphInstance.Slots.Count())
        GraphInstance.Slots.AddOne();
    auto& slot = GraphInstance.Slots[index];
    slot.Name = slotName;
    slot.Animation = anim;
    slot.Speed = speed;
    slot.BlendInTime = blendInTime;
    slot.BlendOutTime = blendOutTime;
    slot.LoopCount = loopCount;
}

void AnimatedModel::StopSlotAnimation()
{
    GraphInstance.Slots.Clear();
}

void AnimatedModel::StopSlotAnimation(const StringView& slotName, Animation* anim)
{
    for (auto& slot : GraphInstance.Slots)
    {
        if (slot.Animation == anim && slot.Name == slotName)
        {
            slot.Animation = nullptr;
            break;
        }
    }
}

void AnimatedModel::PauseSlotAnimation()
{
    for (auto& slot : GraphInstance.Slots)
        slot.Pause = true;
}

void AnimatedModel::PauseSlotAnimation(const StringView& slotName, Animation* anim)
{
    for (auto& slot : GraphInstance.Slots)
    {
        if (slot.Animation == anim && slot.Name == slotName)
        {
            slot.Pause = true;
            break;
        }
    }
}

bool AnimatedModel::IsPlayingSlotAnimation()
{
    for (auto& slot : GraphInstance.Slots)
    {
        if (slot.Animation && !slot.Pause)
            return true;
    }
    return false;
}

bool AnimatedModel::IsPlayingSlotAnimation(const StringView& slotName, Animation* anim)
{
    for (auto& slot : GraphInstance.Slots)
    {
        if (slot.Animation == anim && slot.Name == slotName && !slot.Pause)
            return true;
    }
    return false;
}

void AnimatedModel::ApplyRootMotion(const Transform& rootMotionDelta)
{
    // Skip if no motion
    if (rootMotionDelta.Translation.IsZero() && rootMotionDelta.Orientation.IsIdentity())
        return;

    // Transform translation from actor space into world space
    const Vector3 translation = Vector3::Transform(rootMotionDelta.Translation * GetScale(), GetOrientation());

    // Apply movement
    Actor* target = RootMotionTarget ? RootMotionTarget.Get() : this;
    target->AddMovement(translation, rootMotionDelta.Orientation);
}

void AnimatedModel::SyncParameters()
{
    const int32 targetCount = AnimationGraph ? AnimationGraph->Graph.Parameters.Count() : 0;
    //const int32 currentCount = GraphInstance.Parameters.Count();

    //if (targetCount != currentCount)
    {
        if (targetCount == 0)
        {
            // Clear the data
            GraphInstance.Clear();
        }
        else
        {
            ScopeLock lock(AnimationGraph->Locker);

            // Clone the parameters
            GraphInstance.Parameters.Resize(AnimationGraph->Graph.Parameters.Count(), false);
            for (int32 i = 0; i < GraphInstance.Parameters.Count(); i++)
            {
                const auto src = &AnimationGraph->Graph.Parameters.At(i);
                auto& dst = GraphInstance.Parameters[i];

                dst.Type = src->Type;
                dst.Identifier = src->Identifier;
                dst.Name = src->Name;
                dst.IsPublic = src->IsPublic;
                dst.Value = src->Value;
#if USE_EDITOR
                dst.Meta = src->Meta;
#endif
            }
        }
    }
}

void AnimatedModel::BeginPlay(SceneBeginData* data)
{
    if (SkinnedModel && SkinnedModel->IsLoaded())
        PreInitSkinningData();

    // Base
    ModelInstanceActor::BeginPlay(data);
}

void AnimatedModel::EndPlay()
{
    Animations::RemoveFromUpdate(this);
    SetMasterPoseModel(nullptr);

    // Base
    ModelInstanceActor::EndPlay();
}

void AnimatedModel::OnEnable()
{
    GetScene()->Ticking.Update.AddTick<AnimatedModel, &AnimatedModel::Update>(this);

    // Base
    ModelInstanceActor::OnEnable();
}

void AnimatedModel::OnDisable()
{
    GetScene()->Ticking.Update.RemoveTick(this);

    // Base
    ModelInstanceActor::OnDisable();
}

void AnimatedModel::OnActiveInTreeChanged()
{
    GraphInstance.Invalidate();

    // Base
    ModelInstanceActor::OnActiveInTreeChanged();
}

void AnimatedModel::UpdateBounds()
{
    if (CustomBounds.GetSize().LengthSquared() > 0.01f)
    {
        BoundingBox::Transform(CustomBounds, _transform, _box);
    }
    else if (SkinnedModel && SkinnedModel->IsLoaded())
    {
        if (GraphInstance.NodesPose.Count() != 0)
        {
            // Per-bone bounds estimated from positions
            auto& skeleton = SkinnedModel->Skeleton;
            const int32 bonesCount = skeleton.Bones.Count();
#define GET_NODE_POS(i) _transform.LocalToWorld(GraphInstance.NodesPose[skeleton.Bones[i].NodeIndex].GetTranslation())
            BoundingBox box(GET_NODE_POS(0));
            for (int32 boneIndex = 1; boneIndex < bonesCount; boneIndex++)
                box.Merge(GET_NODE_POS(boneIndex));
            _box = box;
#undef GET_NODE_POS
        }
        else
        {
            _box = SkinnedModel->GetBox(_transform.GetWorld());
        }

        // Apply margin based on model dimensions
        const Vector3 modelBoxSize = SkinnedModel->GetBox().GetSize();
        const Vector3 center = _box.GetCenter();
        const Vector3 sizeHalf = Vector3::Max(_box.GetSize() + modelBoxSize * 0.2f, modelBoxSize) * 0.5f;
        _box = BoundingBox(center - sizeHalf, center + sizeHalf);
    }
    else
    {
        _box = BoundingBox(_transform.Translation);
    }
    BoundingSphere::FromBox(_box, _sphere);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
}

void AnimatedModel::UpdateSockets()
{
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto socket = dynamic_cast<BoneSocket*>(Children[i]);
        if (socket)
            socket->UpdateTransformation();
    }
}

void AnimatedModel::OnAnimationUpdated_Async()
{
    // Update asynchronous stuff
    const auto& skeleton = SkinnedModel->Skeleton;

    // Copy pose from the master
    // TODO: support retargetting master pose to current pose
    if (_masterPose && _masterPose->SkinnedModel->Skeleton.Nodes.Count() == skeleton.Nodes.Count())
    {
        ANIM_GRAPH_PROFILE_EVENT("Copy Master Pose");
        const auto& masterInstance = _masterPose->GraphInstance;
        GraphInstance.NodesPose = masterInstance.NodesPose;
        GraphInstance.RootTransform = masterInstance.RootTransform;
        GraphInstance.RootMotion = masterInstance.RootMotion;
    }

    // Calculate the final bones transformations and update skinning
    {
        ANIM_GRAPH_PROFILE_EVENT("Final Pose");
        const int32 bonesCount = skeleton.Bones.Count();
        Matrix3x4* output = (Matrix3x4*)_skinningData.Data.Get();
        ASSERT(GraphInstance.NodesPose.Count() == skeleton.Nodes.Count());
        ASSERT(_skinningData.Data.Count() == bonesCount * sizeof(Matrix3x4));
        for (int32 boneIndex = 0; boneIndex < bonesCount; boneIndex++)
        {
            const SkeletonBone& bone = skeleton.Bones[boneIndex];
            Matrix matrix;
            Matrix::Multiply(bone.OffsetMatrix, GraphInstance.NodesPose.Get()[bone.NodeIndex], matrix);
            output[boneIndex].SetMatrixTranspose(matrix);
        }
        _skinningData.OnDataChanged(!PerBoneMotionBlur);
    }

    UpdateBounds();
    _blendShapes.Update(SkinnedModel.Get());
}

void AnimatedModel::OnAnimationUpdated_Sync()
{
    // Update synchronous stuff
    UpdateSockets();
    ApplyRootMotion(GraphInstance.RootMotion);
    AnimationUpdated();
}

void AnimatedModel::OnAnimationUpdated()
{
    ANIM_GRAPH_PROFILE_EVENT("OnAnimationUpdated");
    OnAnimationUpdated_Async();
    OnAnimationUpdated_Sync();
}

void AnimatedModel::OnSkinnedModelChanged()
{
    Entries.Release();

    if (SkinnedModel && !SkinnedModel->IsLoaded())
    {
        UpdateBounds();
        GraphInstance.Invalidate();
    }
    GraphInstance.NodesSkeleton = SkinnedModel;
}

void AnimatedModel::OnSkinnedModelLoaded()
{
    Entries.SetupIfInvalid(SkinnedModel);

    GraphInstance.Invalidate();
    if (_blendShapes.Weights.HasItems())
        _blendShapes.WeightsDirty = true;

    PreInitSkinningData();
}

void AnimatedModel::OnGraphChanged()
{
    // Cleanup parameters
    GraphInstance.Clear();
}

void AnimatedModel::OnGraphLoaded()
{
    // Prepare parameters and instance data
    GraphInstance.ClearState();
    SyncParameters();
}

bool AnimatedModel::HasContentLoaded() const
{
    return (SkinnedModel == nullptr || SkinnedModel->IsLoaded()) && Entries.HasContentLoaded();
}

void AnimatedModel::Update()
{
    // Update the mode
    _actualMode = UpdateMode;
    if (_actualMode == AnimationUpdateMode::Auto)
    {
        // TODO: handle low performance platforms

        if (_lastMinDstSqr < 3000.0f * 3000.0f)
            _actualMode = AnimationUpdateMode::EveryUpdate;
        else if (_lastMinDstSqr < 6000.0f * 6000.0f)
            _actualMode = AnimationUpdateMode::EverySecondUpdate;
        else if (_lastMinDstSqr < 10000.0f * 10000.0f)
            _actualMode = AnimationUpdateMode::EveryFourthUpdate;
        else
            _actualMode = AnimationUpdateMode::Manual;
    }

    // Check if update during this tick
    bool updateAnim = false;
    switch (_actualMode)
    {
    case AnimationUpdateMode::EveryFourthUpdate:
        updateAnim = _counter++ % 4 == 0;
        break;
    case AnimationUpdateMode::EverySecondUpdate:
        updateAnim = _counter++ % 2 == 0;
        break;
    case AnimationUpdateMode::EveryUpdate:
        updateAnim = true;
        break;
    default:
        break;
    }
    if (updateAnim && (UpdateWhenOffscreen || _lastMinDstSqr < MAX_Real))
        UpdateAnimation();

    _lastMinDstSqr = MAX_Real;
}

void AnimatedModel::Draw(RenderContext& renderContext)
{
    if (!SkinnedModel || !SkinnedModel->IsLoaded())
        return;
    if (renderContext.View.Pass == DrawPass::GlobalSDF)
        return; // TODO: Animated Model rendering to Global SDF
    if (renderContext.View.Pass == DrawPass::GlobalSurfaceAtlas)
        return; // No supported
    Matrix world;
    const Float3 translation = _transform.Translation - renderContext.View.Origin;
    Matrix::Transformation(_transform.Scale, _transform.Orientation, translation, world);
    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);

    _lastMinDstSqr = Math::Min(_lastMinDstSqr, Vector3::DistanceSquared(_transform.Translation, renderContext.View.Position + renderContext.View.Origin));
    if (_skinningData.IsReady())
    {
        // Flush skinning data with GPU
        if (_skinningData.IsDirty())
        {
            RenderContext::GPULocker.Lock();
            GPUDevice::Instance->GetMainContext()->UpdateBuffer(_skinningData.BoneMatrices, _skinningData.Data.Get(), _skinningData.Data.Count());
            RenderContext::GPULocker.Unlock();
        }

        SkinnedMesh::DrawInfo draw;
        draw.Buffer = &Entries;
        draw.Skinning = &_skinningData;
        draw.BlendShapes = &_blendShapes;
        draw.World = &world;
        draw.DrawState = &_drawState;
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        draw.DrawModes = DrawModes & renderContext.View.GetShadowsDrawPassMask(ShadowsMode);
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
        draw.Bounds = _sphere;
        draw.Bounds.Center -= renderContext.View.Origin;
        draw.PerInstanceRandom = GetPerInstanceRandom();
        draw.LODBias = LODBias;
        draw.ForcedLOD = ForcedLOD;
        draw.SortOrder = SortOrder;

        SkinnedModel->Draw(renderContext, draw);
    }

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
}

void AnimatedModel::Draw(RenderContextBatch& renderContextBatch)
{
    if (!SkinnedModel || !SkinnedModel->IsLoaded())
        return;
    const RenderContext& renderContext = renderContextBatch.GetMainContext();
    Matrix world;
    const Float3 translation = _transform.Translation - renderContext.View.Origin;
    Matrix::Transformation(_transform.Scale, _transform.Orientation, translation, world);
    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);

    _lastMinDstSqr = Math::Min(_lastMinDstSqr, Vector3::DistanceSquared(_transform.Translation, renderContext.View.Position + renderContext.View.Origin));
    if (_skinningData.IsReady())
    {
        // Flush skinning data with GPU
        if (_skinningData.IsDirty())
        {
            RenderContext::GPULocker.Lock();
            GPUDevice::Instance->GetMainContext()->UpdateBuffer(_skinningData.BoneMatrices, _skinningData.Data.Get(), _skinningData.Data.Count());
            RenderContext::GPULocker.Unlock();
        }

        SkinnedMesh::DrawInfo draw;
        draw.Buffer = &Entries;
        draw.Skinning = &_skinningData;
        draw.BlendShapes = &_blendShapes;
        draw.World = &world;
        draw.DrawState = &_drawState;
        draw.DrawModes = DrawModes;
        draw.Bounds = _sphere;
        draw.Bounds.Center -= renderContext.View.Origin;
        draw.PerInstanceRandom = GetPerInstanceRandom();
        draw.LODBias = LODBias;
        draw.ForcedLOD = ForcedLOD;
        draw.SortOrder = SortOrder;

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        if (ShadowsMode != ShadowsCastingMode::All)
        {
            // To handle old ShadowsMode option for all meshes we need to call per-context drawing (no batching opportunity)
            // TODO: maybe deserialize ShadowsMode into ModelInstanceBuffer entries options?
            for (auto& e : renderContextBatch.Contexts)
            {
                draw.DrawModes = DrawModes & e.View.GetShadowsDrawPassMask(ShadowsMode);
                SkinnedModel->Draw(e, draw);
            }
        }
        else
        {
            SkinnedModel->Draw(renderContextBatch, draw);
        }
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void AnimatedModel::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_BOX(_box, Color::Violet.RGBMultiplied(0.8f), 0, true);

    // Base
    ModelInstanceActor::OnDebugDrawSelected();
}

BoundingBox AnimatedModel::GetEditorBox() const
{
    if (SkinnedModel)
        SkinnedModel->WaitForLoaded(100);
    return BoundingBox::MakeScaled(_box, 1.0f / BoundsScale);
}

#endif

bool AnimatedModel::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    bool result = false;

    if (SkinnedModel != nullptr && SkinnedModel->IsLoaded())
    {
        SkinnedMesh* mesh;
        result |= SkinnedModel->Intersects(ray, _transform, distance, normal, &mesh);
    }

    return result;
}

void AnimatedModel::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    ModelInstanceActor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(AnimatedModel);

    SERIALIZE(SkinnedModel);
    SERIALIZE(AnimationGraph);
    SERIALIZE(PerBoneMotionBlur);
    SERIALIZE(UseTimeScale);
    SERIALIZE(UpdateWhenOffscreen);
    SERIALIZE(UpdateSpeed);
    SERIALIZE(UpdateMode);
    SERIALIZE(BoundsScale);
    SERIALIZE(CustomBounds);
    SERIALIZE(LODBias);
    SERIALIZE(ForcedLOD);
    SERIALIZE(SortOrder);
    SERIALIZE(DrawModes);
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    SERIALIZE(ShadowsMode);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    SERIALIZE(RootMotionTarget);

    stream.JKEY("Buffer");
    stream.Object(&Entries, other ? &other->Entries : nullptr);
}

void AnimatedModel::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    ModelInstanceActor::Deserialize(stream, modifier);

    DESERIALIZE(SkinnedModel);
    DESERIALIZE(AnimationGraph);
    DESERIALIZE(PerBoneMotionBlur);
    DESERIALIZE(UseTimeScale);
    DESERIALIZE(UpdateWhenOffscreen);
    DESERIALIZE(UpdateSpeed);
    DESERIALIZE(UpdateMode);
    DESERIALIZE(BoundsScale);
    DESERIALIZE(CustomBounds);
    DESERIALIZE(LODBias);
    DESERIALIZE(ForcedLOD);
    DESERIALIZE(SortOrder);
    DESERIALIZE(DrawModes);
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    DESERIALIZE(ShadowsMode);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    DESERIALIZE(RootMotionTarget);

    Entries.DeserializeIfExists(stream, "Buffer", modifier);

    // [Deprecated on 07.02.2022, expires on 07.02.2024]
    if (modifier->EngineBuild <= 6330)
        DrawModes |= DrawPass::GlobalSDF;
    // [Deprecated on 27.04.2022, expires on 27.04.2024]
    if (modifier->EngineBuild <= 6331)
        DrawModes |= DrawPass::GlobalSurfaceAtlas;
}

bool AnimatedModel::IntersectsEntry(int32 entryIndex, const Ray& ray, Real& distance, Vector3& normal)
{
    auto model = SkinnedModel.Get();
    if (!model || !model->IsInitialized() || model->GetLoadedLODs() == 0)
        return false;

    // Find mesh in the highest loaded LOD that is using the given material slot index and ray hits it
    auto& meshes = model->LODs[model->HighestResidentLODIndex()].Meshes;
    for (int32 i = 0; i < meshes.Count(); i++)
    {
        const auto& mesh = meshes[i];
        if (mesh.GetMaterialSlotIndex() == entryIndex && mesh.Intersects(ray, _transform, distance, normal))
            return true;
    }

    distance = 0;
    normal = Vector3::Up;
    return false;
}

bool AnimatedModel::IntersectsEntry(const Ray& ray, Real& distance, Vector3& normal, int32& entryIndex)
{
    auto model = SkinnedModel.Get();
    if (!model || !model->IsInitialized() || model->GetLoadedLODs() == 0)
        return false;

    // Find mesh in the highest loaded LOD that is using the given material slot index and ray hits it
    bool result = false;
    Real closest = MAX_Real;
    Vector3 closestNormal = Vector3::Up;
    int32 closestEntry = -1;
    auto& meshes = model->LODs[model->HighestResidentLODIndex()].Meshes;
    for (int32 i = 0; i < meshes.Count(); i++)
    {
        // Test intersection with mesh and check if is closer than previous
        const auto& mesh = meshes[i];
        Real dst;
        Vector3 nrm;
        if (mesh.Intersects(ray, _transform, dst, nrm) && dst < closest)
        {
            result = true;
            closest = dst;
            closestNormal = nrm;
            closestEntry = mesh.GetMaterialSlotIndex();
        }
    }

    distance = closest;
    normal = closestNormal;
    entryIndex = closestEntry;
    return result;
}

void AnimatedModel::OnDeleteObject()
{
    // Ensure this object is no longer referenced for anim update
    Animations::RemoveFromUpdate(this);

    ModelInstanceActor::OnDeleteObject();
}

void AnimatedModel::OnTransformChanged()
{
    // Base
    ModelInstanceActor::OnTransformChanged();

    UpdateBounds();
}

void AnimatedModel::WaitForModelLoad()
{
    if (SkinnedModel)
        SkinnedModel->WaitForLoaded();
}
