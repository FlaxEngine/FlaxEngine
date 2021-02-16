// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class/structure interface information for bindings generator.
    /// </summary>
    public class InterfaceInfo : ClassStructInfo
    {
        public override bool IsInterface => true;

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            apiTypeInfo.Namespace = null;

            base.AddChild(apiTypeInfo);
        }

        public override string ToString()
        {
            return "interface " + Name;
        }
    }
}
