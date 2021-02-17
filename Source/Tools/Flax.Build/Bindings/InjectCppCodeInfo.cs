// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The custom C++ code injection information for bindings generator.
    /// </summary>
    public class InjectCppCodeInfo : ApiTypeInfo
    {
        public string Code;

        public override void Write(BinaryWriter writer)
        {
            writer.Write(Code);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Code = reader.ReadString();

            base.Read(reader);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Code;
        }
    }
}
