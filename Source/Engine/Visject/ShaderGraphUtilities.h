// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Engine/Graphics/Materials/MaterialParams.h"
#include "Engine/Utilities/TextWriter.h"
#include "Engine/Animations/Curve.h"

class VisjectMeta;

namespace ShaderGraphUtilities
{
    // Visject parameter metadata entry with the MaterialParameterType (a single byte) a GPU Texture parameter is declared as (2D, array, volume or cube). Matches SurfaceMeta.GPUTextureTypeMetaTypeID in the editor.
    constexpr int32 GPUTextureTypeMetaTypeID = 14;

    FLAXENGINE_API MaterialParameterType GetGPUTextureType(const VisjectMeta& meta);
    void GenerateShaderConstantBuffer(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters);
    const Char* GenerateShaderResources(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters, int32 startRegister);
    const Char* GenerateSamplers(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters, int32 startRegister);
    template<typename T>
    void SampleCurve(TextWriterUnicode& writer, const BezierCurve<T>& curve, const String& time, const String& value);
}

#endif
