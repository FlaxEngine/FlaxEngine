// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native module information for bindings generator.
    /// </summary>
    public class ModuleInfo : ApiTypeInfo
    {
        public Module Module;

        public override string ToString()
        {
            return "module " + Name;
        }

        /// <inheritdoc />
        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            // Sort module files to prevent bindings rebuild due to order changes (list might be created in async)
            Children.Sort();
        }
    }
}
