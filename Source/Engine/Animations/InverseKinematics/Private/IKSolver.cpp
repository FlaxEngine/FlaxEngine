//writen by https://github.com/NoriteSC

#include "Engine/Animations/InverseKinematics/IKSolver.h"

Array<IKBone> IKSolver::ConstructChainFrom(const Span<Actor>& Actors)
{
    Array<IKBone> out;
    //cheak if there is enough actors to create bone's
    if (Actors.Length() < 2)
        return out;

    //resize the Array to Actors Length - 1 element
    out.Resize(Actors.Length() - 2);
    // as a exmaple 4 actors makes 3 bones
    //  0
    //o---o
    //      \ 1
    //        o----o
    //          2

    for (auto i = 0; i < out.Count(); i++)
    {
        out[i].Head = Actors[i].GetTransform().Translation;
        out[i].Taill = Actors[i + 1].GetTransform().Translation;
    }
    return out;
}
Array<IKBone> IKSolver::ConstructChainFrom(const Array<Transform>& Transforms)
{
    Array<IKBone> out;
    //cheak if there is enough Transforms to create bone's
    if (Transforms.Count() < 2)
        return out;

    //resize the Array to Transforms count - 1 element
    out.Resize(Transforms.Count() - 2);
    // as a exmaple 4 Transforms makes 3 bones
    //  0
    //o---o
    //      \ 1
    //        o----o
    //          2

    for (auto i = 0; i < out.Count(); i++)
    {
        out[i].Head = Transforms[i].Translation;
        out[i].Taill = Transforms[i + 1].Translation;
    }
    return out;
}
Array<IKBone> IKSolver::ConstructChainFrom(const Array<Vector3>& Locations)
{
    Array<IKBone> out;
    //cheak if there is enough Locations to create bone's
    if (Locations.Count() < 2)
        return out;

    //resize the Array to Locations count - 1 element
    out.Resize(Locations.Count() - 2);
    // as a exmaple 4 Locations makes 3 bones
    //  0
    //o---o
    //      \ 1
    //        o----o
    //          2

    for (auto i = 0; i < out.Count(); i++)
    {
        out[i].Head = Locations[i];
        out[i].Taill = Locations[i + 1];
    }
    return out;
}
void IKSolver::HandleBonesDrawAndZeroLenghtBones(Span<IKBone>& InOutBones, bool DebugDraw)
{
    if (DebugDraw)
    {
        for (int i = 0; i < InOutBones.Length(); i++)
        {
            //handle debugdraw
            InOutBones[i].Draw();
            //just in case cheak it Length of the bones is acualy set if is 0 set the Lenght of the bone
            //this is needed or else the bones will start to resize on it own
            if (InOutBones[i].Lenght == 0)
            {
                InOutBones[i].Lenght = InOutBones[i].GetDirection().Length();
            }
        }
    }
    else
    {
        //see comment above
        for (int i = 0; i < InOutBones.Length(); i++)
        {
            if (InOutBones[i].Lenght == 0)
            {
                InOutBones[i].Lenght = InOutBones[i].GetDirection().Length();
            }
        }
    }
}
void IKSolver::FinalizeSolveInduration(Span<IKBone>& InOutBones, const Transform& Target)
{
    for (int i = 1; i < InOutBones.Length(); i++)
    {
        //project 
        auto v = Vector3::ProjectOnPlane(Target.Translation, InOutBones[i - 1].GetDirection().GetNormalized());
        auto v1 = Vector3::ProjectOnPlane(InOutBones[i - 1].Head, InOutBones[i - 1].GetDirection().GetNormalized());
        //calculate roll
        v = (v - v1).GetNormalized();
        float angle = Vector3::Angle(v, Vector3::Right);
        if (v.Z > 0)
            angle = -angle;
        InOutBones[i - 1].Roll = angle;


        //set parent Oritentacions
        InOutBones[i].ParentOritentacion = InOutBones[i - 1].Oritentacion;
    }
}
