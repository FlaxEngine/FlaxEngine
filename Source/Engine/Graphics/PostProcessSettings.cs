// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    internal class PostProcessSettingAttribute : Attribute
    {
        public int Bit;

        public PostProcessSettingAttribute(int bit)
        {
            Bit = bit;
        }
    }

    public partial struct AntiAliasingSettings
    {
        /// <summary>
        /// Whether or not to show the TAA settings.
        /// </summary>
        public bool ShowTAASettings => (Mode == AntialiasingMode.TemporalAntialiasing);
    }

    public partial struct ToneMappingSettings
    {
        /// <summary>
        /// Whether or not to show the look (only the AGX mode has looks for now).
        /// </summary>
        public bool ShowLook => (Mode == ToneMappingMode.AGX);
    }
}
