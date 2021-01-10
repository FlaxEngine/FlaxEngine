// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using System.IO.Compression;
using System.Linq;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// libcurl is a free and easy-to-use client-side URL transfer library, supporting DICT, FILE, FTP, FTPS, Gopher, HTTP, HTTPS, IMAP, IMAPS, LDAP, LDAPS, POP3, POP3S, RTMP, RTSP, SCP, SFTP, SMTP, SMTPS, Telnet and TFTP. libcurl supports SSL certificates, HTTP POST, HTTP PUT, FTP uploading, HTTP form based upload, proxies, cookies, user+password authentication (Basic, Digest, NTLM, Negotiate, Kerberos), file transfer resume, http proxy tunneling and more!
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class curl : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetPlatform.Windows,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var packagePath = Path.Combine(root, "package.zip");
            var vcVersion = "VC14";
            var configuration = "LIB Release - DLL Windows SSPI";
            var binariesToCopyWin = new[]
            {
                "libcurl.lib",
                "lib/libcurl.pdb",
            };
            var filesToKeep = new[]
            {
                "curl.Build.cs",
            };

            // Get the source
            Downloader.DownloadFileFromUrlToPath("https://curl.haxx.se/download/curl-7.64.1.zip", packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
            {
                archive.ExtractToDirectory(root);
                root = Path.Combine(root, archive.Entries.First().FullName);
            }

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var vsSolutionPath = Path.Combine(root, "projects", "Windows", vcVersion, "curl-all.sln");
                    var vcxprojPath = Path.Combine(root, "projects", "Windows", vcVersion, "lib", "libcurl.vcxproj");
                    var vcxprojContents = File.ReadAllText(vcxprojPath);
                    vcxprojContents = vcxprojContents.Replace("<RuntimeLibrary>MultiThreaded</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>");
                    vcxprojContents = vcxprojContents.Replace("<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>");
                    vcxprojContents = vcxprojContents.Replace("<WholeProgramOptimization>true</WholeProgramOptimization>", "<WholeProgramOptimization>false</WholeProgramOptimization>");
                    vcxprojContents = vcxprojContents.Replace("<DebugInformationFormat>ProgramDatabase</DebugInformationFormat>", "<DebugInformationFormat></DebugInformationFormat>");
                    File.WriteAllText(vcxprojPath, vcxprojContents);
                    {
                        // Build for Win64
                        Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configuration, "x64");
                        var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Windows, TargetArchitecture.x64);
                        foreach (var filename in binariesToCopyWin)
                            Utilities.FileCopy(Path.Combine(root, "build", "Win64", vcVersion, configuration, filename), Path.Combine(depsFolder, Path.GetFileName(filename)));
                    }

                    break;
                }
                }
            }

            // Backup files
            var srcIncludePath = Path.Combine(root, "include", "curl");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "curl");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstIncludePath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            // Setup headers directory
            SetupDirectory(dstIncludePath, true);

            // Deploy header files, license file and restore files
            Directory.GetFiles(srcIncludePath, "Makefile*").ToList().ForEach(File.Delete);
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "COPYING"), Path.Combine(dstIncludePath, "curl License.txt"));
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                var dst = Path.Combine(dstIncludePath, filename);
                Utilities.FileCopy(src, dst);
            }
        }
    }
}
