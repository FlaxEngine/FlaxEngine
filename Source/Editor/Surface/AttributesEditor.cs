// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
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
        private Proxy _proxy;
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
        /// <param name="attributeTypes">The allowed attribute types to use.</param>
        public AttributesEditor(Attribute[] attributes, IList<Type> attributeTypes)
        {
            // Context menu dimensions
            const float width = 375.0f;
            const float height = 370.0f;
            Size = new Float2(width, height);

            // Title
            var title = new Label(2, 2, width - 4, 23.0f)
            {
                Font = new FontReference(Style.Current.FontLarge),
                Text = "Edit attributes",
                Parent = this
            };

            // Ok and Cancel Buttons
            float buttonsWidth = (width - 12.0f) * 0.5f;
            float buttonsHeight = 20.0f;
            var okButton = new Button(4.0f, Bottom - 4.0f - buttonsHeight, buttonsWidth, buttonsHeight)
            {
                Text = "Ok",
                Parent = this
            };
            okButton.Clicked += OnOkButtonClicked;
            var cancelButton = new Button(okButton.Right + 4.0f, okButton.Y, buttonsWidth, buttonsHeight)
            {
                Text = "Cancel",
                Parent = this
            };
            cancelButton.Clicked += Hide;

            // Actual panel used to display attributes
            var panel1 = new Panel(ScrollBars.Vertical)
            {
                Bounds = new Rectangle(0, title.Bottom + 4.0f, width, height - buttonsHeight - title.Height - 14.0f),
                Parent = this
            };
            var editor = new CustomEditorPresenter(null);
            editor.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            editor.Panel.IsScrollable = true;
            editor.Panel.Parent = panel1;
            editor.Panel.Tag = attributeTypes;
            _presenter = editor;

            // Cache 'previous' state to check if attributes were edited after operation
            _oldData = SurfaceMeta.GetAttributesData(attributes);

            _proxy = new Proxy
            {
                Value = attributes,
            };
            editor.Select(_proxy);

            _presenter.Modified += OnPresenterModified;
            OnPresenterModified();
        }

        private void OnPresenterModified()
        {
            if (_proxy.Value.Length == 0)
            {
                var label = _presenter.Label("No attributes.\nPress the \"+\" button to add a new one and then select an attribute type using the \"Type\" dropdown.", TextAlignment.Center);
                label.Label.Wrapping = TextWrapping.WrapWords;
                label.Control.Height = 35f;
                label.Label.Margin = new Margin(10f);
                label.Label.TextColor = label.Label.TextColorHighlighted = Style.Current.ForegroundGrey;
            }
        }

        private void OnOkButtonClicked()
        {
            var newValue = ((Proxy)_presenter.Selection[0]).Value;
            newValue = newValue.Where(v => v != null).ToArray();

            var newData = SurfaceMeta.GetAttributesData(newValue);
            if (!_oldData.SequenceEqual(newData))
                Edited?.Invoke(newValue);

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
            _proxy = null;
            Edited = null;
            _presenter.Modified -= OnPresenterModified;

            base.OnDestroy();
        }
    }
}
