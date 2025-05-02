// Copyright (c) Wojciech Figat. All rights reserved.

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
        public bool IsAutoSerialization;
        public bool ForceNoPod;
        public bool NoDefault;

        public override bool IsStruct => true;
        public override bool IsValueType => true;
        public override bool IsPod => _isPod;

        private bool _isPod;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            if (ForceNoPod || (Interfaces != null && Interfaces.Count != 0) || IsTemplate)
            {
                _isPod = false;
                return;
            }

            // Structure is POD (plain old data) only if all of it's fields are (and has no base type or base type is also POD)
            _isPod = BaseType == null || (BaseType?.IsPod ?? false);
            for (int i = 0; _isPod && i < Fields.Count; i++)
            {
                var field = Fields[i];
                if (!field.IsStatic && !field.IsPod(buildData, this))
                {
                    _isPod = false;
                }
            }

            foreach (var fieldInfo in Fields)
            {
                if (fieldInfo.Type.IsBitField)
                    throw new NotImplementedException($"TODO: support bit-fields in structure fields (found field {fieldInfo} in structure {Name})");

                // Pointers are fine
                if (fieldInfo.Type.IsPtr)
                    continue;

                // In-build types
                if (BindingsGenerator.CSharpNativeToManagedBasicTypes.ContainsKey(fieldInfo.Type.Type))
                    continue;
                if (BindingsGenerator.CSharpNativeToManagedDefault.ContainsKey(fieldInfo.Type.Type))
                    continue;

                // Find API type info for this field type
                var apiType = BindingsGenerator.FindApiTypeInfo(buildData, fieldInfo.Type, this);
                if (apiType != null)
                    continue;

                throw new Exception($"Unknown field type '{fieldInfo.Type} {fieldInfo.Name}' in structure '{Name}'.");
            }
        }

        public override void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, Fields);
            BindingsGenerator.Write(writer, Functions);
            writer.Write(IsAutoSerialization);
            writer.Write(ForceNoPod);
            writer.Write(NoDefault);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Fields = BindingsGenerator.Read(reader, Fields);
            Functions = BindingsGenerator.Read(reader, Functions);
            IsAutoSerialization = reader.ReadBoolean();
            ForceNoPod = reader.ReadBoolean();
            NoDefault = reader.ReadBoolean();

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
