// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when an I/O error occurs.
    /// </summary>
    class IOException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        IOException()
            : IOException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        IOException(const String& additionalInfo)
            : Exception(TEXT("I/O error occurred."), additionalInfo)
        {
        }
    };
}
