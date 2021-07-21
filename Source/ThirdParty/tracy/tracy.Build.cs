// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/wolfpld/tracy
/// </summary>
public class tracy : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD3Clause;
        LicenseFilePath = "LICENSE";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourcePaths.Clear();
        options.SourceFiles.Clear();
        options.SourceFiles.Add(Path.Combine(FolderPath, "Tracy.h"));
        options.SourceFiles.Add(Path.Combine(FolderPath, "TracyClient.cpp"));

        options.PublicDefinitions.Add("TRACY_ENABLE");
        if (options.Platform.Target == TargetPlatform.Windows)
            options.PrivateDefinitions.Add("TRACY_DBGHELP_LOCK=DbgHelp");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        base.GetFilesToDeploy(files);

        files.Add(Path.Combine(FolderPath, "Tracy.h"));
        files.Add(Path.Combine(FolderPath, "common", "TracySystem.hpp"));
        files.Add(Path.Combine(FolderPath, "client", "TracyCallstack.h"));
    }
}
