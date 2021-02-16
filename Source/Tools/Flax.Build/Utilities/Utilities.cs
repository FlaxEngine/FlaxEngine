// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build
{
    /// <summary>
    /// The utilities.
    /// </summary>
    public static class Utilities
    {
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
            return Enumerable.Empty<T>() as T[];
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
            /// The default options.
            /// </summary>
            Default = AppMustExist,
        }

        private static void StdOut(object sender, DataReceivedEventArgs e)
        {
            if (e.Data != null)
            {
                Log.Verbose(e.Data);
            }
        }

        private static void StdErr(object sender, DataReceivedEventArgs e)
        {
            if (e.Data != null)
            {
                Log.Verbose(e.Data);
            }
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
            // Check if the application exists, including the PATH directories.
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

            var startTime = DateTime.UtcNow;
            if (!options.HasFlag(RunOptions.NoLoggingOfRunCommand))
            {
                Log.Verbose("Running: " + app + " " + (string.IsNullOrEmpty(commandLine) ? "" : commandLine));
            }

            bool redirectStdOut = (options & RunOptions.NoStdOutRedirect) != RunOptions.NoStdOutRedirect;

            Process proc = new Process();
            proc.StartInfo.FileName = app;
            proc.StartInfo.Arguments = string.IsNullOrEmpty(commandLine) ? "" : commandLine;
            proc.StartInfo.UseShellExecute = false;
            proc.StartInfo.RedirectStandardInput = input != null;
            proc.StartInfo.CreateNoWindow = true;

            if (workspace != null)
            {
                proc.StartInfo.WorkingDirectory = workspace;
            }

            if (redirectStdOut)
            {
                proc.StartInfo.RedirectStandardOutput = true;
                proc.StartInfo.RedirectStandardError = true;
                proc.OutputDataReceived += StdOut;
                proc.ErrorDataReceived += StdErr;
            }

            if (envVars != null)
            {
                foreach (var env in envVars)
                {
                    if (env.Key == "PATH")
                    {
                        proc.StartInfo.EnvironmentVariables[env.Key] = proc.StartInfo.EnvironmentVariables[env.Key] + ';' + env.Value;
                    }
                    else
                    {
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
                var buildDuration = (DateTime.UtcNow - startTime).TotalMilliseconds;
                result = proc.ExitCode;
                if (!options.HasFlag(RunOptions.NoLoggingOfRunCommand) || options.HasFlag(RunOptions.NoLoggingOfRunDuration))
                {
                    Log.Info(string.Format("Took {0}s to run {1}, ExitCode={2}", buildDuration / 1000, Path.GetFileName(app), result));
                }
            }

            return result;
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
                        if (popped == "..")
                        {
                            stack.Push(popped);
                            stack.Push(bit);
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
    }
}
