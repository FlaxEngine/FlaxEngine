// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        /// <summary>
        /// The collection of the surface parameters.
        /// </summary>
        /// <remarks>From the root context only.</remarks>
        public List<SurfaceParameter> Parameters => RootContext.Parameters;

        /// <summary>
        /// Gets the parameter by the given ID.
        /// </summary>
        /// <remarks>From the root context only.</remarks>
        /// <param name="id">The identifier.</param>
        /// <returns>Found parameter instance or null if missing.</returns>
        public SurfaceParameter GetParameter(Guid id)
        {
            return RootContext.GetParameter(id);
        }

        /// <summary>
        /// Gets the parameter by the given name.
        /// </summary>
        /// <remarks>From the root context only.</remarks>
        /// <param name="name">The name.</param>
        /// <returns>Found parameter instance or null if missing.</returns>
        public SurfaceParameter GetParameter(string name)
        {
            return RootContext.GetParameter(name);
        }

        /// <inheritdoc />
        public void OnParamReordered()
        {
            MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnParamCreated(SurfaceParameter param)
        {
            for (int i = 0; i < Nodes.Count; i++)
            {
                if (Nodes[i] is IParametersDependantNode node)
                    node.OnParamCreated(param);
            }
            MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnParamRenamed(SurfaceParameter param)
        {
            for (int i = 0; i < Nodes.Count; i++)
            {
                if (Nodes[i] is IParametersDependantNode node)
                    node.OnParamRenamed(param);
            }
            MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnParamEdited(SurfaceParameter param)
        {
            for (int i = 0; i < Nodes.Count; i++)
            {
                if (Nodes[i] is IParametersDependantNode node)
                    node.OnParamEdited(param);
            }
            MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnParamDeleted(SurfaceParameter param)
        {
            for (int i = 0; i < Nodes.Count; i++)
            {
                if (Nodes[i] is IParametersDependantNode node)
                    node.OnParamDeleted(param);
            }
            MarkAsEdited();
        }

        /// <inheritdoc />
        public bool IsParamUsed(SurfaceParameter param)
        {
            for (int i = 0; i < Nodes.Count; i++)
            {
                if (Nodes[i] is IParametersDependantNode node && node.IsParamUsed(param))
                    return true;
            }
            return false;
        }
    }
}
