// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if FLAX_EDITOR
using System;
using FlaxEngine.Utilities;
using FlaxEditor.Scripting;
using FlaxEngine.GUI;
#endif

namespace FlaxEngine
{
    partial class BehaviorTreeRootNode
    {
#if FLAX_EDITOR
        private static bool IsValidBlackboardType(ScriptType type)
        {
            if (ScriptType.FlaxObject.IsAssignableFrom(type))
                return false;
            if (type.Type != null)
            {
                if (type.Type.IsDelegate())
                    return false;
                if (typeof(Control).IsAssignableFrom(type.Type))
                    return false;
                if (typeof(Attribute).IsAssignableFrom(type.Type))
                    return false;
                if (type.Type.FullName.StartsWith("FlaxEditor.", StringComparison.Ordinal))
                    return false;
            }
            return !type.IsGenericType &&
                   !type.IsInterface &&
                   !type.IsStatic &&
                   !type.IsAbstract &&
                   !type.IsArray &&
                   !type.IsVoid &&
                   (type.IsClass || type.IsStructure) &&
                   type.IsPublic &&
                   type.CanCreateInstance;
        }
#endif
    }
}
