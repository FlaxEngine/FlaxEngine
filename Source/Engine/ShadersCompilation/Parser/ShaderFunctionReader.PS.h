// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Pixel Shaders reader
    /// </summary>
    class PixelShaderFunctionReader : public ShaderFunctionReader<PixelShaderMeta>
    {
    DECLARE_SHADER_META_READER_HEADER("META_PS", PS);

        PixelShaderFunctionReader()
        {
        }

        ~PixelShaderFunctionReader()
        {
        }
    };
}

#endif
