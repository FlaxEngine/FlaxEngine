// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native structure information for bindings generator.
    /// </summary>
    public class StructureInfo : ClassStructInfo
    {
        public List<FieldInfo> Fields = new List<FieldInfo>();
        public List<FunctionInfo> Functions = new List<FunctionInfo>();
        public bool IsAutoSerialization;
        public bool ForceNoPod;

        public override bool IsStruct => true;
        public override bool IsValueType => true;
        public override bool IsPod => _isPod;

        private bool _isPod;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            if (ForceNoPod || (InterfaceNames != null && InterfaceNames.Count != 0))
            {
                _isPod = false;
                return;
            }

            // Structure is POD (plain old data) only if all of it's fields are (and has no base type ro base type is also POD)
            _isPod = BaseType == null || (BindingsGenerator.FindApiTypeInfo(buildData, BaseType, Parent)?.IsPod ?? false);
            for (int i = 0; _isPod && i < Fields.Count; i++)
            {
                var field = Fields[i];
                if (!field.IsStatic && !field.Type.IsPod(buildData, this))
                {
                    _isPod = false;
                }
            }
        }

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, Fields);
            BindingsGenerator.Write(writer, Functions);
            writer.Write(IsAutoSerialization);
            writer.Write(ForceNoPod);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Fields = BindingsGenerator.Read(reader, Fields);
            Functions = BindingsGenerator.Read(reader, Functions);
            IsAutoSerialization = reader.ReadBoolean();
            ForceNoPod = reader.ReadBoolean();

            base.Read(reader);
        }

        public override void AddChild(ApiTypeInfo apiTypeInfo)
        {
            if (apiTypeInfo is EnumInfo)
            {
                base.AddChild(apiTypeInfo);
            }
            else
            {
                throw new NotSupportedException("Structures in API can have only enums as sub-types.");
            }
        }

        public override string ToString()
        {
            return "struct " + Name;
        }
    }
}
