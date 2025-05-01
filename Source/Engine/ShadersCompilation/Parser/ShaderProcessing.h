// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderMeta.h"

#if COMPILE_WITH_SHADER_COMPILER

#include "Engine/Core/Types/String.h"
#include "IShaderFunctionReader.h"
#include "ITokenReadersContainer.h"

namespace ShaderProcessing
{
    extern VertexShaderMeta::InputType ParseInputType(const Token& token);
    extern PixelFormat ParsePixelFormat(const Token& token);
    extern ShaderFlags ParseShaderFlags(const Token& token);

    /// <summary>
    /// Shader files meta data processing tool
    /// </summary>
    class Parser : public IShaderParser, public ITokenReadersContainerBase<IShaderFunctionReader>
    {
    private:

        bool failed;
        String targetName;
        Reader text;
        ParserMacros _macros;
        FeatureLevel _featureLevel;

    private:

        Parser(const String& targetName, const char* source, int32 sourceLength, ParserMacros macros, FeatureLevel featureLevel);
        ~Parser();

    public:

        /// <summary>
        /// Process shader source code and generate metadata
        /// </summary>
        /// <param name="targetName">Calling object name (used for warnings/errors logging)</param>
        /// <param name="source">ANSI source code</param>
        /// <param name="sourceLength">Amount of characters in the source code</param>
        /// <param name="macros">The input macros.</param>
        /// <param name="featureLevel">The target feature level.</param>
        /// <param name="result">Output result with metadata</param>
        /// <returns>True if cannot process the file (too many errors), otherwise false</returns>
        static bool Process(const String& targetName, const char* source, int32 sourceLength, ParserMacros macros, FeatureLevel featureLevel, ShaderMeta* result);

    public:

        /// <summary>
        /// Process shader source code and generate metadata
        /// </summary>
        /// <param name="result">Output result with metadata</param>
        void Process(ShaderMeta* result);

    private:

        void init();
        bool process();
        bool collectResults(ShaderMeta* result);

    public:

        // [IShaderParser]
        FeatureLevel GetFeatureLevel() const override
        {
            return _featureLevel;
        }

        bool Failed() const override
        {
            return failed;
        }

        Reader& GetReader() override
        {
            return text;
        }

        ParserMacros GetMacros() const override
        {
            return _macros;
        }

        void OnError(const String& message) override;
        void OnWarning(const String& message) override;
    };
}

#endif
