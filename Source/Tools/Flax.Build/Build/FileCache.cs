using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build
{
    /// <summary>
    /// Cache filesystem related queries like File.Exists and File.GetLastWriteTime.
    /// </summary>
    public static class FileCache
    {
        private static readonly Dictionary<string, FileInfo> _cache = new();

        public static void FileRemoveFromCache(string path)
        {
            _cache.Remove(path);
        }

        public static bool Exists(string path)
        {
            if (_cache.TryGetValue(path, out var fileInfo))
                return fileInfo.Exists;

            fileInfo = new FileInfo(path);
            _cache.Add(path, fileInfo);
            return fileInfo.Exists;
        }

        public static DateTime GetLastWriteTime(string path)
        {
            if (_cache.TryGetValue(path, out var fileInfo))
                return fileInfo.LastWriteTime;

            fileInfo = new FileInfo(path);
            _cache.Add(path, fileInfo);
            return fileInfo.LastWriteTime;
        }
    }
}
