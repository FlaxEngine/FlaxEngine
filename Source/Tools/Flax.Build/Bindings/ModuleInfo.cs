// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native module information for bindings generator.
    /// </summary>
    public class ModuleInfo : ApiTypeInfo
    {
        public Module Module;
        public bool IsFromCache;

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

        public override void Write(BinaryWriter writer)
        {
            writer.Write(Module.Name);
            writer.Write(Module.FilePath);
            BindingsGenerator.Write(writer, Module.BinaryModuleName);
            writer.Write(Module.BuildNativeCode);
            writer.Write(Module.BuildCSharp);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            if (reader.ReadString() != Module.Name ||
                reader.ReadString() != Module.FilePath ||
                BindingsGenerator.Read(reader, Module.BinaryModuleName) != Module.BinaryModuleName ||
                reader.ReadBoolean() != Module.BuildNativeCode ||
                reader.ReadBoolean() != Module.BuildCSharp
            )
                throw new Exception();

            base.Read(reader);
        }
    }
}
