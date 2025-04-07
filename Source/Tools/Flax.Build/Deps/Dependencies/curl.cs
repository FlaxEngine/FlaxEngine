// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
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
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
                    };
                case TargetPlatform.Mac:
                    return new[]
                    {
                        TargetPlatform.Mac,
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
            var configuration = "Release";
            var binariesToCopyWin = new[]
            {
                "libcurl.lib",
            };
            var filesToKeep = new[]
            {
                "curl.Build.cs",
            };

            // Get the source
            if (!File.Exists(packagePath))
                Downloader.DownloadFileFromUrlToPath("https://curl.haxx.se/download/curl-7.88.1.zip", packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
            {
                var newRoot = Path.Combine(root, archive.Entries.First().FullName);
                if (!Directory.Exists(newRoot))
                    archive.ExtractToDirectory(root);
                root = newRoot;
            }

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    // Build for Win64 and ARM64
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        var solutionPath = Path.Combine(buildDir, "CURL.sln");

                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DBUILD_CURL_EXE=OFF -DBUILD_SHARED_LIBS=OFF -DCURL_STATIC_CRT=OFF");
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopyWin)
                            Utilities.FileCopy(Path.Combine(buildDir, "lib", configuration, file), Path.Combine(depsFolder, Path.GetFileName(file)));
                    }
                    break;
                }
                case TargetPlatform.Linux:
                {
                    // Build for Linux
                    var settings = new[]
                    {
                        "--without-librtmp",
                        "--without-ssl",
                        "--with-gnutls",
                        "--disable-ipv6",
                        "--disable-manual",
                        "--disable-verbose",
                        "--disable-shared",
                        "--enable-static",
                        "-disable-ldap --disable-sspi --disable-ftp --disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-smb",
                    };
                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-7" },
                        { "CC_FOR_BUILD", "clang-7" },
                    };
                    var buildDir = Path.Combine(root, "build");
                    SetupDirectory(buildDir, true);
                    Utilities.Run("chmod", "+x configure", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                    Utilities.Run(Path.Combine(root, "configure"), string.Join(" ", settings) + " --prefix=\"" + buildDir + "\"", null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError);
                    Utilities.Run("make", "install", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    var filename = "libcurl.a";
                    Utilities.FileCopy(Path.Combine(buildDir, "lib", filename), Path.Combine(depsFolder, filename));
                    break;
                }
                case TargetPlatform.Mac:
                {
                    // Build for Mac
                    var settings = new[]
                    {
                        "--with-secure-transport",
                        "--without-librtmp",
                        "--disable-ipv6",
                        "--disable-manual",
                        "--disable-verbose",
                        "--disable-shared",
                        "--enable-static",
                        "-disable-ldap --disable-sspi --disable-ftp --disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-smb",
                    };
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var arch = GetAppleArchName(architecture);
                        var archName = arch + "-apple-darwin19";
                        if (architecture == TargetArchitecture.ARM64)
                            archName = "arm-apple-darwin19"; // for configure
                        var compilerFlags = string.Format("-mmacosx-version-min={0} -arch {1}", Configuration.MacOSXMinVer, arch);
                        var envVars = new Dictionary<string, string>
                        {
                            { "CC", "clang" },
                            { "CXX", "clang" },
                            { "CFLAGS", compilerFlags },
                            { "CXXFLAGS", compilerFlags },
                            { "CPPFLAGS", compilerFlags },
                            { "ARCH", arch },
                            { "SDK", "macosx" },
                            { "DEPLOYMENT_TARGET", Configuration.MacOSXMinVer },
                        };
                        var buildDir = Path.Combine(root, "build");
                        SetupDirectory(buildDir, true);
                        Utilities.Run("chmod", "+x configure", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        Utilities.Run("chmod", "+x install-sh", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        Utilities.Run(Path.Combine(root, "configure"), string.Join(" ", settings) + " --host=" + archName + " --prefix=\"" + buildDir + "\"", null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                        Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        Utilities.Run("make", "install", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        var filename = "libcurl.a";
                        Utilities.FileCopy(Path.Combine(buildDir, "lib", filename), Path.Combine(depsFolder, filename));
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
