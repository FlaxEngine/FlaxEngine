// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
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

            /// <summary>
            /// The custom data per-scripting language.
            /// </summary>
            public Dictionary<string, string> CustomScriptingData;
        }

        public delegate void GenerateModuleBindingsDelegate(BuildData buildData, IGrouping<string, Module> binaryModule);

        public delegate void GenerateBinaryModuleBindingsDelegate(BuildData buildData, ModuleInfo moduleInfo, ref BindingsResult bindings);

        public delegate void ParseTypeTagDelegate(ref bool valid, TagParameter tag, ApiTypeInfo typeInfo);

        public delegate void ParseMemberTagDelegate(ref bool valid, TagParameter tag, MemberInfo memberInfo);

        public delegate void ParseFunctionParameterTagDelegate(ref bool valid, TagParameter tag, ref FunctionInfo.ParameterInfo parameterInfo);

        public static event GenerateModuleBindingsDelegate GenerateModuleBindings;
        public static event GenerateBinaryModuleBindingsDelegate GenerateBinaryModuleBindings;
        public static event ParseTypeTagDelegate ParseTypeTag;
        public static event ParseMemberTagDelegate ParseMemberTag;
        public static event ParseFunctionParameterTagDelegate ParseFunctionParameterTag;
        public static ModuleInfo CurrentModule;

        private static List<StringBuilder> _strignBuilderCache;

        public static StringBuilder GetStringBuilder()
        {
            if (_strignBuilderCache == null || _strignBuilderCache.Count == 0)
                return new StringBuilder();
            var idx = _strignBuilderCache.Count - 1;
            var result = _strignBuilderCache[idx];
            _strignBuilderCache.RemoveAt(idx);
            result.Clear();
            return result;
        }

        public static void PutStringBuilder(StringBuilder value)
        {
            if (_strignBuilderCache == null)
                _strignBuilderCache = new List<StringBuilder>();
            _strignBuilderCache.Add(value);
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
            // Setup bindings module info descriptor
            var moduleInfo = new ModuleInfo
            {
                Module = module,
                Name = module.BinaryModuleName,
                Namespace = string.Empty,
            };
            if (string.IsNullOrEmpty(moduleInfo.Name))
                throw new Exception("Module name cannot be empty.");
            if (module.Tags != null && module.Tags.Count != 0)
                moduleInfo.Tags = new Dictionary<string, string>(module.Tags);
            buildData.ModulesInfo.Add(module, moduleInfo);

            // Skip for modules that cannot have API bindings
            if (!module.BuildCSharp || !module.BuildNativeCode)
                return moduleInfo;

            if (moduleOptions == null)
                throw new Exception($"Cannot parse module {module.Name} without options.");

            // Collect all header files that can have public symbols for API
            var headerFiles = new List<string>(moduleOptions.SourceFiles.Count / 2);
            for (int i = 0; i < moduleOptions.SourceFiles.Count; i++)
            {
                if (moduleOptions.SourceFiles[i].EndsWith(".h", StringComparison.OrdinalIgnoreCase))
                    headerFiles.Add(moduleOptions.SourceFiles[i]);
            }
            if (headerFiles.Count == 0)
                return moduleInfo;
            if (module.Name == "Core")
            {
                // Special case for Core module to ignore API tags defines
                for (int i = 0; i < headerFiles.Count; i++)
                {
                    if (headerFiles[i].EndsWith("Config.h", StringComparison.Ordinal))
                    {
                        headerFiles.RemoveAt(i);
                        break;
                    }
                }
            }

            // Sort file paths to have stable results
            headerFiles.Sort();

            // Load cache
            using (new ProfileEventScope("LoadCache"))
            {
                if (LoadCache(ref moduleInfo, moduleOptions, headerFiles))
                {
                    buildData.ModulesInfo[module] = moduleInfo;
                    return moduleInfo;
                }
            }

            // Parse bindings
            Log.Verbose($"Parsing API bindings for {module.Name} ({moduleInfo.Name})");
            int concurrency = Math.Min(Math.Max(1, (int)(Environment.ProcessorCount * Configuration.ConcurrencyProcessorScale)), Configuration.MaxConcurrency);
            concurrency = 1; // Disable concurrency for parsing (the gain is unnoticeable or even worse in some cases)
            if (concurrency == 1 || headerFiles.Count < 2 * concurrency)
            {
                // Single-threaded
                for (int i = 0; i < headerFiles.Count; i++)
                {
                    using (new ProfileEventScope(Path.GetFileName(headerFiles[i])))
                    {
                        ParseModuleInnerAsync(moduleInfo, moduleOptions, headerFiles, i);
                    }
                }
            }
            else
            {
                // Multi-threaded
                ThreadPool.GetMinThreads(out var workerThreads, out var completionPortThreads);
                if (workerThreads != concurrency)
                    ThreadPool.SetMaxThreads(concurrency, completionPortThreads);
                Parallel.For(0, headerFiles.Count, (i, state) =>
                {
                    using (new ProfileEventScope(Path.GetFileName(headerFiles[i])))
                    {
                        ParseModuleInnerAsync(moduleInfo, moduleOptions, headerFiles, i);
                    }
                });
            }

            // Save cache
            using (new ProfileEventScope("SaveCache"))
            {
                try
                {
                    SaveCache(moduleInfo, moduleOptions, headerFiles);
                }
                catch (Exception ex)
                {
                    Log.Error($"Failed to save API cache for module {moduleInfo.Module.Name}");
                    Log.Exception(ex);
                }
            }

            return moduleInfo;
        }

        private static void ParseModuleInnerAsync(ModuleInfo moduleInfo, BuildOptions moduleOptions, List<string> headerFiles, int workIndex)
        {
            // Find and load files with API tags
            bool hasApi = false;
            string headerFileContents = File.ReadAllText(headerFiles[workIndex]);
            for (int j = 0; j < ApiTokens.SearchTags.Length; j++)
            {
                if (headerFileContents.Contains(ApiTokens.SearchTags[j]))
                {
                    hasApi = true;
                    break;
                }
            }
            if (!hasApi)
                return;

            // Process header file to generate the module API code reflection
            var fileInfo = new FileInfo
            {
                Parent = null,
                Name = headerFiles[workIndex],
                Namespace = moduleInfo.Name,
            };
            lock (moduleInfo)
            {
                moduleInfo.AddChild(fileInfo);
            }

            try
            {
                // Tokenize the source
                var tokenizer = new Tokenizer();
                tokenizer.Tokenize(headerFileContents);

                // Init the context
                var context = new ParsingContext
                {
                    File = fileInfo,
                    Tokenizer = tokenizer,
                    ScopeInfo = null,
                    ModuleOptions = moduleOptions,
                    CurrentAccessLevel = AccessLevel.Public,
                    ScopeTypeStack = new Stack<ApiTypeInfo>(),
                    ScopeAccessStack = new Stack<AccessLevel>(),
                    PreprocessorDefines = new Dictionary<string, string>(),
                };
                context.PreprocessorDefines.Add("FLAX_BUILD_BINDINGS", "1");
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
                            else if (context.ScopeInfo is InterfaceInfo interfaceInfo)
                                interfaceInfo.Functions.Add(functionInfo);
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
                        else if (string.Equals(token.Value, ApiTokens.Typedef, StringComparison.Ordinal))
                        {
                            var typeInfo = ParseTypedef(ref context);
                            fileInfo.AddChild(typeInfo);
                        }
                        else if (string.Equals(token.Value, ApiTokens.InjectCode, StringComparison.Ordinal))
                        {
                            var injectCodeInfo = ParseInjectCode(ref context);
                            fileInfo.AddChild(injectCodeInfo);
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
                        OnPreProcessorToken(ref context, ref token);
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

        private static void OnPreProcessorToken(ref ParsingContext context, ref Token token)
        {
            var tokenizer = context.Tokenizer;
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
                case "elif":
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
                        var negate = tokenValue[0] == '!';
                        if (negate)
                            tokenValue = tokenValue.Substring(1);
                        tokenValue = ReplacePreProcessorDefines(tokenValue, context.PreprocessorDefines);
                        tokenValue = ReplacePreProcessorDefines(tokenValue, context.ModuleOptions.PublicDefinitions);
                        tokenValue = ReplacePreProcessorDefines(tokenValue, context.ModuleOptions.PrivateDefinitions);
                        tokenValue = ReplacePreProcessorDefines(tokenValue, context.ModuleOptions.CompileEnv.PreprocessorDefinitions);
                        tokenValue = tokenValue.Replace("false", "0");
                        tokenValue = tokenValue.Replace("true", "1");
                        tokenValue = tokenValue.Replace("||", "|");
                        if (tokenValue.Length != 0 && tokenValue != "1" && tokenValue != "0" && tokenValue != "|")
                            tokenValue = "0";
                        if (negate)
                            tokenValue = tokenValue == "1" ? "0" : "1";

                        condition += tokenValue;
                        token = tokenizer.NextToken(true);
                    }

                    // Filter condition
                    bool modified;
                    do
                    {
                        modified = false;
                        if (condition.Contains("1|1"))
                        {
                            condition = condition.Replace("1|1", "1");
                            modified = true;
                        }
                        if (condition.Contains("1|0"))
                        {
                            condition = condition.Replace("1|0", "1");
                            modified = true;
                        }
                        if (condition.Contains("0|1"))
                        {
                            condition = condition.Replace("0|1", "1");
                            modified = true;
                        }
                    } while (modified);

                    // Skip chunk of code of condition fails
                    if (condition != "1")
                    {
                        ParsePreprocessorIf(context.File, tokenizer, ref token);
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
                    if (!context.PreprocessorDefines.ContainsKey(define) && !context.ModuleOptions.CompileEnv.PreprocessorDefinitions.Contains(define))
                    {
                        ParsePreprocessorIf(context.File, tokenizer, ref token);
                    }

                    break;
                }
                case "endif":
                {
                    token = tokenizer.NextToken(true);
                    break;
                }
            }
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

        private static string ReplacePreProcessorDefines(string text, IDictionary<string, string> defines)
        {
            foreach (var e in defines)
            {
                if (string.Equals(text, e.Key, StringComparison.Ordinal))
                    text = text.Replace(e.Key, string.IsNullOrEmpty(e.Value) ? "1" : e.Value);
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
                    case "else":
                        if (ifsCount == 1)
                        {
                            // Continue with `else` block
                            return;
                        }
                        break;
                    case "elif":
                        if (ifsCount == 1)
                        {
                            // Rollback to process conditional block
                            tokenizer.PreviousToken();
                            tokenizer.PreviousToken();
                            return;
                        }
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

        private static bool UseBindings(object type)
        {
            var apiTypeInfo = type as ApiTypeInfo;
            if (apiTypeInfo != null && apiTypeInfo.SkipGeneration)
                return false;
            if ((type is ModuleInfo || type is FileInfo) && apiTypeInfo != null)
            {
                foreach (var child in apiTypeInfo.Children)
                {
                    if (UseBindings(child))
                        return true;
                }
            }
            return type is ClassInfo ||
                   type is StructureInfo ||
                   type is InterfaceInfo ||
                   type is InjectCodeInfo;
        }

        /// <summary>
        /// The API bindings generation utility that can produce scripting bindings for another languages to the native code.
        /// </summary>
        public static void GenerateBindings(BuildData buildData, Module module, ref BuildOptions moduleOptions, out BindingsResult bindings)
        {
            // Parse module (or load from cache)
            var moduleInfo = ParseModule(buildData, module, moduleOptions);
            bindings = new BindingsResult
            {
                UseBindings = UseBindings(moduleInfo),
                GeneratedCppFilePath = Path.Combine(moduleOptions.IntermediateFolder, module.Name + ".Bindings.Gen.cpp"),
                GeneratedCSharpFilePath = Path.Combine(moduleOptions.IntermediateFolder, module.Name + ".Bindings.Gen.cs"),
            };

            if (bindings.UseBindings)
            {
                buildData.Modules.TryGetValue(moduleInfo.Module, out var moduleBuildInfo);

                // Ensure that generated files are included into build
                if (EngineConfiguration.WithCSharp(buildData.TargetOptions) && !moduleBuildInfo.SourceFiles.Contains(bindings.GeneratedCSharpFilePath))
                    moduleBuildInfo.SourceFiles.Add(bindings.GeneratedCSharpFilePath);
            }

            // Skip if module is cached (no scripting API changed)
            if (moduleInfo.IsFromCache)
                return;

            // Initialize parsed API
            using (new ProfileEventScope("Init"))
            {
                moduleInfo.Init(buildData);
            }

            // Generate bindings for scripting
            if (bindings.UseBindings)
            {
                Log.Verbose($"Generating API bindings for {module.Name} ({moduleInfo.Name})");
                using (new ProfileEventScope("Cpp"))
                    GenerateCpp(buildData, moduleInfo, ref bindings);
                if (EngineConfiguration.WithCSharp(buildData.TargetOptions))
                {
                    using (new ProfileEventScope("CSharp"))
                        GenerateCSharp(buildData, moduleInfo, ref bindings);
                }
                GenerateBinaryModuleBindings?.Invoke(buildData, moduleInfo, ref bindings);
            }
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
                    if (EngineConfiguration.WithCSharp(buildData.TargetOptions))
                    {
                        GenerateCSharp(buildData, binaryModule);
                    }
                    GenerateModuleBindings?.Invoke(buildData, binaryModule);
                }
            }
        }
    }
}
