// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when time interval allotted to an operation has expired.
    /// </summary>
    class TimeoutException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        TimeoutException()
            : TimeoutException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        TimeoutException(const String& additionalInfo)
            : Exception(TEXT("Current operation has timed out."), additionalInfo)
        {
        }
    };
}
