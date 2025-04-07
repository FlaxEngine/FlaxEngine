// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native file information for bindings generator.
    /// </summary>
    public class FileInfo : ApiTypeInfo, IComparable, IComparable<FileInfo>
    {
        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            if (apiTypeInfo.Namespace == null)
                apiTypeInfo.Namespace = Namespace;

            base.AddChild(apiTypeInfo);
        }

        public override void Init(Builder.BuildData buildData)
        {
            try
            {
                base.Init(buildData);
            }
            catch (Exception)
            {
                Log.Error($"Failed to init '{Name}' file scripting API.");
                throw;
            }
        }

        public int CompareTo(FileInfo other)
        {
            return Name.CompareTo(other.Name);
        }

        public override string ToString()
        {
            return System.IO.Path.GetFileName(Name);
        }

        public int CompareTo(object obj)
        {
            if (obj is ApiTypeInfo apiTypeInfo)
                return Name.CompareTo(apiTypeInfo.Name);
            return 0;
        }
    }
}
