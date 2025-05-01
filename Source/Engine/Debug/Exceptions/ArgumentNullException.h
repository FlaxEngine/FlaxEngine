// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when an Argument pointer is null and should not be.
    /// </summary>
    class ArgumentNullException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        ArgumentNullException()
            : ArgumentNullException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        ArgumentNullException(const String& additionalInfo)
            : Exception(TEXT("One or more of provided arguments is null"), additionalInfo)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="argumentName">Argument name</param>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        ArgumentNullException(const String& argumentName, const String& additionalInfo)
            : Exception(String::Format(TEXT("Provided argument {0} is null."), argumentName), additionalInfo)
        {
        }
    };
}
