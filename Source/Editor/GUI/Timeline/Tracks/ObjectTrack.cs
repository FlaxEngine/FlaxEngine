// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The base interface for <see cref="ObjectTrack"/>.
    /// </summary>
    [HideInEditor]
    public interface IObjectTrack
    {
        /// <summary>
        /// Gets the object instance (may be null if reference is invalid or data is missing).
        /// </summary>
        object Object { get; }
    }

    /// <summary>
    /// The timeline track for animating managed objects.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public abstract class ObjectTrack : ConductorTrack, IObjectTrack
    {
        private bool _hasObject;

        /// <summary>
        /// The add button.
        /// </summary>
        protected Button _addButton;

        /// <summary>
        /// Gets the object instance (may be null if reference is invalid or data is missing).
        /// </summary>
        public abstract object Object { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ObjectTrack"/> class.
        /// </summary>
        /// <param name="options">The track initial options.</param>
        /// <param name="useProxyKeyframes">True if show sub-tracks keyframes as a proxy on this track, otherwise false.</param>
        protected ObjectTrack(ref TrackCreateOptions options, bool useProxyKeyframes = true)
        : base(ref options, useProxyKeyframes)
        {
            // Add track button
            const float buttonSize = 14;
            _addButton = new Button
            {
                Text = "+",
                TooltipText = "Add sub-tracks",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Offsets = new Margin(-buttonSize - 2 + _muteCheckbox.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            _addButton.Clicked += OnAddButtonClicked;
        }

        private void OnAddButtonClicked()
        {
            var menu = new ContextMenu.ContextMenu();
            OnShowAddContextMenu(menu);
            menu.Show(_addButton.Parent, _addButton.BottomLeft);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            var obj = Object;
            var hasObject = obj != null;
            TitleTintColor = hasObject ? (Flags.HasFlag(TrackFlags.PrefabObject) ? Style.Current.ProgressNormal : Color.White) : Color.Red;
            if (hasObject != _hasObject)
                OnObjectExistenceChanged(obj);
            _hasObject = hasObject;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _addButton = null;

            base.OnDestroy();
        }

        /// <summary>
        /// Called when object existence gets changed (eg. object found after being not found).
        /// </summary>
        /// <param name="obj">The object.</param>
        protected virtual void OnObjectExistenceChanged(object obj)
        {
        }

        /// <summary>
        /// Called when showing the context menu for add button (for sub-tracks adding).
        /// </summary>
        /// <param name="menu">The menu.</param>
        protected virtual void OnShowAddContextMenu(ContextMenu.ContextMenu menu)
        {
        }

        /// <summary>
        /// The data for add property track buttons tag.
        /// </summary>
        [HideInEditor]
        public struct AddMemberTag
        {
            /// <summary>
            /// The member.
            /// </summary>
            public MemberInfo Member;

            /// <summary>
            /// The archetype.
            /// </summary>
            public TrackArchetype Archetype;
        }

        private static bool IsEventParamInvalid(ParameterInfo p)
        {
            var t = p.ParameterType;
            return !t.IsValueType;
        }

        /// <summary>
        /// Adds the object events track options to menu.
        /// </summary>
        /// <param name="parentTrack">The parent track.</param>
        /// <param name="menu">The menu.</param>
        /// <param name="type">The object type.</param>
        /// <param name="memberCheck">The custom callback that can reject the members that should not be animated. Returns true if member is valid. Can be null to skip this feature.</param>
        /// <returns>The added options count.</returns>
        public static int AddEvents(Track parentTrack, ContextMenu.ContextMenu menu, Type type, Func<MemberInfo, bool> memberCheck = null)
        {
            int count = 0;
            menu.Tag = parentTrack;

            // TODO: implement editor-wide cache for animated properties per object type (add this in CodeEditingModule)
            var members = type.GetMembers(BindingFlags.Public | BindingFlags.Instance);

            var sb = new StringBuilder();
            for (int i = 0; i < members.Length; i++)
            {
                var m = members[i] as MethodInfo;
                if (m == null)
                    continue;
                if (memberCheck != null && !memberCheck(m))
                    continue;

                // Skip properties getters/setters and events add/remove
                var mName = m.Name;
                if (mName.StartsWith("get_", StringComparison.Ordinal) ||
                    mName.StartsWith("set_", StringComparison.Ordinal) ||
                    mName.StartsWith("add_", StringComparison.Ordinal) ||
                    mName.StartsWith("remove_", StringComparison.Ordinal))
                    continue;

                // Allow to invoke only void functions with basic parameter types
                var parameters = m.GetParameters();
                if (m.ReturnType != typeof(void) ||
                    parameters.Length > 8 ||
                    m.IsGenericMethod ||
                    parameters.Any(IsEventParamInvalid))
                    continue;

                var attributes = m.GetCustomAttributes();

                // Check if has attribute to skip animating
                if (attributes.Any(x => x is NoAnimateAttribute))
                    continue;

                // Prevent from adding the same track twice
                if (parentTrack.SubTracks.Any(x => x is MemberTrack y && y.MemberName == m.Name))
                    continue;

                // Build function name for UI
                sb.Clear();
                sb.Append(mName);
                sb.Append('(');
                for (int j = 0; j < parameters.Length; j++)
                {
                    if (j != 0)
                        sb.Append(", ");
                    var p = parameters[j];
                    if (!CustomEditorsUtil.InBuildTypeNames.TryGetValue(p.ParameterType, out var pName))
                        pName = p.ParameterType.Name;
                    sb.Append(pName);
                    sb.Append(' ');
                    sb.Append(p.Name);
                }
                sb.Append(')');

                AddMemberTag tag;
                tag.Member = m;
                tag.Archetype = EventTrack.GetArchetype();
                var tooltip = Surface.SurfaceUtils.GetVisualScriptMemberInfoDescription(new ScriptMemberInfo(m));
                menu.AddButton(sb.ToString(), OnAddMemberTrack).LinkTooltip(tooltip).Tag = tag;
                count++;
            }

            return count;
        }

        /// <summary>
        /// Called on context menu button click to add new object property animation track. Button should have <see cref="AddMemberTag"/> value assigned to the <see cref="Control.Tag"/> field.
        /// </summary>
        /// <param name="button">The button (with <see cref="AddMemberTag"/> value assigned to the <see cref="Control.Tag"/> field.).</param>
        public static void OnAddMemberTrack(ContextMenuButton button)
        {
            var tag = (AddMemberTag)button.Tag;
            var parentTrack = (Track)button.ParentContextMenu.Tag;

            var timeline = parentTrack.Timeline;
            var track = (MemberTrack)timeline.NewTrack(tag.Archetype);
            track.ParentTrack = parentTrack;
            track.TrackIndex = parentTrack.TrackIndex + 1;
            track.Name = Guid.NewGuid().ToString("N");
            track.Member = tag.Member;
            timeline.AddTrack(track);

            parentTrack.Expand();
        }

        /// <summary>
        /// Adds the property or field track to this object track.
        /// </summary>
        /// <param name="m">The member (property or a field).</param>
        /// <returns>The created track or null if failed.</returns>
        public MemberTrack AddPropertyTrack(MemberInfo m)
        {
            if (SubTracks.Any(x => x is MemberTrack y && y.MemberName == m.Name))
                return null;
            if (GetPropertyTrackType(m, out var t))
                return null;
            if (GetPropertyTrackArchetype(t, out var archetype, out var name))
                return null;

            var timeline = Timeline;
            var track = (MemberTrack)timeline.NewTrack(archetype);
            track.ParentTrack = this;
            track.TrackIndex = TrackIndex + 1;
            track.Name = Guid.NewGuid().ToString("N");
            track.Member = m;
            timeline.AddTrack(track);

            Expand();

            return track;
        }

        /// <summary>
        /// Adds the object properties animation track options to menu.
        /// </summary>
        /// <param name="parentTrack">The parent track.</param>
        /// <param name="menu">The menu.</param>
        /// <param name="type">The object type.</param>
        /// <param name="memberCheck">The custom callback that can reject the members that should not be animated. Returns true if member is valid. Can be null to skip this feature.</param>
        /// <returns>The added options count.</returns>
        public static int AddProperties(Track parentTrack, ContextMenu.ContextMenu menu, Type type, Func<MemberInfo, bool> memberCheck = null)
        {
            int count = 0;
            menu.Tag = parentTrack;

            // TODO: implement editor-wide cache for animated properties per object type (add this in CodeEditingModule)
            var members = type.GetMembers(BindingFlags.Public | BindingFlags.Instance);

            for (int i = 0; i < members.Length; i++)
            {
                var m = members[i];

                // Prevent from adding the same track twice
                if (parentTrack.SubTracks.Any(x => x is MemberTrack y && y.MemberName == m.Name))
                    continue;

                if (GetPropertyTrackType(m, out var t))
                    continue;

                if (memberCheck != null && !memberCheck(m))
                    continue;

                var attributes = m.GetCustomAttributes();

                // Check if has attribute to skip animating
                if (attributes.Any(x => x is NoAnimateAttribute))
                    continue;

                // Validate value type and pick the track archetype
                if (GetPropertyTrackArchetype(t, out var archetype, out var name))
                    continue;

                AddMemberTag tag;
                tag.Member = m;
                tag.Archetype = archetype;
                var tooltip = Surface.SurfaceUtils.GetVisualScriptMemberInfoDescription(new ScriptMemberInfo(m));
                menu.AddButton(name + " " + m.Name, OnAddMemberTrack).LinkTooltip(tooltip).Tag = tag;
                count++;
            }

            return count;
        }

        private static bool GetPropertyTrackType(MemberInfo m, out Type t)
        {
            t = null;
            if (m is PropertyInfo p)
            {
                // Properties with read/write
                if (!(p.CanRead && p.CanWrite && p.GetIndexParameters().GetLength(0) == 0))
                    return true;

                t = p.PropertyType;
            }
            else if (m is FieldInfo f)
            {
                t = f.FieldType;
            }
            else
            {
                return true;
            }

            return false;
        }

        private static bool GetPropertyTrackArchetype(Type valueType, out TrackArchetype archetype, out string name)
        {
            // Validate value type and pick the track archetype
            name = valueType.Name;
            if (BasicTypesTrackArchetypes.TryGetValue(valueType, out archetype))
            {
                // Basic type
                if (!CustomEditorsUtil.InBuildTypeNames.TryGetValue(valueType, out name))
                    name = valueType.Name;
            }
            else if (valueType.IsEnum)
            {
                // Enum
                archetype = KeyframesPropertyTrack.GetArchetype();
            }
            else if (valueType.IsValueType)
            {
                // Structure
                archetype = StructPropertyTrack.GetArchetype();
            }
            else if (typeof(FlaxEngine.Object).IsAssignableFrom(valueType))
            {
                // Flax object reference
                archetype = ObjectReferencePropertyTrack.GetArchetype();
            }
            else if (CanAnimateObjectType(valueType))
            {
                // Nested object
                archetype = ObjectPropertyTrack.GetArchetype();
            }
            else
            {
                // Not supported
                return true;
            }

            return false;
        }

        private static bool CanAnimateObjectType(Type type)
        {
            if (InvalidGenericTypes.Contains(type) || (type.IsGenericType && InvalidGenericTypes.Contains(type.GetGenericTypeDefinition())))
                return false;

            // Skip delegates
            if (typeof(MulticastDelegate).IsAssignableFrom(type.BaseType))
                return false;

            return !type.ContainsGenericParameters &&
                   !type.IsArray &&
                   !type.IsGenericType &&
                   type.IsClass;
        }

        private static readonly HashSet<Type> InvalidGenericTypes = new HashSet<Type>
        {
            typeof(Action), typeof(Action<>), typeof(Action<,>),
            typeof(Func<>), typeof(Func<,>), typeof(Func<,,>),
        };

        /// <summary>
        /// Maps the type to the default track archetype for it.
        /// </summary>
        protected static readonly Dictionary<Type, TrackArchetype> BasicTypesTrackArchetypes = new Dictionary<Type, TrackArchetype>
        {
            { typeof(bool), KeyframesPropertyTrack.GetArchetype() },
            { typeof(byte), KeyframesPropertyTrack.GetArchetype() },
            { typeof(sbyte), KeyframesPropertyTrack.GetArchetype() },
            { typeof(char), KeyframesPropertyTrack.GetArchetype() },
            { typeof(short), KeyframesPropertyTrack.GetArchetype() },
            { typeof(ushort), KeyframesPropertyTrack.GetArchetype() },
            { typeof(int), KeyframesPropertyTrack.GetArchetype() },
            { typeof(uint), KeyframesPropertyTrack.GetArchetype() },
            { typeof(long), KeyframesPropertyTrack.GetArchetype() },
            { typeof(float), CurvePropertyTrack.GetArchetype() },
            { typeof(double), CurvePropertyTrack.GetArchetype() },
            { typeof(Vector2), CurvePropertyTrack.GetArchetype() },
            { typeof(Vector3), CurvePropertyTrack.GetArchetype() },
            { typeof(Vector4), CurvePropertyTrack.GetArchetype() },
            { typeof(Float2), CurvePropertyTrack.GetArchetype() },
            { typeof(Float3), CurvePropertyTrack.GetArchetype() },
            { typeof(Float4), CurvePropertyTrack.GetArchetype() },
            { typeof(Double2), CurvePropertyTrack.GetArchetype() },
            { typeof(Double3), CurvePropertyTrack.GetArchetype() },
            { typeof(Double4), CurvePropertyTrack.GetArchetype() },
            { typeof(Quaternion), CurvePropertyTrack.GetArchetype() },
            { typeof(Color), CurvePropertyTrack.GetArchetype() },
            { typeof(Color32), CurvePropertyTrack.GetArchetype() },
            { typeof(Guid), KeyframesPropertyTrack.GetArchetype() },
            { typeof(DateTime), KeyframesPropertyTrack.GetArchetype() },
            { typeof(TimeSpan), KeyframesPropertyTrack.GetArchetype() },
            { typeof(LocalizedString), KeyframesPropertyTrack.GetArchetype() },
            { typeof(string), StringPropertyTrack.GetArchetype() },
        };
    }
}
