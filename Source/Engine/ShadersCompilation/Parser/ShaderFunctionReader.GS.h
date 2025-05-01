// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Geometry Shaders reader
    /// </summary>
    class GeometryShaderFunctionReader : public ShaderFunctionReader<GeometryShaderMeta>
    {
        class MaxVertexCountReader : public ITokenReader
        {
        private:

            Token _startToken;

        public:

            MaxVertexCountReader()
                : _startToken("maxvertexcount")
            {
            }

        public:

            // [ITokenReader]
            bool CheckStartToken(const Token& token) override
            {
                return token == _startToken;
            }

            void Process(IShaderParser* parser, Reader& text) override
            {
                // Read line to end
                text.ReadLine();
            }
        };

    DECLARE_SHADER_META_READER_HEADER("META_GS", GS);

        GeometryShaderFunctionReader()
        {
            _childReaders.Add(New<MaxVertexCountReader>());
        }

        ~GeometryShaderFunctionReader()
        {
        }
    };
}

#endif
