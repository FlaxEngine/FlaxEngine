// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build.Projects;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for Web with Emscripten.
    /// </summary>
    /// <seealso cref="Platform" />
    public sealed class WebPlatform : Platform, IProjectCustomizer
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Web;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled { get; }

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => false;

        /// <inheritdoc />
        public override bool HasModularBuildSupport => false;

        /// <inheritdoc />
        public override bool HasDynamicCodeExecutionSupport => false;

        /// <inheritdoc />
        public override string ExecutableFileExtension => ".html";

        /// <inheritdoc />
        public override string SharedLibraryFileExtension => ".wasm";

        /// <inheritdoc />
        public override string StaticLibraryFileExtension => ".a";

        /// <inheritdoc />
        public override string ProgramDatabaseFileExtension => string.Empty;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.WebPlatform"/> class.
        /// </summary>
        public WebPlatform()
        {
            HasRequiredSDKsInstalled = EmscriptenSdk.Instance.IsValid;
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            if (architecture != TargetArchitecture.x86)
                throw new InvalidArchitectureException(architecture, "Web is compiled into WebAssembly and doesn't have specific architecture but is mocked as x86.");
            return new WebToolchain(this, architecture);
        }

        /// <inheritdoc />
        void IProjectCustomizer.GetSolutionArchitectureName(TargetArchitecture architecture, ref string name)
        {
            name = "Web";
        }

        /// <inheritdoc />
        void IProjectCustomizer.GetProjectArchitectureName(Project project, Platform platform, TargetArchitecture architecture, ref string name)
        {
            name = "Win32";
        }
    }
}
