// (c) 2012-2020 Wojciech Figat. All rights reserved.

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

        private static List<string> _commentCache;

        private static string[] ParseComment(ref ParsingContext context)
        {
            if (_commentCache == null)
                _commentCache = new List<string>();
            else
                _commentCache.Clear();

            int tokensCount = 0;
            bool isValid = true;
            while (isValid)
            {
                var token = context.Tokenizer.PreviousToken(true, true);
                tokensCount++;
                switch (token.Type)
                {
                case TokenType.Whitespace: break;
                case TokenType.CommentMultiLine:
                {
                    // TODO: multi-line comments parsing support for API docs
                    break;
                }
                case TokenType.CommentSingleLine:
                {
                    var commentLine = token.Value.Trim();

                    // Fix '//' comments
                    if (commentLine.StartsWith("// "))
                        commentLine = "/// " + commentLine.Substring(3);

                    _commentCache.Insert(0, commentLine);
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

            if (_commentCache.Count == 1)
            {
                // Ensure to have summary begin/end pair
                _commentCache.Insert(0, "/// <summary>");
                _commentCache.Add("/// </summary>");
            }

            return _commentCache.ToArray();
        }

        private struct TagParameter
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
                        parameters.Add(tag);
                        break;
                    case TokenType.Whitespace:
                    case TokenType.Comma:
                        parameters.Add(tag);
                        break;
                    case TokenType.RightParent:
                        parameters.Add(tag);
                        return parameters;
                    default: throw new Exception($"Expected right parent or next argument, but got {token.Type}.");
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
                    type.GenericArgs.Add(ParseType(ref context));
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
                    if (type.IsRef)
                        type.Type += '&';
                    else
                        type.IsRef = true;
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
                throw new Exception($"Failed to parse the field {entry} array size '{length}'");
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
                        case "attributes":
                            currentParam.Attributes = tag.Value;
                            break;
                        default:
                            Log.Warning($"Unknown or not supported tag parameter {tag} used on parameter at line {context.Tokenizer.CurrentLine}.");
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
                currentParam.Name = context.Tokenizer.ExpectToken(TokenType.Identifier).Value;

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

        private static ClassInfo ParseClass(ref ParsingContext context)
        {
            var desc = new ClassInfo
            {
                Children = new List<ApiTypeInfo>(),
                Access = context.CurrentAccessLevel,
                BaseTypeInheritance = AccessLevel.Private,
                Functions = new List<FunctionInfo>(),
                Properties = new List<PropertyInfo>(),
                Fields = new List<FieldInfo>(),
                Events = new List<EventInfo>(),
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'class' keyword
            var token = context.Tokenizer.NextToken();
            if (token.Value != "class")
                throw new Exception($"Invalid API_CLASS usage (expected 'class' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");

            // Read name
            desc.Name = desc.NativeName = ParseName(ref context);

            // Read class inheritance
            token = context.Tokenizer.NextToken();
            var isFinal = token.Value == "final";
            if (isFinal)
                token = context.Tokenizer.NextToken();
            if (token.Type == TokenType.Colon)
            {
                // Current class does have inheritance defined
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
                }

                desc.BaseType = new TypeInfo
                {
                    Type = token.Value,
                };
                token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.LeftAngleBracket)
                {
                    var genericType = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    context.Tokenizer.ExpectToken(TokenType.RightAngleBracket);
                    desc.BaseType.GenericArgs = new List<TypeInfo>
                    {
                        new TypeInfo
                        {
                            Type = genericType.Value,
                        }
                    };

                    // TODO: find better way to resolve this (custom base type attribute?)
                    if (desc.BaseType.Type == "ShaderAssetTypeBase")
                    {
                        desc.BaseType = desc.BaseType.GenericArgs[0];
                    }
                }
                else
                {
                    token = context.Tokenizer.PreviousToken();
                }
            }
            else
            {
                // No base type
                token = context.Tokenizer.PreviousToken();
                desc.BaseType = null;
            }

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
                default:
                    Log.Warning($"Unknown or not supported tag parameter {tag} used on class {desc.Name} at line {context.Tokenizer.CurrentLine}");
                    break;
                }
            }

            // C++ class marked as final is meant to be sealed
            if (isFinal && !desc.IsSealed && !desc.IsStatic)
                desc.IsSealed = true;

            return desc;
        }

        private static FunctionInfo ParseFunction(ref ParsingContext context)
        {
            var desc = new FunctionInfo
            {
                Access = context.CurrentAccessLevel,
                Parameters = new List<FunctionInfo.ParameterInfo>(),
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);
            context.Tokenizer.SkipUntil(TokenType.Identifier);

            // Read 'static' or 'FORCE_INLINE' or 'virtual'
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
                else
                {
                    context.Tokenizer.PreviousToken();
                    break;
                }
            }

            // Read return type
            desc.ReturnType = ParseType(ref context);

            // Read name
            desc.Name = ParseName(ref context);
            if (desc.Name == "STDCALL" || desc.Name == "CDECL")
                desc.Name = ParseName(ref context);

            // Read parameters
            desc.Parameters.AddRange(ParseFunctionParameters(ref context));

            // Read ';' or 'const' or 'override' or '= 0' or '{'
            while (true)
            {
                var token = context.Tokenizer.ExpectAnyTokens(new[] { TokenType.SemiColon, TokenType.LeftCurlyBrace, TokenType.Equal, TokenType.Identifier });
                if (token.Type == TokenType.Equal)
                {
                    context.Tokenizer.SkipUntil(TokenType.SemiColon);
                    break;
                }
                else if (token.Type == TokenType.Identifier)
                {
                    switch (token.Value)
                    {
                    case "const":
                        if (desc.IsConst)
                            throw new Exception($"Invalid double 'const' specifier in function {desc.Name} at line {context.Tokenizer.CurrentLine}.");
                        desc.IsConst = true;
                        break;
                    case "override":
                        desc.IsVirtual = true;
                        break;
                    default: throw new Exception($"Unknown identifier '{token.Value}' in function {desc.Name} at line {context.Tokenizer.CurrentLine}.");
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
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "noproxy":
                    desc.NoProxy = true;
                    break;
                default:
                    Log.Warning($"Unknown or not supported tag parameter {tag} used on function {desc.Name}");
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
                throw new Exception($"Found property {propertyName} outside the class at line {context.Tokenizer.CurrentLine}.");

            var isGetter = !functionInfo.ReturnType.IsVoid;
            if (!isGetter && functionInfo.Parameters.Count == 0)
                throw new Exception($"Property {propertyName} setter method in class {classInfo.Name} has to have value parameter to set (line {context.Tokenizer.CurrentLine}).");
            var propertyType = isGetter ? functionInfo.ReturnType : functionInfo.Parameters[0].Type;

            var propertyInfo = classInfo.Properties.FirstOrDefault(x => x.Name == propertyName);
            if (propertyInfo == null)
            {
                propertyInfo = new PropertyInfo
                {
                    Name = propertyName,
                    Comment = functionInfo.Comment,
                    IsStatic = functionInfo.IsStatic,
                    Access = functionInfo.Access,
                    Attributes = functionInfo.Attributes,
                    Type = propertyType,
                };
                classInfo.Properties.Add(propertyInfo);
            }
            else
            {
                if (propertyInfo.IsStatic != functionInfo.IsStatic)
                    throw new Exception($"Property {propertyName} in class {classInfo.Name} has to have both getter and setter methods static or non-static (line {context.Tokenizer.CurrentLine}).");
                if (propertyInfo.Access != functionInfo.Access)
                    throw new Exception($"Property {propertyName} in class {classInfo.Name} has to have both getter and setter methods with the same access level (line {context.Tokenizer.CurrentLine}).");
            }

            if (isGetter && propertyInfo.Getter != null)
                throw new Exception($"Property {propertyName} in class {classInfo.Name} cannot have multiple getter method (line {context.Tokenizer.CurrentLine}). Getter methods of properties must return value, while setters take this as a parameter.");
            if (!isGetter && propertyInfo.Setter != null)
                throw new Exception($"Property {propertyName} in class {classInfo.Name} cannot have multiple setter method (line {context.Tokenizer.CurrentLine}). Getter methods of properties must return value, while setters take this as a parameter.");

            if (isGetter)
                propertyInfo.Getter = functionInfo;
            else
                propertyInfo.Setter = functionInfo;

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
                    throw new Exception($"Property {propertyName} in class {classInfo.Name} (line {context.Tokenizer.CurrentLine}) has mismatching getter return type ({getterType}) and setter parameter type ({setterType}). Both getter and setter methods must use the same value type used for property.");
                }

                // Fix documentation comment to reflect both getter and setters available
                for (var i = 0; i < propertyInfo.Comment.Length; i++)
                {
                    ref var comment = ref propertyInfo.Comment[i];
                    comment = comment.Replace("/// Gets ", "/// Gets or sets ");
                }
            }

            return propertyInfo;
        }

        private static EnumInfo ParseEnum(ref ParsingContext context)
        {
            var desc = new EnumInfo
            {
                Children = new List<ApiTypeInfo>(),
                Access = context.CurrentAccessLevel,
                Entries = new List<EnumInfo.EntryInfo>(),
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'enum' or `enum class` keywords
            var token = context.Tokenizer.NextToken();
            if (token.Value != "enum")
                throw new Exception($"Invalid API_ENUM usage at line {context.Tokenizer.CurrentLine} (expected 'enum' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");
            token = context.Tokenizer.NextToken();
            if (token.Value != "class")
                context.Tokenizer.PreviousToken();

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
                                Log.Warning($"Unknown or not supported tag parameter {tag} used on enum {desc.Name} entry at line {context.Tokenizer.CurrentLine}");
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
                default:
                    Log.Warning($"Unknown or not supported tag parameter {tag} used on enum {desc.Name} at line {context.Tokenizer.CurrentLine}");
                    break;
                }
            }
            return desc;
        }

        private static StructureInfo ParseStructure(ref ParsingContext context)
        {
            var desc = new StructureInfo
            {
                Children = new List<ApiTypeInfo>(),
                Access = context.CurrentAccessLevel,
                Fields = new List<FieldInfo>(),
                Functions = new List<FunctionInfo>(),
            };

            // Read the documentation comment
            desc.Comment = ParseComment(ref context);

            // Read parameters from the tag
            var tagParams = ParseTagParameters(ref context);

            // Read 'struct' keyword
            var token = context.Tokenizer.NextToken();
            if (token.Value != "struct")
                throw new Exception($"Invalid API_STRUCT usage (expected 'struct' keyword but got '{token.Value} {context.Tokenizer.NextToken().Value}').");

            // Read name
            desc.Name = desc.NativeName = ParseName(ref context);

            // Read structure inheritance
            token = context.Tokenizer.NextToken();
            if (token.Type == TokenType.Colon)
            {
                // Current class does have inheritance defined
                var accessToken = context.Tokenizer.ExpectToken(TokenType.Identifier);
                switch (accessToken.Value)
                {
                case "public":
                    token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    break;
                case "protected":
                    token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    break;
                case "private":
                    token = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    break;
                default:
                    token = accessToken;
                    break;
                }

                desc.BaseType = new TypeInfo
                {
                    Type = token.Value,
                };
                token = context.Tokenizer.NextToken();
                if (token.Type == TokenType.LeftAngleBracket)
                {
                    var genericType = context.Tokenizer.ExpectToken(TokenType.Identifier);
                    context.Tokenizer.ExpectToken(TokenType.RightAngleBracket);
                    desc.BaseType.GenericArgs = new List<TypeInfo>
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
                // No base type
                token = context.Tokenizer.PreviousToken();
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
                case "inbuild":
                    desc.IsInBuild = true;
                    break;
                case "nopod":
                    desc.ForceNoPod = true;
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
                default:
                    Log.Warning($"Unknown or not supported tag parameter {tag} used on struct {desc.Name} at line {context.Tokenizer.CurrentLine}");
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

            // Read 'static' keyword
            desc.IsStatic = context.Tokenizer.NextToken().Value == "static";
            if (!desc.IsStatic)
                context.Tokenizer.PreviousToken();

            // Read type
            desc.Type = ParseType(ref context);

            // Read name
            desc.Name = ParseName(ref context);

            // Read ';' or default value or array size or bit-field size
            var token = context.Tokenizer.ExpectAnyTokens(new[] { TokenType.SemiColon, TokenType.Equal, TokenType.LeftBracket, TokenType.Colon });
            if (token.Type == TokenType.Equal)
            {
                //context.Tokenizer.SkipUntil(TokenType.SemiColon, out var defaultValue);
                context.Tokenizer.SkipUntil(TokenType.SemiColon);
            }
            else if (token.Type == TokenType.LeftBracket)
            {
                // Read the fixed array length
                ParseTypeArray(ref context, desc.Type, desc);
                context.Tokenizer.ExpectToken(TokenType.SemiColon);
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
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                case "readonly":
                    desc.IsReadOnly = true;
                    break;
                case "noarray":
                    desc.NoArray = true;
                    break;
                default:
                    Log.Warning($"Unknown or not supported tag parameter {tag} used on field {desc.Name} at line {context.Tokenizer.CurrentLine}");
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
                throw new Exception($"Invalid API_EVENT type. Only Action and Delegate<> types are supported. '{desc.Type}' used on event at line {context.Tokenizer.CurrentLine}.");

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
                case "attributes":
                    desc.Attributes = tag.Value;
                    break;
                case "name":
                    desc.Name = tag.Value;
                    break;
                default:
                    Log.Warning($"Unknown or not supported tag parameter {tag} used on event {desc.Name} at line {context.Tokenizer.CurrentLine}");
                    break;
                }
            }
            return desc;
        }

        private static InjectCppCodeInfo ParseInjectCppCode(ref ParsingContext context)
        {
            context.Tokenizer.ExpectToken(TokenType.LeftParent);
            var desc = new InjectCppCodeInfo
            {
                Children = new List<ApiTypeInfo>(),
                Code = context.Tokenizer.ExpectToken(TokenType.String).Value.Replace("\\\"", "\""),
            };
            desc.Code = desc.Code.Substring(1, desc.Code.Length - 2);
            context.Tokenizer.ExpectToken(TokenType.RightParent);
            return desc;
        }
    }
}
