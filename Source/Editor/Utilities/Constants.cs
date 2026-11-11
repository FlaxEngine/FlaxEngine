// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Editor constants and global properties.
    /// </summary>
    internal class Constants
    {
        public const string DiscordUrl = "https://flaxengine.com/discord";
        public const string DocsUrl = "https://docs.flaxengine.com/";
        public const string BugTrackerUrl = "https://github.com/FlaxEngine/FlaxEngine/issues";
        public const string WebsiteUrl = "https://flaxengine.com";
        public const string FacebookUrl = "https://facebook.com/FlaxEngine";
        public const string YoutubeUrl = "https://youtube.com/c/FlaxEngine";
        public const string TwitterUrl = "https://twitter.com/FlaxEngine";

#if PLATFORM_MAC
        public const string ShowInExplorer = "Show in Finder";
#else
        public const string ShowInExplorer = "Show in explorer";
#endif

        public const float UIMargin = 4.0f;
        public const float UIAccent = 4.0f;
        public const uint ColorModifiedValue = 0xA9A906;
        public const uint ColorModifiedValuePrefab = 0x007ACC;
        public const uint ColorPrefab = 0x3ECB4C;
    }
}
