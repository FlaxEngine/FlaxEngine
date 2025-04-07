// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"

namespace Log
{
    /// <summary>
    /// The exception that is thrown when an index is outside the bounds of an array or collection.
    /// </summary>
    class IndexOutOfRangeException : public Exception
    {
    public:

        /// <summary>
        /// Init
        /// </summary>
        IndexOutOfRangeException()
            : IndexOutOfRangeException(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        IndexOutOfRangeException(const String& additionalInfo)
            : Exception(TEXT("Index is out of range for items in array"), additionalInfo)
        {
        }
    };
}
