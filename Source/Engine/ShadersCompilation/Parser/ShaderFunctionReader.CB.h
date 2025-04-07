// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ShaderFunctionReader.h"
#include "Engine/Graphics/Config.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Constant Buffers reader
    /// </summary>
    class ConstantBufferReader : public ShaderMetaReader<ConstantBufferMeta>
    {
    private:

        Token _endToken;

    DECLARE_SHADER_META_READER_HEADER("META_CB_BEGIN", CB);

        ConstantBufferReader()
            : _endToken("META_CB_END")
        {
        }

        ~ConstantBufferReader()
        {
        }

        // [ShaderMetaReader]
        void OnParseBefore(IShaderParser* parser, Reader& text) override
        {
            Token token;

            // Clear current meta
            _current.Name.Clear();

            // Here we read '(x)\n' where 'x' is a shader function slot
            text.ReadToken(&token);
            if (StringUtils::Parse(token.Start, token.Length, &_current.Slot))
            {
                parser->OnError(TEXT("Invalid constant buffer slot index."));
                return;
            }

            // Read buffer name
            text.ReadToken(&token);
            _current.Name = token.ToString();

            // Check if name is unique
            for (int32 i = 0; i < _cache.Count(); i++)
            {
                if (_cache[i].Name == _current.Name)
                {
                    parser->OnError(String::Format(TEXT("Duplicated constant buffer \'{0}\'. Buffer with that name already exists."), String(_current.Name)));
                    return;
                }
            }

            // Read rest of the line
            text.ReadLine();
        }

        void OnParse(IShaderParser* parser, Reader& text) override
        {
            Token token;

            // Read function properties
            bool foundEnd = false;
            while (text.CanRead())
            {
                text.ReadToken(&token);

                // Try to find the ending
                if (token == _endToken)
                {
                    foundEnd = true;
                    break;
                }
            }

            // Check if end has not been found
            if (!foundEnd)
            {
                parser->OnError(String::Format(TEXT("Missing constant buffer \'{0}\' ending."), String(_current.Name)));
                return;
            }
        }

        void OnParseAfter(IShaderParser* parser, Reader& text) override
        {
            // Cache buffer
            _cache.Add(_current);
        }

        void CollectResults(IShaderParser* parser, ShaderMeta* result) override
        {
            // Validate constant buffer slots overlapping
            for (int32 i = 0; i < _cache.Count(); i++)
            {
                auto& first = _cache[i];
                for (int32 j = i + 1; j < _cache.Count(); j++)
                {
                    auto& second = _cache[j];
                    if (first.Slot == second.Slot)
                    {
                        parser->OnError(TEXT("Constant buffers slots are overlapping."));
                        return;
                    }
                }
            }

            // Validate amount of used constant buffers
            for (int32 i = 0; i < _cache.Count(); i++)
            {
                auto& f = _cache[i];
                if (f.Slot >= GPU_MAX_CB_BINDED)
                {
                    parser->OnError(String::Format(TEXT("Constant buffer {0} is using invalid slot {1}. Maximum supported slot is {2}."), String(f.Name), f.Slot, GPU_MAX_CB_BINDED - 1));
                    return;
                }
            }

            // Base
            ShaderMetaReader::CollectResults(parser, result);
        }
    };
}

#endif
