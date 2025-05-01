// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "IShaderParser.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Interface for objects that can read shader source code tokens
    /// </summary>
    class ITokenReader
    {
    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~ITokenReader()
        {
        }

    public:

        /// <summary>
        /// Checks if given token can be processed by this reader
        /// </summary>
        /// <param name="token">Starting token to check</param>
        /// <returns>True if given token is valid starting token, otherwise false</returns>
        virtual bool CheckStartToken(const Token& token) = 0;

        /// <summary>
        /// Start processing source after reading start token
        /// </summary>
        /// <param name="parser">Parser object</param>
        /// <param name="text">Source code reader</param>
        virtual void Process(IShaderParser* parser, Reader& text) = 0;
    };
}

#endif
