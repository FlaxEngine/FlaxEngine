// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace Flax.Build
{
    /// <summary>
    /// Utility for building C# assemblies from custom set of source files.
    /// </summary>
    public class Assembler
    {
        /// <summary>
        /// The default assembly references added to the projects.
        /// </summary>
        public static readonly Assembly[] DefaultReferences =
        {
            //typeof(IntPtr).Assembly, // mscorlib.dll
            typeof(Enumerable).Assembly, // System.Linq.dll
            typeof(ISet<>).Assembly, // System.dll
            typeof(Builder).Assembly, // Flax.Build.exe
        };

        /// <summary>
        /// The output assembly path. Use null to store assembly in the process memory.
        /// </summary>
        public string OutputPath = null;

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

        /// <summary>
        /// Builds the assembly.
        /// </summary>
        /// <remarks>Throws an exception in case of any errors.</remarks>
        /// <returns>The created and loaded assembly.</returns>
        public Assembly Build()
        {
            Dictionary<string, string> providerOptions = new Dictionary<string, string>();
            providerOptions.Add("CompilerVersion", "v4.0");
            CodeDomProvider provider = new Microsoft.CSharp.CSharpCodeProvider(providerOptions);

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

            // Setup compilation options
            CompilerParameters cp = new CompilerParameters();
            cp.GenerateExecutable = false;
            cp.WarningLevel = 4;
            cp.TreatWarningsAsErrors = false;
            cp.ReferencedAssemblies.AddRange(references.ToArray());
            if (string.IsNullOrEmpty(OutputPath))
            {
                cp.GenerateInMemory = true;
                cp.IncludeDebugInformation = false;
            }
            else
            {
                cp.GenerateInMemory = false;
                cp.IncludeDebugInformation = true;
                cp.OutputAssembly = OutputPath;
            }

            // HACK: C# will give compilation errors if a LIB variable contains non-existing directories
            Environment.SetEnvironmentVariable("LIB", null);

            // Run the compilation
            CompilerResults cr = provider.CompileAssemblyFromFile(cp, SourceFiles.ToArray());

            // Process warnings and errors
            bool hasError = false;
            foreach (CompilerError ce in cr.Errors)
            {
                if (ce.IsWarning)
                {
                    Log.Warning(string.Format("{0} at {1}: {2}", ce.FileName, ce.Line, ce.ErrorText));
                }
                else
                {
                    Log.Error(string.Format("{0} at line {1}: {2}", ce.FileName, ce.Line, ce.ErrorText));
                    hasError = true;
                }
            }
            if (hasError)
                throw new Exception("Failed to build assembly.");

            return cr.CompiledAssembly;
        }
    }
}
