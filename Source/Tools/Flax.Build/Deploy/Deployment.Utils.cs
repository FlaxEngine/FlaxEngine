// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

using System;
using System.IO;
using Flax.Build;

namespace Flax.Deploy
{
    partial class Deployment
    {
        private static void DeployFiles(string src, string dst, string searchPattern)
        {
            if (!Directory.Exists(dst))
                Directory.CreateDirectory(dst);

            string[] files = Directory.GetFiles(src, searchPattern);
            for (int i = 0; i < files.Length; i++)
            {
                var filename = Path.GetFileName(files[i]);

                Log.Verbose("Deploy file " + filename);

                File.Copy(files[i], Path.Combine(dst, filename), true);
            }
        }

        private static void DeployFile(string srcPath, string dstPath, bool optional = false)
        {
            Log.Verbose("Deploy file " + Path.GetFileName(srcPath));

            var dst = Path.GetDirectoryName(dstPath);
            if (!Directory.Exists(dst))
                Directory.CreateDirectory(dst);

            if (!File.Exists(srcPath))
            {
                if (optional)
                    return;
                throw new Exception("Missing source file " + srcPath);
            }

            File.Copy(srcPath, dstPath);
        }

        private static void DeployFile(string src, string dst, string filename, bool optional = false)
        {
            Log.Verbose("Deploy file " + filename);

            if (!Directory.Exists(dst))
                Directory.CreateDirectory(dst);

            var srcPath = Path.Combine(src, filename);
            if (!File.Exists(srcPath))
            {
                if (optional)
                    return;
                throw new Exception("Missing source file " + srcPath);
            }

            File.Copy(srcPath, Path.Combine(dst, filename));
        }

        private static void DeployFile(string src, string dst, string subdir, string filename)
        {
            Log.Verbose("Deploy file " + subdir + "/" + filename);

            string dstPath = Path.Combine(dst, subdir);
            if (!Directory.Exists(dstPath))
                Directory.CreateDirectory(dstPath);

            var srcPath = Path.Combine(src, subdir, filename);
            if (!File.Exists(srcPath))
                throw new Exception("Missing source file " + srcPath);

            File.Copy(srcPath, Path.Combine(dstPath, filename));
        }

        private static void DeployFolder(string src, string dst, string subdir)
        {
            Log.Verbose("Deploy folder " + subdir);

            Utilities.DirectoryCopy(Path.Combine(src, subdir), Path.Combine(dst, subdir));
        }
    }
}
