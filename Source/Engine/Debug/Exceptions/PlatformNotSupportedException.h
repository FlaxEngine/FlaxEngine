// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when operation is not supported on the current platform.
    /// </summary>
    class PlatformNotSupportedException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        PlatformNotSupportedException()
            : PlatformNotSupportedException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        PlatformNotSupportedException(const String& additionalInfo)
            : Exception(TEXT("Method or operation in not supported on current platform."), additionalInfo)
        {
        }
    };
}
