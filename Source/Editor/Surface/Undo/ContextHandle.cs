// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Text;
using FlaxEngine;
using FlaxEngine.Json;

namespace FlaxEditor.Surface.Undo
{
    /// <summary>
    /// The helper structure for Surface context handle.
    /// </summary>
    [HideInEditor]
    public struct ContextHandle : IEquatable<ContextHandle>
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
        /// Initializes a new instance of the <see cref="ContextHandle"/> struct.
        /// </summary>
        /// <param name="child">The child context provider.</param>
        public ContextHandle(ISurfaceContext child)
        {
            if (child == null)
                throw new ArgumentNullException();
            VisjectSurfaceContext parent = child.ParentContext;
            VisjectSurfaceContext context = parent;
            int count = 1;
            while (parent != null)
            {
                count++;
                parent = parent.Parent;
            }
            _path = new string[count];
            _path[0] = child.SurfaceName;
            for (int i = 1; i < count; i++)
            {
                _path[i] = context.Context.SurfaceName;
                context = context.Parent;
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

        /// <inheritdoc />
        public bool Equals(ContextHandle other)
        {
            return JsonSerializer.ValueEquals(_path, other._path);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is ContextHandle other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            int hash = 17;
            if (_path != null)
            {
                unchecked
                {
                    for (var i = 0; i < _path.Length; i++)
                    {
                        var item = _path[i];
                        hash = hash * 23 + (item != null ? item.GetHashCode() : 0);
                    }
                }
            }
            return hash;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            var sb = new StringBuilder();
            if (_path == null)
                return string.Empty;
            for (int i = _path.Length - 1; i >= 0; i--)
                sb.Append(_path[i]).Append('/');
            return sb.ToString();
        }
    }
}
