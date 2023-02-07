// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL && USE_EDITOR

#include "ModelTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/Serialization.h"

BoundingBox ImportedModelData::LOD::GetBox() const
{
    if (Meshes.IsEmpty())
        return BoundingBox::Empty;

    BoundingBox box;
    Meshes[0]->CalculateBox(box);
    for (int32 i = 1; i < Meshes.Count(); i++)
    {
        if (Meshes[i]->Positions.HasItems())
        {
            BoundingBox t;
            Meshes[i]->CalculateBox(t);
            BoundingBox::Merge(box, t, box);
        }
    }

    return box;
}

void ModelTool::Options::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(ModelTool::Options);

    SERIALIZE(Type);
    SERIALIZE(CalculateNormals);
    SERIALIZE(SmoothingNormalsAngle);
    SERIALIZE(FlipNormals);
    SERIALIZE(CalculateTangents);
    SERIALIZE(SmoothingTangentsAngle);
    SERIALIZE(OptimizeMeshes);
    SERIALIZE(MergeMeshes);
    SERIALIZE(ImportLODs);
    SERIALIZE(ImportVertexColors);
    SERIALIZE(ImportBlendShapes);
    SERIALIZE(LightmapUVsSource);
    SERIALIZE(CollisionMeshesPrefix);
    SERIALIZE(Scale);
    SERIALIZE(Rotation);
    SERIALIZE(Translation);
    SERIALIZE(CenterGeometry);
    SERIALIZE(Duration);
    SERIALIZE(FramesRange);
    SERIALIZE(DefaultFrameRate);
    SERIALIZE(SamplingRate);
    SERIALIZE(ImportScaleTrack);
    SERIALIZE(SkipEmptyCurves);
    SERIALIZE(OptimizeKeyframes);
    SERIALIZE(EnableRootMotion);
    SERIALIZE(RootNodeName);
    SERIALIZE(GenerateLODs);
    SERIALIZE(BaseLOD);
    SERIALIZE(LODCount);
    SERIALIZE(TriangleReduction);
    SERIALIZE(ImportMaterials);
    SERIALIZE(ImportTextures);
    SERIALIZE(RestoreMaterialsOnReimport);
    SERIALIZE(GenerateSDF);
    SERIALIZE(SDFResolution);
    SERIALIZE(SplitObjects);
    SERIALIZE(ObjectIndex);
}

void ModelTool::Options::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(Type);
    DESERIALIZE(CalculateNormals);
    DESERIALIZE(SmoothingNormalsAngle);
    DESERIALIZE(FlipNormals);
    DESERIALIZE(CalculateTangents);
    DESERIALIZE(SmoothingTangentsAngle);
    DESERIALIZE(OptimizeMeshes);
    DESERIALIZE(MergeMeshes);
    DESERIALIZE(ImportLODs);
    DESERIALIZE(ImportVertexColors);
    DESERIALIZE(ImportBlendShapes);
    DESERIALIZE(LightmapUVsSource);
    DESERIALIZE(CollisionMeshesPrefix);
    DESERIALIZE(Scale);
    DESERIALIZE(Rotation);
    DESERIALIZE(Translation);
    DESERIALIZE(CenterGeometry);
    DESERIALIZE(Duration);
    DESERIALIZE(FramesRange);
    DESERIALIZE(DefaultFrameRate);
    DESERIALIZE(SamplingRate);
    DESERIALIZE(ImportScaleTrack);
    DESERIALIZE(SkipEmptyCurves);
    DESERIALIZE(OptimizeKeyframes);
    DESERIALIZE(EnableRootMotion);
    DESERIALIZE(RootNodeName);
    DESERIALIZE(GenerateLODs);
    DESERIALIZE(BaseLOD);
    DESERIALIZE(LODCount);
    DESERIALIZE(TriangleReduction);
    DESERIALIZE(ImportMaterials);
    DESERIALIZE(ImportTextures);
    DESERIALIZE(RestoreMaterialsOnReimport);
    DESERIALIZE(GenerateSDF);
    DESERIALIZE(SDFResolution);
    DESERIALIZE(SplitObjects);
    DESERIALIZE(ObjectIndex);

    // [Deprecated on 23.11.2021, expires on 21.11.2023]
    int32 AnimationIndex = -1;
    DESERIALIZE(AnimationIndex);
    if (AnimationIndex != -1)
        ObjectIndex = AnimationIndex;
}

#endif
