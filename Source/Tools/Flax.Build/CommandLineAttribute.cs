// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build
{
    /// <summary>
    /// The attribute to indicate the name of a command line argument and additional help description info.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Method)]
    public class CommandLineAttribute : Attribute
    {
        /// <summary>
        /// The command name. Without leading leading '-' nor '/' and trailing '=' character if a value is expected.
        /// </summary>
        public string Name;

        /// <summary>
        /// The specified fixed value for the argument.
        /// </summary>
        public string Value;

        /// <summary>
        /// The optional or required argument value hint for command line help.
        /// </summary>
        public string ValueHint;

        /// <summary>
        /// The option helper description.
        /// </summary>
        public string Description;

        /// <summary>
        /// Initializes a new instance of the <see cref="CommandLineAttribute"/> class.
        /// </summary>
        /// <param name="name">The option name.</param>
        /// <param name="description">The option description.</param>
        public CommandLineAttribute(string name, string description)
        {
            if (name.IndexOf('-') != -1)
                throw new Exception("Command line argument cannot contain '-'.");
            Name = name;
            Description = description;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="CommandLineAttribute"/> class.
        /// </summary>
        /// <param name="name">The option name.</param>
        /// <param name="valueHint">The option value hint for help.</param>
        /// <param name="description">The option description.</param>
        public CommandLineAttribute(string name, string valueHint, string description)
        {
            if (name.IndexOf('-') != -1)
                throw new Exception("Command line argument cannot contain '-'.");
            Name = name;
            ValueHint = valueHint;
            Description = description;
        }
    }
}
