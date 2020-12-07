// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Surface.Undo
{
    /// <summary>
    /// The helper structure for Surface context handle.
    /// </summary>
    [HideInEditor]
    public struct ContextHandle
    {
        private readonly string[] _path;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextHandle"/> struct.
        /// </summary>
        /// <param name="context">The context.</param>
        public ContextHandle(VisjectSurfaceContext context)
        {
            int count = 0;
            var parent = context.Parent;
            while (parent != null)
            {
                count++;
                parent = parent.Parent;
            }
            if (count == 0)
            {
                // Root context
                _path = null;
            }
            else
            {
                _path = new string[count];
                for (int i = 0; i < count; i++)
                {
                    _path[i] = context.Context.SurfaceName;
                    context = context.Parent;
                }
            }
        }

        /// <summary>
        /// Gets the context.
        /// </summary>
        /// <param name="surface">The Surface.</param>
        /// <returns>The restored context.</returns>
        public VisjectSurfaceContext Get(VisjectSurface surface)
        {
            var context = surface.RootContext;
            var count = _path?.Length ?? 0;
            for (int i = count - 1; i >= 0; i--)
            {
                var found = false;
                foreach (var child in context.Children)
                {
                    if (child.Context.SurfaceName == _path[i])
                    {
                        found = true;
                        context = child;
                        break;
                    }
                }
                if (!found)
                    throw new Exception(string.Format("Missing context \'{0}\' in \'{1}\'", _path[i], context.Context.SurfaceName));
            }

            return context;
        }
    }
}
