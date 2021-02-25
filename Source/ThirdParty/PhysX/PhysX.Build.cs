// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/NVIDIAGameWorks/PhysX
/// </summary>
public class PhysX : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD3Clause;
        LicenseFilePath = "License.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // PhysX 4 configuration:
        // PX_BUILDSNIPPETS = False
        // PX_BUILDSAMPLES = False
        // PX_BUILDPUBLICSAMPLES = False
        // PX_GENERATE_STATIC_LIBRARIES = True
        // NV_USE_STATIC_WINCRT = False
        // NV_USE_DEBUG_WINCRT = False
        // PX_FLOAT_POINT_PRECISE_MATH = False

        bool useDynamicLinking = false;
        bool usePVD = false;
        bool useVehicle = false;
        bool usePhysicsCooking = Physics.WithCooking;

        var depsRoot = options.DepsFolder;

        if (usePVD)
            options.PublicDefinitions.Add("WITH_PVD");
        if (useVehicle)
            options.PublicDefinitions.Add("WITH_VEHICLE");

        string archPostFix = string.Empty;
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.UWP:
        case TargetPlatform.Linux:
        case TargetPlatform.XboxScarlett:
            switch (options.Architecture)
            {
            case TargetArchitecture.x86:
                archPostFix = "_32";
                break;
            case TargetArchitecture.x64:
                archPostFix = "_64";
                break;
            }
            break;
        }

        options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, "Source/ThirdParty/PhysX"));

        if (useDynamicLinking)
        {
            throw new NotImplementedException("Using dynamic linking for PhysX is not implemented.");
        }
        else
        {
            options.PublicDefinitions.Add("PX_PHYSX_STATIC_LIB");

            AddLib(options, depsRoot, string.Format("PhysX_static{0}", archPostFix));
            AddLib(options, depsRoot, string.Format("PhysXCharacterKinematic_static{0}", archPostFix));
            AddLib(options, depsRoot, string.Format("PhysXCommon_static{0}", archPostFix));
            AddLib(options, depsRoot, string.Format("PhysXExtensions_static{0}", archPostFix));
            AddLib(options, depsRoot, string.Format("PhysXFoundation_static{0}", archPostFix));

            if (usePhysicsCooking)
            {
                AddLib(options, depsRoot, string.Format("PhysXCooking_static{0}", archPostFix));
            }

            if (usePVD)
            {
                AddLib(options, depsRoot, string.Format("PhysXPvdSDK_static{0}", archPostFix));
            }

            if (useVehicle)
            {
                AddLib(options, depsRoot, string.Format("PhysXVehicle_static_{0}", archPostFix));
            }
        }
    }
}
