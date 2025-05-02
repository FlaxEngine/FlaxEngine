// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when a path or file name exceeds the maximum system - defined length.
    /// </summary>
    class PathTooLongException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        PathTooLongException()
            : PathTooLongException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        PathTooLongException(const String& additionalInfo)
            : Exception(TEXT("Path too long."), additionalInfo)
        {
        }
    };
}
