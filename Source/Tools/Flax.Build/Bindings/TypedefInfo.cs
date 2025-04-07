// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The type definition for bindings generator.
    /// </summary>
    public class TypedefInfo : ApiTypeInfo
    {
        public bool IsAlias;
        public TypeInfo Type;
        public ApiTypeInfo TypeInfo; // Cached info of Type
        public ApiTypeInfo Typedef;

        // Guards to prevent looped initialization for typedefs that are recursive
        internal static TypedefInfo Current;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            // Remove previous typedef (if any)
            if (Typedef == null)
            {
                foreach (var e in Parent.Children)
                {
                    if (e != this && e.Name == Name)
                    {
                        Typedef = e;
                        break;
                    }
                }
            }
            if (Typedef != null)
            {
                Typedef.Parent = null;
                Parent.Children.Remove(Typedef);
                Typedef = null;
            }

            // Find typedef type
            var current = Current;
            Current = this;
            var apiTypeInfo = BindingsGenerator.FindApiTypeInfo(buildData, Type, Parent);
            Current = current;
            if (apiTypeInfo == null)
                throw new Exception(string.Format("Unknown type '{0}' for typedef '{1}'.", Type, Name));
            apiTypeInfo.EnsureInited(buildData);
            TypeInfo = apiTypeInfo;

            // Alias type without introducing any new type
            if (IsAlias || apiTypeInfo is LangType)
            {
                Typedef = apiTypeInfo;
                return;
            }

            try
            {
                // Duplicate type
                var typedef = (ApiTypeInfo)apiTypeInfo.Clone();
                typedef.Instigator = this;
                typedef.NativeName = NativeName ?? Name;
                typedef.Name = Name;
                typedef.Namespace = Namespace;
                if (!string.IsNullOrEmpty(Attributes))
                {
                    if (string.IsNullOrEmpty(typedef.Attributes))
                        typedef.Attributes = Attributes;
                    else
                        typedef.Attributes += ',' + Attributes;
                }
                if (Comment != null && Comment.Length != 0)
                    typedef.Comment = Comment;
                typedef.IsInBuild |= IsInBuild;
                typedef.DeprecatedMessage = DeprecatedMessage;
                if (typedef is ClassStructInfo typedefClassStruct && typedefClassStruct.IsTemplate)
                {
                    // Inflate template type
                    typedefClassStruct.IsTemplate = false;
                    if (typedefClassStruct is ClassInfo typedefClass)
                    {
                        foreach (var fieldInfo in typedefClass.Fields)
                            InflateType(buildData, typedefClassStruct, ref fieldInfo.Type);
                    }
                    else if (typedefClassStruct is StructureInfo typedefStruct)
                    {
                        foreach (var fieldInfo in typedefStruct.Fields)
                            InflateType(buildData, typedefStruct, ref fieldInfo.Type);
                    }
                }

                // Add to the hierarchy
                typedef.Parent = Parent;
                Parent.Children.Add(typedef);
                typedef.EnsureInited(buildData);
                Typedef = typedef;
            }
            catch (Exception)
            {
                Log.Error($"Failed to typedef '{Type}' as '{Name}'.");
                throw;
            }
        }

        private void InflateType(Builder.BuildData buildData, ClassStructInfo typedef, ref TypeInfo typeInfo)
        {
            if (BindingsGenerator.CSharpNativeToManagedBasicTypes.ContainsKey(typeInfo.Type))
                return;
            if (BindingsGenerator.CSharpNativeToManagedDefault.ContainsKey(typeInfo.Type))
                return;

            // Find API type info for this field type
            var apiType = BindingsGenerator.FindApiTypeInfo(buildData, typeInfo, typedef);
            if (apiType == null)
            {
                // TODO: implement more advanced template types inflating with tokenization of the generic definition
                typeInfo = Type.GenericArgs[0];
            }
        }

        public override void Write(BinaryWriter writer)
        {
            writer.Write(IsAlias);
            BindingsGenerator.Write(writer, Type);

            base.Write(writer);
        }

        public override void Read(BinaryReader reader)
        {
            IsAlias = reader.ReadBoolean();
            Type = BindingsGenerator.Read(reader, Type);

            base.Read(reader);
        }

        public override string ToString()
        {
            return "typedef " + Type + " " + Name;
        }
    }
}
