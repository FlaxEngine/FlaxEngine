// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace Flax.Stats
{
    /// <summary>
    /// Single code frame data
    /// </summary>
    public sealed class CodeFrameNode : IDisposable
    {
        private static string[] ignoredFolders = new[]
        {
            ".vs",
            ".git",
            "obj",
            "bin",
            "packages",
            "thirdparty",
            "3rdparty",
            "flaxdeps",
            "platforms",
            "flaxapi",
        };

        /// <summary>
        /// Array with all child nodes
        /// </summary>
        public CodeFrameNode[] Children;

        /// <summary>
        /// Amount of lines of code
        /// </summary>
        public long[] LinesOfCode;

        /// <summary>
        /// Size of the node on the hard drive
        /// </summary>
        public long SizeOnDisk;

        /// <summary>
        /// Amount of files
        /// </summary>
        public int FilesCount;

        /// <summary>
        /// Short directory name
        /// </summary>
        public string ShortName;

        /// <summary>
        /// Gets total amount of lines of code in that node and all child nodes
        /// </summary>
        public long TotalLinesOfCode
        {
            get
            {
                long result = 0;
                for (int i = 0; i < (int)Languages.Max; i++)
                {
                    result += LinesOfCode[i];
                }
                for (int i = 0; i < Children.Length; i++)
                {
                    result += Children[i].TotalLinesOfCode;
                }
                return result;
            }
        }

        /// <summary>
        /// Gets total amount of memory used by that node and all child nodes
        /// </summary>
        public long TotaSizeOnDisk
        {
            get
            {
                long result = SizeOnDisk;
                for (int i = 0; i < Children.Length; i++)
                {
                    result += Children[i].TotaSizeOnDisk;
                }
                return result;
            }
        }

        /// <summary>
        /// Gets total amount of files used by that node and all child nodes
        /// </summary>
        public long TotalFiles
        {
            get
            {
                long result = FilesCount;
                for (int i = 0; i < Children.Length; i++)
                {
                    result += Children[i].TotalFiles;
                }
                return result;
            }
        }

        /// <summary>
        /// Gets code frame node with given index
        /// </summary>
        /// <param name="index">Code frame node index</param>
        /// <returns>Code frame node</returns>
        public CodeFrameNode this[int index]
        {
            get { return Children[index]; }
        }

        /// <summary>
        /// Gets code frame node with given name
        /// </summary>
        /// <param name="name">Code frame node index</param>
        /// <returns>Code frame node</returns>
        public CodeFrameNode this[string name]
        {
            get
            {
                for (int i = 0; i < Children.Length; i++)
                    if (Children[i].ShortName == name)
                        return Children[i];
                return null;
            }
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="name">Directory name</param>
        /// <param name="filesCnt">Amount of files</param>
        /// <param name="lines">Lines of code</param>
        /// <param name="size">Size on disk</param>
        /// <param name="children">Child nodes</param>
        public CodeFrameNode(string name, int filesCnt, long[] lines, long size, CodeFrameNode[] children)
        {
            if(lines == null || lines.Length != (int)Languages.Max)
                throw new InvalidDataException();

            ShortName = name;
            FilesCount = filesCnt;
            Children = children;
            LinesOfCode = lines;
            SizeOnDisk = size;
        }

        /// <summary>
        /// Gets total amount of lines of code per language
        /// </summary>
        /// <param name="languge">Language</param>
        /// <returns>Result amount of lines</returns>
        public long GetTotalLinesOfCode(Languages languge)
        {
            long result = 0;
            result += LinesOfCode[(int)languge];
            for (int i = 0; i < Children.Length; i++)
            {
                result += Children[i].GetTotalLinesOfCode(languge);
            }
            return result;
        }

        /// <summary>
        /// Capture data from the folder
        /// </summary>
        /// <param name="folder">Folder path to capture</param>
        /// <returns>Created code frame of null if cannot create that</returns>
        public static CodeFrameNode Capture(DirectoryInfo folder)
        {
            // Get all files from that folder
            FileInfo[] files;
            try
            {
                files = folder.GetFiles("*.*");
            }
            catch (UnauthorizedAccessException)
            {
                return null;
            }
            catch (Exception exception)
            {
                MessageBox.Show(exception.Message);
                return null;
            }
            
            // Count data
            long[] lines = new long[(int)Languages.Max];
            for (int i = 0; i < (int)Languages.Max; i++)
            {
                lines[i] = 0;
            }
            long size = 0;
            for (int i = 0; i < files.Length; i++)
            {
                // Count files size
                size += files[i].Length;

                // Get file type index
                int index = -1;
                switch (files[i].Extension)
                {
                    // C++
                    case ".h":
                    case ".cpp":
                    case ".cxx":
                    case ".hpp":
                    case ".hxx":
                    case ".c":
                        index = (int)Languages.Cpp;
                        break;

                    // C#
                    case ".cs":
                        index = (int)Languages.CSharp;
                        break;

                    // HLSL
                    case ".hlsl":
                    case ".shader":
                        index = (int)Languages.Hlsl;
                        break;
                }
                if (index == -1)
                    continue;

                // Count lines in the source file
                long ll = 1;
                using (StreamReader streamReader = new StreamReader(files[i].FullName))
                {
                    while (streamReader.ReadLine() != null)
                    {
                        ll++;
                    }
                }
                lines[index] += ll;
            }

            // Get all directories
            DirectoryInfo[] directories = folder.GetDirectories();
            if (directories.Length == 0 && files.Length == 0)
            {
                // Empty folder
                return null;
            }

            // Process child nodes
            List<CodeFrameNode> children = new List<CodeFrameNode>(directories.Length);
            for (int i = 0; i < directories.Length; i++)
            {
                // Validate name
                if(ignoredFolders.Contains(directories[i].Name.ToLower()))
                    continue;

                var child = Capture(directories[i]);
                if (child != null)
                {
                    children.Add(child);
                }
            }
            
            // Create node
            return new CodeFrameNode(folder.Name, files.Length, lines, size, children.ToArray());
        }

        public void CleanupDirectories()
        {
            var chld = Children.ToList();
            chld.RemoveAll(e => ignoredFolders.Contains(e.ShortName.ToLower()));
            Children = chld.ToArray();

            foreach (var a in Children)
            {
                a.CleanupDirectories();
            }
        }

        /// <summary>
        /// Clean all data
        /// </summary>
        public void Dispose()
        {
            if (Children != null)
            {
                // Clear
                for (int i = 0; i < Children.Length; i++)
                {
                    Children[i].Dispose();
                }
                Children = null;
            }
        }

        /// <summary>
        /// To string
        /// </summary>
        /// <returns>String</returns>
        public override string ToString()
        {
            return ShortName;
        }
    }
}
