// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Net;
using System.Threading;
using Flax.Deploy;
using Flax.Deps;

namespace Flax.Build
{
    class Program
    {
        static int Main()
        {
            // Setup
            ServicePointManager.Expect100Continue = true;
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;

            // Show help option
            if (CommandLine.HasOption("help"))
            {
                Console.WriteLine(CommandLine.GetHelp(typeof(Configuration)));
                return 0;
            }

            Mutex singleInstanceMutex = null;
            var startTime = DateTime.Now;
            bool failed = false;

            try
            {
                // Setup
                CommandLine.Configure(typeof(Configuration));
                foreach (var option in CommandLine.GetOptions())
                {
                    if (option.Name.Length > 1 && option.Name[0] == 'D')
                    {
                        var define = option.Name.Substring(1);
                        if (!string.IsNullOrEmpty(option.Value))
                            define += "=" + option.Value;
                        Configuration.CustomDefines.Add(define);
                    }
                }
                if (Configuration.CurrentDirectory != null)
                    Environment.CurrentDirectory = Configuration.CurrentDirectory;
                Globals.Root = Directory.GetCurrentDirectory();
                var executingAssembly = System.Reflection.Assembly.GetExecutingAssembly();
                Globals.EngineRoot = Utilities.RemovePathRelativeParts(Path.Combine(Path.GetDirectoryName(executingAssembly.Location), "..", ".."));
                Log.Init();

                // Log basic info
                Version version = executingAssembly.GetName().Version;
                string versionString = string.Join(".", version.Major, version.Minor, version.Build);
                Log.Info(string.Format("Flax.Build {0}", versionString));
                using (new LogIndentScope())
                {
                    Log.Verbose("Arguments: " + CommandLine.Get());
                    Log.Verbose("Workspace: " + Globals.Root);
                    Log.Verbose("Engine: " + Globals.EngineRoot);
                }

                // Load project
                {
                    var projectFiles = Directory.GetFiles(Globals.Root, "*.flaxproj", SearchOption.TopDirectoryOnly);
                    if (projectFiles.Length == 1)
                        Globals.Project = ProjectInfo.Load(projectFiles[0]);
                    else if (projectFiles.Length > 1)
                        throw new Exception("Too many project files. Don't know which to pick.");
                    else
                        Log.Warning("Missing project file.");
                }

                // Use mutex if required
                if (Configuration.Mutex)
                {
                    singleInstanceMutex = new Mutex(true, "Flax.Build", out var oneInstanceMutexCreated);

                    if (!oneInstanceMutexCreated)
                    {
                        try
                        {
                            if (!singleInstanceMutex.WaitOne(0))
                            {
                                Log.Warning("Wait for another instance(s) of Flax.Build to end...");
                                singleInstanceMutex.WaitOne();
                            }
                        }
                        catch (AbandonedMutexException)
                        {
                            // Can occur if another Flax.Build is killed in the debugger
                        }
                        finally
                        {
                            Log.Info("Waiting done.");
                        }
                    }
                }

                // Collect all targets and modules from the workspace
                Builder.GenerateRulesAssembly();

                // Print SDKs
                if (Configuration.PrintSDKs)
                {
                    Log.Info("Printing SDKs...");
                    Sdk.Print();
                }

                // Deps tool
                if (Configuration.BuildDeps || Configuration.ReBuildDeps)
                {
                    Log.Info("Building dependencies...");
                    failed |= DepsBuilder.Run();
                }

                // Clean
                if (Configuration.Clean || Configuration.Rebuild)
                {
                    Log.Info("Cleaning build workspace...");
                    Builder.Clean();
                }

                // Generate projects for the targets and solution for the workspace
                if (Configuration.GenerateProject)
                {
                    Log.Info("Generating project files...");
                    Builder.GenerateProjects();
                }

                // Build targets
                if (Configuration.Build || Configuration.Rebuild)
                {
                    Log.Info("Building targets...");
                    failed |= Builder.BuildTargets();
                }

                // Deploy tool
                if (Configuration.Deploy)
                {
                    Log.Info("Running deployment...");
                    failed |= Deployer.Run();
                }

                // Performance logging
                if (Configuration.PerformanceInfo)
                {
                    Log.Info(string.Empty);
                    Log.Info("Performance Summary");
                    Profiling.LogStats();
                }
                if (Configuration.TraceEventFile != null)
                {
                    Profiling.GenerateTraceEventFile();
                }
            }
            catch (Exception ex)
            {
                Log.Exception(ex);
                return 1;
            }
            finally
            {
                if (singleInstanceMutex != null)
                {
                    singleInstanceMutex.Dispose();
                    singleInstanceMutex = null;
                }

                var endTime = DateTime.Now;
                Log.Info(string.Format("Total time: {0}", endTime - startTime));
                Log.Verbose("End.");
                Log.Dispose();
            }

            return failed ? 1 : 0;
        }
    }
}
