// Copyright (c) Wojciech Figat. All rights reserved.

#include "Ragdoll.h"
#include "AnimatedModel.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Physics/Actors/RigidBody.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Engine/Physics/Joints/Joint.h"
#endif

Ragdoll::Ragdoll(const SpawnParams& params)
    : Actor(params)
{
}

float Ragdoll::GetTotalMass() const
{
    float result = 0.0f;
    for (auto child : Children)
    {
        const auto rigidBody = Cast<RigidBody>(child);
        if (!rigidBody || !rigidBody->IsActiveInHierarchy())
            continue;
        result += rigidBody->GetMass();
    }
    return result;
}

void Ragdoll::SetLinearVelocity(const Vector3& value) const
{
    for (auto child : Children)
    {
        const auto rigidBody = Cast<RigidBody>(child);
        if (!rigidBody || !rigidBody->IsActiveInHierarchy())
            continue;
        rigidBody->SetLinearVelocity(value);
    }
}

void Ragdoll::SetAngularVelocity(const Vector3& value) const
{
    for (auto child : Children)
    {
        const auto rigidBody = Cast<RigidBody>(child);
        if (!rigidBody || !rigidBody->IsActiveInHierarchy())
            continue;
        rigidBody->SetAngularVelocity(value);
    }
}

float Ragdoll::InitBone(RigidBody* rigidBody, int32& nodeIndex, Transform& localOffset)
{
    // Bones with 0 weight are non-simulated (kinematic)
    float weight = BonesWeight;
    BonesWeights.TryGet(rigidBody->GetName(), weight);
    rigidBody->SetIsKinematic(weight < ANIM_GRAPH_BLEND_THRESHOLD);
    nodeIndex = _animatedModel->SkinnedModel->FindNode(rigidBody->GetName());
    if (nodeIndex != -1 && !_bonesOffsets.TryGet(rigidBody, localOffset))
    {
        // Calculate the skeleton node local position of the bone
        auto& node = _animatedModel->GraphInstance.NodesPose[nodeIndex];
        Transform nodeT;
        node.Decompose(nodeT);
        localOffset = nodeT.WorldToLocal(rigidBody->GetLocalTransform());
        _bonesOffsets[rigidBody] = localOffset;

        // Initialize body
        rigidBody->SetSolverIterationCounts(PositionSolverIterations, VelocitySolverIterations);
        rigidBody->SetMaxDepenetrationVelocity(MaxDepenetrationVelocity);

#if USE_EDITOR
        for (auto child : rigidBody->Children)
        {
            auto joint = Cast<Joint>(child);
            if (joint && joint->Target == nullptr && joint->IsActiveInHierarchy())
            {
                LOG(Warning, "Ragdol joint '{0}' has missing target", joint->GetNamePath());
            }
        }
#endif
    }
    return weight;
}

void Ragdoll::OnFixedUpdate()
{
    if (!_animatedModel || !_animatedModel->SkinnedModel)
        return;
    PROFILE_CPU();

    // Synchronize non-simulated bones
    for (auto child : Children)
    {
        auto rigidBody = Cast<RigidBody>(child);
        if (!rigidBody || !rigidBody->IsActiveInHierarchy())
            continue;
        Transform localOffset;
        int32 nodeIndex;
        const float weight = InitBone(rigidBody, nodeIndex, localOffset);
        if (nodeIndex != -1 && weight < ANIM_GRAPH_BLEND_THRESHOLD)
        {
            // Bone is animation driven
            auto& node = _animatedModel->GraphInstance.NodesPose[nodeIndex];
            Transform nodeT;
            node.Decompose(nodeT);
            rigidBody->SetLocalTransform(nodeT.LocalToWorld(localOffset));
        }
    }

    // Synchronize simulated bones with skeleton if Anim Graph is disabled
    if (!_animatedModel->AnimationGraph || _animatedModel->UpdateMode == AnimatedModel::AnimationUpdateMode::Never)
    {
        // Get current pose
        Array<Matrix> currentPose;
        _animatedModel->GetCurrentPose(currentPose);

        // Convert pose into local-bone pose
        auto& skeleton = _animatedModel->SkinnedModel->Skeleton;
        AnimGraphImpulse localPose;
        localPose.Nodes.Resize(skeleton.Nodes.Count());
        for (int32 nodeIndex = 0; nodeIndex < skeleton.Nodes.Count(); nodeIndex++)
        {
            Transform t;
            currentPose[nodeIndex].Decompose(t);
            const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
            if (parentIndex != -1)
            {
                Transform parent;
                currentPose[parentIndex].Decompose(parent);
                t = parent.WorldToLocal(t);
            }
            localPose.Nodes[nodeIndex] = t;
        }

        // Override simulated bones in local pose
        OnAnimationUpdating(&localPose);

        // Convert into skeleton pose
        for (int32 nodeIndex = 0; nodeIndex < skeleton.Nodes.Count(); nodeIndex++)
        {
            const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
            if (parentIndex != -1)
                localPose.Nodes[parentIndex].LocalToWorld(localPose.Nodes[nodeIndex], localPose.Nodes[nodeIndex]);
            localPose.Nodes[nodeIndex].GetWorld(currentPose[nodeIndex]);
        }

        // Set current pose
        _animatedModel->SetCurrentPose(currentPose);
    }
}

void Ragdoll::OnAnimationUpdating(AnimGraphImpulse* localPose)
{
    if (!_animatedModel || !_animatedModel->SkinnedModel)
        return;
    PROFILE_CPU();

    // Synchronize simulated bones
    auto& skeleton = _animatedModel->SkinnedModel->Skeleton;
    for (auto child : Children)
    {
        const auto rigidBody = Cast<RigidBody>(child);
        if (!rigidBody || !rigidBody->IsActiveInHierarchy())
            continue;
        Transform localOffset;
        int32 nodeIndex;
        const float weight = InitBone(rigidBody, nodeIndex, localOffset);
        if (nodeIndex != -1 && weight > ANIM_GRAPH_BLEND_THRESHOLD)
        {
            // Calculate node transformation based on the rigidbody transform and inverted local offset
            Transform nodeT, rigidbodyT = rigidBody->GetLocalTransform();
            nodeT.Scale = rigidbodyT.Scale / localOffset.Scale;
            const Quaternion localOffsetOrientInv = localOffset.Orientation.Conjugated();
            Quaternion::Multiply(rigidbodyT.Orientation, localOffsetOrientInv, nodeT.Orientation);
            nodeT.Orientation.Normalize();
            nodeT.Translation = rigidbodyT.Translation - (nodeT.Orientation * (localOffset.Translation * nodeT.Scale));

            if (weight < 1.0f - ANIM_GRAPH_BLEND_THRESHOLD)
            {
                // Blend between simulated and animated state
                Transform::Lerp(localPose->GetNodeModelTransformation(skeleton, nodeIndex), nodeT, weight, nodeT);
            }

            // Bone is physics driven
            localPose->SetNodeModelTransformation(skeleton, nodeIndex, nodeT);
        }
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Physics/Joints/Joint.h"
#include "Engine/Physics/Colliders/Collider.h"

void Ragdoll::OnDebugDrawSelected()
{
    // Draw whole skeleton
    for (auto child : Children)
    {
        auto rigidBody = Cast<RigidBody>(child);
        if (!rigidBody || !rigidBody->IsActiveInHierarchy())
            continue;
        for (auto grandChild : rigidBody->Children)
        {
            if (grandChild->Is<Collider>() || grandChild->Is<Joint>())
                grandChild->OnDebugDrawSelected();
        }
    }

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Ragdoll::OnEnable()
{
    GetScene()->Ticking.FixedUpdate.AddTick<Ragdoll, &Ragdoll::OnFixedUpdate>(this);

    // Initialize bones
    if (_animatedModel && _animatedModel->SkinnedModel && _animatedModel->SkinnedModel->IsLoaded())
    {
        if (_animatedModel->GraphInstance.NodesPose.IsEmpty())
            _animatedModel->PreInitSkinningData();
        for (auto child : Children)
        {
            const auto rigidBody = Cast<RigidBody>(child);
            if (rigidBody && rigidBody->IsActiveInHierarchy())
            {
                Transform localOffset;
                int32 nodeIndex;
                InitBone(rigidBody, nodeIndex, localOffset);
            }
        }
    }

    Actor::OnEnable();
}

void Ragdoll::OnDisable()
{
    Actor::OnDisable();

    _bonesOffsets.Clear();
    GetScene()->Ticking.FixedUpdate.RemoveTick(this);
}

void Ragdoll::OnParentChanged()
{
    Actor::OnParentChanged();

    // Update for new parent
    if (_animatedModel)
    {
        _animatedModel->GraphInstance.LocalPoseOverride.Unbind<Ragdoll, &Ragdoll::OnAnimationUpdating>(this);
    }
    _animatedModel = Cast<AnimatedModel>(_parent);
    if (_animatedModel)
    {
        _animatedModel->GraphInstance.LocalPoseOverride.Bind<Ragdoll, &Ragdoll::OnAnimationUpdating>(this);
    }
}

void Ragdoll::OnTransformChanged()
{
    // Force to be linked into parent
    _localTransform = Transform::Identity;

    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
