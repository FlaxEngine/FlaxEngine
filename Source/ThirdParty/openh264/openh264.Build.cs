using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/cisco/openh264
/// </summary>
public class openh264 : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD2Clause;
        LicenseFilePath = "LICENSE";

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
            options.OutputFiles.Add(Path.Combine(depsRoot, "libopenh264.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}