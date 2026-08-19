using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/mstorsjo/fdk-aac
/// </summary>
public class fdkaac : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "NOTICE";

        // Merge third-party modules into engine binary
        //BinaryModuleName = "FlaxEngine";
    }

        /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;

        switch (options.Platform.Target)
        {
        case TargetPlatform.Linux:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libfdk-aac.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}