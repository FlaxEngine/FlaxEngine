// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace Flax.Build
{
    /// <summary>
    /// The utilities.
    /// </summary>
    public static class Utilities
    {
        /// <summary>
        /// Gets the .Net SDK path.
        /// </summary>
        /// <returns>The path.</returns>
        public static string GetDotNetPath()
        {
            var buildPlatform = Platform.BuildTargetPlatform;
            var dotnetSdk = DotNetSdk.Instance;
            if (!dotnetSdk.IsValid)
                throw new DotNetSdk.MissingException();
            var dotnetPath = "dotnet";
            switch (buildPlatform)
            {
            case TargetPlatform.Windows:
                dotnetPath = Path.Combine(dotnetSdk.RootPath, "dotnet.exe");
                break;
            case TargetPlatform.Linux: break;
            case TargetPlatform.Mac:
                dotnetPath = Path.Combine(dotnetSdk.RootPath, "dotnet");
                break;
            default: throw new InvalidPlatformException(buildPlatform);
            }
            return dotnetPath;
        }

        /// <summary>
        /// Restores a targets nuget packages.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="target">The target.</param>
        /// <param name="dotNetPath">The dotnet path.</param>
        public static void RestoreNugetPackages(Graph.TaskGraph graph, Target target)
        {
            var dotNetPath = GetDotNetPath();
            var task = graph.Add<Graph.Task>();
            task.WorkingDirectory = target.FolderPath;
            task.InfoMessage = $"Restoring Nuget Packages for {target.Name}";
            task.CommandPath = dotNetPath;
            task.CommandArguments = $"restore";
        }
        
        /// <summary>
        /// Gets the hash code for the string (the same for all platforms). Matches Engine algorithm for string hashing.
        /// </summary>
        /// <param name="str">The input string.</param>
        /// <returns>The file size text.</returns>
        public static uint GetHashCode(string str)
        {
            uint hash = 5381;
            if (str != null)
            {
                for (int i = 0; i < str.Length; i++)
                {
                    char c = str[i];
                    hash = ((hash << 5) + hash) + (uint)c;
                }
            }
            return hash;
        }

        /// <summary>
        /// Gets the empty array of the given type (shared one).
        /// </summary>
        /// <typeparam name="T">The type.</typeparam>
        /// <returns>The empty array object.</returns>
        public static T[] GetEmptyArray<T>()
        {
#if USE_NETCORE
            return Array.Empty<T>();
#else
            return Enumerable.Empty<T>() as T[];
#endif
        }

        /// <summary>
        /// Gets the static field value from a given type.
        /// </summary>
        /// <param name="typeName">Name of the type.</param>
        /// <param name="fieldName">Name of the field.</param>
        /// <returns>The field value.</returns>
        public static object GetStaticValue(string typeName, string fieldName)
        {
            var type = Type.GetType(typeName);
            if (type == null)
                throw new Exception($"Cannot find type \'{typeName}\'.");
            var field = type.GetField(fieldName, BindingFlags.Public | BindingFlags.Static);
            if (field == null)
                throw new Exception($"Cannot find static public field \'{fieldName}\' in \'{typeName}\'.");
            return field.GetValue(null);
        }

        /// <summary>
        /// Gets the size of the file as a readable string.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The file size text.</returns>
        public static string GetFileSize(string path)
        {
            return (Math.Floor(new FileInfo(path).Length / 1024.0f / 1024 * 100) / 100) + " MB";
        }

        /// <summary>
        /// Adds the range of the items to the hash set.
        /// </summary>
        /// <typeparam name="T">The item type.</typeparam>
        /// <param name="source">The hash set to modify.</param>
        /// <param name="items">The items collection to append.</param>
        public static void AddRange<T>(this HashSet<T> source, IEnumerable<T> items)
        {
            foreach (T item in items)
            {
                source.Add(item);
            }
        }

        /// <summary>
        /// Gets the two way enumerator for the given enumerable collection.
        /// </summary>
        /// <typeparam name="T">The item type.</typeparam>
        /// <param name="source">The source collection.</param>
        /// <returns>The enumerator.</returns>
        public static ITwoWayEnumerator<T> GetTwoWayEnumerator<T>(this IEnumerable<T> source)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));
            return new TwoWayEnumerator<T>(source.GetEnumerator());
        }

        /// <summary>
        /// Copies the file.
        /// </summary>
        /// <param name="srcFilePath">The source file path.</param>
        /// <param name="dstFilePath">The destination file path.</param>
        /// <param name="overwrite"><see langword="true" /> if the destination file can be overwritten; otherwise, <see langword="false" />.</param>
        public static void FileCopy(string srcFilePath, string dstFilePath, bool overwrite = true)
        {
            if (string.IsNullOrEmpty(srcFilePath))
                throw new ArgumentNullException(nameof(srcFilePath));
            if (string.IsNullOrEmpty(dstFilePath))
                throw new ArgumentNullException(nameof(dstFilePath));

            Log.Verbose(srcFilePath + " -> " + dstFilePath);

            try
            {
                File.Copy(srcFilePath, dstFilePath, overwrite);
            }
            catch (Exception ex)
            {
                if (!File.Exists(srcFilePath))
                    Log.Error("Failed to copy file. Missing source file.");
                else
                    Log.Error("Failed to copy file. " + ex.Message);
                throw;
            }
        }

        /// <summary>
        /// Copies the directories.
        /// </summary>
        /// <param name="srcDirectoryPath">The source directory path.</param>
        /// <param name="dstDirectoryPath">The destination directory path.</param>
        /// <param name="copySubdirs">If set to <c>true</c> copy sub-directories (recursive copy operation).</param>
        /// <param name="overrideFiles">If set to <c>true</c> override target files if any is existing.</param>
        /// <param name="searchPattern">The custom filter for the filenames to copy. Can be used to select files to copy. Null if unused.</param>
        public static void DirectoryCopy(string srcDirectoryPath, string dstDirectoryPath, bool copySubdirs = true, bool overrideFiles = false, string searchPattern = null)
        {
            Log.Verbose(srcDirectoryPath + " -> " + dstDirectoryPath);

            var dir = new DirectoryInfo(srcDirectoryPath);

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException("Missing source directory to copy. " + srcDirectoryPath);
            }

            if (!Directory.Exists(dstDirectoryPath))
            {
                Directory.CreateDirectory(dstDirectoryPath);
            }

            var files = searchPattern != null ? dir.GetFiles(searchPattern) : dir.GetFiles();
            for (int i = 0; i < files.Length; i++)
            {
                string tmp = Path.Combine(dstDirectoryPath, files[i].Name);
                File.Copy(files[i].FullName, tmp, overrideFiles);
            }

            if (copySubdirs)
            {
                var dirs = dir.GetDirectories();
                for (int i = 0; i < dirs.Length; i++)
                {
                    string tmp = Path.Combine(dstDirectoryPath, dirs[i].Name);
                    DirectoryCopy(dirs[i].FullName, tmp, true, overrideFiles, searchPattern);
                }
            }
        }

        private static void DirectoryDelete(DirectoryInfo dir)
        {
            var subdirs = dir.GetDirectories();
            foreach (var subdir in subdirs)
            {
                DirectoryDelete(subdir);
            }

            var files = dir.GetFiles();
            foreach (var file in files)
            {
                try
                {
                    file.Delete();
                }
                catch (UnauthorizedAccessException)
                {
                    File.SetAttributes(file.FullName, FileAttributes.Normal);
                    file.Delete();
                }
            }

            dir.Delete();
        }

        /// <summary>
        /// Deletes the file.
        /// </summary>
        /// <param name="filePath">The file path.</param>
        public static void FileDelete(string filePath)
        {
            var file = new FileInfo(filePath);
            if (!file.Exists)
                return;

            file.Delete();
        }

        /// <summary>
        /// Deletes the directory.
        /// </summary>
        /// <param name="directoryPath">The directory path.</param>
        /// <param name="withSubdirs">if set to <c>true</c> with sub-directories (recursive delete operation).</param>
        public static void DirectoryDelete(string directoryPath, bool withSubdirs = true)
        {
            var dir = new DirectoryInfo(directoryPath);
            if (!dir.Exists)
                return;

            if (withSubdirs)
                DirectoryDelete(dir);
            else
                dir.Delete();
        }

        /// <summary>
        /// Deletes the files inside a directory.
        /// </summary>
        /// <param name="directoryPath">The directory path.</param>
        /// <param name="searchPattern">The custom filter for the filenames to delete. Can be used to select files to delete. Null if unused.</param>
        /// <param name="withSubdirs">if set to <c>true</c> with sub-directories (recursive delete operation).</param>
        public static void FilesDelete(string directoryPath, string searchPattern = null, bool withSubdirs = true)
        {
            if (!Directory.Exists(directoryPath))
                return;
            if (searchPattern == null)
                searchPattern = "*";

            var files = Directory.GetFiles(directoryPath, searchPattern, withSubdirs ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly);
            for (int i = 0; i < files.Length; i++)
            {
                File.Delete(files[i]);
            }
        }

        /// <summary>
        /// Deletes the directories inside a directory.
        /// </summary>
        /// <param name="directoryPath">The directory path.</param>
        /// <param name="searchPattern">The custom filter for the directories to delete. Can be used to select files to delete. Null if unused.</param>
        /// <param name="withSubdirs">if set to <c>true</c> with sub-directories (recursive delete operation).</param>
        public static void DirectoriesDelete(string directoryPath, string searchPattern = null, bool withSubdirs = true)
        {
            if (!Directory.Exists(directoryPath))
                return;
            if (searchPattern == null)
                searchPattern = "*";

            var directories = Directory.GetDirectories(directoryPath, searchPattern, withSubdirs ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly);
            for (int i = 0; i < directories.Length; i++)
            {
                DirectoryDelete(directories[i]);
            }
        }

        /// <summary>
        /// The process run options.
        /// </summary>
        [Flags]
        public enum RunOptions
        {
            /// <summary>
            /// The none.
            /// </summary>
            None = 0,

            /// <summary>
            /// The application must exist.
            /// </summary>
            AppMustExist = 1 << 0,

            /// <summary>
            /// Skip waiting for exit.
            /// </summary>
            NoWaitForExit = 1 << 1,

            /// <summary>
            /// Skip standard output redirection to log.
            /// </summary>
            NoStdOutRedirect = 1 << 2,

            /// <summary>
            /// Skip logging of command run.
            /// </summary>
            NoLoggingOfRunCommand = 1 << 3,

            /// <summary>
            /// Uses UTF-8 output encoding format.
            /// </summary>
            UTF8Output = 1 << 4,

            /// <summary>
            /// Skips logging of run duration.
            /// </summary>
            NoLoggingOfRunDuration = 1 << 5,

            /// <summary>
            /// Throws exception when app returns non-zero return code.
            /// </summary>
            ThrowExceptionOnError = 1 << 6,

            /// <summary>
            /// Logs program output to the console, otherwise only when using verbose log.
            /// </summary>
            ConsoleLogOutput = 1 << 7,

            /// <summary>
            /// Enables <see cref="ProcessStartInfo.UseShellExecute"/> to run process via Shell instead of standard process startup.
            /// </summary>
            Shell = 1 << 8,

            /// <summary>
            /// The default options.
            /// </summary>
            Default = AppMustExist,

            /// <summary>
            /// The default options for tools execution in build tool.
            /// </summary>
            DefaultTool = ConsoleLogOutput | ThrowExceptionOnError,
        }

        private static void StdLogInfo(object sender, DataReceivedEventArgs e)
        {
            if (e.Data != null)
            {
                Log.Info(e.Data);
            }
        }

        private static void StdLogVerbose(object sender, DataReceivedEventArgs e)
        {
            if (e.Data != null)
            {
                Log.Verbose(e.Data);
            }
        }

        /// <summary>
        /// Calls <see cref="Type.GetType()"/> within Flax.Build context - can be used by build scripts to properly find a type by name. 
        /// </summary>
        /// <param name="name">The full name (namespace with class name).</param>
        /// <returns>Type or null if not found.</returns>
        public static Type GetType(string name)
        {
            return Type.GetType(name);
        }

        /// <summary>
        /// Runs the external program.
        /// </summary>
        /// <param name="app">Program filename.</param>
        /// <param name="commandLine">Commandline</param>
        /// <param name="input">Optional Input for the program (will be provided as stdin)</param>
        /// <param name="workspace">Optional custom workspace directory. Use null to keep the same directory.</param>
        /// <param name="options">Defines the options how to run. See RunOptions.</param>
        /// <param name="envVars">Custom environment variables to pass to the child process.</param>
        /// <returns>The exit code of the program.</returns>
        public static int Run(string app, string commandLine = null, string input = null, string workspace = null, RunOptions options = RunOptions.Default, Dictionary<string, string> envVars = null)
        {
            if (string.IsNullOrEmpty(app))
                throw new ArgumentNullException(nameof(app), "Missing app to run.");

            // Check if the application exists, including the PATH directories
            if (options.HasFlag(RunOptions.AppMustExist) && !File.Exists(app))
            {
                bool existsInPath = false;
                if (!app.Contains(Path.DirectorySeparatorChar) && !app.Contains(Path.AltDirectorySeparatorChar))
                {
                    string[] pathDirectories = Environment.GetEnvironmentVariable("PATH").Split(Path.PathSeparator);
                    foreach (string pathDirectory in pathDirectories)
                    {
                        string tryApp = Path.Combine(pathDirectory, app);
                        if (File.Exists(tryApp))
                        {
                            app = tryApp;
                            existsInPath = true;
                            break;
                        }
                    }
                }

                if (!existsInPath)
                {
                    throw new Exception(string.Format("Couldn't find the executable to run: {0}", app));
                }
            }

            Stopwatch stopwatch = Stopwatch.StartNew();
            if (!options.HasFlag(RunOptions.NoLoggingOfRunCommand))
            {
                var msg = "Running: " + app + (string.IsNullOrEmpty(commandLine) ? "" : " " + commandLine);
                if (options.HasFlag(RunOptions.ConsoleLogOutput))
                    Log.Info(msg);
                else
                    Log.Verbose(msg);
            }

            bool redirectStdOut = (options & RunOptions.NoStdOutRedirect) != RunOptions.NoStdOutRedirect;
            bool shell = options.HasFlag(RunOptions.Shell);

            Process proc = new Process();
            proc.StartInfo.FileName = app;
            proc.StartInfo.Arguments = commandLine != null ? commandLine : "";
            proc.StartInfo.UseShellExecute = shell;
            proc.StartInfo.RedirectStandardInput = input != null;
            proc.StartInfo.CreateNoWindow = true;

            if (workspace != null)
            {
                proc.StartInfo.WorkingDirectory = workspace;
            }

            if (redirectStdOut && !shell)
            {
                proc.StartInfo.RedirectStandardOutput = true;
                proc.StartInfo.RedirectStandardError = true;
                if (options.HasFlag(RunOptions.ConsoleLogOutput))
                {
                    proc.OutputDataReceived += StdLogInfo;
                    proc.ErrorDataReceived += StdLogInfo;
                }
                else
                {
                    proc.OutputDataReceived += StdLogVerbose;
                    proc.ErrorDataReceived += StdLogVerbose;
                }
            }

            if (envVars != null && !shell)
            {
                foreach (var env in envVars)
                {
                    if (env.Key == "PATH")
                    {
                        // Append to path
                        proc.StartInfo.EnvironmentVariables[env.Key] = env.Value + ';' + proc.StartInfo.EnvironmentVariables[env.Key];
                    }
                    else if (env.Value == null)
                    {
                        // Remove variable
                        if (proc.StartInfo.EnvironmentVariables.ContainsKey(env.Key))
                            proc.StartInfo.EnvironmentVariables.Remove(env.Key);
                    }
                    else
                    {
                        // Set variable
                        proc.StartInfo.EnvironmentVariables[env.Key] = env.Value;
                    }
                }
            }

            if ((options & RunOptions.UTF8Output) == RunOptions.UTF8Output)
            {
                proc.StartInfo.StandardOutputEncoding = new UTF8Encoding(false, false);
            }

            proc.Start();

            if (redirectStdOut)
            {
                proc.BeginOutputReadLine();
                proc.BeginErrorReadLine();
            }

            if (string.IsNullOrEmpty(input) == false)
            {
                proc.StandardInput.WriteLine(input);
                proc.StandardInput.Close();
            }

            if (!options.HasFlag(RunOptions.NoWaitForExit))
            {
                proc.WaitForExit();
            }

            int result = -1;

            if (!options.HasFlag(RunOptions.NoWaitForExit))
            {
                stopwatch.Stop();
                result = proc.ExitCode;
                if (!options.HasFlag(RunOptions.NoLoggingOfRunCommand) || options.HasFlag(RunOptions.NoLoggingOfRunDuration))
                {
                    Log.Info(string.Format("Took {0}s to run {1}, ExitCode={2}", stopwatch.Elapsed.TotalSeconds, Path.GetFileName(app), result));
                }
                if (result != 0 && options.HasFlag(RunOptions.ThrowExceptionOnError))
                {
                    var format = options.HasFlag(RunOptions.NoLoggingOfRunCommand) ? "App failed with exit code {2}." : "{0} {1} failed with exit code {2}";
                    throw new Exception(string.Format(format, app, commandLine, result));
                }
            }

            return result;
        }

        /// <summary>
        /// Runs the process and reds its standard output as a string.
        /// </summary>
        /// <param name="filename">The executable file path.</param>
        /// <param name="args">The custom arguments.</param>
        /// <returns>Returned process output.</returns>
        public static string ReadProcessOutput(string filename, string args = null)
        {
            Process p = new Process
            {
                StartInfo =
                {
                    FileName = filename,
                    Arguments = args,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    RedirectStandardOutput = true,
                }
            };
            p.Start();
            return p.StandardOutput.ReadToEnd().Trim();
        }

        /// <summary>
        /// Constructs a relative path from the given base directory.
        /// </summary>
        /// <param name="path">The source path to convert from absolute into a relative format.</param>
        /// <param name="directory">The directory to create a relative path from.</param>
        /// <returns>Thew relative path from the given directory.</returns>
        public static string MakePathRelativeTo(string path, string directory)
        {
            int sharedDirectoryLength = -1;
            for (int i = 0;; i++)
            {
                if (i == path.Length)
                {
                    // Paths are the same
                    if (i == directory.Length)
                    {
                        return string.Empty;
                    }

                    // Finished on a complete directory
                    if (directory[i] == Path.DirectorySeparatorChar)
                    {
                        sharedDirectoryLength = i;
                    }

                    break;
                }

                if (i == directory.Length)
                {
                    // End of the directory name starts with a boundary for the current name
                    if (path[i] == Path.DirectorySeparatorChar)
                    {
                        sharedDirectoryLength = i;
                    }

                    break;
                }

                if (string.Compare(path, i, directory, i, 1, StringComparison.OrdinalIgnoreCase) != 0)
                {
                    break;
                }

                if (path[i] == Path.DirectorySeparatorChar)
                {
                    sharedDirectoryLength = i;
                }
            }

            // No shared path found
            if (sharedDirectoryLength == -1)
            {
                return path;
            }

            // Add all the '..' separators to get back to the shared directory,
            StringBuilder result = new StringBuilder();
            for (int i = sharedDirectoryLength + 1; i < directory.Length; i++)
            {
                // Move up a directory
                result.Append("..");
                result.Append(Path.DirectorySeparatorChar);

                // Scan to the next directory separator
                while (i < directory.Length && directory[i] != Path.DirectorySeparatorChar)
                {
                    i++;
                }
            }

            if (sharedDirectoryLength + 1 < path.Length)
            {
                result.Append(path, sharedDirectoryLength + 1, path.Length - sharedDirectoryLength - 1);
            }

            return result.ToString();
        }

        /// <summary>
        /// Normalizes the path to the standard Flax format (all separators are '/' except for drive 'C:\').
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The normalized path.</returns>
        public static string NormalizePath(string path)
        {
            if (string.IsNullOrEmpty(path))
                return path;
            var chars = path.ToCharArray();

            // Convert all '\' to '/'
            for (int i = 0; i < chars.Length; i++)
            {
                if (chars[i] == '\\')
                    chars[i] = '/';
            }

            // Fix case 'C:/' to 'C:\'
            if (chars.Length > 2 && !char.IsDigit(chars[0]) && chars[1] == ':')
            {
                chars[2] = '\\';
            }

            return new string(chars);
        }

        /// <summary>
        /// Removes the relative parts from the path. For instance it replaces 'xx/yy/../zz' with 'xx/zz'.
        /// </summary>
        /// <param name="path">The input path.</param>
        /// <returns>The output path.</returns>
        public static string RemovePathRelativeParts(string path)
        {
            if (string.IsNullOrEmpty(path))
                return path;

            path = NormalizePath(path);

            string[] components = path.Split('/');

            Stack<string> stack = new Stack<string>();
            foreach (var bit in components)
            {
                if (bit == "..")
                {
                    if (stack.Count != 0)
                    {
                        var popped = stack.Pop();
                        var windowsDriveStart = popped.IndexOf('\\');
                        if (popped == "..")
                        {
                            stack.Push(popped);
                            stack.Push(bit);
                        }
                        else if (windowsDriveStart != -1)
                        {
                            stack.Push(popped.Substring(windowsDriveStart + 1));
                        }
                    }
                    else
                    {
                        stack.Push(bit);
                    }
                }
                else if (bit == ".")
                {
                    // Skip /./
                }
                else
                {
                    stack.Push(bit);
                }
            }

            bool isRooted = path.StartsWith("/");
            string result = string.Join(Path.DirectorySeparatorChar.ToString(), stack.Reverse());
            if (isRooted && result[0] != '/')
                result = result.Insert(0, "/");
            return result;
        }

        /// <summary>
        /// Writes the file contents. Before writing reads the existing file and discards operation if contents are the same.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="contents">The file contents.</param>
        /// <returns>True if file has been modified, otherwise false.</returns>
        public static bool WriteFileIfChanged(string path, string contents)
        {
            if (File.Exists(path))
            {
                string oldContents = null;
                try
                {
                    oldContents = File.ReadAllText(path);
                }
                catch (Exception)
                {
                    Log.Warning(string.Format("Failed to read file contents while trying to save it.", path));
                }

                if (string.Equals(contents, oldContents, StringComparison.OrdinalIgnoreCase))
                {
                    Log.Verbose(string.Format("Skipped saving file to {0}", path));
                    return false;
                }
            }

            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(path));
                File.WriteAllText(path, contents, new UTF8Encoding());
                Log.Verbose(string.Format("Saved file to {0}", path));
            }
            catch
            {
                Log.Error(string.Format("Failed to save file {0}", path));
                throw;
            }

            return true;
        }

        /// <summary>
        /// Replaces the given text with other one in the files.
        /// </summary>
        /// <param name="folderPath">The relative or absolute path to the directory to search.</param>
        /// <param name="searchPattern">The search string to match against the names of files in <paramref name="folderPath" />. This parameter can contain a combination of valid literal path and wildcard (* and ?) characters (see Remarks), but doesn't support regular expressions.</param>
        /// <param name="searchOption">One of the enumeration values that specifies whether the search operation should include all subdirectories or only the current directory.</param>
        /// <param name="findWhat">The text to replace.</param>
        /// <param name="replaceWith">The value to replace to.</param>
        public static void ReplaceInFiles(string folderPath, string searchPattern, SearchOption searchOption, string findWhat, string replaceWith)
        {
            var files = Directory.GetFiles(folderPath, searchPattern, searchOption);
            ReplaceInFiles(files, findWhat, replaceWith);
        }

        /// <summary>
        /// Replaces the given text with other one in the files.
        /// </summary>
        /// <param name="files">The list of the files to process.</param>
        /// <param name="findWhat">The text to replace.</param>
        /// <param name="replaceWith">The value to replace to.</param>
        public static void ReplaceInFiles(string[] files, string findWhat, string replaceWith)
        {
            foreach (var file in files)
            {
                ReplaceInFile(file, findWhat, replaceWith);
            }
        }

        /// <summary>
        /// Replaces the given text with other one in the file.
        /// </summary>
        /// <param name="file">The file to process.</param>
        /// <param name="findWhat">The text to replace.</param>
        /// <param name="replaceWith">The value to replace to.</param>
        public static void ReplaceInFile(string file, string findWhat, string replaceWith)
        {
            var text = File.ReadAllText(file);
            text = text.Replace(findWhat, replaceWith);
            File.WriteAllText(file, text);
        }

        /// <summary>
        /// Returns back the exe ext for the current platform
        /// </summary>
        public static string GetPlatformExecutableExt()
        {
            var extEnding = ".exe";
            if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX) || RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                extEnding = "";
            }
            return extEnding;
        }

        /// <summary>
        /// Sorts the directories by name assuming they contain version text. Sorted from lowest to the highest version.
        /// </summary>
        /// <param name="paths">The paths array to sort.</param>
        public static void SortVersionDirectories(string[] paths)
        {
            if (paths == null || paths.Length == 0)
                return;
            Array.Sort(paths, (a, b) =>
            {
                Version va, vb;
                var fa = Path.GetFileName(a);
                var fb = Path.GetFileName(b);
                if (!string.IsNullOrEmpty(fa))
                    a = fa;
                if (!string.IsNullOrEmpty(fb))
                    b = fb;
                if (Version.TryParse(a, out va))
                {
                    if (Version.TryParse(b, out vb))
                        return va.CompareTo(vb);
                    return 1;
                }
                if (Version.TryParse(b, out vb))
                    return -1;
                return 0;
            });
        }
    }
}
