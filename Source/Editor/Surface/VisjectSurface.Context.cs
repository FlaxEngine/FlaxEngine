// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Surface.Undo;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        private VisjectSurfaceContext _root;
        private VisjectSurfaceContext _context;
        private readonly Dictionary<ContextHandle, VisjectSurfaceContext> _contextCache = new Dictionary<ContextHandle, VisjectSurfaceContext>();

        /// <summary>
        /// The surface context stack.
        /// </summary>
        public readonly Stack<VisjectSurfaceContext> ContextStack = new Stack<VisjectSurfaceContext>(8);

        /// <summary>
        /// Gets the root surface context.
        /// </summary>
        public VisjectSurfaceContext RootContext => _root;

        /// <summary>
        /// Gets the active surface context.
        /// </summary>
        public VisjectSurfaceContext Context => _context;

        /// <summary>
        /// Occurs when context gets changed.
        /// </summary>
        public event Action<VisjectSurfaceContext> ContextChanged;

        /// <summary>
        /// Finds the surface context with the given owning nodes IDs path.
        /// </summary>
        /// <param name="nodePath">The node ids path.</param>
        /// <returns>Found context or null if cannot.</returns>
        public VisjectSurfaceContext FindContext(Span<uint> nodePath)
        {
            // Get size of the path
            int nodePathSize = 0;
            while (nodePathSize < nodePath.Length && nodePath[nodePathSize] != 0)
                nodePathSize++;

            // Follow each context path to verify if it matches with the path in the input path
            foreach (var e in _contextCache)
            {
                var c = e.Value;
                for (int i = nodePathSize - 1; i >= 0 && c != null; i--)
                    c = c.OwnerNodeID == nodePath[i] ? c.Parent : null;
                if (c != null)
                    return e.Value;
            }
            return null;
        }

        /// <summary>
        /// Opens the surface context with the given owning nodes IDs path.
        /// </summary>
        /// <param name="nodePath">The node ids path.</param>
        /// <returns>Found context or null if cannot.</returns>
        public VisjectSurfaceContext OpenContext(Span<uint> nodePath)
        {
            OpenContext(RootContext.Context);
            if (nodePath != null && nodePath.Length != 0)
            {
                for (int i = 0; i < nodePath.Length; i++)
                {
                    var node = Context.FindNode(nodePath[i]);
                    if (node is ISurfaceContext context)
                        OpenContext(context);
                    else
                        return null;
                }
            }
            return Context;
        }

        /// <summary>
        /// Creates the Visject surface context for the given surface data source context.
        /// </summary>
        /// <param name="parent">The parent context.</param>
        /// <param name="context">The context.</param>
        /// <returns></returns>
        protected virtual VisjectSurfaceContext CreateContext(VisjectSurfaceContext parent, ISurfaceContext context)
        {
            return new VisjectSurfaceContext(this, parent, context);
        }

        /// <summary>
        /// Opens the child context of the current context.
        /// </summary>
        /// <param name="context">The context.</param>
        public void OpenContext(ISurfaceContext context)
        {
            if (context == null)
                throw new ArgumentNullException(nameof(context));
            if (_context != null && _context.Context == context)
                return;

            // Get or create context
            var contextHandle = new ContextHandle(context);
            if (!_contextCache.TryGetValue(contextHandle, out VisjectSurfaceContext surfaceContext))
            {
                surfaceContext = CreateContext(_context, context);
                _context?.Children.Add(surfaceContext);
                _contextCache.Add(contextHandle, surfaceContext);
                if (context is SurfaceNode asNode)
                    surfaceContext.OwnerNodeID = asNode.ID;

                context.OnContextCreated(surfaceContext);

                // Load context
                if (_root != null)
                {
                    if (surfaceContext.Load())
                        throw new Exception("Failed to load graph.");
                }
            }
            if (_root == null)
                _root = surfaceContext;
            else if (ContextStack.Contains(surfaceContext))
            {
                // Go up until the given context
                while (ContextStack.First() != surfaceContext)
                    CloseContext();
                return;
            }

            // Change stack
            ContextStack.Push(surfaceContext);

            // Update
            OnContextChanged();
        }

        /// <summary>
        /// Closes the last opened context (the current one).
        /// </summary>
        public void CloseContext()
        {
            if (ContextStack.Count == 0)
                throw new ArgumentException("No context to close.");

            // Change stack
            ContextStack.Pop();

            // Update
            OnContextChanged();
        }

        /// <summary>
        /// Removes the context from the surface and any related cached data.
        /// </summary>
        /// <param name="context">The context.</param>
        public void RemoveContext(ISurfaceContext context)
        {
            // Skip if surface is already disposing
            if (IsDisposing || _isReleasing)
                return;

            // Validate input
            if (context == null)
                throw new ArgumentNullException(nameof(context));

            // Removing root requires to close every context
            if (RootContext != null && context == RootContext.Context)
            {
                while (ContextStack.Count > 0)
                    CloseContext();
            }

            // Check if has context in cache
            var contextHandle = new ContextHandle(context);
            if (_contextCache.TryGetValue(contextHandle, out VisjectSurfaceContext surfaceContext))
            {
                // Remove from navigation path
                while (ContextStack.Contains(surfaceContext))
                    CloseContext();

                // Dispose
                surfaceContext.Clear();
                _contextCache.Remove(contextHandle);
            }
        }

        /// <summary>
        /// Changes the current opened context to the given one. Used as a navigation method.
        /// </summary>
        /// <param name="context">The target context.</param>
        public void ChangeContext(ISurfaceContext context)
        {
            if (context == null)
                throw new ArgumentNullException(nameof(context));
            if (_context == null)
            {
                OpenContext(context);
                return;
            }
            if (_context.Context == context)
                return;

            // Check if already in a path
            var contextHandle = new ContextHandle(context);
            if (_contextCache.TryGetValue(contextHandle, out VisjectSurfaceContext surfaceContext) && ContextStack.Contains(surfaceContext))
            {
                // Change stack
                do
                {
                    ContextStack.Pop();
                } while (ContextStack.Peek() != surfaceContext);
            }
            else
            {
                // TODO: implement this case (need to find first parent of the context that is in path)
                throw new NotSupportedException("TODO: support changing context to one not in the active path");
            }

            // Update
            OnContextChanged();
        }

        /// <summary>
        /// Called when context gets changed. Updates current context and UI. Updates the current context based on the first element in the stack.
        /// </summary>
        protected virtual void OnContextChanged()
        {
            // Cache viewport of the context (used to restore when leaving it)
            if (_context != null)
            {
                _context._cachedViewCenterPosition = ViewCenterPosition;
                _context._cachedViewScale = ViewScale;
            }

            var context = ContextStack.Count > 0 ? ContextStack.Peek() : null;
            _context = context;
            if (ContextStack.Count == 0)
                _root = null;

            // Update root control linkage
            if (_rootControl != null)
            {
                _rootControl.Parent = null;
            }
            if (context != null)
            {
                _rootControl = _context.RootControl;
                _rootControl.Parent = this;
            }
            else
            {
                _rootControl = null;
            }

            ContextChanged?.Invoke(_context);

            // Restore viewport in the context
            if (_context?._cachedViewScale > 0.0f)
            {
                ViewScale = _context._cachedViewScale;
                ViewCenterPosition = _context._cachedViewCenterPosition;
            }
            else
            {
                // Show whole surface on load
                ShowWholeGraph();
            }
        }
    }
}
