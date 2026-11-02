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
        public List<PropertyInfo> Properties = new List<PropertyInfo>();
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

            // Give struct methods stable, unique binding names. These names are used by both
            // the native wrapper and the generated LibraryImport entry point. Class methods
            // get this in VirtualClassInfo.Init, but structures do not derive from that type.
            var usedFunctionNames = new HashSet<string>(StringComparer.Ordinal);
            for (var baseType = BaseType; baseType != null; baseType = baseType.BaseType)
            {
                foreach (var baseFunction in baseType.Functions)
                    usedFunctionNames.Add(string.IsNullOrEmpty(baseFunction.UniqueName) ? baseFunction.Name : baseFunction.UniqueName);
                if (baseType is ClassInfo baseClass)
                {
                    foreach (var baseProperty in baseClass.Properties)
                    {
                        if (baseProperty.Getter != null)
                            usedFunctionNames.Add(string.IsNullOrEmpty(baseProperty.Getter.UniqueName) ? baseProperty.Getter.Name : baseProperty.Getter.UniqueName);
                        if (baseProperty.Setter != null)
                            usedFunctionNames.Add(string.IsNullOrEmpty(baseProperty.Setter.UniqueName) ? baseProperty.Setter.Name : baseProperty.Setter.UniqueName);
                    }
                }
                else if (baseType is StructureInfo baseStructure)
                {
                    foreach (var baseProperty in baseStructure.Properties)
                    {
                        if (baseProperty.Getter != null)
                            usedFunctionNames.Add(string.IsNullOrEmpty(baseProperty.Getter.UniqueName) ? baseProperty.Getter.Name : baseProperty.Getter.UniqueName);
                        if (baseProperty.Setter != null)
                            usedFunctionNames.Add(string.IsNullOrEmpty(baseProperty.Setter.UniqueName) ? baseProperty.Setter.Name : baseProperty.Setter.UniqueName);
                    }
                }
            }
            var structFunctions = new List<FunctionInfo>(Functions);
            foreach (var propertyInfo in Properties)
            {
                if (propertyInfo.Getter != null)
                    structFunctions.Add(propertyInfo.Getter);
                if (propertyInfo.Setter != null)
                    structFunctions.Add(propertyInfo.Setter);
            }
            foreach (var functionInfo in structFunctions)
            {
                var uniqueName = functionInfo.Name;
                var suffix = 1;
                while (!usedFunctionNames.Add(uniqueName))
                    uniqueName = functionInfo.Name + suffix++;
                functionInfo.UniqueName = uniqueName;
            }

            // Virtual methods add a native vtable pointer, which cannot be represented by a POD C# struct.
            // Require an explicit NoPod annotation so the struct uses the non-POD binding path.
            if (!ForceNoPod)
            {
                foreach (var functionInfo in Functions)
                {
                    if (functionInfo.IsVirtual)
                        throw new NotSupportedException($"Structure '{FullNameNative}' declares virtual API function '{functionInfo.Name}' but is not marked API_STRUCT(NoPod). Virtual struct methods require the non-POD layout.");
                }
                foreach (var propertyInfo in Properties)
                {
                    if (propertyInfo.Getter?.IsVirtual == true)
                        throw new NotSupportedException($"Structure '{FullNameNative}' declares virtual API property getter '{propertyInfo.Getter.Name}' but is not marked API_STRUCT(NoPod).");
                    if (propertyInfo.Setter?.IsVirtual == true)
                        throw new NotSupportedException($"Structure '{FullNameNative}' declares virtual API property setter '{propertyInfo.Setter.Name}' but is not marked API_STRUCT(NoPod).");
                }
            }

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
            BindingsGenerator.Write(writer, Properties);
            BindingsGenerator.Write(writer, Functions);
            writer.Write(IsAutoSerialization);
            writer.Write(ForceNoPod);
            writer.Write(NoDefault);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            Fields = BindingsGenerator.Read(reader, Fields);
            Properties = BindingsGenerator.Read(reader, Properties);
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
