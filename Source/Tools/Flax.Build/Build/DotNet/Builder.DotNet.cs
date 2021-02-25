// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Graph;

namespace Flax.Build
{
    static partial class Builder
    {
        private static void BuildTargetDotNet(RulesAssembly rules, TaskGraph graph, Target target, Platform platform, TargetConfiguration configuration)
        {
            throw new NotImplementedException("TODO: building C# targets");
        }

        private static void BuildTargetBindings(RulesAssembly rules, TaskGraph graph, BuildData buildData)
        {
            var workspaceRoot = buildData.TargetOptions.WorkingDirectory;
            var args = new List<string>();
            var referencesToCopy = new HashSet<KeyValuePair<string, string>>();
            var buildOptions = buildData.TargetOptions;
            foreach (var binaryModule in buildData.BinaryModules)
            {
                if (binaryModule.All(x => !x.BuildCSharp))
                    continue;
                var binaryModuleName = binaryModule.Key;
                using (new ProfileEventScope(binaryModuleName))
                {
                    // TODO: add support for extending this code and support generating bindings projects for other scripting languages
                    var project = GetModuleProject(binaryModule.First(), buildData);

                    // Get source files
                    var sourceFiles = new List<string>();
                    foreach (var module in binaryModule)
                        sourceFiles.AddRange(buildData.Modules[module].SourceFiles.Where(x => x.EndsWith(".cs")));
                    sourceFiles.RemoveAll(x => x.EndsWith(BuildFilesPostfix));
                    var moduleGen = Path.Combine(project.ProjectFolderPath, "Source", binaryModuleName + ".Gen.cs");
                    if (!sourceFiles.Contains(moduleGen))
                        sourceFiles.Add(moduleGen);
                    sourceFiles.Sort();

                    // Setup build options
                    var buildPlatform = Platform.BuildPlatform.Target;
                    var outputPath = Path.GetDirectoryName(buildData.Target.GetOutputFilePath(buildOptions));
                    var outputFile = Path.Combine(outputPath, binaryModuleName + ".CSharp.dll");
                    var outputDocFile = Path.Combine(outputPath, binaryModuleName + ".CSharp.xml");
                    string monoRoot, exePath;
                    switch (buildPlatform)
                    {
                    case TargetPlatform.Windows:
                        monoRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Editor", "Windows", "Mono");
                        exePath = Path.Combine(monoRoot, "bin", "mono.exe");
                        break;
                    case TargetPlatform.Linux:
                        monoRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Editor", "Linux", "Mono");
                        exePath = Path.Combine(monoRoot, "bin", "mono");
                        break;
                    default: throw new InvalidPlatformException(buildPlatform);
                    }
                    var cscPath = Path.Combine(monoRoot, "lib", "mono", "4.5", "csc.exe");
                    var referenceAssemblies = Path.Combine(monoRoot, "lib", "mono", "4.5-api");
                    var references = new HashSet<string>(buildOptions.ScriptingAPI.FileReferences);
                    foreach (var module in binaryModule)
                    {
                        if (!buildData.Modules.TryGetValue(module, out var moduleBuildOptions))
                            continue;

                        // Find references based on the modules dependencies
                        foreach (var dependencyName in moduleBuildOptions.PublicDependencies.Concat(moduleBuildOptions.PrivateDependencies))
                        {
                            var dependencyModule = buildData.Rules.GetModule(dependencyName);
                            if (dependencyModule != null &&
                                !string.IsNullOrEmpty(dependencyModule.BinaryModuleName) &&
                                dependencyModule.BuildCSharp &&
                                dependencyModule.BinaryModuleName != binaryModuleName &&
                                buildData.Modules.TryGetValue(dependencyModule, out var dependencyModuleOptions))
                            {
                                foreach (var x in buildData.BinaryModules)
                                {
                                    if (x.Key == null || x.Key != dependencyModule.BinaryModuleName)
                                        continue;

                                    // Reference module output binary
                                    references.Add(Path.Combine(outputPath, dependencyModule.BinaryModuleName + ".CSharp.dll"));
                                }
                                foreach (var e in buildData.ReferenceBuilds)
                                {
                                    foreach (var q in e.Value.BuildInfo.BinaryModules)
                                    {
                                        if (q.Name == dependencyModule.BinaryModuleName && !string.IsNullOrEmpty(q.ManagedPath))
                                        {
                                            // Reference binary module build build for referenced target
                                            references.Add(q.ManagedPath);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Setup C# compiler arguments
                    args.Clear();
                    args.Add("/nologo");
                    args.Add("/target:library");
                    args.Add("/platform:AnyCPU");
                    args.Add("/debug+");
                    args.Add("/debug:portable");
                    args.Add("/errorreport:prompt");
                    args.Add("/preferreduilang:en-US");
                    args.Add("/highentropyva+");
                    args.Add("/deterministic");
                    args.Add("/nostdlib+");
                    args.Add("/errorendlocation");
                    args.Add("/utf8output");
                    args.Add("/warn:4");
                    args.Add("/unsafe");
                    args.Add("/fullpaths");
                    args.Add("/langversion:7.3");
                    if (buildOptions.ScriptingAPI.IgnoreMissingDocumentationWarnings)
                        args.Add("-nowarn:1591");
                    args.Add(buildData.Configuration == TargetConfiguration.Debug ? "/optimize-" : "/optimize+");
                    args.Add(string.Format("/out:\"{0}\"", outputFile));
                    args.Add(string.Format("/doc:\"{0}\"", outputDocFile));
                    if (buildOptions.ScriptingAPI.Defines.Count != 0)
                        args.Add("/define:" + string.Join(";", buildOptions.ScriptingAPI.Defines));
                    if (buildData.Configuration == TargetConfiguration.Debug)
                        args.Add("/define:DEBUG");
                    args.Add(string.Format("/reference:\"{0}{1}mscorlib.dll\"", referenceAssemblies, Path.DirectorySeparatorChar));
                    foreach (var reference in buildOptions.ScriptingAPI.SystemReferences)
                        args.Add(string.Format("/reference:\"{0}{2}{1}.dll\"", referenceAssemblies, reference, Path.DirectorySeparatorChar));
                    foreach (var reference in references)
                        args.Add(string.Format("/reference:\"{0}\"", reference));
                    foreach (var sourceFile in sourceFiles)
                        args.Add("\"" + sourceFile + "\"");

                    // Generate response file with source files paths and compilation arguments
                    string responseFile = Path.Combine(buildOptions.IntermediateFolder, binaryModuleName + ".CSharp.response");
                    Utilities.WriteFileIfChanged(responseFile, string.Join(Environment.NewLine, args));

                    // Create C# compilation task
                    var task = graph.Add<Task>();
                    task.PrerequisiteFiles.Add(responseFile);
                    task.PrerequisiteFiles.AddRange(sourceFiles);
                    task.PrerequisiteFiles.AddRange(references);
                    task.ProducedFiles.Add(outputFile);
                    task.WorkingDirectory = workspaceRoot;
                    task.CommandPath = exePath;
                    task.CommandArguments = $"\"{cscPath}\" /noconfig @\"{responseFile}\"";
                    task.InfoMessage = "Compiling " + outputFile;
                    task.Cost = task.PrerequisiteFiles.Count;

                    // Copy referenced assemblies
                    foreach (var reference in buildOptions.ScriptingAPI.FileReferences)
                    {
                        var dstFile = Path.Combine(outputPath, Path.GetFileName(reference));
                        if (dstFile == reference)
                            continue;

                        referencesToCopy.Add(new KeyValuePair<string, string>(dstFile, reference));
                    }
                }
            }

            // Copy files (using hash set to prevent copying the same file twice when building multiple scripting modules using the same files)
            foreach (var e in referencesToCopy)
            {
                var dst = e.Key;
                var src = e.Value;
                graph.AddCopyFile(dst, src);

                var srcPdb = Path.ChangeExtension(src, "pdb");
                if (File.Exists(srcPdb))
                    graph.AddCopyFile(Path.ChangeExtension(dst, "pdb"), srcPdb);

                var srcXml = Path.ChangeExtension(src, "xml");
                if (File.Exists(srcXml))
                    graph.AddCopyFile(Path.ChangeExtension(dst, "xml"), srcXml);
            }
        }
    }
}
