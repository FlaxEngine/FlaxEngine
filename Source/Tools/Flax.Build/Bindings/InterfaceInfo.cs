// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class/structure interface information for bindings generator.
    /// </summary>
    public class InterfaceInfo : VirtualClassInfo
    {
        public override int GetScriptVTableSize(out int offset)
        {
            offset = 0;
            return Functions.Count(x => x.IsVirtual);
        }

        public override int GetScriptVTableOffset(VirtualClassInfo classInfo)
        {
            return 0;
        }

        public override bool IsInterface => true;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            if (BaseType != null)
                throw new Exception(string.Format("Interface {0} cannot inherit from {1}.", FullNameNative, BaseType));
            if (Interfaces != null && Interfaces.Count != 0)
                throw new Exception(string.Format("Interface {0} cannot inherit from {1}.", FullNameNative, "interfaces"));
        }

        public override string ToString()
        {
            return "interface " + Name;
        }
    }
}
