// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Domain Shaders reader
    /// </summary>
    class DomainShaderFunctionReader : public ShaderFunctionReader<DomainShaderMeta>
    {
    DECLARE_SHADER_META_READER_HEADER("META_DS", DS);

        DomainShaderFunctionReader()
        {
            _childReaders.Add(New<StripLineReader>("domain"));
        }

        ~DomainShaderFunctionReader()
        {
        }
    };
}

#endif
