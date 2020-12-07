// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/Exception.h"
#include "Engine/Serialization/Json.h"
#include <ThirdParty/rapidjson/error/en.h>

namespace Log
{
    /// <summary>
    /// The exception that is thrown when parsing json file fails.
    /// </summary>
    class JsonParseException : public Exception
    {
    public:

        typedef rapidjson::ParseErrorCode ErrorCode;

    public:

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="error">Parsing error code.</param>
        JsonParseException(ErrorCode error)
            : JsonParseException(error, String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="error">Parsing error code.</param>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        JsonParseException(ErrorCode error, const String& additionalInfo)
            : Exception(String::Format(TEXT("Parsing Json failed with error code {0}. {1}"), static_cast<int32>(error), GetParseError_En(error)), additionalInfo)
        {
        }
    };
}
