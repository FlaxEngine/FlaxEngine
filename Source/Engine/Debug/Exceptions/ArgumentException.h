// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when an Arguments are not valid.
    /// </summary>
    class ArgumentException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        ArgumentException()
            : ArgumentException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        ArgumentException(const String& additionalInfo)
            : Exception(TEXT("One or more of provided arguments are not valid."), additionalInfo)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="argumentName">Argument name</param>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        ArgumentException(const String& argumentName, const String& additionalInfo)
            : Exception(String::Format(TEXT("Provided argument {0} is not valid."), argumentName), additionalInfo)
        {
        }
    };
}
