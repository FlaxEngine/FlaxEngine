// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IShaderFunctionReader.h"
#include "ShaderMeta.h"
#include "Config.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Implementation of shader meta functions reader
    /// </summary>
    template<typename MetaType>
    class ShaderMetaReader : public IShaderFunctionReader, public ITokenReadersContainer
    {
    protected:

        Array<MetaType> _cache;
        MetaType _current;

        ShaderMetaReader()
        {
        }

        ~ShaderMetaReader()
        {
        }

    protected:

        /// <summary>
        /// Event called before parsing shader function
        /// </summary>
        /// <param name="parser">Parser object</param>
        /// <param name="text">Source code reader</param>
        virtual void OnParseBefore(IShaderParser* parser, Reader& text) = 0;

        /// <summary>
        /// Event called for parsing shader function
        /// </summary>
        /// <param name="parser">Parser object</param>
        /// <param name="text">Source code reader</param>
        virtual void OnParse(IShaderParser* parser, Reader& text)
        {
            Token token;

            // Read function properties
            while (text.CanRead())
            {
                text.ReadToken(&token);

                // Call children
                if (ProcessChildren(token, parser))
                    break;
            }

            // Token should contain function output type, now read function name
            text.ReadToken(&token);
            _current.Name = token.ToString();

            // Check if name is unique
            for (int32 i = 0; i < _cache.Count(); i++)
            {
                if (_cache[i].Name == _current.Name)
                {
                    parser->OnError(String::Format(TEXT("Duplicated function \'{0}\'. Function with that name already exists."), String(_current.Name)));
                    return;
                }
            }
        }

        /// <summary>
        /// Event called after parsing shader function
        /// </summary>
        /// <param name="parser">Parser object</param>
        /// <param name="text">Source code reader</param>
        virtual void OnParseAfter(IShaderParser* parser, Reader& text) = 0;

        /// <summary>
        /// Process final results and send them to the Shader Meta object
        /// </summary>
        /// <param name="parser">Parser object</param>
        /// <param name="result">Output shader meta</param>
        virtual void FlushCache(IShaderParser* parser, ShaderMeta* result) = 0;

    public:

        // [IShaderFunctionReader]
        void Process(IShaderParser* parser, Reader& text) override
        {
            OnParseBefore(parser, text);
            if (parser->Failed())
                return;

            OnParse(parser, text);
            if (parser->Failed())
                return;

            OnParseAfter(parser, text);
        }

        void CollectResults(IShaderParser* parser, ShaderMeta* result) override
        {
            // Validate function names
            for (int32 i = 0; i < _cache.Count(); i++)
            {
                auto& first = _cache[i];
                for (int32 j = i + 1; j < _cache.Count(); j++)
                {
                    auto& second = _cache[j];
                    if (first.Name == second.Name)
                    {
                        parser->OnError(TEXT("Duplicated shader function names."));
                        return;
                    }
                }
            }

            FlushCache(parser, result);
        }
    };

    /// <summary>
    /// Implementation of shader functions reader
    /// </summary>
    template<typename MetaType>
    class ShaderFunctionReader : public ShaderMetaReader<MetaType>
    {
    public:

        typedef ShaderMetaReader<MetaType> ShaderMetaReaderType;

    protected:

        class StripLineReader : public ITokenReader
        {
        private:

            Token _startToken;

        public:

            explicit StripLineReader(const char* token)
                : _startToken(token)
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

    public:

        /// <summary>
        /// Shader function permutations reader
        /// </summary>
        class PermutationReader : public ITokenReader
        {
        protected:

            ShaderFunctionReader* _parent;
            int32 _startTokenPermutationSize;

            const char* PermutationTokens[SHADER_PERMUTATIONS_MAX_PARAMS_COUNT] =
            {
                "META_PERMUTATION_1",
                "META_PERMUTATION_2",
                "META_PERMUTATION_3",
                "META_PERMUTATION_4",
            };
            static_assert(SHADER_PERMUTATIONS_MAX_PARAMS_COUNT == 4, "Invalid maximum amount of shader permutations.");

        public:

            /// <summary>
            /// Init
            /// </summary>
            /// <param name="parent">Parent shader function reader object</param>
            PermutationReader(ShaderFunctionReader* parent)
                : _parent(parent)
                , _startTokenPermutationSize(0)
            {
            }

        public:

            /// <summary>
            /// Clear cache
            /// </summary>
            void Clear()
            {
                _startTokenPermutationSize = 0;
            }

        public:

            // [ITokenReader]
            bool CheckStartToken(const Token& token) override
            {
                for (int32 i = 0; i < ARRAY_COUNT(PermutationTokens); i++)
                {
                    if (token == PermutationTokens[i])
                    {
                        _startTokenPermutationSize = i + 1;
                        return true;
                    }
                }

                return false;
            }

            void Process(IShaderParser* parser, Reader& text) override
            {
                Token token;
                auto& current = _parent->_current;

                // Add permutation
                int32 permutationIndex = current.Permutations.Count();
                auto& permutation = current.Permutations.AddOne();

                // Read all parameters for the permutation
                ASSERT(_startTokenPermutationSize > 0);
                for (int32 paramIndex = 0; paramIndex < _startTokenPermutationSize; paramIndex++)
                {
                    // Check for missing end
                    if (!text.CanRead())
                    {
                        parser->OnError(TEXT("Missing ending of shader function permutation."));
                        return;
                    }

                    // Read definition name
                    text.ReadToken(&token);
                    if (token.Length == 0)
                    {
                        parser->OnError(TEXT("Incorrect shader permutation. Definition name is empty."));
                        return;
                    }
                    StringAnsi name = token.ToString();

                    // Read '=' character
                    if (token.Separator != Separator('='))
                    {
                        if (token.Separator.IsWhiteSpace())
                            text.EatWhiteSpaces();
                        if (text.PeekChar() != '=')
                        {
                            parser->OnError(TEXT("Incorrect shader permutation. Missing \'='\' character for definition value."));
                            return;
                        }
                    }

                    // Read definition value
                    text.ReadToken(&token);
                    if (token.Length == 0)
                    {
                        parser->OnError(TEXT("Incorrect shader permutation. Definition value is empty."));
                        return;
                    }
                    StringAnsi value = token.ToString();

                    // Read ',' or ')' character (depends on parameter index)
                    char checkChar = (paramIndex == _startTokenPermutationSize - 1) ? ')' : ',';
                    if (token.Separator != Separator(checkChar))
                    {
                        parser->OnError(TEXT("Incorrect shader permutation declaration."));
                        return;
                    }

                    // Check if hasn't been already defined in that permutation
                    if (current.HasDefinition(permutationIndex, name))
                    {
                        parser->OnError(String::Format(TEXT("Incorrect shader function permutation definition. Already defined \'{0}\'."), String(name)));
                        return;
                    }

                    // Add entry to the meta
                    permutation.Entries.Add({ name, value });
                }
            }
        };

        /// <summary>
        /// Shader function flag reader
        /// </summary>
        class FlagReader : public ITokenReader
        {
        protected:

            ShaderFunctionReader* _parent;
            Token _startToken;

        public:

            FlagReader(ShaderFunctionReader* parent)
                : _parent(parent)
                , _startToken("META_FLAG")
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

                // Shader Flag type
                text.ReadToken(&token);
                current.Flags |= ParseShaderFlags(token);
            }
        };

    protected:

        PermutationReader* _permutationReader;

        ShaderFunctionReader()
        {
            ShaderMetaReaderType::_childReaders.Add(_permutationReader = New<PermutationReader>(this));
            ShaderMetaReaderType::_childReaders.Add(New<FlagReader>(this));
        }

        ~ShaderFunctionReader()
        {
        }

    protected:

        // [ShaderMetaReader]
        void OnParseBefore(IShaderParser* parser, Reader& text) override
        {
            Token token;

            // Clear current meta
            auto& current = ShaderMetaReaderType::_current;
            current.Name.Clear();
            current.Permutations.Clear();
            current.Flags = ShaderFlags::Default;
            current.MinFeatureLevel = FeatureLevel::ES2;
            _permutationReader->Clear();

            // Here we read '(x, y)\n' where 'x' is a shader function 'visible' flag, and 'y' is mini feature level
            text.ReadToken(&token);
            token = parser->GetMacros().GetValue(token);
            if (token == "true" || token == "1")
            {
                // Visible shader
            }
            else if (token == "false" || token == "0" || token.Length == 0)
            {
                // Hidden shader
                current.Flags = ShaderFlags::Hidden;
            }
            else
            {
                // Undefined macro (fallback into hidden shader)
                current.Flags = ShaderFlags::Hidden;
                //parser->OnError(String::Format(TEXT("Invalid shader function \'isVisible\' option value \'{0}\'."), String(token.ToString())));
                //return;
            }
            text.ReadToken(&token);
            token = parser->GetMacros().GetValue(token);
            struct MinFeatureLevel
            {
                FeatureLevel Level;
                const char* Token;
            };
            MinFeatureLevel levels[] =
            {
                { FeatureLevel::ES2, "FEATURE_LEVEL_ES2" },
                { FeatureLevel::ES3, "FEATURE_LEVEL_ES3" },
                { FeatureLevel::ES3_1, "FEATURE_LEVEL_ES3_1" },
                { FeatureLevel::SM4, "FEATURE_LEVEL_SM4" },
                { FeatureLevel::SM5, "FEATURE_LEVEL_SM5" },
            };
            bool missing = true;
            for (int32 i = 0; i < ARRAY_COUNT(levels); i++)
            {
                if (token == levels[i].Token)
                {
                    current.MinFeatureLevel = levels[i].Level;
                    missing = false;
                    break;
                }
            }
            if (missing)
            {
                parser->OnError(TEXT("Invalid shader function \'minFeatureLevel\' option value."));
                return;
            }

            // Read rest of the line
            text.ReadLine();
        }

        void OnParseAfter(IShaderParser* parser, Reader& text) override
        {
            auto& current = ShaderMetaReaderType::_current;

            // Validate amount of permutations
            if (current.Permutations.Count() > SHADER_PERMUTATIONS_MAX_COUNT)
            {
                parser->OnError(String::Format(TEXT("Function \'{0}\' uses too many permutations. Maximum allowed amount is {1}."), String(current.Name), SHADER_PERMUTATIONS_MAX_COUNT));
                return;
            }

            // Check if shader has no permutations
            if (current.Permutations.IsEmpty())
            {
                // Just add blank permutation (rest of the code will work without any hacks for empty permutations list)
                current.Permutations.Add(ShaderPermutation());
            }

            // Check if use this shader program
            if ((current.Flags & ShaderFlags::Hidden) == (ShaderFlags)0 && current.MinFeatureLevel <= parser->GetFeatureLevel())
            {
                // Cache read function
                ShaderMetaReaderType::_cache.Add(current);
            }
        }
    };
}

#define DECLARE_SHADER_META_READER_HEADER(tokenName, shaderMetaMemberCollection) public: \
	bool CheckStartToken(const Token& token) override \
	{ return token == tokenName; } \
	void FlushCache(IShaderParser* parser, ShaderMeta* result) override \
	{ result->shaderMetaMemberCollection.Add(_cache); }

#endif
