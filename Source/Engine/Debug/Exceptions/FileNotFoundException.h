// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when a file does not exist.
    /// </summary>
    class FileNotFoundException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        FileNotFoundException()
            : FileNotFoundException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        FileNotFoundException(const String& additionalInfo)
            : Exception(TEXT("File not found"), additionalInfo)
        {
        }
    };
}
