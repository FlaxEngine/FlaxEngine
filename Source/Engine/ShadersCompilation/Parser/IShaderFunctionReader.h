// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ITokenReader.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Interface for shader functions readers like Pixel Shader readers or Constant Buffer readers
    /// </summary>
    class IShaderFunctionReader : public ITokenReader
    {
    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~IShaderFunctionReader()
        {
        }

    public:

        /// <summary>
        /// Collects shader function reader results to the final Shader Meta
        /// </summary>
        /// <param name="parser">Parser object</param>
        /// <param name="result">Parsing result</param>
        virtual void CollectResults(IShaderParser* parser, ShaderMeta* result) = 0;
    };
}

#endif
