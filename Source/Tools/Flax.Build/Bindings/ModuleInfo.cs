// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

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
    }
}
