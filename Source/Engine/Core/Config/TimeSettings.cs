// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEditor.Content.Settings
{
    public partial class TimeSettings
    {
#if FLAX_EDITOR
        // ReSharper disable once UnusedMember.Local
        internal bool IsDrawFPSUsed => UpdateMode == TimeUpdateMode.UpdateAndDrawSeparated;
#endif
    }
}
