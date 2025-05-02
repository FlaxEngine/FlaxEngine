// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Engine/Graphics/Materials/MaterialParams.h"
#include "Engine/Utilities/TextWriter.h"
#include "Engine/Animations/Curve.h"

namespace ShaderGraphUtilities
{
    void GenerateShaderConstantBuffer(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters);
    const Char* GenerateShaderResources(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters, int32 startRegister);
    const Char* GenerateSamplers(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters, int32 startRegister);
    template<typename T>
    void SampleCurve(TextWriterUnicode& writer, const BezierCurve<T>& curve, const String& time, const String& value);
}

#endif
