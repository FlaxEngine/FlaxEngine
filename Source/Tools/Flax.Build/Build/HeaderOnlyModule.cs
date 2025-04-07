// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build
{
    /// <summary>
    /// The build module from 3rd Party source that contains only header files.
    /// </summary>
    /// <seealso cref="Flax.Build.ThirdPartyModule" />
    public class HeaderOnlyModule : ThirdPartyModule
    {
        /// <inheritdoc />
        public HeaderOnlyModule()
        {
            // Don't generate any binaries
            BinaryModuleName = null;
        }
    }
}
