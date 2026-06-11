// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build.NativeCpp;
using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace Flax.Build
{
    /// <summary>
    /// Helper methods for querying <c>pkg-config</c>.
    /// </summary>
    public static class PkgConfig
    {
        private const string ToolName = "pkg-config";

        /// <summary>
        /// Tries to run <c>pkg-config</c> with the specified arguments.
        /// </summary>
        /// <param name="arguments">Arguments for <c>pkg-config</c>.</param>
        /// <param name="output">The trimmed standard output on success.</param>
        /// <param name="sysroot">Optional sysroot to set via <c>PKG_CONFIG_SYSROOT_DIR</c>.</param>
        /// <param name="pkgConfigLibDirs">Optional package config search directories for <c>PKG_CONFIG_LIBDIR</c>.</param>
        /// <returns><see langword="true"/> on success, otherwise <see langword="false"/>.</returns>
        public static bool TryRun(string arguments, out string output, string sysroot = null, IEnumerable<string> pkgConfigLibDirs = null)
        {
            output = null;
            try
            {
                using (var process = new Process())
                {
                    process.StartInfo = new ProcessStartInfo
                    {
                        FileName = ToolName,
                        Arguments = arguments,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true,
                    };

                    process.StartInfo.EnvironmentVariables["PKG_CONFIG_DIR"] = string.Empty;
                    if (!string.IsNullOrEmpty(sysroot))
                        process.StartInfo.EnvironmentVariables["PKG_CONFIG_SYSROOT_DIR"] = sysroot;

                    if (pkgConfigLibDirs != null)
                    {
                        var dirs = new List<string>();
                        foreach (var dir in pkgConfigLibDirs)
                        {
                            if (!string.IsNullOrEmpty(dir) && !dirs.Contains(dir))
                                dirs.Add(dir);
                        }
                        if (dirs.Count > 0)
                            process.StartInfo.EnvironmentVariables["PKG_CONFIG_LIBDIR"] = string.Join(System.IO.Path.PathSeparator.ToString(), dirs);
                    }

                    process.Start();
                    output = process.StandardOutput.ReadToEnd().Trim();
                    var error = process.StandardError.ReadToEnd().Trim();
                    process.WaitForExit();
                    if (process.ExitCode == 0)
                        return true;

                    if (!string.IsNullOrEmpty(error))
                        Log.Verbose($"{ToolName} {arguments} failed: {error}");
                }
            }
            catch (Exception ex)
            {
                Log.Verbose($"{ToolName} unavailable: {ex.Message}");
            }
            return false;
        }

        /// <summary>
        /// Tries to add include paths returned by <c>pkg-config --cflags-only-I</c>.
        /// </summary>
        /// <param name="options">Build options to modify.</param>
        /// <param name="packages">Space-separated package names.</param>
        /// <param name="sysroot">Optional sysroot to set via <c>PKG_CONFIG_SYSROOT_DIR</c>.</param>
        /// <param name="pkgConfigLibDirs">Optional package config search directories for <c>PKG_CONFIG_LIBDIR</c>.</param>
        /// <returns><see langword="true"/> if at least one include path was added.</returns>
        public static bool TryAddIncludePaths(BuildOptions options, string packages, string sysroot = null, IEnumerable<string> pkgConfigLibDirs = null)
        {
            if (options == null)
                throw new ArgumentNullException(nameof(options));

            if (!TryRun($"--silence-errors --cflags-only-I {packages}", out var output, sysroot, pkgConfigLibDirs) || string.IsNullOrWhiteSpace(output))
                return false;

            var addedAny = false;
            var tokens = output.Split(new[] { ' ', '\t', '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var token in tokens)
            {
                if (!token.StartsWith("-I"))
                    continue;

                var includePath = token.Substring(2).Trim('"');
                if (string.IsNullOrEmpty(includePath) || options.PublicIncludePaths.Contains(includePath))
                    continue;

                options.PublicIncludePaths.Add(includePath);
                addedAny = true;
            }

            return addedAny;
        }

        /// <summary>
        /// Adds library linker arguments from <c>pkg-config --libs</c> or fallback flags if unavailable.
        /// </summary>
        /// <param name="args">Linker arguments list to modify.</param>
        /// <param name="packages">Space-separated package names.</param>
        /// <param name="fallbackLibraries">Fallback linker flags if pkg-config query fails.</param>
        /// <param name="sysroot">Optional sysroot to set via <c>PKG_CONFIG_SYSROOT_DIR</c>.</param>
        /// <param name="pkgConfigLibDirs">Optional package config search directories for <c>PKG_CONFIG_LIBDIR</c>.</param>
        public static void AddLibsOrFallback(List<string> args, string packages, IEnumerable<string> fallbackLibraries, string sysroot = null, IEnumerable<string> pkgConfigLibDirs = null)
        {
            if (args == null)
                throw new ArgumentNullException(nameof(args));

            if (TryRun($"--silence-errors --libs {packages}", out var libs, sysroot, pkgConfigLibDirs) && !string.IsNullOrWhiteSpace(libs))
            {
                args.AddRange(libs.Split(new[] { ' ', '\t', '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries));
            }
            else if (fallbackLibraries != null)
            {
                args.AddRange(fallbackLibraries);
            }
        }
    }
}
