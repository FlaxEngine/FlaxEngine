//writen by https://github.com/NoriteSC

#include "Engine/Animations/InverseKinematics/IKSolver.h"

void IKSolver::ForwardAndBackwardReachingInverseKinematic
(
    Span<IKBone>& InOutBones,
    int MaxIturation,
    const Transform& Base,
    const Transform& Target,
    const Vector3& PullTargetDirection,
    bool DebugDraw
)
{
    if (InOutBones.Length() < 1)
        return;

    HandleBonesDrawAndZeroLenghtBones(InOutBones, DebugDraw);


    Vector3 refTarget = Target.Translation;
    Vector3 LastBoneTaill = Vector3::Zero;
    Vector3 FirstBoneHead = Vector3::Zero;

    auto lastbone = &InOutBones[InOutBones.Length() - 1];
    auto bonescount = InOutBones.Length();
    for (int itur = 0; itur < MaxIturation; itur++)
    {
        if (lastbone->Taill == Target.Translation && Base.Translation == InOutBones[0].Head)
            break;
        if (LastBoneTaill == lastbone->Taill)
            break;

        LastBoneTaill = lastbone->Taill;

        //backward pass
        for (int i = bonescount - 1; i >= 1; i--)
        {
            if (i < bonescount - 1)
            {
                InOutBones[i].SetOrientationUnconstrained(Quaternion::FromDirection((InOutBones[i + 1].Head - InOutBones[i].Head - PullTargetDirection).GetNormalized()));
                InOutBones[i].SnapTaillTo(InOutBones[i + 1].Head);
            }
            else
            {
                InOutBones[i].SetOrientation(Quaternion::FromDirection((Target.Translation - InOutBones[i].Head - PullTargetDirection).GetNormalized()));
                InOutBones[i].SnapTaillTo(Target.Translation);
            }

        }

        //forward pass
        for (int i = 1; i < bonescount; i++)
        {
            InOutBones[i - 1].SetOrientation(Quaternion::FromDirection((InOutBones[i].Head - InOutBones[i - 1].Head).GetNormalized()));
            InOutBones[i].SnapHeadTo(InOutBones[i - 1].Taill);
        }
        //InOutBones[0].SetOrientation(Base.Orientation);
        //applay Refrence Orientacion
        InOutBones[0].ParentOritentacion = Base.Orientation;
        InOutBones[0].SnapHeadTo(Base.Translation);

        FinalizeSolveInduration(InOutBones, Target);
    }
}

void IKSolver::BackwardReachingInverseKinematic
(
    Span<IKBone>& InOutBones,
    int MaxIturation,
    const Transform& Target,
    const Vector3& PullTargetDirection,
    bool DebugDraw
)
{
    if (InOutBones.Length() < 1)
        return;

    HandleBonesDrawAndZeroLenghtBones(InOutBones, DebugDraw);


    Vector3 refTarget = Target.Translation;
    Vector3 LastBoneTaill = Vector3::Zero;
    Vector3 FirstBoneHead = Vector3::Zero;

    auto lastbone = &InOutBones[InOutBones.Length() - 1];
    auto bonescount = InOutBones.Length();
    for (int itur = 0; itur < MaxIturation; itur++)
    {
        if (Target.Translation == InOutBones[0].Head)
            break;
        if (LastBoneTaill == lastbone->Taill)
            break;

        LastBoneTaill = lastbone->Taill;

        //backward pass
        for (int i = bonescount - 1; i >= 1; i--)
        {
            if (i < bonescount - 1)
            {
                InOutBones[i].SetOrientationUnconstrained(Quaternion::FromDirection((InOutBones[i + 1].Head - InOutBones[i].Head - PullTargetDirection).GetNormalized()));
                InOutBones[i].SnapTaillTo(InOutBones[i + 1].Head);
            }
            else
            {
                InOutBones[i].SetOrientation(Quaternion::FromDirection((Target.Translation - InOutBones[i].Head - PullTargetDirection).GetNormalized()));
                InOutBones[i].SnapTaillTo(Target.Translation);
            }

        }
        FinalizeSolveInduration(InOutBones, Target);
    }
}

void IKSolver::ForwardReachingInverseKinematic
(
    Span<IKBone>& InOutBones,
    int MaxIturation,
    const Transform& Target,
    const Vector3& PullTargetDirection,
    bool DebugDraw
)
{
    if (InOutBones.Length() < 1)
        return;

    HandleBonesDrawAndZeroLenghtBones(InOutBones, DebugDraw);


    Vector3 refTarget = Target.Translation;
    Vector3 LastBoneTaill = Vector3::Zero;
    Vector3 FirstBoneHead = Vector3::Zero;

    auto lastbone = &InOutBones[InOutBones.Length() - 1];
    auto bonescount = InOutBones.Length();
    for (int itur = 0; itur < MaxIturation; itur++)
    {
        if (lastbone->Taill == Target.Translation)
            break;
        if (LastBoneTaill == lastbone->Taill)
            break;

        LastBoneTaill = lastbone->Taill;

        //forward pass
        for (int i = 1; i < bonescount; i++)
        {
            InOutBones[i - 1].SetOrientation(Quaternion::FromDirection((InOutBones[i].Head - InOutBones[i - 1].Head).GetNormalized()));
            InOutBones[i].SnapHeadTo(InOutBones[i - 1].Taill);
        }

        FinalizeSolveInduration(InOutBones, Target);
    }
}
