// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;

namespace Flax.Build.Bindings
{
    partial class BindingsGenerator
    {
        private struct ParsingContext
        {
            public FileInfo File;
            public Tokenizer Tokenizer;
            public ApiTypeInfo ScopeInfo;
            public AccessLevel CurrentAccessLevel;
            public Stack<ApiTypeInfo> ScopeTypeStack;
            public Stack<AccessLevel> ScopeAccessStack;
            public Dictionary<string, string> PreprocessorDefines;
            public List<string> StringCache;

            public ApiTypeInfo ValidScopeInfoFromStack
            {
                get
                {
                    foreach (var typeInfo in ScopeTypeStack)
                    {
                        if (typeInfo != null)
                            return typeInfo;
                    }
                    return ScopeInfo;
                }
            }

            public void EnterScope(ApiTypeInfo type)
            {
                ScopeAccessStack.Push(CurrentAccessLevel);
                ScopeTypeStack.Push(type);
                ScopeInfo = ScopeTypeStack.Peek();
            }

            public void LeaveScope()
            {
                ScopeTypeStack.Pop();
                ScopeInfo = ScopeTypeStack.Peek();
                CurrentAccessLevel = ScopeAccessStack.Pop();
            }
        }

        private class ParseException : Exception
        {
            public ParseException(ref ParsingContext context, string msg)
            : base(GetParseErrorLocation(ref context, msg))
            {
            }
        }

        private static string GetParseErrorLocation(ref ParsingContext context, string msg)
        {
            // Make it a link clickable in Visual Studio build output
            return $"{context.File.Name}({context.Tokenizer.CurrentLine}): {msg}";
        }

        private static string[] ParseComment(ref ParsingContext context)
        {
            if (context.StringCache == null)
                context.StringCache = new List<string>();
            else
                context.StringCache.Clear();

            int tokensCount = 0;
            bool isValid = true;
            while (isValid)
            {
                var token = context.Tokenizer.PreviousToken(true, true);
                tokensCount++;
                switch (token.Type)
                {
                case TokenType.Newline:
                case TokenType.Whitespace: break;
                case TokenType.CommentMultiLine:
                {
                    // TODO: multi-line comments parsing support for API docs
                    break;
                }
                case TokenType.CommentSingleLine:
                {
                    var commentLine = token.Value.Trim();
                    if (commentLine.StartsWith("// "))
                    {
                        // Fix '//' comments
                        commentLine = "/// " + commentLine.Substring(3);
                    }
                    else if (commentLine.StartsWith("/// <summary>") && commentLine.EndsWith("</summary>"))
                    {
                        // Fix inlined summary
                        commentLine = "/// " + commentLine.Substring(13, commentLine.Length - 23);
                        isValid = false;
                    }
                    else if (commentLine.StartsWith("/// <summary>"))
                    {
                        // End searching after summary begin found
                        isValid = false;
                    }
                    context.StringCache.Insert(0, commentLine);
                    break;
                }
                case TokenType.GreaterThan:
                {
                    // Template definition
                    // TODO: return created template definition for Template types
                    while (isValid)
                    {
                        token = context.Tokenizer.PreviousToken(true, true);
                        tokensCount++;
                        if (token.Type == TokenType.LessThan)
                        {
                            token = context.Tokenizer.PreviousToken(true, true);
                            tokensCount++;
                            break;
                        }
                    }
                    break;
                }
                default:
                    isValid = false;
                    break;
                }
            }

            // Revert the position back to the start
            for (var i = 0; i < tokensCount; i++)
                context.Tokenizer.NextToken(true, true);

            if (context.StringCache.Count == 0)
                return null;
            if (context.StringCache.Count == 1)
            {
                // Ensure to have summary begin/end pair
                context.StringCache.Insert(0, "/// <summary>");
                context.StringCache.Add("/// </summary>");
            }

            return context.StringCache.ToArray();
        }

        public struct TagParameter
        {
            public string Tag;
            public string Value;

            public override string ToString()
            {
                if (Value != null)
                    return Tag + '=' + Value;
                return Tag;
            }
        }

        private static List<TagParameter> ParseTagParameters(ref ParsingContext context)
        {
            var parameters = new List<TagParameter>();
            context.Tokenizer.ExpectToken(TokenType.LeftParent);

            while (true)
            {
                var token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.RightParent)
                    return parameters;

                if (token.Type == TokenType.Identifier || token.Type == TokenType.String || token.Type == TokenType.Number)
                {
                    var tag = new TagParameter
                    {
                        Tag = token.Value,
                    };

                    var nextToken = context.Tokenizer.NextToken();
                    switch (nextToken.Type)
                    {
                    case TokenType.Equal:
                        token = context.Tokenizer.NextToken();
                        tag.Value = token.Value;
                        if (tag.Value[0] == '\"' && tag.Value[tag.Value.Length - 1] == '\"')
                            tag.Value = tag.Value.Substring(1, tag.Value.Length - 2);
                        if (tag.Value.Contains("\\\""))
                            tag.Value = tag.Value.Replace("\\\"", "\"");
                        token = context.Tokenizer.NextToken();
                        if (token.Type == TokenType.Multiply)
                            tag.Value += token.Value;
                        else if (token.Type == TokenType.LeftAngleBracket)
                        {
                            context.Tokenizer.SkipUntil(TokenType.RightAngleBracket, out var s);
                            tag.Value += '<';
                            tag.Value += s;
                            tag.Value += '>';
                        }
                        else
                            context.Tokenizer.PreviousToken();
                        parameters.Add(tag);
                        break;
                    case TokenType.Whitespace:
                    case TokenType.Comma:
                        parameters.Add(tag);
                        break;
                    case TokenType.RightParent:
                        parameters.Add(tag);
                        return parameters;
                    default: throw new ParseException(ref context, $"Expected right parent or next argument, but got {token.Type}.");
                    }
                }
            }
        }

        private static TypeInfo ParseType(ref ParsingContext context)
        {
            var type = new TypeInfo();
            var token = context.Tokenizer.NextToken();

            // Global namespace
            if (token.Type == TokenType.DoubleColon)
            {
                token = context.Tokenizer.NextToken();
            }
            else if (token.Type == TokenType.Colon)
            {
                context.Tokenizer.ExpectToken(TokenType.Colon);
                token = context.Tokenizer.NextToken();
            }
            else if (token.Type == TokenType.Number)
            {
                // Constant
                type.Type = token.Value;
                return type;
            }
            else if (token.Type != TokenType.Identifier)
            {
                // Empty type
                context.Tokenizer.PreviousToken();
                return type;
            }

            // Const
            if (token.Value == "const")
            {
                type.IsConst = true;
                context.Tokenizer.SkipUntil(TokenType.Identifier);
                token = context.Tokenizer.CurrentToken;
            }

            // Forward type
            if (token.Value == "enum")
            {
                context.Tokenizer.SkipUntil(TokenType.Identifier);
                token = context.Tokenizer.CurrentToken;
            }
            if (token.Value == "class")
            {
                context.Tokenizer.SkipUntil(TokenType.Identifier);
                token = context.Tokenizer.CurrentToken;
            }
            else if (token.Value == "struct")
            {
                context.Tokenizer.SkipUntil(TokenType.Identifier);
                token = context.Tokenizer.CurrentToken;
            }

            // Typename
            type.Type = token.Value;
            token = context.Tokenizer.NextToken();

            // Generics
            if (token.Type == TokenType.LeftAngleBracket)
            {
                type.GenericArgs = new List<TypeInfo>();
                do
                {
                    var argType = ParseType(ref context);
                    if (argType.Type != null)
                        type.GenericArgs.Add(argType);
                    token = context.Tokenizer.NextToken();
                } while (token.Type != TokenType.RightAngleBracket);
                token = context.Tokenizer.NextToken();
            }

            while (true)
            {
                // Pointer '*' character
                if (token.Type == TokenType.Multiply)
                {
                    if (type.IsPtr)
                        type.Type += '*';
                    else
                        type.IsPtr = true;
                }
                // Reference `&` character
                else if (token.Type == TokenType.And)
                {
                    if (!type.IsRef)
                        type.IsRef = true;
                    else if (!type.IsMoveRef)
                        type.IsMoveRef = true;
                    else
                        type.Type += '&';
                }
                // Namespace
                else if (token.Type == TokenType.Colon)
                {
                    context.Tokenizer.ExpectToken(TokenType.Colon);
                    type.Type += "::";
                    type.Type += context.Tokenizer.ExpectToken(TokenType.Identifier).Value;
                }
                else
                {
                    // Undo
                    token = context.Tokenizer.PreviousToken();
                    break;
                }

                // Move
                token = context.Tokenizer.NextToken();
            }

            return type;
        }

        private static void ParseTypeArray(ref ParsingContext context, TypeInfo type, object entry)
        {
            // Read the fixed array length
            type.IsArray = true;
            context.Tokenizer.SkipUntil(TokenType.RightBracket, out var length);
            if (context.PreprocessorDefines.TryGetValue(length, out var define))
                length = define;
            if (!int.TryParse(length, out type.ArraySize))
                throw new ParseException(ref context, $"Failed to parse the field {entry} array size '{length}'");
        }

        private static List<FunctionInfo.ParameterInfo> ParseFunctionParameters(ref ParsingContext context)
        {
            var parameters = new List<FunctionInfo.ParameterInfo>();
            context.Tokenizer.ExpectToken(TokenType.LeftParent);
            var currentParam = new FunctionInfo.ParameterInfo();

            do
            {
                var token = context.Tokenizer.NextToken();

                // Exit when there is no any parameters
                if (token.Type == TokenType.RightParent)
                    return parameters;

                // Check for param meta
                if (token.Value == ApiTokens.Param)
                {
                    // Read parameters from the tag
                    var tagParams = ParseTagParameters(ref context);

                    // Process tag parameters
                    foreach (var tag in tagParams)
                    {
                        switch (tag.Tag.ToLower())
                        {
                        case "ref":
                            currentParam.IsRef = true;
                            break;
                        case "out":
                            currentParam.IsOut = true;
                            break;
                        case "this":
                            currentParam.IsThis = true;
                            break;
                        case "params":
                            currentParam.IsParams = true;
                            break;
                        case "attributes":
                            currentParam.Attributes = tag.Value;
                            break;
                        case "defaultvalue":
                            currentParam.DefaultValue = tag.Value;
                            break;
                        default:
                            bool valid = false;
                            ParseFunctionParameterTag?.Invoke(ref valid, tag, ref currentParam);
                            if (valid)
                                break;
                            var location = "function parameter";
                            Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{location}'"));
                            break;
                        }
                    }
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                }

                // Read parameter type and name
                currentParam.Type = ParseType(ref context);
                token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.Identifier)
                {
                    currentParam.Name = token.Value;
                }
                // Support nameless arguments. assume optional usage
                else
                {
                    context.Tokenizer.PreviousToken();
                    if (string.IsNullOrEmpty(currentParam.Attributes))
                        currentParam.Attributes = "Optional";
                    else
                        currentParam.Attributes += ", Optional";
                    currentParam.Name = $"namelessArg{parameters.Count}";
                }

                if (currentParam.IsOut && (currentParam.Type.IsPtr || currentParam.Type.IsRef) && currentParam.Type.Type.EndsWith("*"))
                {
                    // Pointer to value passed as output pointer
                    currentParam.Type.Type = currentParam.Type.Type.Remove(currentParam.Type.Type.Length - 1);
                }

                // Read what's next
                token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.Equal)
                {
                    // Read default value
                    token = context.Tokenizer.NextToken();
                    currentParam.DefaultValue = token.Value;
                    while (true)
                    {
                        token = context.Tokenizer.NextToken();
                        if (token.Type == TokenType.DoubleColon)
                        {
                            currentParam.DefaultValue += "::" + token.Value;
                        }
                        else if (token.Type == TokenType.Colon)
                        {
                            context.Tokenizer.ExpectToken(TokenType.Colon);
                            token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                            currentParam.DefaultValue += "::" + token.Value;
                        }
                        else if (token.Type == TokenType.LeftParent)
                        {
                            currentParam.DefaultValue += token.Value;
                            context.Tokenizer.SkipUntil(TokenType.RightParent, out var parenthesis);
                            currentParam.DefaultValue += parenthesis;
                            currentParam.DefaultValue += context.Tokenizer.CurrentToken.Value;
                        }
                        else
                        {
                            context.Tokenizer.PreviousToken();
                            break;
                        }
                    }
                }
                else if (token.Type == TokenType.LeftBracket)
                {
                    // Read the fixed array length
                    ParseTypeArray(ref context, currentParam.Type, currentParam);
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                }

                // Check for end or next param
                token = context.Tokenizer.ExpectAnyTokens(new[] { TokenType.Comma, TokenType.RightParent });
                if (token.Type == TokenType.Comma)
                {
                    parameters.Add(currentParam);
                    currentParam = new FunctionInfo.ParameterInfo();
                }
                else if (token.Type == TokenType.RightParent)
                {
                    parameters.Add(currentParam);
                    return parameters;
                }
            } while (true);
        }

        private static string ParseName(ref ParsingContext context)
        {
            var name = context.Tokenizer.ExpectToken(TokenType.Identifier).Value;
            if (name.EndsWith("_API"))
            {
                // Skip API export/import define
                name = context.Tokenizer.ExpectToken(TokenType.Identifier).Value;
            }
            return name;
        }

        private static void ParseInheritance(ref ParsingContext context, ClassStructInfo desc, out bool isFinal)
        {
            desc.BaseType = null;
            desc.BaseTypeInheritance = AccessLevel.Private;

            var token = context.Tokenizer.NextToken();
            isFinal = token.Value == "final";
            if (isFinal)
                token = context.Tokenizer.NextToken();

            if (token.Type == TokenType.Colon)
            {
                while (token.Type != TokenType.LeftCurlyBrace)
                {
                    var accessToken = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    switch (accessToken.Value)
                    {
                    case "public":
                        desc.BaseTypeInheritance = AccessLevel.Public;
                        token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                        break;
                    case "protected":
                        desc.BaseTypeInheritance = AccessLevel.Protected;
                        token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                        break;
                    case "private":
                        token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                        break;
                    default:
                        token = accessToken;
                        break;
                    }
                    var inheritType = new TypeInfo
                    {
                        Type = token.Value,
                    };
                    if (desc.Inheritance == null)
                        desc.Inheritance = new List<TypeInfo>();
                    desc.Inheritance.Add(inheritType);
                    token = context.Tokenizer.NextToken();
                    while (token.Type == TokenType.CommentSingleLine || token.Type == TokenType.CommentMultiLine)
                    {
                        token = context.Tokenizer.NextToken();
                    }
                    if (token.Type == TokenType.LeftCurlyBrace)
                        break;
                    if (token.Type == TokenType.Colon)
                    {
                        token = context.Tokenizer.ExpectToken(TokenType.Colon);
                        token = context.Tokenizer.NextToken();
                        inheritType.Type = token.Value;
                        token = context.Tokenizer.NextToken();
                        continue;
                    }
                    if (token.Type == TokenType.DoubleColon)
                    {
                        token = context.Tokenizer.NextToken();
                        inheritType.Type += token.Value;
                        token = context.Tokenizer.NextToken();
                        continue;
                    }
                    if (token.Type == TokenType.LeftAngleBracket)
                    {
                        inheritType.GenericArgs = new List<TypeInfo>();
                        while (true)
                        {
                            token = context.Tokenizer.NextToken();
                            if (token.Type == TokenType.RightAngleBracket)
                                break;
                            if (token.Type == TokenType.Comma)
                                continue;
                            if (token.Type == TokenType.Identifier)
                                inheritType.GenericArgs.Add(new TypeInfo { Type = token.Value });
                            else
                                throw new ParseException(ref context, "Incorrect inheritance");
                        }

                        // TODO: find better way to resolve this (custom base type attribute?)
                        if (inheritType.Type == "ShaderAssetTypeBase")
                        {
                            desc.Inheritance[desc.Inheritance.Count - 1] = inheritType.GenericArgs[0];
                        }

                        token = context.Tokenizer.NextToken();
                    }
                }
                token = context.Tokenizer.PreviousToken();
            }
            else
            {
                // No base type
                token = context.Tokenizer.PreviousToken();
            }
        }

        private static void ParseTag(ref Dictionary<string, string> tags, TagParameter tag)
        {
            if (tags == null)
                tags = new Dictionary<string, string>();
            var idx = tag.Value.IndexOf('=');
            if (idx == -1)
                tags[tag.Value] = string.Empty;
            else
                tags[tag.Value.Substring(0, idx)] = tag.Value.Substring(idx + 1);
        }

        private static ClassInfo ParseClass(ref ParsingContext context)
        {
            var desc = new ClassInfo
            {
                Access = context.CurrentAccessLevel,
                BaseTypeInheritance = AccessLevel.Private,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'class' keyword
            var token = context.Tokenizer.NextToken();
            if (token.Value != "class")
                throw new ParseException(ref context, $"Invalid {ApiTokens.Class} usage (expected 'class' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");

            // Read specifiers
            while (true)
            {
                token = context.Tokenizer.NextToken();
                if (!desc.IsDeprecated && token.Value == "DEPRECATED")
                {
                    token = context.Tokenizer.NextToken();
                    string message = "";
                    if (token.Type == TokenType.LeftParent)
                        context.Tokenizer.SkipUntil(TokenType.RightParent, out message);
                    else
                        context.Tokenizer.PreviousToken();
                    desc.DeprecatedMessage = message.Trim('"');
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Read name
            desc.Name = desc.NativeName = ParseName(ref context);

            // Read inheritance
            ParseInheritance(ref context, desc, out var isFinal);

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "static":
                    desc.IsStatic = true;
                    break;
                case "sealed":
                    desc.IsSealed = true;
                    break;
                case "abstract":
                    desc.IsAbstract = true;
                    break;
                case "nospawn":
                    desc.NoSpawn = true;
                    break;
                case "noconstructor":
                    desc.NoConstructor = true;
                    break;
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "template":
                    desc.IsTemplate = true;
                    break;
                case "inbuild":
                    desc.IsInBuild = true;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "namespace":
                    desc.Namespace = tag.Value;
                    break;
                case "marshalas":
                    desc.MarshalAs = TypeInfo.FromString(tag.Value);
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseTypeTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }

            // C++ class marked as final is meant to be sealed
            if (isFinal && !desc.IsSealed && !desc.IsStatic)
                desc.IsSealed = true;

            return desc;
        }

        private static InterfaceInfo ParseInterface(ref ParsingContext context)
        {
            var desc = new InterfaceInfo
            {
                Access = context.CurrentAccessLevel,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'class' keyword
            var token = context.Tokenizer.NextToken();
            if (token.Value != "class")
                throw new ParseException(ref context, $"Invalid {ApiTokens.Interface} usage (expected 'class' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");

            // Read specifiers
            while (true)
            {
                token = context.Tokenizer.NextToken();
                if (!desc.IsDeprecated && token.Value == "DEPRECATED")
                {
                    token = context.Tokenizer.NextToken();
                    string message = "";
                    if (token.Type == TokenType.LeftParent)
                        context.Tokenizer.SkipUntil(TokenType.RightParent, out message);
                    else
                        context.Tokenizer.PreviousToken();
                    desc.DeprecatedMessage = message.Trim('"');
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Read name
            desc.Name = desc.NativeName = ParseName(ref context);

            // Read inheritance
            ParseInheritance(ref context, desc, out _);

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "template":
                    desc.IsTemplate = true;
                    break;
                case "inbuild":
                    desc.IsInBuild = true;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "namespace":
                    desc.Namespace = tag.Value;
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseTypeTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }

            return desc;
        }

        private static FunctionInfo ParseFunction(ref ParsingContext context)
        {
            var desc = new FunctionInfo
            {
                Access = context.CurrentAccessLevel,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);
            context.Tokenizer.SkipUntil(TokenType.Identifier);

            // Read specifiers
            var isForceInline = false;
            while (true)
            {
                var token = context.Tokenizer.CurrentToken;
                if (!desc.IsStatic && token.Value == "static")
                {
                    desc.IsStatic = true;
                    context.Tokenizer.NextToken();
                }
                else if (!isForceInline && token.Value == "FORCE_INLINE")
                {
                    isForceInline = true;
                    context.Tokenizer.NextToken();
                }
                else if (!desc.IsVirtual && token.Value == "virtual")
                {
                    desc.IsVirtual = true;
                    context.Tokenizer.NextToken();
                }
                else if (!desc.IsDeprecated && token.Value == "DEPRECATED")
                {
                    token = context.Tokenizer.NextToken();
                    string message = "";
                    if (token.Type == TokenType.LeftParent)
                    {
                        context.Tokenizer.SkipUntil(TokenType.RightParent, out message);
                        context.Tokenizer.NextToken();
                    }
                    desc.DeprecatedMessage = message.Trim('"');
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Read return type
            // Handle if "auto" later
            desc.ReturnType = ParseType(ref context);

            // Read name
            desc.Name = ParseName(ref context);
            if (desc.Name == "STDCALL" || desc.Name == "CDECL")
                desc.Name = ParseName(ref context);

            // Read parameters
            desc.Parameters.AddRange(ParseFunctionParameters(ref context));

            // Read ';' or 'const' or 'override' or '= 0' or '{' or '-'
            while (true)
            {
                var token = context.Tokenizer.ExpectAnyTokens(new[] { TokenType.SemiColon, TokenType.LeftCurlyBrace, TokenType.Equal, TokenType.Sub, TokenType.Identifier });
                if (token.Type == TokenType.Equal)
                {
                    context.Tokenizer.SkipUntil(TokenType.SemiColon);
                    break;
                }
                // Support auto FunctionName() -> Type
                else if (token.Type == TokenType.Sub && desc.ReturnType.ToString() == "auto")
                {
                    context.Tokenizer.SkipUntil(TokenType.GreaterThan);
                    desc.ReturnType = ParseType(ref context);
                }
                else if (token.Type == TokenType.Identifier)
                {
                    switch (token.Value)
                    {
                    case "const":
                        if (desc.IsConst)
                            throw new ParseException(ref context, $"Invalid double 'const' specifier in function {desc.Name}");
                        desc.IsConst = true;
                        break;
                    case "override":
                        desc.IsVirtual = true;
                        break;
                    default: throw new ParseException(ref context, $"Unknown identifier '{token.Value}' in function {desc.Name}");
                    }
                }
                else if (token.Type == TokenType.LeftCurlyBrace)
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
                else if (token.Type == TokenType.SemiColon)
                {
                    break;
                }
            }

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "noproxy":
                    desc.NoProxy = true;
                    break;
                case "hidden":
                    desc.IsHidden = true;
                    break;
                case "sealed":
                    desc.IsVirtual = false;
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseMemberTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }
            return desc;
        }

        private static PropertyInfo ParseProperty(ref ParsingContext context)
        {
            var functionInfo = ParseFunction(ref context);

            var propertyName = functionInfo.Name;
            if (propertyName.StartsWith("Get") || propertyName.StartsWith("Set"))
                propertyName = propertyName.Remove(0, 3);

            var classInfo = context.ScopeInfo as ClassInfo;
            if (classInfo == null)
                throw new ParseException(ref context, $"Found property {propertyName} outside the class");

            var isGetter = !functionInfo.ReturnType.IsVoid;
            if (!isGetter && functionInfo.Parameters.Count == 0)
                throw new ParseException(ref context, $"Property {propertyName} setter method in class {classInfo.Name} has to have value parameter to set (line {context.Tokenizer.CurrentLine}).");
            var propertyType = isGetter ? functionInfo.ReturnType : functionInfo.Parameters[0].Type;

            var propertyInfo = classInfo.Properties.FirstOrDefault(x => x.Name == propertyName);
            if (propertyInfo == null)
            {
                propertyInfo = new PropertyInfo
                {
                    Name = propertyName,
                    Comment = functionInfo.Comment,
                    IsStatic = functionInfo.IsStatic,
                    IsHidden = functionInfo.IsHidden,
                    Access = functionInfo.Access,
                    Attributes = functionInfo.Attributes,
                    Type = propertyType,
                };
                classInfo.Properties.Add(propertyInfo);
            }
            else
            {
                if (propertyInfo.IsStatic != functionInfo.IsStatic)
                    throw new ParseException(ref context, $"Property {propertyName} in class {classInfo.Name} has to have both getter and setter methods static or non-static (line {context.Tokenizer.CurrentLine}).");
            }
            if (functionInfo.Tags != null)
            {
                // Populate tags from getter/setter methods
                if (propertyInfo.Tags == null)
                {
                    propertyInfo.Tags = new Dictionary<string, string>(functionInfo.Tags);
                }
                else
                {
                    foreach (var e in functionInfo.Tags)
                        propertyInfo.Tags[e.Key] = e.Value;
                }
            }

            if (isGetter && propertyInfo.Getter != null)
                throw new ParseException(ref context, $"Property {propertyName} in class {classInfo.Name} cannot have multiple getter method (line {context.Tokenizer.CurrentLine}). Getter methods of properties must return value, while setters take this as a parameter.");
            if (!isGetter && propertyInfo.Setter != null)
                throw new ParseException(ref context, $"Property {propertyName} in class {classInfo.Name} cannot have multiple setter method (line {context.Tokenizer.CurrentLine}). Getter methods of properties must return value, while setters take this as a parameter.");

            if (isGetter)
                propertyInfo.Getter = functionInfo;
            else
                propertyInfo.Setter = functionInfo;
            propertyInfo.DeprecatedMessage = functionInfo.DeprecatedMessage;
            propertyInfo.IsHidden |= functionInfo.IsHidden;

            if (propertyInfo.Getter != null && propertyInfo.Setter != null)
            {
                // Check if getter and setter types are matching (const and ref specifiers are skipped)
                var getterType = propertyInfo.Getter.ReturnType;
                var setterType = propertyInfo.Setter.Parameters[0].Type;
                if (!string.Equals(getterType.Type, setterType.Type) ||
                    getterType.IsPtr != setterType.IsPtr ||
                    getterType.IsArray != setterType.IsArray ||
                    getterType.IsBitField != setterType.IsBitField ||
                    getterType.ArraySize != setterType.ArraySize ||
                    getterType.BitSize != setterType.BitSize ||
                    !TypeInfo.Equals(getterType.GenericArgs, setterType.GenericArgs))
                {
                    // Skip compatible types
                    if (getterType.Type == "String" && setterType.Type == "StringView")
                        return propertyInfo;
                    if (getterType.Type == "Array" && setterType.Type == "Span" && getterType.GenericArgs?.Count == 1 && setterType.GenericArgs?.Count == 1 && getterType.GenericArgs[0].Equals(setterType.GenericArgs[0]))
                        return propertyInfo;
                    throw new ParseException(ref context, $"Property {propertyName} in class {classInfo.Name} (line {context.Tokenizer.CurrentLine}) has mismatching getter return type ({getterType}) and setter parameter type ({setterType}). Both getter and setter methods must use the same value type used for property.");
                }

                if (propertyInfo.Comment != null)
                {
                    // Fix documentation comment to reflect both getter and setters available
                    for (var i = 0; i < propertyInfo.Comment.Length; i++)
                    {
                        ref var comment = ref propertyInfo.Comment[i];
                        comment = comment.Replace("/// Gets ", "/// Gets or sets ");
                    }
                }
            }

            return propertyInfo;
        }

        private static EnumInfo ParseEnum(ref ParsingContext context)
        {
            var desc = new EnumInfo
            {
                Access = context.CurrentAccessLevel,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'enum' or `enum class` keywords
            var token = context.Tokenizer.NextToken();
            if (token.Value != "enum")
                throw new ParseException(ref context, $"Invalid {ApiTokens.Enum} usage (expected 'enum' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");
            token = context.Tokenizer.NextToken();
            if (token.Value != "class")
                context.Tokenizer.PreviousToken();

            // Read specifiers
            while (true)
            {
                token = context.Tokenizer.NextToken();
                if (!desc.IsDeprecated && token.Value == "DEPRECATED")
                {
                    token = context.Tokenizer.NextToken();
                    string message = "";
                    if (token.Type == TokenType.LeftParent)
                        context.Tokenizer.SkipUntil(TokenType.RightParent, out message);
                    else
                        context.Tokenizer.PreviousToken();
                    desc.DeprecatedMessage = message.Trim('"');
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Read name
            desc.Name = desc.NativeName = ParseName(ref context);

            // Read enum underlying type
            token = context.Tokenizer.NextToken();
            if (token.Type == TokenType.Colon)
            {
                token = context.Tokenizer.NextToken();
                desc.UnderlyingType = new TypeInfo
                {
                    Type = token.Value,
                };
                token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.LeftAngleBracket)
                {
                    var genericType = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    context.Tokenizer.ExpectToken(TokenType.RightAngleBracket);
                    desc.UnderlyingType.GenericArgs = new List<TypeInfo>
                    {
                        new TypeInfo
                        {
                            Type = genericType.Value,
                        }
                    };
                }
                else
                {
                    token = context.Tokenizer.PreviousToken();
                }
            }
            else
            {
                // No underlying type
                token = context.Tokenizer.PreviousToken();
            }

            // Read enum entries
            context.Tokenizer.ExpectToken(TokenType.LeftCurlyBrace);
            do
            {
                token = context.Tokenizer.NextToken();
                if (token == null || token.Type == TokenType.RightCurlyBrace || token.Type == TokenType.EndOfFile)
                    break;

                if (token.Type == TokenType.Identifier)
                {
                    var entry = new EnumInfo.EntryInfo();
                    entry.Comment = ParseComment(ref context);
                    if (token.Value == ApiTokens.Enum)
                    {
                        // Read enum entry tag parameters
                        var entryTagParameters = ParseTagParameters(ref context);
                        token = context.Tokenizer.NextToken();

                        // Process enum entry tag parameters
                        foreach (var tag in entryTagParameters)
                        {
                            switch (tag.Tag.ToLower())
                            {
                            case "attributes":
                                entry.Attributes = tag.Value;
                                break;
                            default:
                                Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                                break;
                            }
                        }
                    }
                    entry.Name = token.Value;

                    if ((entry.Comment == null || entry.Comment.Length == 0) && entry.Name == "MAX")
                    {
                        // Automatic comment for enum items count entry
                        entry.Comment = new[]
                        {
                            "/// <summary>",
                            "/// The count of items in the " + desc.Name + " enum.",
                            "/// </summary>",
                        };
                    }

                    token = context.Tokenizer.NextToken();
                    if (token.Type == TokenType.Equal)
                    {
                        token = context.Tokenizer.NextToken();
                        entry.Value = string.Empty;
                        while (token.Type != TokenType.EndOfFile &&
                               token.Type != TokenType.RightCurlyBrace &&
                               token.Type != TokenType.Comma)
                        {
                            entry.Value += token.Value;
                            token = context.Tokenizer.NextToken(true);
                        }
                    }
                    context.Tokenizer.PreviousToken();

                    desc.Entries.Add(entry);
                }
            } while (true);

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "inbuild":
                    desc.IsInBuild = true;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "namespace":
                    desc.Namespace = tag.Value;
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseTypeTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }
            return desc;
        }

        private static StructureInfo ParseStructure(ref ParsingContext context)
        {
            var desc = new StructureInfo
            {
                Access = context.CurrentAccessLevel,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'struct' keyword
            var token = context.Tokenizer.NextToken();
            if (token.Value != "struct")
                throw new ParseException(ref context, $"Invalid {ApiTokens.Struct} usage (expected 'struct' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");

            // Read name
            desc.Name = desc.NativeName = ParseName(ref context);

            // Read inheritance
            ParseInheritance(ref context, desc, out _);

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "template":
                    desc.IsTemplate = true;
                    break;
                case "inbuild":
                    desc.IsInBuild = true;
                    break;
                case "nopod":
                    desc.ForceNoPod = true;
                    break;
                case "nodefault":
                    desc.NoDefault = true;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "namespace":
                    desc.Namespace = tag.Value;
                    break;
                case "marshalas":
                    desc.MarshalAs = TypeInfo.FromString(tag.Value);
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseTypeTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }
            return desc;
        }

        private static FieldInfo ParseField(ref ParsingContext context)
        {
            var desc = new FieldInfo
            {
                Access = context.CurrentAccessLevel,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);
            context.Tokenizer.SkipUntil(TokenType.Identifier);

            // Read specifiers
            Token token;
            bool isMutable = false, isVolatile = false;
            while (true)
            {
                token = context.Tokenizer.CurrentToken;
                if (!desc.IsStatic && token.Value == "static")
                {
                    desc.IsStatic = true;
                    context.Tokenizer.NextToken();
                }
                else if (!desc.IsConstexpr && token.Value == "constexpr")
                {
                    desc.IsConstexpr = true;
                    context.Tokenizer.NextToken();
                }
                else if (!isMutable && token.Value == "mutable")
                {
                    isMutable = true;
                    context.Tokenizer.NextToken();
                }
                else if (!isVolatile && token.Value == "volatile")
                {
                    isVolatile = true;
                    context.Tokenizer.NextToken();
                }
                else if (!desc.IsDeprecated && token.Value == "DEPRECATED")
                {
                    token = context.Tokenizer.NextToken();
                    string message = "";
                    if (token.Type == TokenType.LeftParent)
                    {
                        context.Tokenizer.SkipUntil(TokenType.RightParent, out message);
                        context.Tokenizer.NextToken();
                    }
                    desc.DeprecatedMessage = message.Trim('"');
                }
                else
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Read type
            desc.Type = ParseType(ref context);

            // Read name
            desc.Name = ParseName(ref context);

            // Read ';' or default value or array size or bit-field size
            token = context.Tokenizer.ExpectAnyTokens(new[] { TokenType.SemiColon, TokenType.Equal, TokenType.LeftBracket, TokenType.LeftCurlyBrace, TokenType.Colon });
            if (token.Type == TokenType.Equal)
            {
                context.Tokenizer.SkipUntil(TokenType.SemiColon, out desc.DefaultValue, false);
            }
            // Handle ex: API_FIELD() Type FieldName {DefaultValue};
            else if (token.Type == TokenType.LeftCurlyBrace)
            {
                context.Tokenizer.SkipUntil(TokenType.SemiColon, out desc.DefaultValue, false);
                desc.DefaultValue = '{' + desc.DefaultValue;
            }
            else if (token.Type == TokenType.LeftBracket)
            {
                // Read the fixed array length
                ParseTypeArray(ref context, desc.Type, desc);
                token = context.Tokenizer.ExpectAnyTokens(new[] { TokenType.SemiColon, TokenType.Equal });
                if (token.Type == TokenType.Equal)
                {
                    // Fixed array initializer
                    context.Tokenizer.ExpectToken(TokenType.LeftCurlyBrace);
                    context.Tokenizer.SkipUntil(TokenType.RightCurlyBrace);
                    context.Tokenizer.ExpectToken(TokenType.SemiColon);
                }
            }
            else if (token.Type == TokenType.Colon)
            {
                // Read the bit-field size
                var bitSize = context.Tokenizer.ExpectToken(TokenType.Number).Value;
                desc.Type.IsBitField = true;
                desc.Type.BitSize = int.Parse(bitSize);
                context.Tokenizer.ExpectToken(TokenType.SemiColon);
            }

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "readonly":
                    desc.IsReadOnly = true;
                    break;
                case "hidden":
                    desc.IsHidden = true;
                    break;
                case "noarray":
                    desc.NoArray = true;
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseMemberTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }
            return desc;
        }

        private static EventInfo ParseEvent(ref ParsingContext context)
        {
            var desc = new EventInfo
            {
                Access = context.CurrentAccessLevel,
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'static' keyword
            desc.IsStatic = context.Tokenizer.NextToken().Value == "static";
            if (!desc.IsStatic)
                context.Tokenizer.PreviousToken();

            // Read type
            desc.Type = ParseType(ref context);
            if (desc.Type.Type == "Action")
                desc.Type = new TypeInfo { Type = "Delegate", GenericArgs = new List<TypeInfo>() };
            else if (desc.Type.Type != "Delegate")
                throw new ParseException(ref context, $"Invalid {ApiTokens.Event} type. Only Action and Delegate<> types are supported. '{desc.Type}' used on event.");

            // Read name
            desc.Name = ParseName(ref context);

            // Read ';'
            context.Tokenizer.ExpectToken(TokenType.SemiColon);

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "public":
                    desc.Access = AccessLevel.Public;
                    break;
                case "protected":
                    desc.Access = AccessLevel.Protected;
                    break;
                case "private":
                    desc.Access = AccessLevel.Private;
                    break;
                case "internal":
                    desc.Access = AccessLevel.Internal;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "hidden":
                    desc.IsHidden = true;
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseMemberTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }
            return desc;
        }

        private static string ParseString(ref ParsingContext context)
        {
            // Read string (support multi-line string literals)
            string str = string.Empty;
            int startLine = -1;
            while (true)
            {
                var token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.String)
                {
                    if (startLine == -1)
                        startLine = context.Tokenizer.CurrentLine;
                    else if (startLine != context.Tokenizer.CurrentLine)
                        str += "\n";
                    var tokenStr = token.Value;
                    if (tokenStr.Length >= 2 && tokenStr[0] == '\"' && tokenStr[^1] == '\"')
                        tokenStr = tokenStr.Substring(1, tokenStr.Length - 2);
                    str += tokenStr;
                }
                else if (token.Type == TokenType.EndOfFile)
                {
                    break;
                }
                else
                {
                    if (str == string.Empty)
                        throw new Exception($"Expected {TokenType.String}, but got {token} at line {context.Tokenizer.CurrentLine}.");
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Apply automatic formatting for special characters
            str = str.Replace("\\\"", "\"");
            str = str.Replace("\\n", "\n");
            str = str.Replace("\\\n", "\n");
            str = str.Replace("\\\r\n", "\n");
            str = str.Replace("\t", "    ");
            str = str.Replace("\\t", "    ");
            return str;
        }

        private static InjectCodeInfo ParseInjectCode(ref ParsingContext context)
        {
            context.Tokenizer.ExpectToken(TokenType.LeftParent);
            var desc = new InjectCodeInfo();
            context.Tokenizer.SkipUntil(TokenType.Comma, out desc.Lang);
            desc.Code = ParseString(ref context);
            context.Tokenizer.ExpectToken(TokenType.RightParent);
            return desc;
        }

        private static TypedefInfo ParseTypedef(ref ParsingContext context)
        {
            var desc = new TypedefInfo
            {
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'typedef' or 'using' keyword
            var token = context.Tokenizer.NextToken();
            var isUsing = token.Value == "using";
            if (token.Value != "typedef" && !isUsing)
                throw new ParseException(ref context, $"Invalid {ApiTokens.Typedef} usage (expected 'typedef' or 'using' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");

            if (isUsing)
            {
                // Read name
                desc.Name = ParseName(ref context);

                context.Tokenizer.ExpectToken(TokenType.Equal);

                // Read type definition
                desc.Type = ParseType(ref context);
            }
            else
            {
                // Read type definition
                desc.Type = ParseType(ref context);

                // Read name
                desc.Name = ParseName(ref context);
            }

            // Read ';'
            context.Tokenizer.ExpectToken(TokenType.SemiColon);

            // Process tag parameters
            foreach (var tag in tagParams)
            {
                switch (tag.Tag.ToLower())
                {
                case "alias":
                    desc.IsAlias = true;
                    break;
                case "inbuild":
                    desc.IsInBuild = true;
                    break;
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "namespace":
                    desc.Namespace = tag.Value;
                    break;
                case "tag":
                    ParseTag(ref desc.Tags, tag);
                    break;
                default:
                    bool valid = false;
                    ParseTypeTag?.Invoke(ref valid, tag, desc);
                    if (valid)
                        break;
                    Log.Warning(GetParseErrorLocation(ref context, $"Unknown or not supported tag parameter '{tag}' used on '{desc.Name}'"));
                    break;
                }
            }
            return desc;
        }
    }
}
