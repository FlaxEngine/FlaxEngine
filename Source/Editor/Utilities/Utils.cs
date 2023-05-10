// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Globalization;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tree;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    partial struct TextureGroup
    {
        internal bool IsAnisotropic => SamplerFilter == GPUSamplerFilter.Anisotropic;
    }
}

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Editor utilities and helper functions.
    /// </summary>
    public static class Utils
    {
        private static readonly StringBuilder CachedSb = new StringBuilder(256);

        private static readonly string[] MemorySizePostfixes =
        {
            "B",
            "kB",
            "MB",
            "GB",
            "TB",
            "PB"
        };

        /// <summary>
        /// The name of the Flax Engine C# assembly name.
        /// </summary>
        public static readonly string FlaxEngineAssemblyName = "FlaxEngine.CSharp";

        /// <summary>
        /// Formats the amount of bytes to get a human-readable data size in bytes with abbreviation. Eg. 32 kB
        /// </summary>
        /// <param name="bytes">The bytes.</param>
        /// <returns>The formatted amount of bytes.</returns>
        public static string FormatBytesCount(int bytes)
        {
            int order = 0;
            while (bytes >= 1024 && order < MemorySizePostfixes.Length - 1)
            {
                order++;
                bytes /= 1024;
            }

            return string.Format("{0:0.##} {1}", bytes, MemorySizePostfixes[order]);
        }

        /// <summary>
        /// Formats the amount of bytes to get a human-readable data size in bytes with abbreviation. Eg. 32 kB
        /// </summary>
        /// <param name="bytes">The bytes.</param>
        /// <returns>The formatted amount of bytes.</returns>
        public static string FormatBytesCount(ulong bytes)
        {
            int order = 0;
            while (bytes >= 1024 && order < MemorySizePostfixes.Length - 1)
            {
                order++;
                bytes /= 1024;
            }

            return string.Format("{0:0.##} {1}", bytes, MemorySizePostfixes[order]);
        }

        internal static string GetTooltip(SceneObject obj)
        {
            var actor = obj as Actor;
            var str = actor != null ? actor.Name : TypeUtils.GetObjectType(obj).Name;
            var o = obj.Parent;
            while (o)
            {
                str = o.Name + " -> " + str;
                o = o.Parent;
            }
            if (actor != null)
                str += string.Format(" ({0})", TypeUtils.GetObjectType(obj).Name);
            return str;
        }

        /// <summary>
        /// The colors for the keyframes used by the curve editor.
        /// </summary>
        internal static readonly Color[] CurveKeyframesColors =
        {
            Color.OrangeRed,
            Color.ForestGreen,
            Color.CornflowerBlue,
            Color.White,
        };

        /// <summary>
        /// The time/value axes tick steps for editors with timeline.
        /// </summary>
        internal static readonly float[] CurveTickSteps =
        {
            0.0000001f, 0.0000005f, 0.000001f, 0.000005f, 0.00001f,
            0.00005f, 0.0001f, 0.0005f, 0.001f, 0.005f,
            0.01f, 0.05f, 0.1f, 0.5f, 1,
            5, 10, 50, 100, 500,
            1000, 5000, 10000, 50000, 100000,
            500000, 1000000, 5000000, 10000000, 100000000
        };

        /// <summary>
        /// Determines whether the specified path string contains any invalid character.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns><c>true</c> if the given string cannot be used as a path because it contains one or more illegal characters; otherwise, <c>false</c>.</returns>
        public static bool HasInvalidPathChar(string path)
        {
            char[] illegalChars =
            {
                '?',
                '\\',
                '/',
                '\"',
                '<',
                '>',
                '|',
                ':',
                '*',
                '\u0001',
                '\u0002',
                '\u0003',
                '\u0004',
                '\u0005',
                '\u0006',
                '\a',
                '\b',
                '\t',
                '\n',
                '\v',
                '\f',
                '\r',
                '\u000E',
                '\u000F',
                '\u0010',
                '\u0011',
                '\u0012',
                '\u0013',
                '\u0014',
                '\u0015',
                '\u0016',
                '\u0017',
                '\u0018',
                '\u0019',
                '\u001A',
                '\u001B',
                '\u001C',
                '\u001D',
                '\u001E',
                '\u001F'
            };
            return path.IndexOfAny(illegalChars) != -1;
        }

        internal static Transform[] GetTransformsAndBounds(List<SceneGraphNode> nodes, out BoundingBox bounds)
        {
            var transforms = new Transform[nodes.Count];
            bounds = BoundingBox.Empty;
            for (int i = 0; i < nodes.Count; i++)
            {
                transforms[i] = nodes[i].Transform;
                if (nodes[i] is ActorNode actorNode)
                {
                    bounds = BoundingBox.Merge(bounds, actorNode.Actor.BoxWithChildren);
                }
            }
            return transforms;
        }

        /// <summary>
        /// Removes the file if it exists.
        /// </summary>
        /// <param name="file">The file path.</param>
        public static void RemoveFileIfExists(string file)
        {
            if (File.Exists(file))
                File.Delete(file);
        }

        /// <summary>
        /// Copies the directory. Supports subdirectories copy with files override option.
        /// </summary>
        /// <param name="srcDirectoryPath">The source directory path.</param>
        /// <param name="dstDirectoryPath">The destination directory path.</param>
        /// <param name="copySubDirs">If set to <c>true</c> copy subdirectories.</param>
        /// <param name="overrideFiles">if set to <c>true</c> override existing files.</param>
        public static void DirectoryCopy(string srcDirectoryPath, string dstDirectoryPath, bool copySubDirs = true, bool overrideFiles = false)
        {
            var dir = new DirectoryInfo(srcDirectoryPath);
            if (!dir.Exists)
                throw new DirectoryNotFoundException("Missing source directory to copy. " + srcDirectoryPath);
            DirectoryCopy(dir, dstDirectoryPath, copySubDirs, overrideFiles);
        }

        private static void DirectoryCopy(DirectoryInfo dir, string dstDirectoryPath, bool copySubDirs = true, bool overrideFiles = false)
        {
            if (!Directory.Exists(dstDirectoryPath))
            {
                Directory.CreateDirectory(dstDirectoryPath);
            }

            var files = dir.GetFiles();
            for (int i = 0; i < files.Length; i++)
            {
                string tmp = Path.Combine(dstDirectoryPath, files[i].Name);
                files[i].CopyTo(tmp, overrideFiles);
            }

            if (copySubDirs)
            {
                var dirs = dir.GetDirectories();
                for (int i = 0; i < dirs.Length; i++)
                {
                    string tmp = Path.Combine(dstDirectoryPath, dirs[i].Name);
                    DirectoryCopy(dirs[i], tmp, true, overrideFiles);
                }
            }
        }

        /// <summary>
        /// Converts the raw bytes into the structure. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <typeparam name="T">The structure type.</typeparam>
        /// <param name="bytes">The data bytes.</param>
        /// <returns>The structure.</returns>
        public static T ByteArrayToStructure<T>(byte[] bytes) where T : struct
        {
            // #stupid c#
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            T stuff = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();
            return stuff;
        }

        /// <summary>
        /// Converts the raw bytes into the structure. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <param name="bytes">The data bytes.</param>
        /// <param name="type">The structure type.</param>
        /// <returns>The structure.</returns>
        public static object ByteArrayToStructure(byte[] bytes, Type type)
        {
            // #stupid c#
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            object stuff = Marshal.PtrToStructure(handle.AddrOfPinnedObject(), type);
            handle.Free();
            return stuff;
        }

        /// <summary>
        /// Converts the raw bytes into the structure. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <typeparam name="T">The structure type.</typeparam>
        /// <param name="bytes">The data bytes.</param>
        /// <param name="result">The result.</param>
        public static void ByteArrayToStructure<T>(byte[] bytes, out T result) where T : struct
        {
            // #stupid c#
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            result = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();
        }

        /// <summary>
        /// Converts the structure to the raw bytes. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <typeparam name="T">The structure type.</typeparam>
        /// <param name="value">The structure value.</param>
        /// <returns>The bytes array that contains a structure data.</returns>
        public static byte[] StructureToByteArray<T>(ref T value) where T : struct
        {
            // #stupid c#
            int size = Marshal.SizeOf(typeof(T));
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(value, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        /// <summary>
        /// Converts the structure to the raw bytes. Supported only for structures with simple types and no GC objects.
        /// </summary>
        /// <param name="value">The structure value.</param>
        /// <param name="type">The structure type.</param>
        /// <returns>The bytes array that contains a structure data.</returns>
        public static byte[] StructureToByteArray(object value, Type type)
        {
            // #stupid c#
            int size = Marshal.SizeOf(type);
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(value, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        internal static unsafe string ReadStr(this BinaryReader stream, int check)
        {
            int length = stream.ReadInt32();
            if (length > 0 && length < 4 * 1024)
            {
                var str = stream.ReadBytes(length * 2);
                fixed (byte* strPtr = str)
                {
                    var ptr = (char*)strPtr;
                    for (int j = 0; j < length; j++)
                        ptr[j] = (char)(ptr[j] ^ check);
                }
                return System.Text.Encoding.Unicode.GetString(str);
            }
            return string.Empty;
        }

        internal static unsafe string ReadStrAnsi(this BinaryReader stream, int check)
        {
            int length = stream.ReadInt32();
            if (length > 0 && length < 4 * 1024)
            {
                var str = stream.ReadBytes(length);
                fixed (byte* strPtr = str)
                {
                    var ptr = strPtr;
                    for (int j = 0; j < length; j++)
                        ptr[j] = (byte)(ptr[j] ^ check);
                }
                return System.Text.Encoding.ASCII.GetString(str);
            }
            return string.Empty;
        }

        internal static unsafe void WriteStr(this BinaryWriter stream, string str, int check)
        {
            int length = str?.Length ?? 0;
            stream.Write(length);
            if (length == 0)
                return;
            var bytes = System.Text.Encoding.Unicode.GetBytes(str);
            if (bytes.Length != length * 2)
                throw new ArgumentException();
            fixed (byte* bytesPtr = bytes)
            {
                var ptr = (char*)bytesPtr;
                for (int j = 0; j < length; j++)
                    ptr[j] = (char)(ptr[j] ^ check);
            }
            stream.Write(bytes);
        }

        internal static unsafe void WriteStrAnsi(this BinaryWriter stream, string str, int check)
        {
            int length = str?.Length ?? 0;
            stream.Write(length);
            if (length == 0)
                return;
            var bytes = System.Text.Encoding.ASCII.GetBytes(str);
            if (bytes.Length != length)
                throw new ArgumentException();
            fixed (byte* bytesPtr = bytes)
            {
                var ptr = bytesPtr;
                for (int j = 0; j < length; j++)
                    ptr[j] = (byte)(ptr[j] ^ check);
            }
            stream.Write(bytes);
        }

        internal static Guid ReadGuid(this BinaryReader stream)
        {
            // TODO: use static bytes array to reduce dynamic allocs
            return new Guid(stream.ReadBytes(16));
        }

        internal static void WriteGuid(this BinaryWriter stream, ref Guid value)
        {
            // TODO: use static bytes array to reduce dynamic allocs
            stream.Write(value.ToByteArray());
        }

        internal static Rectangle ReadRectangle(this BinaryReader stream)
        {
            return new Rectangle(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
        }

        internal static void WriteRectangle(this BinaryWriter stream, ref Rectangle value)
        {
            stream.Write(value.Location.X);
            stream.Write(value.Location.Y);
            stream.Write(value.Size.X);
            stream.Write(value.Size.Y);
        }

        internal static void ReadCommonValue(this BinaryReader stream, ref object value)
        {
            byte type = stream.ReadByte();

            switch (type)
            {
            case 0: // CommonType::Bool:
                value = stream.ReadByte() != 0;
                break;
            case 1: // CommonType::Integer:
            {
                value = stream.ReadInt32();
            }
                break;
            case 2: // CommonType::Float:
            {
                value = stream.ReadSingle();
            }
                break;
            case 3: // CommonType::Vector2:
            {
                value = stream.ReadFloat2();
            }
                break;
            case 4: // CommonType::Vector3:
            {
                value = stream.ReadFloat3();
            }
                break;
            case 5: // CommonType::Vector4:
            {
                value = stream.ReadFloat4();
            }
                break;
            case 6: // CommonType::Color:
            {
                value = stream.ReadColor();
            }
                break;
            case 7: // CommonType::Guid:
            {
                value = stream.ReadGuid();
            }
                break;
            case 8: // CommonType::String:
            {
                int length = stream.ReadInt32();
                if (length <= 0)
                {
                    value = string.Empty;
                }
                else
                {
                    var data = new char[length];
                    for (int i = 0; i < length; i++)
                    {
                        var c = stream.ReadUInt16();
                        data[i] = (char)(c ^ 953);
                    }
                    value = new string(data);
                }
                break;
            }
            case 9: // CommonType::Box:
            {
                value = new BoundingBox(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                        new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()));
            }
                break;
            case 10: // CommonType::Rotation:
            {
                value = stream.ReadQuaternion();
            }
                break;
            case 11: // CommonType::Transform:
            {
                value = new Transform(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                      new Quaternion(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                      new Float3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()));
            }
                break;
            case 12: // CommonType::Sphere:
            {
                value = new BoundingSphere(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                           stream.ReadSingle());
            }
                break;
            case 13: // CommonType::Rect:
            {
                value = new Rectangle(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
            }
                break;
            case 15: // CommonType::Matrix
            {
                value = new Matrix(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(),
                                   stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(),
                                   stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(),
                                   stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle());
                break;
            }
            case 16: // CommonType::Blob
            {
                int length = stream.ReadInt32();
                value = stream.ReadBytes(length);
                break;
            }
            case 18: // CommonType::Ray
            {
                value = new Ray(new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()),
                                new Vector3(stream.ReadSingle(), stream.ReadSingle(), stream.ReadSingle()));
                break;
            }
            case 19: // CommonType::Int2
            {
                value = stream.ReadInt2();
                break;
            }
            case 20: // CommonType::Int3
            {
                value = stream.ReadInt3();
                break;
            }
            case 21: // CommonType::Int4
            {
                value = stream.ReadInt4();
                break;
            }
            default: throw new SystemException();
            }
        }

        internal static void WriteCommonValue(this BinaryWriter stream, object value)
        {
            if (value is bool asBool)
            {
                stream.Write((byte)0);
                stream.Write((byte)(asBool ? 1 : 0));
            }
            else if (value is int asInt)
            {
                stream.Write((byte)1);
                stream.Write(asInt);
            }
            else if (value is long asLong)
            {
                stream.Write((byte)1);
                stream.Write((int)asLong);
            }
            else if (value is float asFloat)
            {
                stream.Write((byte)2);
                stream.Write(asFloat);
            }
            else if (value is double asDouble)
            {
                stream.Write((byte)2);
                stream.Write((float)asDouble);
            }
            else if (value is Vector2 asVector2)
            {
                stream.Write((byte)3);
                stream.Write((float)asVector2.X);
                stream.Write((float)asVector2.Y);
            }
            else if (value is Vector3 asVector3)
            {
                stream.Write((byte)4);
                stream.Write((float)asVector3.X);
                stream.Write((float)asVector3.Y);
                stream.Write((float)asVector3.Z);
            }
            else if (value is Vector4 asVector4)
            {
                stream.Write((byte)5);
                stream.Write((float)asVector4.X);
                stream.Write((float)asVector4.Y);
                stream.Write((float)asVector4.Z);
                stream.Write((float)asVector4.W);
            }
            else if (value is Color asColor)
            {
                stream.Write((byte)6);
                stream.Write(asColor.R);
                stream.Write(asColor.G);
                stream.Write(asColor.B);
                stream.Write(asColor.A);
            }
            else if (value is Guid asGuid)
            {
                stream.Write((byte)7);
                stream.WriteGuid(ref asGuid);
            }
            else if (value is string asString)
            {
                stream.Write((byte)8);
                stream.Write(asString.Length);
                for (int i = 0; i < asString.Length; i++)
                    stream.Write((ushort)(asString[i] ^ -14));
            }
            else if (value is BoundingBox asBox)
            {
                stream.Write((byte)9);
                stream.Write((float)asBox.Minimum.X);
                stream.Write((float)asBox.Minimum.Y);
                stream.Write((float)asBox.Minimum.Z);
                stream.Write((float)asBox.Maximum.X);
                stream.Write((float)asBox.Maximum.Y);
                stream.Write((float)asBox.Maximum.Z);
            }
            else if (value is Quaternion asRotation)
            {
                stream.Write((byte)10);
                stream.Write(asRotation.X);
                stream.Write(asRotation.Y);
                stream.Write(asRotation.Z);
                stream.Write(asRotation.X);
            }
            else if (value is Transform asTransform)
            {
                stream.Write((byte)11);
                stream.Write((float)asTransform.Translation.X);
                stream.Write((float)asTransform.Translation.Y);
                stream.Write((float)asTransform.Translation.Z);
                stream.Write(asTransform.Orientation.X);
                stream.Write(asTransform.Orientation.Y);
                stream.Write(asTransform.Orientation.Z);
                stream.Write(asTransform.Orientation.X);
                stream.Write(asTransform.Scale.X);
                stream.Write(asTransform.Scale.Y);
                stream.Write(asTransform.Scale.Z);
            }
            else if (value is BoundingSphere asSphere)
            {
                stream.Write((byte)12);
                stream.Write((float)asSphere.Center.X);
                stream.Write((float)asSphere.Center.Y);
                stream.Write((float)asSphere.Center.Z);
                stream.Write((float)asSphere.Radius);
            }
            else if (value is Rectangle asRect)
            {
                stream.Write((byte)13);
                stream.Write(asRect.Location.X);
                stream.Write(asRect.Location.Y);
                stream.Write(asRect.Size.X);
                stream.Write(asRect.Size.Y);
            }
            else if (value is Matrix asMatrix)
            {
                stream.Write((byte)15);
                stream.Write(asMatrix.M11);
                stream.Write(asMatrix.M12);
                stream.Write(asMatrix.M13);
                stream.Write(asMatrix.M14);
                stream.Write(asMatrix.M21);
                stream.Write(asMatrix.M22);
                stream.Write(asMatrix.M23);
                stream.Write(asMatrix.M24);
                stream.Write(asMatrix.M31);
                stream.Write(asMatrix.M32);
                stream.Write(asMatrix.M33);
                stream.Write(asMatrix.M34);
                stream.Write(asMatrix.M41);
                stream.Write(asMatrix.M42);
                stream.Write(asMatrix.M43);
                stream.Write(asMatrix.M44);
            }
            else if (value is byte[] asBlob)
            {
                stream.Write((byte)16);
                stream.Write(asBlob.Length);
                stream.Write(asBlob);
            }
            else if (value is Ray asRay)
            {
                stream.Write((byte)18);
                stream.Write((float)asRay.Position.X);
                stream.Write((float)asRay.Position.Y);
                stream.Write((float)asRay.Position.Z);
                stream.Write((float)asRay.Direction.X);
                stream.Write((float)asRay.Direction.Y);
                stream.Write((float)asRay.Direction.Z);
            }
            else if (value is Int2 asInt2)
            {
                stream.Write((byte)19);
                stream.Write(asInt2);
            }
            else if (value is Int3 asInt3)
            {
                stream.Write((byte)20);
                stream.Write(asInt3);
            }
            else if (value is Int4 asInt4)
            {
                stream.Write((byte)21);
                stream.Write(asInt4);
            }
            else
            {
                throw new NotSupportedException(string.Format("Invalid Common Value type {0}", value != null ? value.GetType().ToString() : "null"));
            }
        }

        /// <summary>
        /// Shows the source code window.
        /// </summary>
        /// <param name="source">The source code.</param>
        /// <param name="title">The window title.</param>
        /// <param name="parentWindow">The optional parent window.</param>
        public static void ShowSourceCodeWindow(string source, string title, Window parentWindow = null)
        {
            if (string.IsNullOrEmpty(source))
            {
                MessageBox.Show("No generated shader source code.", "No source.");
                return;
            }

            var settings = CreateWindowSettings.Default;
            settings.ActivateWhenFirstShown = true;
            settings.AllowMaximize = true;
            settings.AllowMinimize = false;
            settings.HasSizingFrame = true;
            settings.HasBorder = true;
            settings.StartPosition = WindowStartPosition.CenterParent;
            settings.Size = new Float2(500, 600) * (parentWindow?.DpiScale ?? Platform.DpiScale);
            settings.Parent = parentWindow;
            settings.Title = title;
            var dialog = Platform.CreateWindow(ref settings);

            var copyButton = new Button(4, 4, 100)
            {
                Text = "Copy",
                Parent = dialog.GUI,
            };
            copyButton.Clicked += () => Clipboard.Text = source;

            var backPanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, copyButton.Bottom + 4, 0),
                ScrollBars = ScrollBars.Both,
                IsScrollable = true,
                Parent = dialog.GUI,
            };

            var sourceTextBox = new TextBox(true, 0, 0, 0);
            sourceTextBox.Parent = backPanel;
            sourceTextBox.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            sourceTextBox.Text = source;
            sourceTextBox.Height = sourceTextBox.TextSize.Y;
            sourceTextBox.IsReadOnly = true;
            sourceTextBox.IsMultilineScrollable = false;
            sourceTextBox.IsScrollable = true;

            backPanel.SizeChanged += control => { sourceTextBox.Width = (control.Size.X >= sourceTextBox.TextSize.X) ? control.Width : sourceTextBox.TextSize.X + 30; };

            dialog.Show();
            dialog.Focus();
        }

        private static OrientedBoundingBox GetWriteBox(ref Vector3 min, ref Vector3 max, Real margin)
        {
            var box = new OrientedBoundingBox();
            Vector3 vec = max - min;
            Vector3 dir = Vector3.Normalize(vec);
            Quaternion orientation;
            if (Vector3.Dot(dir, Vector3.Up) >= 0.999f)
                orientation = Quaternion.RotationAxis(Vector3.Left, Mathf.PiOverTwo);
            else
                orientation = Quaternion.LookRotation(dir, Vector3.Cross(Vector3.Cross(dir, Vector3.Up), dir));
            Vector3 up = Vector3.Up * orientation;
            Matrix world = Matrix.CreateWorld(min + vec * 0.5f, dir, up);
            world.Decompose(out box.Transformation);
            Matrix.Invert(ref world, out Matrix invWorld);
            Vector3 vecLocal = Vector3.TransformNormal(vec * 0.5f, invWorld);
            box.Extents.X = margin;
            box.Extents.Y = margin;
            box.Extents.Z = vecLocal.Z;
            return box;
        }

        /// <summary>
        /// Checks if given ray intersects with the oriented bounding wire box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="ray">The ray.</param>
        /// <param name="distance">The result intersection distance.</param>
        /// <param name="viewPosition">The view position used to scale the wires thickness depending on the wire distance from the view.</param>
        /// <returns>True ray hits bounds, otherwise false.</returns>
        public static unsafe bool RayCastWire(ref OrientedBoundingBox box, ref Ray ray, out Real distance, ref Vector3 viewPosition)
        {
            var corners = stackalloc Vector3[8];
            box.GetCorners(corners);

            var minDistance = Vector3.DistanceSquared(ref viewPosition, ref corners[0]);
            for (int i = 1; i < 8; i++)
                minDistance = Mathf.Min(minDistance, Vector3.DistanceSquared(ref viewPosition, ref corners[i]));
            minDistance = Mathf.Sqrt(minDistance);
            var margin = Mathf.Clamp(minDistance / 80.0f, 0.1f, 100.0f);

            if (GetWriteBox(ref corners[0], ref corners[1], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[0], ref corners[3], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[0], ref corners[4], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[1], ref corners[2], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[1], ref corners[5], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[2], ref corners[3], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[2], ref corners[6], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[3], ref corners[7], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[4], ref corners[5], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[4], ref corners[7], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[5], ref corners[6], margin).Intersects(ref ray, out distance) ||
                GetWriteBox(ref corners[6], ref corners[7], margin).Intersects(ref ray, out distance))
                return true;

            distance = 0;
            return false;
        }

        /// <summary>
        /// Initializes the object fields and properties with their default values based on <see cref="System.ComponentModel.DefaultValueAttribute"/>.
        /// </summary>
        /// <param name="obj">The object.</param>
        public static void InitDefaultValues(object obj)
        {
            var scriptType = TypeUtils.GetObjectType(obj);
            if (!scriptType)
                return;
            var isStructure = scriptType.IsStructure;

            var fields = scriptType.GetFields(BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public);
            for (var i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                var attr = field.GetAttribute<System.ComponentModel.DefaultValueAttribute>();
                if (attr != null)
                {
                    // Prevent value type conflicts
                    var value = attr.Value;
                    var fieldType = field.ValueType.Type;
                    if (value != null && value.GetType() != fieldType)
                        value = Convert.ChangeType(value, fieldType);

                    field.SetValue(obj, value);
                }
                else if (isStructure)
                {
                    // C# doesn't support default values for structure members so initialize them
                    field.SetValue(obj, TypeUtils.GetDefaultValue(field.ValueType));
                }
            }

            var properties = scriptType.GetProperties(BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public);
            for (var i = 0; i < properties.Length; i++)
            {
                var property = properties[i];
                var attr = property.GetAttribute<System.ComponentModel.DefaultValueAttribute>();
                if (attr != null)
                {
                    // Prevent value type conflicts
                    var value = attr.Value;
                    var propertyType = property.ValueType.Type;
                    if (value != null && value.GetType() != propertyType)
                        value = Convert.ChangeType(value, propertyType);

                    property.SetValue(obj, value);
                }
            }
        }

        /// <summary>
        /// Gets the property name for UI. Removes unnecessary characters and filters text. Makes it more user-friendly.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The result.</returns>
        public static string GetPropertyNameUI(string name)
        {
            int length = name.Length;
            StringBuilder sb = CachedSb;
            sb.Clear();
            sb.EnsureCapacity(length + 8);
            int startIndex = 0;

            // Skip some prefixes
            if (name.StartsWith("g_") || name.StartsWith("m_"))
                startIndex = 2;

            if (name.StartsWith("_"))
                startIndex = 1;

            // Filter text
            var lastChar = '\0';
            for (int i = startIndex; i < length; i++)
            {
                var c = name[i];

                if (i == startIndex)
                {
                    sb.Append(char.ToUpper(c));
                    continue;
                }

                // Space before word starting with uppercase letter
                if (char.IsUpper(c) && i > 0)
                {
                    if (lastChar != ' ' && (!char.IsUpper(name[i - 1]) || (i + 1 != length && !char.IsUpper(name[i + 1]))))
                    {
                        lastChar = ' ';
                        sb.Append(lastChar);
                    }
                }
                // Space instead of underscore
                else if (c == '_')
                {
                    if (sb.Length > 0 && lastChar != ' ')
                    {
                        lastChar = ' ';
                        sb.Append(lastChar);
                    }
                    continue;
                }
                // Space before digits sequence
                else if (i > 1 && char.IsDigit(c) && !char.IsDigit(name[i - 1]) && lastChar != ' ')
                {
                    lastChar = ' ';
                    sb.Append(lastChar);
                }

                lastChar = c;
                sb.Append(lastChar);
            }

            return sb.ToString();
        }

        /// <summary>
        /// Creates the search popup with a tree.
        /// </summary>
        /// <param name="searchBox">The search box.</param>
        /// <param name="tree">The tree control.</param>
        /// <param name="headerHeight">Amount of additional space above the search box to put custom UI.</param>
        /// <returns>The created menu to setup and show.</returns>
        public static ContextMenuBase CreateSearchPopup(out TextBox searchBox, out Tree tree, float headerHeight = 0)
        {
            var menu = new ContextMenuBase
            {
                Size = new Float2(320, 220 + headerHeight),
            };
            searchBox = new SearchBox(false, 1, headerHeight + 1)
            {
                Width = menu.Width - 3,
                Parent = menu,
            };
            var panel1 = new Panel(ScrollBars.Vertical)
            {
                Bounds = new Rectangle(0, searchBox.Bottom + 1, menu.Width, menu.Height - searchBox.Bottom - 2),
                Parent = menu
            };
            var panel2 = new VerticalPanel
            {
                Parent = panel1,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
            };
            tree = new Tree(false)
            {
                Parent = panel2,
            };
            return menu;
        }

        /// <summary>
        /// Updates (recursively) search popup tree structures based on the filter text.
        /// </summary>
        public static void UpdateSearchPopupFilter(TreeNode node, string filterText)
        {
            // Update children
            bool isAnyChildVisible = false;
            for (int i = 0; i < node.Children.Count; i++)
            {
                if (node.Children[i] is TreeNode child)
                {
                    UpdateSearchPopupFilter(child, filterText);
                    isAnyChildVisible |= child.Visible;
                }
            }

            // Update itself
            bool noFilter = string.IsNullOrWhiteSpace(filterText);
            bool isThisVisible = noFilter || QueryFilterHelper.Match(filterText, node.Text);
            bool isExpanded = isAnyChildVisible;
            if (isExpanded)
                node.Expand(true);
            else
                node.Collapse(true);
            node.Visible = isThisVisible | isAnyChildVisible;
        }

        /// <summary>
        /// Gets the asset name relative to the project root folder (with asset file extension)
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <returns>The processed name path.</returns> 
        public static string GetAssetNamePathWithExt(string path)
        {
            var projectFolder = Globals.ProjectFolder;
            if (path.StartsWith(projectFolder))
                path = path.Substring(projectFolder.Length + 1);
            return path;
        }

        /// <summary>
        /// Gets the asset name relative to the project root folder (without asset file extension)
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <returns>The processed name path.</returns>
        public static string GetAssetNamePath(string path)
        {
            var projectFolder = Globals.ProjectFolder;
            if (path.StartsWith(projectFolder))
                path = path.Substring(projectFolder.Length + 1);
            return StringUtils.GetPathWithoutExtension(path);
        }

        /// <summary>
        /// Formats the floating point value (double precision) into the readable text representation.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The text representation.</returns>
        public static string FormatFloat(float value)
        {
            if (float.IsPositiveInfinity(value) || value == float.MaxValue)
                return "Infinity";
            if (float.IsNegativeInfinity(value) || value == float.MinValue)
                return "-Infinity";
            string str = value.ToString("r", CultureInfo.InvariantCulture);
            return FormatFloat(str, value < 0);
        }

        /// <summary>
        /// Formats the floating point value (single precision) into the readable text representation.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The text representation.</returns>
        public static string FormatFloat(double value)
        {
            if (double.IsPositiveInfinity(value) || value == double.MaxValue)
                return "Infinity";
            if (double.IsNegativeInfinity(value) || value == double.MinValue)
                return "-Infinity";
            string str = value.ToString("r", CultureInfo.InvariantCulture);
            return FormatFloat(str, value < 0);
        }

        internal static string FormatFloat(string str, bool isNegative)
        {
            // Reference: https://stackoverflow.com/questions/1546113/double-to-string-conversion-without-scientific-notation
            int x = str.IndexOf('E');
            if (x < 0)
                return str;
            int x1 = x + 1;
            string s, exp = str.Substring(x1, str.Length - x1);
            int e = int.Parse(exp);
            int numDecimals = 0;
            if (isNegative)
            {
                int len = x - 3;
                if (e >= 0)
                {
                    if (len > 0)
                    {
                        s = str.Substring(0, 2) + str.Substring(3, len);
                        numDecimals = len;
                    }
                    else
                        s = str.Substring(0, 2);
                }
                else
                {
                    if (len > 0)
                    {
                        s = str.Substring(1, 1) + str.Substring(3, len);
                        numDecimals = len;
                    }
                    else
                        s = str.Substring(1, 1);
                }
            }
            else
            {
                int len = x - 2;
                if (len > 0)
                {
                    s = str[0] + str.Substring(2, len);
                    numDecimals = len;
                }
                else
                    s = str[0].ToString();
            }
            if (e >= 0)
            {
                e -= numDecimals;
                string z = new string('0', e);
                s = s + z;
            }
            else
            {
                e = -e - 1;
                string z = new string('0', e);
                if (isNegative)
                    s = "-0." + z + s;
                else
                    s = "0." + z + s;
            }
            return s;
        }
    }
}
