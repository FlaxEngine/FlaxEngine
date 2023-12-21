//writen by https://github.com/NoriteSC

#include "Engine/Animations/InverseKinematics/IKSolver.h"

void IKSolver::ForwardAndBackwardReachingInverseKinematic
(
    Array<IKBone>& InOutBones,
    int MaxIturation,
    const Transform& Base,
    const Transform& Target,
    const Vector3& PullTargetDirection,
    bool DebugDraw
)
{
    if (InOutBones.Count() < 1)
        return;

    HandleBonesDrawAndZeroLenghtBones(InOutBones, DebugDraw);


    Vector3 refTarget = Target.Translation;
    Vector3 LastBoneTaill = Vector3::Zero;
    Vector3 FirstBoneHead = Vector3::Zero;
    for (int itur = 0; itur < MaxIturation; itur++)
    {
        if (InOutBones.Last().Taill == Target.Translation && Base.Translation == InOutBones[0].Head)
            break;
        if (LastBoneTaill == InOutBones.Last().Taill)
            break;

        LastBoneTaill = InOutBones.Last().Taill;

        //backward pass
        for (int i = InOutBones.Count() - 1; i >= 1; i--)
        {
            if (i < InOutBones.Count() - 1)
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
        for (int i = 1; i < InOutBones.Count(); i++)
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
    Array<IKBone>& InOutBones,
    int MaxIturation,
    const Transform& Target,
    const Vector3& PullTargetDirection,
    bool DebugDraw
)
{
    if (InOutBones.Count() < 1)
        return;

    HandleBonesDrawAndZeroLenghtBones(InOutBones, DebugDraw);

    Vector3 refTarget = Target.Translation;
    for (int itur = 0; itur < MaxIturation; itur++)
    {
        if (Target.Translation == InOutBones[0].Head)
            break;

        //backward pass
        for (int i = InOutBones.Count() - 1; i >= 1; i--)
        {
            if (i < InOutBones.Count() - 1)
            {
                InOutBones[i].SetOrientation(Quaternion::FromDirection((InOutBones[i + 1].Head - InOutBones[i].Head - PullTargetDirection).GetNormalized()));
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
    Array<IKBone>& InOutBones,
    int MaxIturation,
    const Transform& Target,
    const Vector3& PullTargetDirection,
    bool DebugDraw
)
{
    if (InOutBones.Count() < 1)
        return;

    HandleBonesDrawAndZeroLenghtBones(InOutBones, DebugDraw);

    Vector3 refTarget = Target.Translation;
    Vector3 LastBoneTaill = Vector3::Zero;
    for (int itur = 0; itur < MaxIturation; itur++)
    {
        if (InOutBones.Last().Taill == Target.Translation)
            break;
        if (LastBoneTaill == InOutBones.Last().Taill)
            break;

        LastBoneTaill = InOutBones.Last().Taill;

        //forward pass
        for (int i = 1; i < InOutBones.Count(); i++)
        {
            InOutBones[i - 1].SetOrientation(Quaternion::FromDirection((InOutBones[i].Head - InOutBones[i - 1].Head - PullTargetDirection).GetNormalized()));
            InOutBones[i].SnapHeadTo(InOutBones[i - 1].Taill);
        }

        FinalizeSolveInduration(InOutBones, Target);
    }
}
