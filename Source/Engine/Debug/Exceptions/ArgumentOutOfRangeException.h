// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when an Argument array has index out of range.
    /// </summary>
    class ArgumentOutOfRangeException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        ArgumentOutOfRangeException()
            : ArgumentOutOfRangeException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        ArgumentOutOfRangeException(const String& additionalInfo)
            : Exception(TEXT("One or more of provided arguments has index out of range"), additionalInfo)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="argumentName">Argument name</param>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        ArgumentOutOfRangeException(const String& argumentName, const String& additionalInfo)
            : Exception(String::Format(TEXT("Provided argument {0} is out of range."), argumentName), additionalInfo)
        {
        }
    };
}
