// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using Flax.Build.Projects;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Xbox One platform implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="UWPPlatform" />
    public sealed class XboxOnePlatform : UWPPlatform, IProjectCustomizer
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.XboxOne;

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => false;

        /// <summary>
        /// Initializes a new instance of the <see cref="XboxOnePlatform"/> class.
        /// </summary>
        public XboxOnePlatform()
        {
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new XboxOneToolchain(this, architecture);
        }

        /// <inheritdoc />
        void IProjectCustomizer.GetSolutionArchitectureName(TargetArchitecture architecture, ref string name)
        {
            name = "XboxOne";
        }
    }
}
