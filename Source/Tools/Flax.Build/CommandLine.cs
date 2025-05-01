// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Flax.Build
{
    /// <summary>
    /// The command line utilities.
    /// </summary>
    public class CommandLine
    {
        internal static List<MethodInfo> ConsoleCommands;

        /// <summary>
        /// The command line option data.
        /// </summary>
        public class Option
        {
            /// <summary>
            /// The name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The value.
            /// </summary>
            public string Value;

            /// <inheritdoc />
            public override string ToString()
            {
                if (string.IsNullOrEmpty(Value))
                    return $"/{Name}";
                return $"/{Name}={Value}";
            }
        }

        /// <summary>
        /// Gets the program executable string. Post-processed to improve parsing performance.
        /// </summary>
        /// <returns>The command line.</returns>
        public static string Get()
        {
            string[] args = Environment.GetCommandLineArgs();
            if (args.Length > 1)
            {
                string commandLine = Environment.CommandLine.Remove(0, args[0].Length + 2);
                commandLine = commandLine.Trim(' ');
                return commandLine;
            }

            return string.Empty;
        }

        /// <summary>
        /// Gets the help information for the command line options for the given type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The options help information.</returns>
        public static string GetHelp(Type type)
        {
            StringWriter result = new StringWriter();

            result.WriteLine("Usage: Flax.Build.exe [options]");
            result.WriteLine("Options:");

            var options = GetMembers(type);

            foreach (var option in options)
            {
                result.Write("  -" + option.Key.Name);

                if (!string.IsNullOrEmpty(option.Key.ValueHint))
                {
                    result.Write("={0}", option.Key.ValueHint);
                }

                result.WriteLine();

                if (!string.IsNullOrEmpty(option.Key.Description))
                {
                    result.WriteLine("\t{0}", option.Key.Description.Replace(Environment.NewLine, Environment.NewLine + "\t"));
                }

                result.WriteLine();
            }

            return result.ToString();
        }

        /// <summary>
        /// Gets the options for command line configuration for the given type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The list of configurable options.</returns>
        public static Dictionary<CommandLineAttribute, MemberInfo> GetMembers(Type type)
        {
            if (type == null)
                throw new ArgumentNullException();

            var result = new Dictionary<CommandLineAttribute, MemberInfo>();

            var members = type.GetMembers(BindingFlags.Static | BindingFlags.Public);

            for (int i = 0; i < members.Length; i++)
            {
                var member = members[i];

                var attribute = member.GetCustomAttribute<CommandLineAttribute>();
                if (attribute != null)
                {
                    result.Add(attribute, member);
                }
            }

            return result;
        }

        /// <summary>
        /// Gets the options for command line configuration for the given object instance.
        /// </summary>
        /// <param name="obj">The object instance.</param>
        /// <returns>The list of configurable options.</returns>
        public static Dictionary<CommandLineAttribute, MemberInfo> GetMembers(object obj)
        {
            if (obj == null)
                throw new ArgumentNullException();

            var result = new Dictionary<CommandLineAttribute, MemberInfo>();

            var type = obj.GetType();
            var members = type.GetMembers(BindingFlags.Instance | BindingFlags.Public);

            for (int i = 0; i < members.Length; i++)
            {
                var member = members[i];

                var attribute = member.GetCustomAttribute<CommandLineAttribute>();
                if (attribute != null)
                {
                    result.Add(attribute, member);
                }
            }

            return result;
        }

        private static int _cacheHash;
        private static readonly object _cacheLock = new object();
        private static Option[] _cacheOptions;

        /// <summary>
        /// Determines whether the specified option has been specified in the environment command line.
        /// </summary>
        /// <param name="name">The option name.</param>
        /// <returns><c>true</c> if the specified option has been specified; otherwise, <c>false</c>.</returns>
        public static bool HasOption(string name)
        {
            return HasOption(name, Get());
        }

        /// <summary>
        /// Determines whether the specified option has been specified in the environment command line.
        /// </summary>
        /// <param name="name">The option name.</param>
        /// <param name="commandLine">The command line.</param>
        /// <returns><c>true</c> if the specified option has been specified; otherwise, <c>false</c>.</returns>
        private static bool HasOption(string name, string commandLine)
        {
            return commandLine.Length > 0 && GetOptions(commandLine).Any(p => (string.Equals(p.Name, name, StringComparison.OrdinalIgnoreCase)));
        }

        /// <summary>
        /// Gets the options for the current environment command line.
        /// </summary>
        /// <returns>The options.</returns>
        public static Option[] GetOptions()
        {
            return GetOptions(Get());
        }

        /// <summary>
        /// Gets the options for the given command line.
        /// </summary>
        /// <param name="commandLine">The command line.</param>
        /// <returns>The options.</returns>
        public static Option[] GetOptions(string commandLine)
        {
            int hash = commandLine.GetHashCode();
            Option[] options;
            lock (_cacheLock)
            {
                if (hash != _cacheHash)
                {
                    _cacheHash = hash;
                    _cacheOptions = Parse(commandLine);
                }

                options = _cacheOptions;
            }

            return options;
        }

        /// <summary>
        /// Gets the options for the given configuration (key=value pairs).
        /// </summary>
        /// <param name="configuration">The configuration (key=value pairs).</param>
        /// <returns>The options.</returns>
        public static Option[] GetOptions(Dictionary<string, string> configuration)
        {
            var options = new Option[configuration.Count];
            int i = 0;
            foreach (var e in configuration)
            {
                options[i++] = new Option
                {
                    Name = e.Key,
                    Value = e.Value,
                };
            }
            return options;
        }

        /// <summary>
        /// Parses the specified command line.
        /// </summary>
        /// <param name="commandLine">The command line.</param>
        /// <returns>The options.</returns>
        public static Option[] Parse(string commandLine)
        {
            var options = new List<Option>();
            var length = commandLine.Length;

            for (int i = 0; i < length;)
            {
                // Skip white space
                while (i < length && char.IsWhiteSpace(commandLine[i]))
                    i++;

                // Read option prefix
                if (i == length)
                    break;
                var wholeQuote = commandLine[i] == '\"';
                if (wholeQuote)
                    i++;
                if (i == length)
                    break;
                if (commandLine[i] == '-')
                    i++;
                else if (commandLine[i] == '/')
                    i++;

                // Skip white space
                while (i < length && char.IsWhiteSpace(commandLine[i]))
                    i++;

                // Read option name
                int nameStart = i;
                while (i < length && commandLine[i] != '-' && commandLine[i] != '=' && !char.IsWhiteSpace(commandLine[i]))
                    i++;
                int nameEnd = i;
                string name = commandLine.Substring(nameStart, nameEnd - nameStart);

                // Skip white space
                while (i < length && char.IsWhiteSpace(commandLine[i]))
                    i++;

                // Check if has no value
                if (i >= length - 1 || commandLine[i] != '=')
                {
                    options.Add(new Option
                    {
                        Name = name,
                        Value = string.Empty
                    });
                    if (wholeQuote)
                        i++;
                    if (i < length && commandLine[i] != '\"')
                        i++;
                    continue;
                }

                // Read value
                i++;
                int valueStart, valueEnd;
                if (commandLine.Length > i + 1 && commandLine[i] == '\\' && commandLine[i + 1] == '\"')
                {
                    valueStart = i + 2;
                    i++;
                    while (i + 1 < length && commandLine[i] != '\\' && commandLine[i + 1] != '\"')
                        i++;
                    valueEnd = i;
                    i += 2;
                    if (wholeQuote)
                    {
                        while (i < length && commandLine[i] != '\"')
                            i++;
                        i++;
                    }
                }
                else if (commandLine[i] == '\"' || commandLine[i] == '\'')
                {
                    var quoteChar = commandLine[i];
                    valueStart = i + 1;
                    i++;
                    while (i < length && commandLine[i] != quoteChar)
                        i++;
                    valueEnd = i;
                    i++;
                    if (wholeQuote)
                    {
                        while (i < length && commandLine[i] != '\"')
                            i++;
                        i++;
                    }
                }
                else if (wholeQuote)
                {
                    valueStart = i;
                    while (i < length && commandLine[i] != '\"')
                        i++;
                    valueEnd = i;
                    i++;
                }
                else
                {
                    valueStart = i;
                    while (i < length && commandLine[i] != ' ')
                        i++;
                    valueEnd = i;
                }
                string value = commandLine.Substring(valueStart, valueEnd - valueStart);
                value = value.Trim();
                if (value.StartsWith("\\\"") && value.EndsWith("\\\""))
                    value = value.Substring(2, value.Length - 4);
                options.Add(new Option
                {
                    Name = name,
                    Value = value
                });
            }

            return options.ToArray();
        }

        /// <summary>
        /// Configures the members of specified object using the command line options.
        /// </summary>
        /// <param name="obj">The object.</param>
        public static void Configure(object obj)
        {
            Configure(obj, Get());
        }

        /// <summary>
        /// Configures the static members of specified type using the command line options.
        /// </summary>
        /// <param name="type">The type.</param>
        public static void Configure(Type type)
        {
            Configure(type, Get());
        }

        /// <summary>
        /// Configures the members of the specified object using the command line options.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="commandLine">The command line.</param>
        public static void Configure(Type type, string commandLine)
        {
            Configure(GetMembers(type), null, commandLine);
        }

        /// <summary>
        /// Configures the members of the specified object using the command line options.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="commandLine">The command line.</param>
        public static void Configure(object obj, string commandLine)
        {
            Configure(GetMembers(obj), obj, commandLine);
        }

        /// <summary>
        /// Configures the members of the specified object using the command line options.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="configuration">The configuration (key=value pairs).</param>
        public static void Configure(Type type, Dictionary<string, string> configuration)
        {
            Configure(GetMembers(type), null, configuration);
        }

        /// <summary>
        /// Configures the members of the specified object using the command line options.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="configuration">The configuration (key=value pairs).</param>
        public static void Configure(object obj, Dictionary<string, string> configuration)
        {
            Configure(GetMembers(obj), obj, configuration);
        }

        private static void Configure(Dictionary<CommandLineAttribute, MemberInfo> members, object instance, string commandLine)
        {
            if (commandLine == null)
                throw new ArgumentNullException();
            var options = GetOptions(commandLine);
            Configure(members, instance, options);
        }


        private static void Configure(Dictionary<CommandLineAttribute, MemberInfo> members, object instance, Dictionary<string, string> configuration)
        {
            if (configuration == null)
                throw new ArgumentNullException();
            var options = GetOptions(configuration);
            Configure(members, instance, options);
        }

        private static void Configure(Dictionary<CommandLineAttribute, MemberInfo> members, object instance, Option[] options)
        {
            foreach (var e in members)
            {
                // Get option from command line
                var member = e.Value;
                var option = options.FirstOrDefault(x => x != null && string.Equals(x.Name, e.Key.Name, StringComparison.OrdinalIgnoreCase));
                if (option == null)
                    continue;

                // Convert text value to actual value object
                object value;
                Type type;
                {
                    if (member is FieldInfo field)
                    {
                        type = field.FieldType;
                    }
                    else if (member is PropertyInfo property)
                    {
                        type = property.PropertyType;
                    }
                    else if (member is MethodInfo method)
                    {
                        // Add console command to be invoked by build tool
                        if (ConsoleCommands == null)
                            ConsoleCommands = new List<MethodInfo>();
                        ConsoleCommands.Add(method);
                        continue;
                    }
                    else
                    {
                        throw new Exception("Unknown member type.");
                    }
                }
                try
                {
                    // Implicit casting to boolean value
                    if (type == typeof(bool) && string.IsNullOrEmpty(option.Value))
                    {
                        value = true;
                    }
                    // Arrays handling
                    else if (type.IsArray)
                    {
                        var elementType = type.GetElementType();
                        var values = option.Value.Split(',');
                        value = Array.CreateInstance(elementType, values.Length);
                        TypeConverter typeConverter = TypeDescriptor.GetConverter(elementType);
                        for (int i = 0; i < values.Length; i++)
                        {
                            ((Array)value).SetValue(typeConverter.ConvertFromString(values[i]), i);
                        }
                    }
                    // Generic case
                    else
                    {
                        TypeConverter typeConverter = TypeDescriptor.GetConverter(type);
                        value = typeConverter.ConvertFromString(option.Value);
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception(string.Format("Failed to convert configuration property {0} value \"{1}\" to type {2}", member.Name, option.Value, type), ex);
                }

                // Set the target member value
                try
                {
                    if (member is FieldInfo field)
                    {
                        field.SetValue(instance, value);
                    }
                    else if (member is PropertyInfo property)
                    {
                        property.SetValue(instance, value);
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception(string.Format("Failed to set configuration property {0} with argument {1} to value \"{2}\"", member.Name, option.Name, option.Value), ex);
                }
            }
        }
    }
}
