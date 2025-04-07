// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The custom C++/C# code injection information for bindings generator.
    /// </summary>
    public class InjectCodeInfo : ApiTypeInfo
    {
        public string Lang;
        public string Code;

        public override void Write(BinaryWriter writer)
        {
            writer.Write(Lang);
            writer.Write(Code);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Lang = reader.ReadString();
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
