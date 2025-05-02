// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "ITokenReader.h"

#if COMPILE_WITH_SHADER_COMPILER

namespace ShaderProcessing
{
    /// <summary>
    /// Interface for objects that can have child token readers
    /// </summary>
    template<typename Type>
    class ITokenReadersContainerBase
    {
    protected:

        Array<Type*> _childReaders;

    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~ITokenReadersContainerBase()
        {
            // Cleanup
            _childReaders.ClearDelete();
        }

    protected:

        /// <summary>
        /// Try to process given token by any child reader
        /// </summary>
        /// <param name="token">Starting token to check</param>
        /// <param name="parser">Parser object</param>
        /// <returns>True if no token processing has been done, otherwise false</returns>
        virtual bool ProcessChildren(const Token& token, IShaderParser* parser)
        {
            for (int32 i = 0; i < _childReaders.Count(); i++)
            {
                if (_childReaders[i]->CheckStartToken(token))
                {
                    _childReaders[i]->Process(parser, parser->GetReader());
                    return false;
                }
            }

            return true;
        }
    };

    /// <summary>
    /// Interface for objects that can have child ITokenReader objects
    /// </summary>
    class ITokenReadersContainer : public ITokenReadersContainerBase<ITokenReader>
    {
    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~ITokenReadersContainer()
        {
            // Cleanup
            _childReaders.ClearDelete();
        }
    };
}

#endif
