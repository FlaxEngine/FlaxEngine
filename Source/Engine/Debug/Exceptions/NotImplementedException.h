// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when a method or operation is not implemented.
    /// </summary>
    class NotImplementedException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        NotImplementedException()
            : NotImplementedException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        NotImplementedException(const String& additionalInfo)
            : Exception(TEXT("Current method or operation is not implemented."), additionalInfo)
        {
        }
    };
}
