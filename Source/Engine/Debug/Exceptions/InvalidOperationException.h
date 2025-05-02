// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when a method call is invalid in an object's current state.
    /// </summary>
    class InvalidOperationException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        InvalidOperationException()
            : InvalidOperationException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        InvalidOperationException(const String& additionalInfo)
            : Exception(TEXT("Current object didn't exists or its state was invalid."), additionalInfo)
        {
        }
    };
}
