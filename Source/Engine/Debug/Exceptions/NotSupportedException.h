// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when a method or operation is not supported.
    /// </summary>
    class NotSupportedException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        NotSupportedException()
            : NotSupportedException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        NotSupportedException(const String& additionalInfo)
            : Exception(TEXT("Current method or operation is not supported in current context"), additionalInfo)
        {
        }
    };
}
