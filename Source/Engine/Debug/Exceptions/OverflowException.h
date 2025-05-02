// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when an arithmetic, casting, or conversion operation results in an overflow.
    /// </summary>
    class OverflowException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        OverflowException()
            : OverflowException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        OverflowException(const String& additionalInfo)
            : Exception(TEXT("Arithmetic, casting, or conversion operation results in an overflow."), additionalInfo)
        {
        }
    };
}
