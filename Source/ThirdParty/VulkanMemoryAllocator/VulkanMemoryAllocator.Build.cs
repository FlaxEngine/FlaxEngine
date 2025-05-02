// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
/// </summary>
public class VulkanMemoryAllocator : HeaderOnlyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }
}
