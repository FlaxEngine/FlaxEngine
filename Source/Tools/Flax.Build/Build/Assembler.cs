// Copyright (c) 2012-2023 Flax Engine. All rights reserved.

using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.Loader;
using System.Collections.Generic;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Emit;
using Microsoft.CodeAnalysis.Text;

namespace Flax.Build
{
    /// <summary>
    /// Utility for building C# assemblies from custom set of source files.
    /// </summary>
    public class Assembler
    {
        private string _cacheAssemblyPath = null;
        private string _cachePath = null;

        /// <summary>
        /// The default assembly references added to the projects.
        /// </summary>
        public static readonly Assembly[] DefaultReferences =
        {
            typeof(IntPtr).Assembly, // mscorlib.dll
            typeof(Enumerable).Assembly, // System.Linq.dll
            typeof(ISet<>).Assembly, // System.dll
            typeof(Builder).Assembly, // Flax.Build.exe
        };

        /// <summary>
        /// The source files for compilation.
        /// </summary>
        public readonly List<string> SourceFiles = new List<string>();

        /// <summary>
        /// The external user assembly references to use while compiling
        /// </summary>
        public readonly List<Assembly> Assemblies = new List<Assembly>();

        /// <summary>
        /// The external user assembly file names to use while compiling
        /// </summary>
        public readonly List<string> References = new List<string>();

        public Assembler(List<string> sourceFiles, string cachePath = null)
        {
            SourceFiles.AddRange(sourceFiles);
            _cachePath = cachePath;
            _cacheAssemblyPath = cachePath != null ? Path.Combine(Directory.GetParent(cachePath).FullName, "BuilderRulesCache.dll") : null;
        }

        /// <summary>
        /// Builds the assembly.
        /// </summary>
        /// <remarks>Throws an exception in case of any errors.</remarks>
        /// <returns>The created and loaded assembly.</returns>
        public Assembly Build()
        {
            DateTime recentWriteTime = DateTime.MinValue;
            if (_cachePath != null)
            {
                foreach (var sourceFile in SourceFiles)
                {
                    // FIXME: compare and cache individual write times!
                    DateTime lastWriteTime = File.GetLastWriteTime(sourceFile);
                    if (lastWriteTime > recentWriteTime)
                        recentWriteTime = lastWriteTime;
                }

                // Include build tool version (eg. skip using cached assembly after editing build tool)
                {
                    var executingAssembly = Assembly.GetExecutingAssembly();
                    DateTime lastWriteTime = File.GetLastWriteTime(executingAssembly.Location);
                    if (lastWriteTime > recentWriteTime)
                        recentWriteTime = lastWriteTime;
                }
                
                // Skip when project references were changed
                if (Globals.Project != null)
                {
                    DateTime lastWriteTime = File.GetLastWriteTime(Globals.Project.ProjectPath);
                    if (lastWriteTime > recentWriteTime)
                        recentWriteTime = lastWriteTime;
                }

                DateTime cacheTime = File.Exists(_cachePath)
                    ? DateTime.FromBinary(long.Parse(File.ReadAllText(_cachePath)))
                    : DateTime.MinValue;
                if (recentWriteTime <= cacheTime && File.Exists(_cacheAssemblyPath))
                {
                    Assembly cachedAssembly = AssemblyLoadContext.Default.LoadFromAssemblyPath(_cacheAssemblyPath);
                    return cachedAssembly;
                }
            }

            Stopwatch sw = Stopwatch.StartNew();

            // Collect references
            HashSet<string> references = new HashSet<string>();
            foreach (var defaultReference in DefaultReferences)
                references.Add(defaultReference.Location);
            foreach (var assemblyFile in References)
                references.Add(assemblyFile);
            foreach (var assembly in Assemblies)
            {
                if (!assembly.IsDynamic)
                    references.Add(assembly.Location);
            }
            references.Add(Path.Combine(Directory.GetParent(DefaultReferences[0].Location).FullName, "System.dll"));
            references.Add(Path.Combine(Directory.GetParent(DefaultReferences[0].Location).FullName, "System.Runtime.dll"));
            references.Add(Path.Combine(Directory.GetParent(DefaultReferences[0].Location).FullName, "System.Collections.dll"));
            references.Add(Path.Combine(Directory.GetParent(DefaultReferences[0].Location).FullName, "Microsoft.Win32.Registry.dll"));

            // HACK: C# will give compilation errors if a LIB variable contains non-existing directories
            Environment.SetEnvironmentVariable("LIB", null);

            CSharpCompilationOptions defaultCompilationOptions =
            new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
                    .WithUsings(new[] { "System", })
                    .WithPlatform(Microsoft.CodeAnalysis.Platform.AnyCpu)
                    .WithOptimizationLevel(OptimizationLevel.Debug);

            List<MetadataReference> defaultReferences = new List<MetadataReference>();
            foreach (var r in references)
                defaultReferences.Add(MetadataReference.CreateFromFile(r));

            // Run the compilation
            using var memoryStream = new MemoryStream();

            var syntaxTrees = new List<SyntaxTree>();
            foreach (var sourceFile in SourceFiles)
            {
                var stringText = SourceText.From(File.ReadAllText(sourceFile), Encoding.UTF8);
                var parsedSyntaxTree = SyntaxFactory.ParseSyntaxTree(stringText,
                    CSharpParseOptions.Default.WithLanguageVersion(LanguageVersion.CSharp9), sourceFile);
                syntaxTrees.Add(parsedSyntaxTree);
            }

            var compilation = CSharpCompilation.Create("BuilderRulesCache.dll", syntaxTrees.ToArray(), defaultReferences, defaultCompilationOptions);
            EmitResult emitResult = compilation.Emit(memoryStream);

            // Process warnings and errors
            foreach (var diagnostic in emitResult.Diagnostics)
            {
                var msg = diagnostic.ToString();
                if (diagnostic.Severity == DiagnosticSeverity.Warning)
                    Log.Warning(msg);
                else if (diagnostic.IsWarningAsError || diagnostic.Severity == DiagnosticSeverity.Error)
                    Log.Error(msg);
            }
            if (!emitResult.Success)
                throw new Exception("Failed to build assembly.");

            memoryStream.Seek(0, SeekOrigin.Begin);
            Assembly compiledAssembly = AssemblyLoadContext.Default.LoadFromStream(memoryStream);

            if (_cachePath != null && _cacheAssemblyPath != null)
            {
                memoryStream.Seek(0, SeekOrigin.Begin);

                var cacheDirectory = Path.GetDirectoryName(_cacheAssemblyPath);
                if (!Directory.Exists(cacheDirectory))
                    Directory.CreateDirectory(cacheDirectory);

                using (FileStream fileStream = File.Open(_cacheAssemblyPath, FileMode.Create, FileAccess.Write))
                {
                    memoryStream.CopyTo(fileStream);
                    fileStream.Close();
                }

                File.WriteAllText(_cachePath, recentWriteTime.ToBinary().ToString());
            }

            sw.Stop();
            Log.Verbose("Assembler time: " + sw.Elapsed.TotalSeconds.ToString() + "s");
            return compiledAssembly;
        }
    }
}
