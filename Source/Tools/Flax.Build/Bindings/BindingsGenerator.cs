// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.NativeCpp;
using BuildData = Flax.Build.Builder.BuildData;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The API bindings generation utility that can produce scripting bindings for another languages to the native code.
    /// </summary>
    public static partial class BindingsGenerator
    {
        /// <summary>
        /// The bindings generation result.
        /// </summary>
        public struct BindingsResult
        {
            /// <summary>
            /// True if module uses bindings, otherwise false.
            /// </summary>
            public bool UseBindings;

            /// <summary>
            /// The generated C++ file path that contains a API bindings wrapper code to setup the glue code.
            /// </summary>
            public string GeneratedCppFilePath;

            /// <summary>
            /// The generated C# file path that contains a API bindings wrapper code to setup the glue code.
            /// </summary>
            public string GeneratedCSharpFilePath;
        }

        public static ModuleInfo ParseModule(BuildData buildData, Module module, BuildOptions moduleOptions = null)
        {
            if (buildData.ModulesInfo.TryGetValue(module, out var moduleInfo))
                return moduleInfo;

            // Try to reuse already parsed bindings info from one of the referenced projects
            foreach (var referenceBuild in buildData.ReferenceBuilds)
            {
                if (referenceBuild.Value.ModulesInfo.TryGetValue(module, out moduleInfo))
                {
                    buildData.ModulesInfo.Add(module, moduleInfo);
                    return moduleInfo;
                }
            }

            using (new ProfileEventScope("ParseModule" + module.Name))
            {
                return ParseModuleInner(buildData, module, moduleOptions);
            }
        }

        private static ModuleInfo ParseModuleInner(BuildData buildData, Module module, BuildOptions moduleOptions = null)
        {
            if (buildData.ModulesInfo.TryGetValue(module, out var moduleInfo))
                return moduleInfo;

            // Setup bindings module info descriptor
            moduleInfo = new ModuleInfo
            {
                Module = module,
                Name = module.BinaryModuleName,
                Namespace = string.Empty,
                Children = new List<ApiTypeInfo>(),
            };
            if (string.IsNullOrEmpty(moduleInfo.Name))
                throw new Exception("Module name cannot be empty.");
            buildData.ModulesInfo.Add(module, moduleInfo);

            // Skip for modules that cannot have API bindings
            if (module is ThirdPartyModule || !module.BuildNativeCode)
                return moduleInfo;

            if (moduleOptions == null)
                throw new Exception($"Cannot parse module {module.Name} without options.");

            // Collect all header files that can have public symbols for API
            var headerFiles = moduleOptions.SourceFiles.Where(x => x.EndsWith(".h")).ToList();

            // Skip if no header files to process
            if (headerFiles.Count == 0)
                return moduleInfo;

            // TODO: use aynsc tasks or thread pool to load and parse multiple header files at once using multi-threading

            // Find and load files with API tags
            string[] headerFilesContents = null;
            //using (new ProfileEventScope("LoadHeaderFiles"))
            {
                var anyApi = false;
                for (int i = 0; i < headerFiles.Count; i++)
                {
                    // Skip scripting tags definitions file
                    if (headerFiles[i].Replace('\\', '/').EndsWith("Engine/Core/Config.h", StringComparison.Ordinal))
                        continue;

                    // Check if file contains any valid API tag
                    var contents = File.ReadAllText(headerFiles[i]);
                    for (int j = 0; j < ApiTokens.SearchTags.Length; j++)
                    {
                        if (contents.Contains(ApiTokens.SearchTags[j]))
                        {
                            if (headerFilesContents == null)
                                headerFilesContents = new string[headerFiles.Count];
                            headerFilesContents[i] = contents;
                            anyApi = true;
                            break;
                        }
                    }
                }

                if (!anyApi)
                    return moduleInfo;
            }

            // Skip if none of the headers was modified after last time generated C++ file was edited
            // TODO: skip parsing if module has API cache file -> then load it from disk
            /*if (!forceGenerate)
            {
                var lastGenerateTime = File.GetLastWriteTime(bindings.GeneratedCppFilePath);
                var anyModified = false;
                for (int i = 0; i < headerFiles.Count; i++)
                {
                    if (File.GetLastWriteTime(headerFiles[i]) > lastGenerateTime)
                    {
                        anyModified = true;
                        break;
                    }
                }

                if (!anyModified)
                    return;
            }*/

            Log.Verbose($"Parsing API bindings for {module.Name} ({moduleInfo.Name})");

            // Process all header files to generate the module API code reflection
            var context = new ParsingContext
            {
                CurrentAccessLevel = AccessLevel.Public,
                ScopeTypeStack = new Stack<ApiTypeInfo>(),
                ScopeAccessStack = new Stack<AccessLevel>(),
                PreprocessorDefines = new Dictionary<string, string>(),
            };
            for (int i = 0; i < headerFiles.Count; i++)
            {
                if (headerFilesContents[i] == null)
                    continue;
                var fileInfo = new FileInfo
                {
                    Parent = null,
                    Children = new List<ApiTypeInfo>(),
                    Name = headerFiles[i],
                    Namespace = moduleInfo.Name,
                };
                moduleInfo.AddChild(fileInfo);

                try
                {
                    // Tokenize the source
                    var tokenizer = new Tokenizer();
                    tokenizer.Tokenize(headerFilesContents[i]);

                    // Init the context
                    context.Tokenizer = tokenizer;
                    context.File = fileInfo;
                    context.ScopeInfo = null;
                    context.ScopeTypeStack.Clear();
                    context.ScopeAccessStack.Clear();
                    context.PreprocessorDefines.Clear();
                    context.EnterScope(fileInfo);

                    // Process the source code
                    ApiTypeInfo scopeType = null;
                    Token prevToken = null;
                    while (true)
                    {
                        // Move to the next token
                        var token = tokenizer.NextToken();
                        if (token == null)
                            continue;
                        if (token.Type == TokenType.EndOfFile)
                            break;

                        // Parse API_.. tags in source code
                        if (token.Type == TokenType.Identifier && token.Value.StartsWith("API_", StringComparison.Ordinal))
                        {
                            if (string.Equals(token.Value, ApiTokens.Class, StringComparison.Ordinal))
                            {
                                if (!(context.ScopeInfo is FileInfo))
                                    throw new NotImplementedException("TODO: add support for nested classes in scripting API");

                                var classInfo = ParseClass(ref context);
                                scopeType = classInfo;
                                context.ScopeInfo.AddChild(scopeType);
                                context.CurrentAccessLevel = AccessLevel.Public;
                            }
                            else if (string.Equals(token.Value, ApiTokens.Property, StringComparison.Ordinal))
                            {
                                var propertyInfo = ParseProperty(ref context);
                            }
                            else if (string.Equals(token.Value, ApiTokens.Function, StringComparison.Ordinal))
                            {
                                var functionInfo = ParseFunction(ref context);

                                if (context.ScopeInfo is ClassInfo classInfo)
                                    classInfo.Functions.Add(functionInfo);
                                else if (context.ScopeInfo is StructureInfo structureInfo)
                                    structureInfo.Functions.Add(functionInfo);
                                else
                                    throw new Exception($"Not supported free-function {functionInfo.Name} at line {tokenizer.CurrentLine}. Place it in the class to use API bindings for it.");
                            }
                            else if (string.Equals(token.Value, ApiTokens.Enum, StringComparison.Ordinal))
                            {
                                var enumInfo = ParseEnum(ref context);
                                context.ScopeInfo.AddChild(enumInfo);
                            }
                            else if (string.Equals(token.Value, ApiTokens.Struct, StringComparison.Ordinal))
                            {
                                var structureInfo = ParseStructure(ref context);
                                scopeType = structureInfo;
                                context.ScopeInfo.AddChild(scopeType);
                                context.CurrentAccessLevel = AccessLevel.Public;
                            }
                            else if (string.Equals(token.Value, ApiTokens.Field, StringComparison.Ordinal))
                            {
                                var fieldInfo = ParseField(ref context);
                                var scopeInfo = context.ValidScopeInfoFromStack;

                                if (scopeInfo is ClassInfo classInfo)
                                    classInfo.Fields.Add(fieldInfo);
                                else if (scopeInfo is StructureInfo structureInfo)
                                    structureInfo.Fields.Add(fieldInfo);
                                else
                                    throw new Exception($"Not supported location for field {fieldInfo.Name} at line {tokenizer.CurrentLine}. Place it in the class or structure to use API bindings for it.");
                            }
                            else if (string.Equals(token.Value, ApiTokens.Event, StringComparison.Ordinal))
                            {
                                var eventInfo = ParseEvent(ref context);
                                var scopeInfo = context.ValidScopeInfoFromStack;

                                if (scopeInfo is ClassInfo classInfo)
                                    classInfo.Events.Add(eventInfo);
                                else
                                    throw new Exception($"Not supported location for event {eventInfo.Name} at line {tokenizer.CurrentLine}. Place it in the class to use API bindings for it.");
                            }
                            else if (string.Equals(token.Value, ApiTokens.InjectCppCode, StringComparison.Ordinal))
                            {
                                var injectCppCodeInfo = ParseInjectCppCode(ref context);
                                fileInfo.AddChild(injectCppCodeInfo);
                            }
                            else if (string.Equals(token.Value, ApiTokens.Interface, StringComparison.Ordinal))
                            {
                                if (!(context.ScopeInfo is FileInfo))
                                    throw new NotImplementedException("TODO: add support for nested interfaces in scripting API");

                                var interfaceInfo = ParseInterface(ref context);
                                scopeType = interfaceInfo;
                                context.ScopeInfo.AddChild(scopeType);
                                context.CurrentAccessLevel = AccessLevel.Public;
                            }
                            else if (string.Equals(token.Value, ApiTokens.AutoSerialization, StringComparison.Ordinal))
                            {
                                if (context.ScopeInfo is ClassInfo classInfo)
                                    classInfo.IsAutoSerialization = true;
                                else if (context.ScopeInfo is StructureInfo structureInfo)
                                    structureInfo.IsAutoSerialization = true;
                                else
                                    throw new Exception($"Not supported location for {ApiTokens.AutoSerialization} at line {tokenizer.CurrentLine}. Place it in the class or structure that uses API bindings.");
                            }
                        }

                        // Track access level inside class
                        if (context.ScopeInfo != null && token.Type == TokenType.Colon && prevToken != null && prevToken.Type == TokenType.Identifier)
                        {
                            if (string.Equals(prevToken.Value, "public", StringComparison.Ordinal))
                            {
                                context.CurrentAccessLevel = AccessLevel.Public;
                            }
                            else if (string.Equals(prevToken.Value, "protected", StringComparison.Ordinal))
                            {
                                context.CurrentAccessLevel = AccessLevel.Protected;
                            }
                            else if (string.Equals(prevToken.Value, "private", StringComparison.Ordinal))
                            {
                                context.CurrentAccessLevel = AccessLevel.Private;
                            }
                        }

                        // Handle preprocessor blocks
                        if (token.Type == TokenType.Preprocessor)
                        {
                            token = tokenizer.NextToken();
                            switch (token.Value)
                            {
                            case "define":
                            {
                                token = tokenizer.NextToken();
                                var name = token.Value;
                                var value = string.Empty;
                                token = tokenizer.NextToken(true);
                                while (token.Type != TokenType.Newline)
                                {
                                    value += token.Value;
                                    token = tokenizer.NextToken(true);
                                }
                                value = value.Trim();
                                context.PreprocessorDefines[name] = value;
                                break;
                            }
                            case "if":
                            {
                                // Parse condition
                                var condition = string.Empty;
                                token = tokenizer.NextToken(true);
                                while (token.Type != TokenType.Newline)
                                {
                                    var tokenValue = token.Value.Trim();
                                    if (tokenValue.Length == 0)
                                    {
                                        token = tokenizer.NextToken(true);
                                        continue;
                                    }

                                    // Very simple defines processing
                                    tokenValue = ReplacePreProcessorDefines(tokenValue, context.PreprocessorDefines.Keys);
                                    tokenValue = ReplacePreProcessorDefines(tokenValue, moduleOptions.PublicDefinitions);
                                    tokenValue = ReplacePreProcessorDefines(tokenValue, moduleOptions.PrivateDefinitions);
                                    tokenValue = ReplacePreProcessorDefines(tokenValue, moduleOptions.CompileEnv.PreprocessorDefinitions);
                                    tokenValue = tokenValue.Replace("false", "0");
                                    tokenValue = tokenValue.Replace("true", "1");
                                    tokenValue = tokenValue.Replace("||", "|");
                                    if (tokenValue.Length != 0 && tokenValue != "1" && tokenValue != "0" && tokenValue != "|")
                                        tokenValue = "0";

                                    condition += tokenValue;
                                    token = tokenizer.NextToken(true);
                                }

                                // Filter condition
                                condition = condition.Replace("1|1", "1");
                                condition = condition.Replace("1|0", "1");
                                condition = condition.Replace("0|1", "1");

                                // Skip chunk of code of condition fails
                                if (condition != "1")
                                {
                                    ParsePreprocessorIf(fileInfo, tokenizer, ref token);
                                }

                                break;
                            }
                            case "ifdef":
                            {
                                // Parse condition
                                var define = string.Empty;
                                token = tokenizer.NextToken(true);
                                while (token.Type != TokenType.Newline)
                                {
                                    define += token.Value;
                                    token = tokenizer.NextToken(true);
                                }

                                // Check condition
                                define = define.Trim();
                                if (!context.PreprocessorDefines.ContainsKey(define) && !moduleOptions.CompileEnv.PreprocessorDefinitions.Contains(define))
                                {
                                    ParsePreprocessorIf(fileInfo, tokenizer, ref token);
                                }

                                break;
                            }
                            }
                        }

                        // Scope tracking
                        if (token.Type == TokenType.LeftCurlyBrace)
                        {
                            context.ScopeTypeStack.Push(scopeType);
                            context.ScopeInfo = context.ScopeTypeStack.Peek();
                            scopeType = null;
                        }
                        else if (token.Type == TokenType.RightCurlyBrace)
                        {
                            context.ScopeTypeStack.Pop();
                            if (context.ScopeTypeStack.Count == 0)
                                throw new Exception($"Mismatch of the {{}} braces pair in file '{fileInfo.Name}' at line {tokenizer.CurrentLine}.");
                            context.ScopeInfo = context.ScopeTypeStack.Peek();
                            if (context.ScopeInfo is FileInfo)
                                context.CurrentAccessLevel = AccessLevel.Public;
                        }

                        prevToken = token;
                    }
                }
                catch (Exception ex)
                {
                    Log.Error($"Failed to parse '{fileInfo.Name}' file to generate bindings.");
                    Log.Exception(ex);
                    throw;
                }
            }

            // Initialize API
            moduleInfo.Init(buildData);

            return moduleInfo;
        }

        private static string ReplacePreProcessorDefines(string text, IEnumerable<string> defines)
        {
            foreach (var define in defines)
            {
                if (string.Equals(text, define, StringComparison.Ordinal))
                    text = text.Replace(define, "1");
            }
            return text;
        }

        private static void ParsePreprocessorIf(FileInfo fileInfo, Tokenizer tokenizer, ref Token token)
        {
            int ifsCount = 1;
            while (token.Type != TokenType.EndOfFile && ifsCount != 0)
            {
                token = tokenizer.NextToken();
                if (token.Type == TokenType.Preprocessor)
                {
                    token = tokenizer.NextToken();
                    switch (token.Value)
                    {
                    case "if":
                    case "ifdef":
                        ifsCount++;
                        break;
                    case "endif":
                        ifsCount--;
                        break;
                    }
                }
            }

            if (ifsCount != 0)
                throw new Exception($"Invalid #if/endif pairing in file '{fileInfo.Name}'. Failed to generate API bindings for it.");
        }

        /// <summary>
        /// The API bindings generation utility that can produce scripting bindings for another languages to the native code.
        /// </summary>
        public static void GenerateBindings(BuildData buildData, Module module, ref BuildOptions moduleOptions, out BindingsResult bindings)
        {
            // Parse module (or load from cache)
            bindings = new BindingsResult
            {
                GeneratedCppFilePath = Path.Combine(moduleOptions.IntermediateFolder, module.Name + ".Bindings.Gen.cpp"),
                GeneratedCSharpFilePath = Path.Combine(moduleOptions.IntermediateFolder, module.Name + ".Bindings.Gen.cs"),
            };
            var moduleInfo = ParseModule(buildData, module, moduleOptions);

            // Process parsed API
            foreach (var child in moduleInfo.Children)
            {
                try
                {
                    foreach (var apiTypeInfo in child.Children)
                        ProcessAndValidate(buildData, apiTypeInfo);
                }
                catch (Exception)
                {
                    if (child is FileInfo fileInfo)
                        Log.Error($"Failed to validate '{fileInfo.Name}' file to generate bindings.");
                    throw;
                }
            }

            // Generate bindings for scripting
            Log.Verbose($"Generating API bindings for {module.Name} ({moduleInfo.Name})");
            GenerateCpp(buildData, moduleInfo, ref bindings);
            GenerateCSharp(buildData, moduleInfo, ref bindings);

            // TODO: add support for extending this code and support generating bindings for other scripting languages
        }

        /// <summary>
        /// The API bindings generation utility that can produce scripting bindings for another languages to the native code.
        /// </summary>
        public static void GenerateBindings(BuildData buildData)
        {
            foreach (var binaryModule in buildData.BinaryModules)
            {
                using (new ProfileEventScope(binaryModule.Key))
                {
                    // Generate bindings for binary module
                    Log.Verbose($"Generating binary module bindings for {binaryModule.Key}");
                    GenerateCpp(buildData, binaryModule);
                    GenerateCSharp(buildData, binaryModule);

                    // TODO: add support for extending this code and support generating bindings for other scripting languages
                }
            }
        }

        private static void ProcessAndValidate(BuildData buildData, ApiTypeInfo apiTypeInfo)
        {
            if (apiTypeInfo is ClassInfo classInfo)
                ProcessAndValidate(buildData, classInfo);
            else if (apiTypeInfo is StructureInfo structureInfo)
                ProcessAndValidate(buildData, structureInfo);

            foreach (var child in apiTypeInfo.Children)
                ProcessAndValidate(buildData, child);
        }

        private static void ProcessAndValidate(BuildData buildData, ClassInfo classInfo)
        {
            if (classInfo.UniqueFunctionNames == null)
                classInfo.UniqueFunctionNames = new HashSet<string>();

            foreach (var fieldInfo in classInfo.Fields)
            {
                if (fieldInfo.Access == AccessLevel.Private)
                    continue;

                fieldInfo.Getter = new FunctionInfo
                {
                    Name = "Get" + fieldInfo.Name,
                    Comment = fieldInfo.Comment,
                    IsStatic = fieldInfo.IsStatic,
                    Access = fieldInfo.Access,
                    Attributes = fieldInfo.Attributes,
                    ReturnType = fieldInfo.Type,
                    Parameters = new List<FunctionInfo.ParameterInfo>(),
                    IsVirtual = false,
                    IsConst = true,
                    Glue = new FunctionInfo.GlueInfo()
                };
                ProcessAndValidate(classInfo, fieldInfo.Getter);
                fieldInfo.Getter.Name = fieldInfo.Name;

                if (!fieldInfo.IsReadOnly)
                {
                    if (fieldInfo.Type.IsArray)
                        throw new NotImplementedException("Use ReadOnly on field. TODO: add support for setter for fixed-array fields.");

                    fieldInfo.Setter = new FunctionInfo
                    {
                        Name = "Set" + fieldInfo.Name,
                        Comment = fieldInfo.Comment,
                        IsStatic = fieldInfo.IsStatic,
                        Access = fieldInfo.Access,
                        Attributes = fieldInfo.Attributes,
                        ReturnType = new TypeInfo
                        {
                            Type = "void",
                        },
                        Parameters = new List<FunctionInfo.ParameterInfo>
                        {
                            new FunctionInfo.ParameterInfo
                            {
                                Name = "value",
                                Type = fieldInfo.Type,
                            },
                        },
                        IsVirtual = false,
                        IsConst = true,
                        Glue = new FunctionInfo.GlueInfo()
                    };
                    ProcessAndValidate(classInfo, fieldInfo.Setter);
                    fieldInfo.Setter.Name = fieldInfo.Name;
                }
            }

            foreach (var propertyInfo in classInfo.Properties)
            {
                if (propertyInfo.Getter != null)
                    ProcessAndValidate(classInfo, propertyInfo.Getter);
                if (propertyInfo.Setter != null)
                    ProcessAndValidate(classInfo, propertyInfo.Setter);
            }

            foreach (var functionInfo in classInfo.Functions)
                ProcessAndValidate(classInfo, functionInfo);
        }

        private static void ProcessAndValidate(BuildData buildData, StructureInfo structureInfo)
        {
            foreach (var fieldInfo in structureInfo.Fields)
            {
                // TODO: support bit-fields in structure fields
                if (fieldInfo.Type.IsBitField)
                    throw new NotImplementedException($"TODO: support bit-fields in structure fields (found field {fieldInfo} in structure {structureInfo.Name})");

                // Pointers are fine
                if (fieldInfo.Type.IsPtr)
                    continue;

                // In-build types
                if (CSharpNativeToManagedBasicTypes.ContainsKey(fieldInfo.Type.Type))
                    continue;
                if (CSharpNativeToManagedDefault.ContainsKey(fieldInfo.Type.Type))
                    continue;

                // Find API type info for this field type
                var apiType = FindApiTypeInfo(buildData, fieldInfo.Type, structureInfo);
                if (apiType != null)
                    continue;

                throw new Exception($"Unknown field type '{fieldInfo.Type} {fieldInfo.Name}' in structure '{structureInfo.Name}'.");
            }
        }

        private static void ProcessAndValidate(ClassInfo classInfo, FunctionInfo functionInfo)
        {
            // Ensure that methods have unique names for bindings
            if (classInfo.UniqueFunctionNames == null)
                classInfo.UniqueFunctionNames = new HashSet<string>();
            int idx = 1;
            functionInfo.UniqueName = functionInfo.Name;
            while (classInfo.UniqueFunctionNames.Contains(functionInfo.UniqueName))
                functionInfo.UniqueName = functionInfo.Name + idx++;
            classInfo.UniqueFunctionNames.Add(functionInfo.UniqueName);
        }
    }
}
