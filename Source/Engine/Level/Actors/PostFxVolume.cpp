// Copyright (c) Wojciech Figat. All rights reserved.

#include "PostFxVolume.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Level/Scene/SceneRendering.h"

PostFxVolume::PostFxVolume(const SpawnParams& params)
    : BoxVolume(params)
    , _priority(0)
    , _blendRadius(10.0f)
    , _blendWeight(1.0f)
    , _isBounded(true)
{
}

void PostFxVolume::Collect(RenderContext& renderContext)
{
    // Calculate blend weight
    float weight = _blendWeight;
    if (_isBounded)
    {
        Real distance;
        if (_bounds.Contains(renderContext.View.WorldPosition, &distance) == ContainmentType::Contains)
        {
            if (_blendRadius > 0.0f)
                weight = Math::Saturate((float)distance / _blendRadius) * weight;
        }
        else
        {
            weight = 0.0f;
        }
    }

    if (weight > ZeroTolerance && renderContext.View.RenderLayersMask.HasLayer(GetLayer()))
    {
        const float totalSizeSqrt = (_transform.Scale * _size).LengthSquared();
        renderContext.List->AddSettingsBlend((IPostFxSettingsProvider*)this, weight, _priority, totalSizeSqrt);
    }
}

void PostFxVolume::Blend(PostProcessSettings& other, float weight)
{
    other.AmbientOcclusion.BlendWith(AmbientOcclusion, weight);
    other.GlobalIllumination.BlendWith(GlobalIllumination, weight);
    other.Bloom.BlendWith(Bloom, weight);
    other.ToneMapping.BlendWith(ToneMapping, weight);
    other.ColorGrading.BlendWith(ColorGrading, weight);
    other.EyeAdaptation.BlendWith(EyeAdaptation, weight);
    other.CameraArtifacts.BlendWith(CameraArtifacts, weight);
    other.LensFlares.BlendWith(LensFlares, weight);
    other.DepthOfField.BlendWith(DepthOfField, weight);
    other.MotionBlur.BlendWith(MotionBlur, weight);
    other.ScreenSpaceReflections.BlendWith(ScreenSpaceReflections, weight);
    other.AntiAliasing.BlendWith(AntiAliasing, weight);
    other.PostFxMaterials.BlendWith(PostFxMaterials, weight);
}

void PostFxVolume::AddPostFxMaterial(MaterialBase* material)
{
    if (material)
        PostFxMaterials.Materials.Add(material);
}

void PostFxVolume::RemovePostFxMaterial(MaterialBase* material)
{
    if (material)
    {
        for (int32 i = 0; i < PostFxMaterials.Materials.Count(); i++)
        {
            if (PostFxMaterials.Materials[i] == material)
            {
                PostFxMaterials.Materials.RemoveAtKeepOrder(i);
                return;
            }
        }
    }
}

bool PostFxVolume::HasContentLoaded() const
{
    // Textures
    if (LensFlares.LensColor && !LensFlares.LensColor->IsLoaded())
        return false;
    if (LensFlares.LensDirt && !LensFlares.LensDirt->IsLoaded())
        return false;
    if (LensFlares.LensStar && !LensFlares.LensStar->IsLoaded())
        return false;
    if (DepthOfField.BokehShapeCustom && !DepthOfField.BokehShapeCustom->IsLoaded())
        return false;

    // PostFx materials
    for (int32 i = 0; i < PostFxMaterials.Materials.Count(); i++)
    {
        const auto material = PostFxMaterials.Materials[i].Get();
        if (material && !material->IsLoaded())
            return false;
    }

    return true;
}

void PostFxVolume::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    BoxVolume::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(PostFxVolume);

    SERIALIZE_MEMBER(Priority, _priority);
    SERIALIZE_MEMBER(BlendRadius, _blendRadius);
    SERIALIZE_MEMBER(BlendWeight, _blendWeight);
    SERIALIZE_MEMBER(IsBounded, _isBounded);

    stream.JKEY("Settings");
    stream.StartObject();
    {
        stream.JKEY("AO");
        stream.Object(&AmbientOcclusion, other ? &other->AmbientOcclusion : nullptr);

        stream.JKEY("GI");
        stream.Object(&GlobalIllumination, other ? &other->GlobalIllumination : nullptr);

        stream.JKEY("Bloom");
        stream.Object(&Bloom, other ? &other->Bloom : nullptr);

        stream.JKEY("ToneMapping");
        stream.Object(&ToneMapping, other ? &other->ToneMapping : nullptr);

        stream.JKEY("ColorGrading");
        stream.Object(&ColorGrading, other ? &other->ColorGrading : nullptr);

        stream.JKEY("EyeAdaptation");
        stream.Object(&EyeAdaptation, other ? &other->EyeAdaptation : nullptr);

        stream.JKEY("CameraArtifacts");
        stream.Object(&CameraArtifacts, other ? &other->CameraArtifacts : nullptr);

        stream.JKEY("LensFlares");
        stream.Object(&LensFlares, other ? &other->LensFlares : nullptr);

        stream.JKEY("DepthOfField");
        stream.Object(&DepthOfField, other ? &other->DepthOfField : nullptr);

        stream.JKEY("MotionBlur");
        stream.Object(&MotionBlur, other ? &other->MotionBlur : nullptr);

        stream.JKEY("SSR");
        stream.Object(&ScreenSpaceReflections, other ? &other->ScreenSpaceReflections : nullptr);

        stream.JKEY("AA");
        stream.Object(&AntiAliasing, other ? &other->AntiAliasing : nullptr);

        stream.JKEY("PostFxMaterials");
        stream.Object(&PostFxMaterials, other ? &other->PostFxMaterials : nullptr);
    }
    stream.EndObject();
}

void PostFxVolume::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    BoxVolume::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Priority, _priority);
    DESERIALIZE_MEMBER(BlendRadius, _blendRadius);
    DESERIALIZE_MEMBER(BlendWeight, _blendWeight);
    DESERIALIZE_MEMBER(IsBounded, _isBounded);

    auto settingsMember = stream.FindMember("Settings");
    if (settingsMember != stream.MemberEnd())
    {
        auto& settingsStream = settingsMember->value;
        AmbientOcclusion.DeserializeIfExists(settingsStream, "AO", modifier);
        GlobalIllumination.DeserializeIfExists(settingsStream, "GI", modifier);
        Bloom.DeserializeIfExists(settingsStream, "Bloom", modifier);
        ToneMapping.DeserializeIfExists(settingsStream, "ToneMapping", modifier);
        ColorGrading.DeserializeIfExists(settingsStream, "ColorGrading", modifier);
        EyeAdaptation.DeserializeIfExists(settingsStream, "EyeAdaptation", modifier);
        CameraArtifacts.DeserializeIfExists(settingsStream, "CameraArtifacts", modifier);
        LensFlares.DeserializeIfExists(settingsStream, "LensFlares", modifier);
        DepthOfField.DeserializeIfExists(settingsStream, "DepthOfField", modifier);
        MotionBlur.DeserializeIfExists(settingsStream, "MotionBlur", modifier);
        ScreenSpaceReflections.DeserializeIfExists(settingsStream, "SSR", modifier);
        AntiAliasing.DeserializeIfExists(settingsStream, "AA", modifier);
        PostFxMaterials.DeserializeIfExists(settingsStream, "PostFxMaterials", modifier);
    }
}

void PostFxVolume::OnEnable()
{
    GetSceneRendering()->AddPostFxProvider(this);

    // Base
    Actor::OnEnable();
}

void PostFxVolume::OnDisable()
{
    GetSceneRendering()->RemovePostFxProvider(this);

    // Base
    Actor::OnDisable();
}

#if USE_EDITOR

Color PostFxVolume::GetWiresColor()
{
    return Color::Azure;
}

#endif
