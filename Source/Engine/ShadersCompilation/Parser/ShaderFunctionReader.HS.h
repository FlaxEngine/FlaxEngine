// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"
#include "Engine/Core/Math/Math.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Hull Shaders reader
    /// </summary>
    class HullShaderFunctionReader : public ShaderFunctionReader<HullShaderMeta>
    {
        class PatchSizeReader : public ITokenReader
        {
        protected:

            HullShaderFunctionReader* _parent;
            Token _startToken;

        public:

            PatchSizeReader(HullShaderFunctionReader* parent)
                : _parent(parent)
                , _startToken("META_HS_PATCH")
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
                Token token;
                auto& current = _parent->_current;

                // Input control points count
                text.ReadToken(&token);
                token = parser->GetMacros().GetValue(token);
                int32 controlPointsCount;
                if (StringUtils::Parse(token.Start, token.Length, &controlPointsCount))
                {
                    parser->OnError(TEXT("Cannot parse Hull shader input control points count."));
                    return;
                }
                if (Math::IsNotInRange(controlPointsCount, 1, 32))
                {
                    parser->OnError(TEXT("Invalid amount of control points. Valid range is [1-32]."));
                    return;
                }
                current.ControlPointsCount = controlPointsCount;
            }
        };

    DECLARE_SHADER_META_READER_HEADER("META_HS", HS);

        HullShaderFunctionReader()
        {
            _childReaders.Add(New<PatchSizeReader>(this));
            _childReaders.Add(New<StripLineReader>("domain"));
            _childReaders.Add(New<StripLineReader>("partitioning"));
            _childReaders.Add(New<StripLineReader>("outputtopology"));
            _childReaders.Add(New<StripLineReader>("maxtessfactor"));
            _childReaders.Add(New<StripLineReader>("outputcontrolpoints"));
            _childReaders.Add(New<StripLineReader>("patchconstantfunc"));
        }

        ~HullShaderFunctionReader()
        {
        }

        void OnParseBefore(IShaderParser* parser, Reader& text) override
        {
            // Clear current meta
            _current.ControlPointsCount = 0;

            // Base
            ShaderFunctionReader::OnParseBefore(parser, text);
        }

        void OnParseAfter(IShaderParser* parser, Reader& text) override
        {
            // Check if errors in parsed data
            if (_current.ControlPointsCount == 0)
            {
                parser->OnError(String::Format(TEXT("Hull Shader \'{0}\' has missing META_HS_PATCH macro that defines the amount of the input control points from the Input Assembler."), String(_current.Name)));
                return;
            }

            // Base
            ShaderFunctionReader::OnParseAfter(parser, text);
        }
    };
}

#endif
