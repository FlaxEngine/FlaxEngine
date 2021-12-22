// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Modules.SourceCodeEditing
{
    /// <summary>
    /// Source code documentation module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class CodeDocsModule : EditorModule
    {
        internal CodeDocsModule(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Gets the tooltip text for the type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="attributes">The type attributes. Optional, if null type attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptType type, object[] attributes = null)
        {
            if (attributes == null)
                attributes = type.GetAttributes(false);
            var text = type.TypeName;
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text += '\n' + tooltip.Text;
            return text;
        }

        /// <summary>
        /// Gets the tooltip text for the type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="attributes">The type attributes. Optional, if null type attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptMemberInfo type, object[] attributes = null)
        {
            if (attributes == null)
                attributes = type.GetAttributes(true);
            string text = null;
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text = tooltip.Text;
            return text;
        }
    }
}
