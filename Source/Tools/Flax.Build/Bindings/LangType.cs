// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The in-build language type for bindings generator.
    /// </summary>
    public class LangType : ApiTypeInfo
    {
        public override bool IsValueType => true;
        public override bool IsPod => true;

        public LangType(string name)
        {
            Name = name;
            IsInBuild = true;
        }

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            throw new NotSupportedException("API lang types cannot have sub-types.");
        }
    }
}
