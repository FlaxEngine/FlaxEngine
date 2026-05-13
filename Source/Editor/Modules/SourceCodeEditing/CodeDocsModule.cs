// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
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
        public string GetTooltip(Type type, object[] attributes = null)
        {
            return GetTooltip(new ScriptType(type), attributes);
        }

        /// <summary>
        /// Gets the tooltip text for the type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="attributes">The type attributes. Optional, if null type attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptType type, object[] attributes = null)
        {
            // Try to use cache
            if (_typeCache.TryGetValue(type, out var text))
                return text;

            // Try to use tooltip attribute
            if (attributes == null)
                attributes = type.GetAttributes(false);
            text = type.TypeName;
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text += '\n' + tooltip.Text;
            else if (type.Type != null)
            {
                // Try to use xml docs for managed type
                var xmlDoc = DebugCommands.GetXml(type.Type);
                if (xmlDoc != null)
                    text += '\n' + xmlDoc;
            }

            _typeCache.Add(type, text);
            return text;
        }

        /// <summary>
        /// Gets the tooltip text for the type member.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="memberName">The member name.</param>
        /// <param name="attributes">The member attributes. Optional, if null member attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(Type type, string memberName, object[] attributes = null)
        {
            var member = new ScriptType(type).GetMember(memberName, MemberTypes.All, BindingFlags.Public | BindingFlags.Static | BindingFlags.Instance);
            return GetTooltip(member, attributes);
        }

        /// <summary>
        /// Gets the tooltip text for the type member.
        /// </summary>
        /// <param name="member">The type member.</param>
        /// <param name="attributes">The member attributes. Optional, if null member attributes will be used.</param>
        /// <returns>The documentation tooltip.</returns>
        public string GetTooltip(ScriptMemberInfo member, object[] attributes = null)
        {
            // Try to use cache
            if (_memberCache.TryGetValue(member, out var text))
                return text;

            // Try to use tooltip attribute
            if (attributes == null)
                attributes = member.GetAttributes(true);
            var tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
            if (tooltip != null)
                text = tooltip.Text;
            else if (member.Type != null)
            {
                // Try to use xml docs for managed member
                var xmlDoc = DebugCommands.GetXml(member.Type);
                if (xmlDoc != null)
                    text = xmlDoc;
            }

            _memberCache.Add(member, text);
            return text;
        }

        private void OnTypesCleared()
        {
            _typeCache.Clear();
            _memberCache.Clear();
            DebugCommands.ClearXml();
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            base.OnInit();

            Editor.CodeEditing.TypesCleared += OnTypesCleared;
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            OnTypesCleared();

            base.OnExit();
        }
    }
}
