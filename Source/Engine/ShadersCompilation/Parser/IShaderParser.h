// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    struct ParserMacros
    {
        const Array<ShaderMacro>* Data;

        ParserMacros(const Array<ShaderMacro>& data)
        {
            Data = &data;
        }

        Token GetValue(Token& token) const
        {
            for (int32 i = 0; i < Data->Count(); i++)
            {
                if (token == Data->At(i).Name)
                {
                    // Use macro value
                    return Token(Data->At(i).Definition);
                }
            }

            // Fallback to token
            return token;
        }
    };

    /// <summary>
    /// Interface describing shader source code parser
    /// </summary>
    class IShaderParser
    {
    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~IShaderParser()
        {
        }

    public:

        /// <summary>
        /// Gets the parser feature level of the target platform graphics backend.
        /// </summary>
        /// <returns>The graphics feature level</returns>
        virtual FeatureLevel GetFeatureLevel() const = 0;

        /// <summary>
        /// Gets the parser macros.
        /// </summary>
        /// <returns>The macros</returns>
        virtual ParserMacros GetMacros() const = 0;

        /// <summary>
        /// Gets value indicating that shader processing operation failed
        /// </summary>
        /// <returns>True if shader processing failed, otherwise false</returns>
        virtual bool Failed() const = 0;

        /// <summary>
        /// Gets source code reader
        /// </summary>
        /// <returns>Source code reader</returns>
        virtual Reader& GetReader() = 0;

        /// <summary>
        /// Event send to the parser on reading shader source code error
        /// </summary>
        /// <param name="message">Message to send</param>
        virtual void OnError(const String& message) = 0;

        /// <summary>
        /// Event send to the parser on reading shader source code warning
        /// </summary>
        /// <param name="message">Message to send</param>
        virtual void OnWarning(const String& message) = 0;
    };
}

#endif
