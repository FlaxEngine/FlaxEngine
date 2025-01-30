// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ShaderProcessing.h"

#if COMPILE_WITH_SHADER_COMPILER

#include "Engine/Core/Collections/Array.h"
#include "Engine/Utilities/TextProcessing.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Log.h"
#include "ShaderFunctionReader.CB.h"
#include "ShaderFunctionReader.VS.h"
#include "ShaderFunctionReader.HS.h"
#include "ShaderFunctionReader.DS.h"
#include "ShaderFunctionReader.GS.h"
#include "ShaderFunctionReader.PS.h"
#include "ShaderFunctionReader.CS.h"
#include "Config.h"

ShaderProcessing::Parser::Parser(const String& targetName, const char* source, int32 sourceLength, ParserMacros macros, FeatureLevel featureLevel)
    : failed(false)
    , targetName(targetName)
    , text(source, sourceLength)
    , _macros(macros)
    , _featureLevel(featureLevel)
{
}

ShaderProcessing::Parser::~Parser()
{
}

bool ShaderProcessing::Parser::Process(const String& targetName, const char* source, int32 sourceLength, ParserMacros macros, FeatureLevel featureLevel, ShaderMeta* result)
{
    PROFILE_CPU_NAMED("Shader.Parse");
    Parser parser(targetName, source, sourceLength, macros, featureLevel);
    parser.Process(result);
    return parser.Failed();
}

void ShaderProcessing::Parser::Process(ShaderMeta* result)
{
    init();

    failed = process();

    if (!failed)
        failed = collectResults(result);
}

void ShaderProcessing::Parser::init()
{
    // Init text processing tokens for hlsl language
    text.Setup_HLSL();

    // Init shader functions readers
    _childReaders.Add(New<ConstantBufferReader>());
    _childReaders.Add(New<VertexShaderFunctionReader>());
    _childReaders.Add(New<HullShaderFunctionReader>());
    _childReaders.Add(New<DomainShaderFunctionReader>());
    _childReaders.Add(New<GeometryShaderFunctionReader>());
    _childReaders.Add(New<PixelShaderFunctionReader>());
    _childReaders.Add(New<ComputeShaderFunctionReader>());
}

bool ShaderProcessing::Parser::process()
{
    const Token defineToken("#define");

    // Read whole source code
    Token token;
    while (text.CanRead())
    {
        text.ReadToken(&token);

        // Preprocessor definition
        if (token == defineToken)
        {
            // Skip
            text.ReadLine();
        }
        else
        {
            // Call children
            ProcessChildren(token, this);
        }
    }

    return false;
}

bool ShaderProcessing::Parser::collectResults(ShaderMeta* result)
{
    // Collect results from all the readers
    for (int32 i = 0; i < _childReaders.Count(); i++)
        _childReaders[i]->CollectResults(this, result);

    return false;
}

void ShaderProcessing::Parser::OnError(const String& message)
{
    // Set flag
    failed = true;

    // Send event
    LOG(Error, "Processing shader '{0}' error at line {1}. {2}", targetName, text.GetLine(), message);
}

void ShaderProcessing::Parser::OnWarning(const String& message)
{
    // Send event
    LOG(Warning, "Processing shader '{0}' warning at line {1}. {2}", targetName, text.GetLine(), message);
}

#endif
