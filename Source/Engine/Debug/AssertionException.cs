// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.Assertions
{
    /// <summary>
    /// An exception that is thrown on a failure. To enable this feature <see cref="Assert.RaiseExceptions"/> needs to be set to true.
    /// </summary>
    [HideInEditor]
    public class AssertionException : Exception
    {
        private string _userMessage;

        /// <inheritdoc />
        public override string Message
        {
            get
            {
                string message = base.Message;
                if (_userMessage != null)
                    message = string.Concat(message, '\n', _userMessage);
                return message;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssertionException"/> class.
        /// </summary>
        public AssertionException()
        : this(string.Empty, string.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssertionException"/> class.
        /// </summary>
        /// <param name="userMessage">The user message.</param>
        public AssertionException(string userMessage)
        : this("Assertion failed!", userMessage)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssertionException"/> class.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="userMessage">The user message.</param>
        public AssertionException(string message, string userMessage)
        : base(message)
        {
            _userMessage = userMessage;
        }
    }
}
