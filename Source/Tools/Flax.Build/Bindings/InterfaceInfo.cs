// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;

namespace Flax.Build.Bindings
{
    /// <summary>
    /// The native class/structure interface information for bindings generator.
    /// </summary>
    public class InterfaceInfo : VirtualClassInfo
    {
        public List<FieldInfo> Fields = new List<FieldInfo>();
        public override int GetScriptVTableSize(out int offset)
        {
            offset = 0;
            return Functions.Count(x => x.IsVirtual);
        }

        public override int GetScriptVTableOffset(VirtualClassInfo classInfo)
        {
            return 0;
        }

        public override bool IsInterface => true;

        public override void Init(Builder.BuildData buildData)
        {
            base.Init(buildData);

            if (BaseType != null)
                throw new Exception(string.Format("Interface {0} cannot inherit from {1}.", FullNameNative, BaseType));
            if (Interfaces != null)
            {
                for (int i = 0; i < Interfaces.Count; i++)
                {
                    if (!Interfaces[i].IsInterface)
                    {
                        throw new Exception(string.Format("Interface {0} cannot inherit from {1}. Because the {1} is not a Interface", FullNameNative, Interfaces[i]));
                    }
                }
            }
            foreach (var fieldInfo in Fields)
            {
                if (fieldInfo.IsHidden)
                    continue;

                fieldInfo.Getter = new FunctionInfo
                {
                    Name = "Get" + fieldInfo.Name,
                    Comment = fieldInfo.Comment,
                    IsStatic = fieldInfo.IsStatic,
                    Access = fieldInfo.Access,
                    Attributes = fieldInfo.Attributes,
                    ReturnType = fieldInfo.Type,
                    Parameters = new List<FunctionInfo.ParameterInfo>(),
                    IsVirtual = false,
                    IsConst = true,
                    Glue = new FunctionInfo.GlueInfo()
                };
                ProcessAndValidate(fieldInfo.Getter);
                fieldInfo.Getter.Name = fieldInfo.Name;

                if (!fieldInfo.IsReadOnly)
                {
                    fieldInfo.Setter = new FunctionInfo
                    {
                        Name = "Set" + fieldInfo.Name,
                        Comment = fieldInfo.Comment,
                        IsStatic = fieldInfo.IsStatic,
                        Access = fieldInfo.Access,
                        Attributes = fieldInfo.Attributes,
                        ReturnType = new TypeInfo
                        {
                            Type = "void",
                        },
                        Parameters = new List<FunctionInfo.ParameterInfo>
                        {
                            new FunctionInfo.ParameterInfo
                            {
                                Name = "value",
                                Type = fieldInfo.Type,
                            },
                        },
                        IsVirtual = false,
                        IsConst = true,
                        Glue = new FunctionInfo.GlueInfo()
                    };
                    ProcessAndValidate(fieldInfo.Setter);
                    fieldInfo.Setter.Name = fieldInfo.Name;
                }
            }
        }

        public override string ToString()
        {
            return "interface " + Name;
        }
    }
}
