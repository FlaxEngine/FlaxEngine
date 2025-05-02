// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using System.Runtime.Serialization.Formatters.Binary;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    class AttributesEditor : ContextMenuBase
    {
        private CustomEditorPresenter _presenter;
        private byte[] _oldData;

        private class Proxy
        {
            [Collection(OverrideEditorTypeName = "FlaxEditor.Surface.AttributesEditor+ValuesEditor", Spacing = 10)]
            [EditorDisplay(null, EditorDisplayAttribute.InlineStyle)]
            public Attribute[] Value;
        }

        public sealed class ValuesEditor : ObjectSwitcherEditor
        {
            private OptionType[] _options;

            /// <inheritdoc />
            protected override OptionType[] Options => _options;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                var options = (IList<Type>)Presenter.Panel.Tag;
                _options = new OptionType[options.Count];
                for (int i = 0; i < options.Count; i++)
                {
                    var type = options[i];
                    _options[i] = new OptionType(Utilities.Utils.GetPropertyNameUI(type.Name), type, Creator);
                }

                base.Initialize(layout);
            }

            /// <inheritdoc />
            protected override void Deinitialize()
            {
                base.Deinitialize();

                _options = null;
            }

            private object Creator(Type type)
            {
                var ctor = type.GetConstructor(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public, null, Utils.GetEmptyArray<Type>(), null);
                return ctor.Invoke(Utils.GetEmptyArray<object>());
            }
        }

        /// <summary>
        /// The action fired when parameters editing finishes and value gets accepted to be set.
        /// </summary>
        public Action<Attribute[]> Edited;

        /// <summary>
        /// Initializes a new instance of the <see cref="AttributesEditor"/> class.
        /// </summary>
        /// <param name="attributes">The attributes list to edit.</param>
        /// <param name="attributeType">The allowed attribute types to use.</param>
        public AttributesEditor(Attribute[] attributes, IList<Type> attributeType)
        {
            // Context menu dimensions
            const float width = 340.0f;
            const float height = 370.0f;
            Size = new Float2(width, height);

            // Title
            var title = new Label(2, 2, width - 4, 23.0f)
            {
                Font = new FontReference(Style.Current.FontLarge),
                Text = "Edit attributes",
                Parent = this
            };

            // Buttons
            float buttonsWidth = (width - 16.0f) * 0.5f;
            float buttonsHeight = 20.0f;
            var cancelButton = new Button(4.0f, title.Bottom + 4.0f, buttonsWidth, buttonsHeight)
            {
                Text = "Cancel",
                Parent = this
            };
            cancelButton.Clicked += Hide;
            var okButton = new Button(cancelButton.Right + 4.0f, cancelButton.Y, buttonsWidth, buttonsHeight)
            {
                Text = "OK",
                Parent = this
            };
            okButton.Clicked += OnOkButtonClicked;

            // Actual panel
            var panel1 = new Panel(ScrollBars.Vertical)
            {
                Bounds = new Rectangle(0, okButton.Bottom + 4.0f, width, height - okButton.Bottom - 2.0f),
                Parent = this
            };
            var editor = new CustomEditorPresenter(null);
            editor.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            editor.Panel.IsScrollable = true;
            editor.Panel.Parent = panel1;
            editor.Panel.Tag = attributeType;
            _presenter = editor;

            // Cache 'previous' state to check if attributes were edited after operation
            _oldData = SurfaceMeta.GetAttributesData(attributes);

            editor.Select(new Proxy
            {
                Value = attributes,
            });
        }

        private void OnOkButtonClicked()
        {
            var newValue = ((Proxy)_presenter.Selection[0]).Value;
            for (int i = 0; i < newValue.Length; i++)
            {
                if (newValue[i] == null)
                {
                    MessageBox.Show("One of the attributes is null. Please set it to the valid object.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }

            var newData = SurfaceMeta.GetAttributesData(newValue);
            if (!_oldData.SequenceEqual(newData))
            {
                Edited?.Invoke(newValue);
            }

            Hide();
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            Focus();

            base.OnShow();
        }

        /// <inheritdoc />
        public override void Hide()
        {
            if (!Visible)
                return;

            Focus(null);

            base.Hide();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Escape)
            {
                Hide();
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _presenter = null;
            _oldData = null;
            Edited = null;

            base.OnDestroy();
        }
    }
}
