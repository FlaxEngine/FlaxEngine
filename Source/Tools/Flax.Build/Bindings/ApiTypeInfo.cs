// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native type information for bindings generator.
    /// </summary>
    public class ApiTypeInfo : IBindingsCache
    {
        public ApiTypeInfo Parent;
        public List<ApiTypeInfo> Children = new List<ApiTypeInfo>();
        public string NativeName;
        public string Name;
        public string Namespace;
        public string Attributes;
        public string[] Comment;
        public bool IsInBuild;
        internal bool IsInited;

        public virtual bool IsClass => false;
        public virtual bool IsStruct => false;
        public virtual bool IsEnum => false;
        public virtual bool IsInterface => false;
        public virtual bool IsValueType => false;
        public virtual bool IsScriptingObject => false;
        public virtual bool IsPod => false;

        public FileInfo File
        {
            get
            {
                var result = this;
                while (result != null && !(result is FileInfo))
                    result = result.Parent;
                return result as FileInfo;
            }
        }

        /// <summary>
        /// Gets the name of the type as it would be referenced from the parent module namespace in the native code. It includes the nesting parent types and typename. For instance enum defined in class will have prefix of that class name.
        /// </summary>
        public string FullNameNative
        {
            get
            {
                var result = string.IsNullOrEmpty(NativeName) ? Name : NativeName;
                if (Parent != null && !(Parent is FileInfo))
                    result = Parent.FullNameNative + "::" + result;
                return result;
            }
        }

        /// <summary>
        /// Gets the name of the type as it would be referenced from the parent module namespace in the managed code. It includes the namespace, nesting parent types and typename. For instance enum defined in class will have prefix of that class namespace followed by the class name.
        /// </summary>
        public string FullNameManaged
        {
            get
            {
                string result;
                if (string.IsNullOrEmpty(Namespace))
                    result = Name;
                else
                    result = Namespace + '.' + Name;
                if (Parent != null && !(Parent is FileInfo))
                    result = Parent.FullNameManaged + '+' + result;
                return result;
            }
        }

        public virtual void Init(Builder.BuildData buildData)
        {
            IsInited = true;

            if (Children != null)
            {
                for (int i = 0; i < Children.Count; i++)
                {
                    var child = Children[i];
                    if (!child.IsInited)
                        child.Init(buildData);
                }
            }
        }

        public virtual void AddChild(ApiTypeInfo apiTypeInfo)
        {
            apiTypeInfo.Parent = this;
            Children.Add(apiTypeInfo);
        }

        public virtual void Write(BinaryWriter writer)
        {
            BindingsGenerator.Write(writer, NativeName);
            BindingsGenerator.Write(writer, Name);
            BindingsGenerator.Write(writer, Namespace);
            BindingsGenerator.Write(writer, Attributes);
            BindingsGenerator.Write(writer, Comment);
            writer.Write(IsInBuild);
            BindingsGenerator.Write(writer, Children);
        }

        public virtual void Read(BinaryReader reader)
        {
            NativeName = BindingsGenerator.Read(reader, NativeName);
            Name = BindingsGenerator.Read(reader, Name);
            Namespace = BindingsGenerator.Read(reader, Namespace);
            Attributes = BindingsGenerator.Read(reader, Attributes);
            Comment = BindingsGenerator.Read(reader, Comment);
            IsInBuild = reader.ReadBoolean();
            Children = BindingsGenerator.Read(reader, Children);

            // Fix hierarchy
            for (int i = 0; i < Children.Count; i++)
            {
                var child = Children[i];
                child.Parent = this;
            }
        }

        public override string ToString()
        {
            return Name;
        }
    }
}
