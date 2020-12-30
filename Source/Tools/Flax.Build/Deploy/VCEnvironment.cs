// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

using System;
using System.IO;
using Flax.Build;
using Flax.Build.Platforms;
using Flax.Build.Projects.VisualStudio;
using Microsoft.Win32;

namespace Flax.Deploy
{
    /// <summary>
    /// Stores information about a Visual C++ installation and compile environment.
    /// </summary>
    public static class VCEnvironment
    {
        private static string _msBuildPath;
        private static string _cscPath;

        /// <summary>
        /// Gets the path to the MSBuild executable.
        /// </summary>
        public static string MSBuildPath
        {
            get
            {
                if (_msBuildPath == null)
                    _msBuildPath = GetMSBuildToolPath();
                return _msBuildPath;
            }
        }

        /// <summary>
        /// Gets the path to the C# compiler executable.
        /// </summary>
        public static string CscPath
        {
            get
            {
                if (_cscPath == null)
                    _cscPath = GetCscPath();
                return _cscPath;
            }
        }

        private static string GetMSBuildToolPath()
        {
            string toolPath;
            switch (Platform.BuildPlatform.Target)
            {
            case TargetPlatform.Linux:
            {
                // Use msbuild from Mono
                toolPath = UnixPlatform.Which("msbuild");
                if (toolPath != null)
                {
                    return toolPath;
                }

                break;
            }
            case TargetPlatform.Windows:
            {
                var visualStudioInstances = VisualStudioInstance.GetInstances();
                foreach (var visualStudioInstance in visualStudioInstances)
                {
                    toolPath = Path.Combine(visualStudioInstance.Path, "MSBuild\\Current\\Bin\\MSBuild.exe");
                    if (File.Exists(toolPath))
                    {
                        return toolPath;
                    }
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\VisualStudio\\SxS\\VS7", "15.0", "MSBuild\\15.0\\bin\\MSBuild.exe", out toolPath))
                {
                    return toolPath;
                }

                toolPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "MSBuild", "14.0", "bin", "MSBuild.exe");
                if (File.Exists(toolPath))
                {
                    return toolPath;
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\MSBuild\\ToolsVersions\\14.0", "MSBuildToolsPath", "MSBuild.exe", out toolPath))
                {
                    return toolPath;
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\MSBuild\\ToolsVersions\\12.0", "MSBuildToolsPath", "MSBuild.exe", out toolPath))
                {
                    return toolPath;
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\MSBuild\\ToolsVersions\\4.0", "MSBuildToolsPath", "MSBuild.exe", out toolPath))
                {
                    return toolPath;
                }

                break;
            }
            }

            return string.Empty;
        }

        private static string GetCscPath()
        {
            string toolPath;
            switch (Platform.BuildPlatform.Target)
            {
            case TargetPlatform.Linux:
            {
                // Use csc from Mono
                toolPath = UnixPlatform.Which("csc");
                if (toolPath != null)
                {
                    return toolPath;
                }
                break;
            }
            case TargetPlatform.Windows:
            {
                if (CheckMsBuildPathFromRegistry("Microsoft\\VisualStudio\\SxS\\VS7", "15.0", "MSBuild\\15.0\\bin\\csc.exe", out toolPath))
                {
                    return toolPath;
                }

                toolPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "MSBuild", "14.0", "bin", "csc.exe");
                if (File.Exists(toolPath))
                {
                    return toolPath;
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\MSBuild\\ToolsVersions\\14.0", "MSBuildToolsPath", "csc.exe", out toolPath))
                {
                    return toolPath;
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\MSBuild\\ToolsVersions\\12.0", "MSBuildToolsPath", "csc.exe", out toolPath))
                {
                    return toolPath;
                }

                if (CheckMsBuildPathFromRegistry("Microsoft\\MSBuild\\ToolsVersions\\4.0", "MSBuildToolsPath", "csc.exe", out toolPath))
                {
                    return toolPath;
                }

                break;
            }
            }

            return string.Empty;
        }

        private static bool CheckMsBuildPathFromRegistry(string keyRelativePath, string keyName, string msBuildRelativePath, out string outMsBuildPath)
        {
            string[] prefixes =
            {
                @"HKEY_CURRENT_USER\SOFTWARE\",
                @"HKEY_LOCAL_MACHINE\SOFTWARE\",
                @"HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\",
                @"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\"
            };

            for (var i = 0; i < prefixes.Length; i++)
            {
                if (Registry.GetValue(prefixes[i] + keyRelativePath, keyName, null) is string value)
                {
                    string msBuildPath = Path.Combine(value, msBuildRelativePath);
                    if (File.Exists(msBuildPath))
                    {
                        outMsBuildPath = msBuildPath;
                        return true;
                    }
                }
            }

            outMsBuildPath = null;
            return false;
        }

        /// <summary>
        /// Runs msbuild.exe with the specified arguments.
        /// </summary>
        /// <param name="project">Path to the project to build.</param>
        /// <param name="buildConfig">Configuration to build.</param>
        /// <param name="buildPlatform">Platform to build.</param>
        public static void BuildProject(string project, string buildConfig, string buildPlatform)
        {
            var msBuild = MSBuildPath;
            if (string.IsNullOrEmpty(msBuild))
            {
                throw new Exception(string.Format("Unable to find msbuild.exe at: \"{0}\"", msBuild));
            }

            if (!File.Exists(project))
            {
                throw new Exception(string.Format("Project {0} does not exist!", project));
            }

            string cmdLine = string.Format("\"{0}\" /m /t:Build /p:Configuration=\"{1}\" /p:Platform=\"{2}\" /verbosity:minimal /nologo", project, buildConfig, buildPlatform);
            int result = Utilities.Run(msBuild, cmdLine);
            if (result != 0)
            {
                throw new Exception(string.Format("Unable to build project {0}. MSBuild failed. See log to learn more.", project));
            }
        }

        /// <summary>
        /// Builds a Visual Studio solution with MsBuild.
        /// </summary>
        /// <param name="solutionFile">Path to the solution file</param>
        /// <param name="buildConfig">Configuration to build.</param>
        /// <param name="buildPlatform">Platform to build.</param>
        public static void BuildSolution(string solutionFile, string buildConfig, string buildPlatform)
        {
            var msBuild = MSBuildPath;
            if (string.IsNullOrEmpty(msBuild))
            {
                throw new Exception(string.Format("Unable to find msbuild.exe at: \"{0}\"", msBuild));
            }

            if (!File.Exists(solutionFile))
            {
                throw new Exception(string.Format("Unable to build solution {0}. Solution file not found.", solutionFile));
            }

            string cmdLine = string.Format("\"{0}\" /m /t:Build /p:Configuration=\"{1}\" /p:Platform=\"{2}\" /verbosity:minimal /nologo", solutionFile, buildConfig, buildPlatform);
            int result = Utilities.Run(msBuild, cmdLine);
            if (result != 0)
            {
                throw new Exception(string.Format("Unable to build solution {0}. MSBuild failed. See log to learn more.", solutionFile));
            }
        }

        /// <summary>
        /// Clears the build a Visual Studio solution cache with MsBuild.
        /// </summary>
        /// <param name="solutionFile">Path to the solution file</param>
        public static void CleanSolution(string solutionFile)
        {
            var msBuild = MSBuildPath;
            if (string.IsNullOrEmpty(msBuild))
            {
                throw new Exception(string.Format("Unable to find msbuild.exe at: \"{0}\"", msBuild));
            }

            if (!File.Exists(solutionFile))
            {
                throw new Exception(string.Format("Unable to clean solution {0}. Solution file not found.", solutionFile));
            }

            string cmdLine = string.Format("\"{0}\" /t:Clean /verbosity:minimal /nologo", solutionFile);
            Utilities.Run(msBuild, cmdLine);
        }
    }
}
