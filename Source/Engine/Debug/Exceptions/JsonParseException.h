// Copyright (c) Wojciech Figat. All rights reserved.

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
        /// <param name="offset">Parsing error location.</param>
        JsonParseException(ErrorCode error, size_t offset)
            : JsonParseException(error, offset, StringView::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="error">Parsing error code.</param>
        /// <param name="offset">Parsing error location.</param>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        JsonParseException(ErrorCode error, size_t offset, const StringView& additionalInfo)
            : Exception(String::Format(TEXT("Parsing Json failed with error code {0} (offset {2}). {1}"), static_cast<int32>(error), GetParseError_En(error), offset), additionalInfo)
        {
        }
    };
}
