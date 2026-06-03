// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"
#include "Engine/Scripting/Types.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when a method call is invalid in an object's current state.
    /// </summary>
    class CLRInnerException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        CLRInnerException()
            : CLRInnerException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        CLRInnerException(const String& additionalInfo)
            : Exception(TEXT("Current .NET method has thrown an inner exception"), additionalInfo)
        {
        }
    };
}
