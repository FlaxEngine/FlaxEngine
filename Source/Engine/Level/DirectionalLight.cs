// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    public partial class DirectionalLight
    {
        bool ShowCascade1 => CascadeCount >= 1 && PartitionMode == PartitionMode.Manual;
        bool ShowCascade2 => CascadeCount >= 2 && PartitionMode == PartitionMode.Manual;
        bool ShowCascade3 => CascadeCount >= 3 && PartitionMode == PartitionMode.Manual;
        bool ShowCascade4 => CascadeCount >= 4 && PartitionMode == PartitionMode.Manual;
    }
}
