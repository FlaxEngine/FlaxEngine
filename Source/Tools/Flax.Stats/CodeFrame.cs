// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Stats
{
    /// <summary>
    /// Represents single Code Frame captured to the file
    /// </summary>
    public sealed class CodeFrame : IDisposable
    {
        /// <summary>
        /// Root node for code frames
        /// </summary>
        public CodeFrameNode Root;

        /// <summary>
        /// Code frame date
        /// </summary>
        public DateTime Date;

        /// <summary>
        /// Capture code stats in current working directory
        /// </summary>
        /// <param name="directory">Directory path to capture</param>
        /// <returns>Created code frame</returns>
        public static CodeFrame Capture(string directory)
        {
            // Create code frame
            var frame = new CodeFrame();

            // Create root node and all children
            frame.Root = CodeFrameNode.Capture(new DirectoryInfo(directory));

            // Save capture time
            frame.Date = DateTime.Now;

            // Return result
            return frame;
        }

        /// <summary>
        /// Load code frame from the file
        /// </summary>
        /// <param name="path">Path of the file to load</param>
        /// <returns>Loaded code frame</returns>
        public static CodeFrame Load(string path)
        {
            // Create and load
            var frame = new CodeFrame();
            using (var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                frame.load(fs);
            }
            return frame;
        }

        private void load(Stream stream)
        {
            // Clear state
            if (Root != null)
            {
                Root.Dispose();
                Root = null;
            }

            // Header
            if (stream.ReadInt() != 1032)
            {
                throw new FileLoadException("Invalid Code Frame file.");
            }

            // Version
            int version = stream.ReadInt();

            // Switch version
            switch (version)
            {
                case 1:
                {
                    // Read root node
                    Root = loadNode1(stream);

                    break;
                }

                case 2:
                {
                    // Read date
                    Date = stream.ReadDateTime();

                    // Read root node
                    Root = loadNode1(stream);

                    break;
                }

                case 3:
                {
                    // Read date
                    Date = stream.ReadDateTime();

                    // Read root node
                    Root = loadNode3(stream);

                    break;
                }

                case 4:
                {
                    // Read date
                    Date = stream.ReadDateTime();

                    // Read root node
                    Root = loadNode4(stream);

                    break;
                }

                default:
                    throw new FileLoadException("Unknown Code Frame file version.");
            }

            // Ending char
            if(stream.ReadByte() != 99)
            {
                throw new FileLoadException("Code Frame file. is corrupted");
            }
        }

        /// <summary>
        /// Save to the file
        /// </summary>
        /// <param name="path">File path</param>
        public void Save(string path)
        {
            using (var fs = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                Save(fs);
            }
        }

        /// <summary>
        /// Save to the stream
        /// </summary>
        /// <param name="stream">Stream to write to</param>
        private void Save(Stream stream)
        {
            // Header
            stream.Write(1032);

            // Version
            stream.Write(4);

            // Date
            stream.Write(Date);

            // Write root node
            saveNode4(stream, Root);

            // Ending char
            stream.WriteByte(99);
        }

        #region Version 1 and 2

        private void saveNode1(Stream stream, CodeFrameNode node)
        {
            // Save node data
            stream.Write(node.FilesCount);
            stream.Write(node.LinesOfCode[0]);
            stream.Write(node.LinesOfCode[1]);
            stream.WriteUTF8(node.ShortName, 13);
            stream.Write(node.SizeOnDisk);
            stream.Write(node.Children.Length);

            // Save all child nodes
            for (int i = 0; i < node.Children.Length; i++)
            {
                saveNode1(stream, node.Children[i]);
            }
        }

        private CodeFrameNode loadNode1(Stream stream)
        {
            // Load node data
            int files = stream.ReadInt();
            long[] lines =
            {
                stream.ReadLong(),
                stream.ReadLong(),
                0,
                0
            };
            string name = stream.ReadStringUTF8(13);
            long size = stream.ReadLong();
            int childrenCount = stream.ReadInt();

            // Load all child nodes
            CodeFrameNode[] children = new CodeFrameNode[childrenCount];
            for (int i = 0; i < childrenCount; i++)
            {
                children[i] = loadNode1(stream);
            }

            // Create node
            return new CodeFrameNode(name, files, lines, size, children);
        }

        #endregion

        #region Version 3

        private void saveNode3(Stream stream, CodeFrameNode node)
        {
            // Save node data
            stream.Write(node.FilesCount);
            stream.Write(node.LinesOfCode[0]);
            stream.Write(node.LinesOfCode[1]);
            stream.Write(node.LinesOfCode[2]);
            stream.WriteUTF8(node.ShortName, 13);
            stream.Write(node.SizeOnDisk);
            stream.Write(node.Children.Length);

            // Save all child nodes
            for (int i = 0; i < node.Children.Length; i++)
            {
                saveNode3(stream, node.Children[i]);
            }
        }

        private CodeFrameNode loadNode3(Stream stream)
        {
            // Load node data
            int files = stream.ReadInt();
            long[] lines =
            {
                stream.ReadLong(),
                stream.ReadLong(),
                stream.ReadLong(),
                0,
            };
            string name = stream.ReadStringUTF8(13);
            long size = stream.ReadLong();
            int childrenCount = stream.ReadInt();

            // Load all child nodes
            CodeFrameNode[] children = new CodeFrameNode[childrenCount];
            for (int i = 0; i < childrenCount; i++)
            {
                children[i] = loadNode3(stream);
            }

            // Create node
            return new CodeFrameNode(name, files, lines, size, children);
        }

        #endregion

        #region Version 4

        private void saveNode4(Stream stream, CodeFrameNode node)
        {
            // Save node data
            stream.Write(node.FilesCount);
            stream.Write(node.LinesOfCode[0]);
            stream.Write(node.LinesOfCode[1]);
            stream.Write(node.LinesOfCode[2]);
            stream.Write(node.LinesOfCode[3]);
            stream.WriteUTF8(node.ShortName, 13);
            stream.Write(node.SizeOnDisk);
            stream.Write(node.Children.Length);

            // Save all child nodes
            for (int i = 0; i < node.Children.Length; i++)
            {
                saveNode3(stream, node.Children[i]);
            }
        }

        private CodeFrameNode loadNode4(Stream stream)
        {
            // Load node data
            int files = stream.ReadInt();
            long[] lines =
            {
                stream.ReadLong(),
                stream.ReadLong(),
                stream.ReadLong(),
                stream.ReadLong()
            };
            string name = stream.ReadStringUTF8(13);
            long size = stream.ReadLong();
            int childrenCount = stream.ReadInt();

            // Load all child nodes
            CodeFrameNode[] children = new CodeFrameNode[childrenCount];
            for (int i = 0; i < childrenCount; i++)
            {
                children[i] = loadNode3(stream);
            }

            // Create node
            return new CodeFrameNode(name, files, lines, size, children);
        }

        #endregion

        /// <summary>
        /// Clean all data
        /// </summary>
        public void Dispose()
        {
            // Dispose root
            if (Root != null)
            {
                Root.Dispose();
                Root = null;
            }
        }
    }
}
