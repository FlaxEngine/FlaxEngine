// (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using Flax.Build.NativeCpp;

namespace Flax.Build.Bindings
{
    internal interface IBindingsCache
    {
        void Write(BinaryWriter writer);
        void Read(BinaryReader reader);
    }

    partial class BindingsGenerator
    {
        private static readonly Dictionary<string, Type> _typeCache = new Dictionary<string, Type>();

        internal static void Write(BinaryWriter writer, string e)
        {
            var valid = e != null;
            writer.Write(valid);
            if (valid)
                writer.Write(e);
        }

        internal static void Write(BinaryWriter writer, string[] list)
        {
            if (list != null)
            {
                writer.Write(list.Length);
                for (int i = 0; i < list.Length; i++)
                    writer.Write(list[i]);
            }
            else
            {
                writer.Write(0);
            }
        }

        internal static void Write(BinaryWriter writer, HashSet<string> list)
        {
            if (list != null)
            {
                writer.Write(list.Count());
                foreach (var e in list)
                    writer.Write(e);
            }
            else
            {
                writer.Write(0);
            }
        }

        internal static void Write(BinaryWriter writer, List<string> list)
        {
            if (list != null)
            {
                writer.Write(list.Count());
                foreach (var e in list)
                    writer.Write(e);
            }
            else
            {
                writer.Write(0);
            }
        }

        internal static void Write(BinaryWriter writer, IEnumerable<string> list)
        {
            if (list != null)
            {
                writer.Write(list.Count());
                foreach (var e in list)
                    writer.Write(e);
            }
            else
            {
                writer.Write(0);
            }
        }

        internal static void Write<T>(BinaryWriter writer, T e) where T : IBindingsCache
        {
            if (e != null)
            {
                writer.Write(e.GetType().FullName);
                e.Write(writer);
            }
            else
            {
                writer.Write(string.Empty);
            }
        }

        internal static void Write<T>(BinaryWriter writer, List<T> list) where T : IBindingsCache
        {
            if (list != null)
            {
                var count = list.Count;
                writer.Write(count);
                for (int i = 0; i < count; i++)
                {
                    var e = list[i];
                    writer.Write(e.GetType().FullName);
                    e.Write(writer);
                }
            }
            else
            {
                writer.Write(0);
            }
        }

        internal static void Write<T>(BinaryWriter writer, T[] list) where T : IBindingsCache
        {
            if (list != null)
            {
                var count = list.Length;
                writer.Write(count);
                for (int i = 0; i < count; i++)
                {
                    var e = list[i];
                    writer.Write(e.GetType().FullName);
                    e.Write(writer);
                }
            }
            else
            {
                writer.Write(0);
            }
        }

        internal static string Read(BinaryReader reader, string e)
        {
            var valid = reader.ReadBoolean();
            if (valid)
                e = reader.ReadString();
            return e;
        }

        internal static string[] Read(BinaryReader reader, string[] list)
        {
            var count = reader.ReadInt32();
            if (count != 0)
            {
                list = new string[count];
                for (int i = 0; i < count; i++)
                    list[i] = reader.ReadString();
            }
            return list;
        }

        internal static T Read<T>(BinaryReader reader, T e) where T : IBindingsCache
        {
            var typename = reader.ReadString();
            if (string.IsNullOrEmpty(typename))
                return e;
            if (!_typeCache.TryGetValue(typename, out var type))
            {
                type = Builder.BuildTypes.FirstOrDefault(x => x.FullName == typename);
                if (type == null)
                {
                    var msg = $"Missing type {typename}.";
                    Log.Error(msg);
                    throw new Exception(msg);
                }
                _typeCache.Add(typename, type);
            }
            e = (T)Activator.CreateInstance(type);
            e.Read(reader);
            return e;
        }

        internal static List<T> Read<T>(BinaryReader reader, List<T> list) where T : IBindingsCache
        {
            var count = reader.ReadInt32();
            if (count != 0)
            {
                list = new List<T>(count);
                for (int i = 0; i < count; i++)
                {
                    var typename = reader.ReadString();
                    if (!_typeCache.TryGetValue(typename, out var type))
                    {
                        type = Builder.BuildTypes.FirstOrDefault(x => x.FullName == typename);
                        if (type == null)
                        {
                            var msg = $"Missing type {typename}.";
                            Log.Error(msg);
                            throw new Exception(msg);
                        }
                        _typeCache.Add(typename, type);
                    }
                    var e = (T)Activator.CreateInstance(type);
                    e.Read(reader);
                    list.Add(e);
                }
            }
            return list;
        }

        private static string GetCachePath(Module module, BuildOptions moduleOptions)
        {
            return Path.Combine(moduleOptions.IntermediateFolder, module.Name + ".Bindings.cache");
        }

        private static void SaveCache(ModuleInfo moduleInfo, BuildOptions moduleOptions, List<string> headerFiles)
        {
            if (!Directory.Exists(moduleOptions.IntermediateFolder))
                return;
            var path = GetCachePath(moduleInfo.Module, moduleOptions);
            using (var stream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.Read))
            using (var writer = new BinaryWriter(stream, Encoding.UTF8))
            {
                // Version
                writer.Write(5);
                writer.Write(File.GetLastWriteTime(Assembly.GetExecutingAssembly().Location).Ticks);

                // Build options
                writer.Write(moduleOptions.IntermediateFolder);
                writer.Write((int)moduleOptions.Platform.Target);
                writer.Write((int)moduleOptions.Architecture);
                writer.Write((int)moduleOptions.Configuration);
                Write(writer, moduleOptions.PublicDefinitions);
                Write(writer, moduleOptions.PrivateDefinitions);
                Write(writer, moduleOptions.CompileEnv.PreprocessorDefinitions);

                // Header files
                writer.Write(headerFiles.Count);
                for (int i = 0; i < headerFiles.Count; i++)
                {
                    var headerFile = headerFiles[i];
                    writer.Write(headerFile);
                    writer.Write(File.GetLastWriteTime(headerFile).Ticks);
                }

                // Info
                moduleInfo.Write(writer);
            }
        }

        private static bool LoadCache(ref ModuleInfo moduleInfo, BuildOptions moduleOptions, List<string> headerFiles)
        {
            var path = GetCachePath(moduleInfo.Module, moduleOptions);
            if (!File.Exists(path))
                return false;
            using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (var reader = new BinaryReader(stream, Encoding.UTF8))
            {
                // Version
                var version = reader.ReadInt32();
                if (version != 5)
                    return false;
                if (File.GetLastWriteTime(Assembly.GetExecutingAssembly().Location).Ticks != reader.ReadInt64())
                    return false;

                // Build options
                if (reader.ReadString() != moduleOptions.IntermediateFolder ||
                    reader.ReadInt32() != (int)moduleOptions.Platform.Target ||
                    reader.ReadInt32() != (int)moduleOptions.Architecture ||
                    reader.ReadInt32() != (int)moduleOptions.Configuration)
                    return false;
                var publicDefinitions = Read(reader, Utilities.GetEmptyArray<string>());
                if (publicDefinitions.Length != moduleOptions.PublicDefinitions.Count || publicDefinitions.Any(x => !moduleOptions.PublicDefinitions.Contains(x)))
                    return false;
                var privateDefinitions = Read(reader, Utilities.GetEmptyArray<string>());
                if (privateDefinitions.Length != moduleOptions.PrivateDefinitions.Count || privateDefinitions.Any(x => !moduleOptions.PrivateDefinitions.Contains(x)))
                    return false;
                var preprocessorDefinitions = Read(reader, Utilities.GetEmptyArray<string>());
                if (preprocessorDefinitions.Length != moduleOptions.CompileEnv.PreprocessorDefinitions.Count || preprocessorDefinitions.Any(x => !moduleOptions.CompileEnv.PreprocessorDefinitions.Contains(x)))
                    return false;

                // Header files
                var headerFilesCount = reader.ReadInt32();
                if (headerFilesCount != headerFiles.Count)
                    return false;
                for (int i = 0; i < headerFilesCount; i++)
                {
                    var headerFile = headerFiles[i];
                    if (headerFile != reader.ReadString())
                        return false;
                    if (File.GetLastWriteTime(headerFile).Ticks > reader.ReadInt64())
                        return false;
                }

                // Info
                var newModuleInfo = new ModuleInfo
                {
                    Module = moduleInfo.Module,
                    Name = moduleInfo.Name,
                    Namespace = moduleInfo.Namespace,
                    IsFromCache = true,
                };
                try
                {
                    newModuleInfo.Read(reader);

                    // Skip parsing and use data loaded from cache
                    moduleInfo = newModuleInfo;
                    return true;
                }
                catch
                {
                    // Skip loading cache
                    return false;
                }
            }
        }
    }
}
