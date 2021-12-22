// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
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
        private Dictionary<ScriptType, string> _typeCache = new Dictionary<ScriptType, string>();
        private Dictionary<ScriptMemberInfo, string> _memberCache = new Dictionary<ScriptMemberInfo, string>();

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
            if (_typeCache.TryGetValue(type, out var text))
                return text;

            if (attributes == null)
                attributes = type.GetAttributes(false);
            text = type.TypeName;
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text += '\n' + tooltip.Text;
            _typeCache.Add(type, text);
            return text;
        }

        /// <summary>
        /// Gets the tooltip text for the type member.
        /// </summary>
        /// <param name="member">The type member.</param>
        /// <param name="attributes">The member attributes. Optional, if null member attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptMemberInfo member, object[] attributes = null)
        {
            if (_memberCache.TryGetValue(member, out var text))
                return text;

            if (attributes == null)
                attributes = member.GetAttributes(true);
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text = tooltip.Text;
            _memberCache.Add(member, text);
            return text;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            base.OnInit();

            Editor.CodeEditing.TypesCleared += OnTypesCleared;
        }

        private void OnTypesCleared()
        {
            _typeCache.Clear();
            _memberCache.Clear();
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            OnTypesCleared();

            base.OnExit();
        }
    }
}
