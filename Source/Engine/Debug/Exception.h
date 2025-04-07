// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Object.h"
#include "Engine/Core/Types/String.h"

namespace Log
{
    class Logger;

    /// <summary>
    /// Represents errors that occur during application execution.
    /// </summary>
    class FLAXENGINE_API Exception : public Object
    {
    protected:
        String _message;
        String _additionalInfo;
        LogType _level;

    public:
        /// <summary>
        /// Creates default exception without additional data
        /// </summary>
        Exception()
            : Exception(String::Empty)
        {
        }

        /// <summary>
        /// Creates default exception with additional data
        /// </summary>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        Exception(const String& additionalInfo)
            : Exception(TEXT("An exception has occurred."), additionalInfo)
        {
        }

        /// <summary>
        /// Creates new custom message with additional data
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        /// <param name="additionalInfo">Additional information that help describe error</param>
        Exception(const String& message, const String& additionalInfo)
            : _message(message)
            , _additionalInfo(additionalInfo)
            , _level(LogType::Warning)
        {
        }

    public:
        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~Exception();

    public:
        /// <summary>
        /// Gets a message that describes the current exception.
        /// </summary>
        /// <returns>Message string.</returns>
        FORCE_INLINE const String& GetMessage() const
        {
            return _message;
        }

        /// <summary>
        /// Gets a message that describes the current exception.
        /// </summary>
        /// <returns>Message string.</returns>
        FORCE_INLINE String GetMessage()
        {
            return _message;
        }

        /// <summary>
        /// Gets a additional info that describes the current exception details.
        /// </summary>
        /// <returns>Message string.</returns>
        FORCE_INLINE const String& GetAdditionalInfo() const
        {
            return _additionalInfo;
        }

        /// <summary>
        /// Gets a additional info that describes the current exception details.
        /// </summary>
        /// <returns>Message string.</returns>
        FORCE_INLINE String GetAdditionalInfo()
        {
            return _additionalInfo;
        }

        /// <summary>
        /// Get exception level
        /// </summary>
        /// <returns>Message Type used for display log</returns>
        FORCE_INLINE const LogType& GetLevel() const
        {
            return _level;
        }

        /// <summary>
        /// Get exception level
        /// </summary>
        /// <returns>Message Type used for display log</returns>
        FORCE_INLINE LogType GetLevel()
        {
            return _level;
        }

        /// <summary>
        /// Override exception level
        /// </summary>
        /// <param name="level">Override default value of exception level</param>
        Exception& SetLevel(LogType level)
        {
            _level = level;
            return *this;
        }

        // TODO: implement StackTrace caching: https://www.codeproject.com/kb/threads/stackwalker.aspx

    public:
        // [Object]
        String ToString() const final override
        {
            if (!_additionalInfo.IsEmpty())
            {
                return GetMessage() + TEXT(" \n\n Additional info: ") + GetAdditionalInfo();
            }

            return GetMessage();
        }
    };
}
