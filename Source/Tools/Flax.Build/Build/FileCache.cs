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
        private static Dictionary<string, FileInfo> fileInfoCache = new Dictionary<string, FileInfo>();

        public static void FileRemoveFromCache(string path)
        {
            //fileInfoCache[path].Refresh();
            fileInfoCache.Remove(path);
        }
        
        public static bool Exists(string path)
        {
            if (fileInfoCache.TryGetValue(path, out var fileInfo))
                return fileInfo.Exists;

            fileInfo = new FileInfo(path);
            fileInfoCache.Add(path, fileInfo);
            return fileInfo.Exists;
        }

        public static DateTime GetLastWriteTime(string path)
        {

            if (fileInfoCache.TryGetValue(path, out var fileInfo))
                return fileInfo.LastWriteTime;

            fileInfo = new FileInfo(path);
            fileInfoCache.Add(path, fileInfo);
            return fileInfo.LastWriteTime;
        }
    }
}
