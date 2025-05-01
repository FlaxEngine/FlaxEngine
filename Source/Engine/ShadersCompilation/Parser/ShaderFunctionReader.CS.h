// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Compute Shaders reader
    /// </summary>
    class ComputeShaderFunctionReader : public ShaderFunctionReader<ComputeShaderMeta>
    {
    DECLARE_SHADER_META_READER_HEADER("META_CS", CS);

        ComputeShaderFunctionReader()
        {
            _childReaders.Add(New<StripLineReader>("numthreads"));
        }

        ~ComputeShaderFunctionReader()
        {
        }
    };
}

#endif
