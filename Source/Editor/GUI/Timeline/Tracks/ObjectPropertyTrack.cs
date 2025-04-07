// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating sub-object properties via child tracks.
    /// </summary>
    /// <seealso cref="MemberTrack" />
    public class ObjectPropertyTrack : MemberTrack, IObjectTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 14,
                Name = "Property",
                DisableSpawnViaGUI = true,
                Create = options => new ObjectPropertyTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (ObjectPropertyTrack)track;

            e.ValueSize = stream.ReadInt32();
            int propertyNameLength = stream.ReadInt32();
            int propertyTypeNameLength = stream.ReadInt32();

            var propertyName = stream.ReadBytes(propertyNameLength);
            e.MemberName = Encoding.UTF8.GetString(propertyName, 0, propertyNameLength);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");

            var propertyTypeName = stream.ReadBytes(propertyTypeNameLength);
            e.MemberTypeName = Encoding.UTF8.GetString(propertyTypeName, 0, propertyTypeNameLength);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");

            var propertyType = TypeUtils.GetType(e.MemberTypeName);
            if (!propertyType)
            {
                if (!string.IsNullOrEmpty(e.MemberTypeName))
                    Editor.LogError("Cannot load track " + e.MemberName + " of type " + e.MemberTypeName + ". Failed to find the value type information.");
                return;
            }
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (ObjectPropertyTrack)track;

            var propertyName = e.MemberName ?? string.Empty;
            var propertyNameData = Encoding.UTF8.GetBytes(propertyName);
            if (propertyNameData.Length != propertyName.Length)
                throw new Exception(string.Format("The object member name bytes data has different size as UTF8 bytes. Type {0}.", propertyName));

            var propertyTypeName = e.MemberTypeName ?? string.Empty;
            var propertyTypeNameData = Encoding.UTF8.GetBytes(propertyTypeName);
            if (propertyTypeNameData.Length != propertyTypeName.Length)
                throw new Exception(string.Format("The object member typename bytes data has different size as UTF8 bytes. Type {0}.", propertyTypeName));

            stream.Write(e.ValueSize);
            stream.Write(propertyNameData.Length);
            stream.Write(propertyTypeNameData.Length);

            stream.Write(propertyNameData);
            stream.Write('\0');

            stream.Write(propertyTypeNameData);
            stream.Write('\0');
        }

        private Button _addButton;

        /// <inheritdoc />
        public ObjectPropertyTrack(ref TrackCreateOptions options)
        : base(ref options, false, false, true)
        {
            // Add track button
            const float buttonSize = 14;
            _addButton = new Button
            {
                Text = "+",
                TooltipText = "Add sub-tracks",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                Offsets = new Margin(-buttonSize - 2 + _muteCheckbox.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                IsScrollable = false,
                Parent = this
            };
            _addButton.Clicked += OnAddButtonClicked;
        }

        /// <inheritdoc />
        public object Object
        {
            get
            {
                TryGetValue(out var value);
                return value;
            }
        }

        private void OnAddButtonClicked()
        {
            var menu = new ContextMenu.ContextMenu();

            var obj = Object;
            if (obj == null)
            {
                menu.AddButton("Missing object");
                return;
            }

            var type = obj.GetType();
            if (ObjectTrack.AddProperties(this, menu, type) != 0)
                menu.AddSeparator();
            ObjectTrack.AddEvents(this, menu, type);

            menu.Show(_addButton.Parent, _addButton.BottomLeft);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _addButton = null;

            base.OnDestroy();
        }
    }
}
