// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "AnimatedModel.h"
#include "BoneSocket.h"
#include "Engine/Animations/AnimationManager.h"
#include "Engine/Engine/Engine.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Serialization/Serialization.h"

AnimatedModel::AnimatedModel(const SpawnParams& params)
    : ModelInstanceActor(params)
    , _actualMode(AnimationUpdateMode::Never)
    , _counter(0)
    , _lastMinDstSqr(MAX_float)
    , _lastUpdateFrame(0)
    , GraphInstance(this)
{
    _world = Matrix::Identity;
    UpdateBounds();

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
        || _lastUpdateFrame == Engine::FrameCount)
        return;
    _lastUpdateFrame = Engine::FrameCount;

    if (AnimationGraph && AnimationGraph->IsLoaded() && AnimationGraph->Graph.IsReady())
    {
        // Request an animation update
        AnimationManager::AddToUpdate(this);
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
    ASSERT(SkinnedModel && SkinnedModel->IsLoaded());

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

void AnimatedModel::UpdateSockets()
{
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto socket = dynamic_cast<BoneSocket*>(Children[i]);
        if (socket)
            socket->UpdateTransformation();
    }
}

void AnimatedModel::GetCurrentPose(Array<Matrix>& nodesTransformation, bool worldSpace) const
{
    nodesTransformation = GraphInstance.NodesPose;
    if (worldSpace)
    {
        const auto world = _world;
        for (auto& m : nodesTransformation)
            m = m * world;
    }
}

void AnimatedModel::GetNodeTransformation(int32 nodeIndex, Matrix& nodeTransformation, bool worldSpace) const
{
    if (nodeIndex >= 0 && nodeIndex < GraphInstance.NodesPose.Count())
        nodeTransformation = GraphInstance.NodesPose[nodeIndex];
    else
        nodeTransformation = Matrix::Identity;
    if (worldSpace)
        nodeTransformation = nodeTransformation * _world;
}

void AnimatedModel::GetNodeTransformation(const StringView& nodeName, Matrix& nodeTransformation, bool worldSpace) const
{
    GetNodeTransformation(SkinnedModel ? SkinnedModel->FindNode(nodeName) : -1, nodeTransformation, worldSpace);
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

void AnimatedModel::ApplyRootMotion(const RootMotionData& rootMotionDelta)
{
    // Skip if no motion
    if (rootMotionDelta.Translation.IsZero() && rootMotionDelta.Rotation.IsIdentity())
        return;

    // Transform translation from actor space into world space
    const Vector3 translation = Vector3::Transform(rootMotionDelta.Translation * GetScale(), GetOrientation());

    // Apply movement
    Actor* target = RootMotionTarget ? RootMotionTarget.Get() : this;
    target->AddMovement(translation, rootMotionDelta.Rotation);
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

void AnimatedModel::UpdateBounds()
{
    UpdateLocalBounds();

    BoundingBox::Transform(_boxLocal, _world, _box);
    BoundingSphere::FromBox(_box, _sphere);
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
    AnimationManager::RemoveFromUpdate(this);

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

void AnimatedModel::UpdateLocalBounds()
{
    // Get bounding box in a local space
    BoundingBox box;
    if (CustomBounds.GetSize().LengthSquared() > 0.01f)
    {
        box = CustomBounds;
    }
    else if (SkinnedModel && SkinnedModel->IsLoaded())
    {
        //box = SkinnedModel->GetBox(GraphInstance.RootTransform.GetWorld());
        box = SkinnedModel->GetBox();
    }
    else
    {
        box = BoundingBox(Vector3::Zero, Vector3::Zero);
    }

    // Scale bounds
    box.Minimum *= BoundsScale;
    box.Maximum *= BoundsScale;

    _boxLocal = box;
}

void AnimatedModel::OnAnimUpdate()
{
    UpdateBounds();
    UpdateSockets();
    ApplyRootMotion(GraphInstance.RootMotion);
    _blendShapes.Update(SkinnedModel.Get());
}

void AnimatedModel::OnSkinnedModelChanged()
{
    Entries.Release();

    if (SkinnedModel && !SkinnedModel->IsLoaded())
    {
        UpdateBounds();
        GraphInstance.Invalidate();
    }
}

void AnimatedModel::OnSkinnedModelLoaded()
{
    Entries.SetupIfInvalid(SkinnedModel);

    UpdateBounds();
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
    if (updateAnim && (UpdateWhenOffscreen || _lastMinDstSqr < MAX_float))
        UpdateAnimation();

    _lastMinDstSqr = MAX_float;
}

void AnimatedModel::Draw(RenderContext& renderContext)
{
    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, _world);

    const DrawPass drawModes = (DrawPass)(DrawModes & renderContext.View.Pass & (int32)renderContext.View.GetShadowsDrawPassMask(ShadowsMode));
    if (SkinnedModel && SkinnedModel->IsLoaded() && drawModes != DrawPass::None)
    {
        _lastMinDstSqr = Math::Min(_lastMinDstSqr, Vector3::DistanceSquared(GetPosition(), renderContext.View.Position));

        if (_skinningData.IsReady())
        {
#if USE_EDITOR
            // Disable motion blur effects in editor without play mode enabled to hide minor artifacts on objects moving
            if (!Editor::IsPlayMode)
                _drawState.PrevWorld = _world;
#endif

            _skinningData.Flush(GPUDevice::Instance->GetMainContext());

            SkinnedMesh::DrawInfo draw;
            draw.Buffer = &Entries;
            draw.Skinning = &_skinningData;
            draw.BlendShapes = &_blendShapes;
            draw.World = &_world;
            draw.DrawState = &_drawState;
            draw.DrawModes = drawModes;
            draw.Bounds = _sphere;
            draw.PerInstanceRandom = GetPerInstanceRandom();
            draw.LODBias = LODBias;
            draw.ForcedLOD = ForcedLOD;

            SkinnedModel->Draw(renderContext, draw);
        }
    }

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, _world);
}

void AnimatedModel::DrawGeneric(RenderContext& renderContext)
{
    Draw(renderContext);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void AnimatedModel::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_BOX(_box, Color::Violet * 0.8f, 0, true);

    // Base
    ModelInstanceActor::OnDebugDrawSelected();
}

#endif

bool AnimatedModel::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    bool result = false;

    if (SkinnedModel != nullptr && SkinnedModel->IsLoaded())
    {
        SkinnedMesh* mesh;
        result |= SkinnedModel->Intersects(ray, _world, distance, normal, &mesh);
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
    SERIALIZE(UpdateMode);
    SERIALIZE(BoundsScale);
    SERIALIZE(CustomBounds);
    SERIALIZE(LODBias);
    SERIALIZE(ForcedLOD);
    SERIALIZE(DrawModes);
    SERIALIZE(ShadowsMode);
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
    DESERIALIZE(UpdateMode);
    DESERIALIZE(BoundsScale);
    DESERIALIZE(CustomBounds);
    DESERIALIZE(LODBias);
    DESERIALIZE(ForcedLOD);
    DESERIALIZE(DrawModes);
    DESERIALIZE(ShadowsMode);
    DESERIALIZE(RootMotionTarget);

    Entries.DeserializeIfExists(stream, "Buffer", modifier);
}

bool AnimatedModel::IntersectsEntry(int32 entryIndex, const Ray& ray, float& distance, Vector3& normal)
{
    auto model = SkinnedModel.Get();
    if (!model || !model->IsInitialized() || model->GetLoadedLODs() == 0)
        return false;

    // Find mesh in the highest loaded LOD that is using the given material slot index and ray hits it
    auto& meshes = model->LODs[model->HighestResidentLODIndex()].Meshes;
    for (int32 i = 0; i < meshes.Count(); i++)
    {
        const auto& mesh = meshes[i];
        if (mesh.GetMaterialSlotIndex() == entryIndex && mesh.Intersects(ray, _world, distance, normal))
            return true;
    }

    distance = 0;
    normal = Vector3::Up;
    return false;
}

bool AnimatedModel::IntersectsEntry(const Ray& ray, float& distance, Vector3& normal, int32& entryIndex)
{
    auto model = SkinnedModel.Get();
    if (!model || !model->IsInitialized() || model->GetLoadedLODs() == 0)
        return false;

    // Find mesh in the highest loaded LOD that is using the given material slot index and ray hits it
    bool result = false;
    float closest = MAX_float;
    Vector3 closestNormal = Vector3::Up;
    int32 closestEntry = -1;
    auto& meshes = model->LODs[model->HighestResidentLODIndex()].Meshes;
    for (int32 i = 0; i < meshes.Count(); i++)
    {
        // Test intersection with mesh and check if is closer than previous
        const auto& mesh = meshes[i];
        float dst;
        Vector3 nrm;
        if (mesh.Intersects(ray, _world, dst, nrm) && dst < closest)
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

void AnimatedModel::OnTransformChanged()
{
    // Base
    ModelInstanceActor::OnTransformChanged();

    _transform.GetWorld(_world);
    BoundingBox::Transform(_boxLocal, _world, _box);
    BoundingSphere::FromBox(_box, _sphere);
}
