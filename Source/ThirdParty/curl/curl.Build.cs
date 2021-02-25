// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://curl.haxx.se/
/// </summary>
public class curl : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "curl License.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        options.PublicDefinitions.Add("CURL_STATICLIB");
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libcurl.lib"));
            options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "libcurl.pdb"));
            options.OutputFiles.Add("ws2_32.lib");
            options.OutputFiles.Add("wldap32.lib");
            options.OutputFiles.Add("crypt32.lib");
            break;
        case TargetPlatform.Linux:
            options.Libraries.Add("curl");
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
