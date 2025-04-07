// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"
#include "ShaderProcessing.h"
#include "Engine/Graphics/Shaders/Config.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Vertex Shaders reader
    /// </summary>
    class VertexShaderFunctionReader : public ShaderFunctionReader<VertexShaderMeta>
    {
        class InputLayoutReader : public ITokenReader
        {
        protected:

            VertexShaderFunctionReader* _parent;
            Token _startToken;

        public:

            InputLayoutReader(VertexShaderFunctionReader* parent)
                : _parent(parent)
                , _startToken("META_VS_IN_ELEMENT")
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
                VertexShaderMeta::InputElement element;
                Token token;
                auto& current = _parent->_current;

                // Semantic type
                text.ReadToken(&token);
                element.Type = ParseInputType(token);

                // Semantic index
                text.ReadToken(&token);
                if (StringUtils::Parse(token.Start, token.Length, &element.Index))
                {
                    parser->OnError(TEXT("Cannot parse token."));
                    return;
                }

                // Element format
                text.ReadToken(&token);
                element.Format = ParsePixelFormat(token);
                if (element.Format == PixelFormat::Unknown)
                {
                    parser->OnError(String::Format(TEXT("Unknown input data format \'{0}\' for the Vertex Shader."), String(token.ToString())));
                    return;
                }

                // Input slot
                text.ReadToken(&token);
                if (StringUtils::Parse(token.Start, token.Length, &element.InputSlot))
                {
                    parser->OnError(TEXT("Cannot parse token."));
                    return;
                }

                // Aligned byte offset
                text.ReadToken(&token);
                if (token == "ALIGN")
                {
                    element.AlignedByteOffset = INPUT_LAYOUT_ELEMENT_ALIGN;
                }
                else if (StringUtils::Parse(token.Start, token.Length, &element.AlignedByteOffset))
                {
                    parser->OnError(TEXT("Cannot parse token."));
                    return;
                }
                else if (element.AlignedByteOffset > MAX_uint8)
                {
                    parser->OnError(TEXT("Too big vertex element byte offset."));
                    return;
                }

                // Input slot class
                text.ReadToken(&token);
                if (token == "PER_VERTEX")
                {
                    element.InputSlotClass = INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA;
                }
                else if (token == "PER_INSTANCE")
                {
                    element.InputSlotClass = INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA;
                }
                else
                {
                    parser->OnError(String::Format(TEXT("Invalid input slot class type \'{0}\'."), String(token.ToString())));
                    return;
                }

                // Instance data step rate
                text.ReadToken(&token);
                if (StringUtils::Parse(token.Start, token.Length, &element.InstanceDataStepRate))
                {
                    parser->OnError(TEXT("Cannot parse token."));
                    return;
                }

                // Visible state
                text.ReadToken(&token);
                element.VisibleFlag = token.ToString();

                current.InputLayout.Add(element);
            }
        };

    DECLARE_SHADER_META_READER_HEADER("META_VS", VS);

        VertexShaderFunctionReader()
        {
            _childReaders.Add(New<InputLayoutReader>(this));
        }

        ~VertexShaderFunctionReader()
        {
        }

        void OnParseBefore(IShaderParser* parser, Reader& text) override
        {
            // Clear current meta
            _current.InputLayout.Clear();

            // Base
            ShaderFunctionReader::OnParseBefore(parser, text);
        }

        void OnParseAfter(IShaderParser* parser, Reader& text) override
        {
            // Check if errors in specified input layout
            if (_current.InputLayout.Count() > GPU_MAX_VS_ELEMENTS)
            {
                parser->OnError(String::Format(TEXT("Vertex Shader \'{0}\' has too many input layout elements specified. Maximum allowed amount is {1}."), String(_current.Name), GPU_MAX_VS_ELEMENTS));
                return;
            }

            // Base
            ShaderFunctionReader::OnParseAfter(parser, text);
        }
    };
}

#endif
