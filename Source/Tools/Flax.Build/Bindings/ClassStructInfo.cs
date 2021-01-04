// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class/structure information for bindings generator.
    /// </summary>
    public abstract class ClassStructInfo : ApiTypeInfo
    {
        public AccessLevel Access;
        public AccessLevel BaseTypeInheritance;
        public TypeInfo BaseType;
        public List<InterfaceInfo> Interfaces; // Optional
        public List<TypeInfo> InterfaceNames; // Optional

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            if (Interfaces == null && InterfaceNames != null && InterfaceNames.Count != 0)
            {
                Interfaces = new List<InterfaceInfo>();
                for (var i = 0; i < InterfaceNames.Count; i++)
                {
                    var interfaceName = InterfaceNames[i];
                    var apiTypeInfo = BindingsGenerator.FindApiTypeInfo(buildData, interfaceName, this);
                    if (apiTypeInfo is InterfaceInfo interfaceInfo)
                    {
                        Interfaces.Add(interfaceInfo);
                    }
                }
                if (Interfaces.Count == 0)
                    Interfaces = null;
            }
        }
    }
}
