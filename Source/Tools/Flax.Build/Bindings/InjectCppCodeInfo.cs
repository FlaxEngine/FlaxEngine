// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The custom C++ code injection information for bindings generator.
    /// </summary>
    public class InjectCppCodeInfo : ApiTypeInfo
    {
        public string Code;

        /// <inheritdoc />
        public override string ToString()
        {
            return Code;
        }
    }
}
