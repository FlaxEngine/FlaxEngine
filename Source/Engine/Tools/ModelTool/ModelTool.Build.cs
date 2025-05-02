// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Model data utilities module.
/// </summary>
public class ModelTool : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_MODEL_TOOL");

        if (!options.Target.IsEditor)
            return;

        bool useAssimp = true;
        bool useAutodeskFbxSdk = false;
        bool useOpenFBX = true;

        if (useAssimp)
        {
            options.PrivateDependencies.Add("assimp");
            options.PrivateDefinitions.Add("USE_ASSIMP");
        }

        if (useAutodeskFbxSdk)
        {
            options.PrivateDefinitions.Add("USE_AUTODESK_FBX_SDK");

            // FBX SDK 2020.0.1 VS2015
            // TODO: convert this into AutodeskFbxSdk and implement proper SDK lookup with multiple versions support
            // TODO: link against dll with delay loading
            var sdkRoot = @"C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1";
            var libSubDir = "lib\\vs2015\\x64\\release";
            options.PrivateIncludePaths.Add(Path.Combine(sdkRoot, "include"));
            options.OutputFiles.Add(Path.Combine(sdkRoot, libSubDir, "libfbxsdk-md.lib"));
            options.OutputFiles.Add(Path.Combine(sdkRoot, libSubDir, "zlib-md.lib"));
            options.OutputFiles.Add(Path.Combine(sdkRoot, libSubDir, "libxml2-md.lib"));
        }

        if (useOpenFBX)
        {
            options.PrivateDependencies.Add("OpenFBX");
            options.PrivateDefinitions.Add("USE_OPEN_FBX");
        }

        options.PrivateDependencies.Add("TextureTool");
        options.PrivateDefinitions.Add("COMPILE_WITH_ASSETS_IMPORTER");

        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.PrivateDependencies.Add("DirectXMesh");
            options.PrivateDependencies.Add("UVAtlas");
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Mac: break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        options.PrivateDependencies.Add("meshoptimizer");
        options.PrivateDependencies.Add("MikkTSpace");
        options.PrivateDependencies.Add("Physics");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.Add(Path.Combine(FolderPath, "ModelTool.h"));
        files.Add(Path.Combine(FolderPath, "MeshAccelerationStructure.h"));
    }
}
