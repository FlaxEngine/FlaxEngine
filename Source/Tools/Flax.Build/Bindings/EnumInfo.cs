// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native enumeration information for bindings generator.
    /// </summary>
    public class EnumInfo : ApiTypeInfo
    {
        public struct EntryInfo
        {
            public string Name;
            public string[] Comment;
            public string Value;
            public string Attributes;

            public override string ToString()
            {
                return Name + (string.IsNullOrEmpty(Value) ? string.Empty : " = " + Value);
            }
        }

        public AccessLevel Access;
        public TypeInfo UnderlyingType;
        public List<EntryInfo> Entries;

        public override bool IsValueType => true;
        public override bool IsEnum => true;
        public override bool IsPod => true;

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            throw new NotSupportedException("API enums cannot have sub-types.");
        }

        public override string ToString()
        {
            return "enum " + Name;
        }
    }
}
