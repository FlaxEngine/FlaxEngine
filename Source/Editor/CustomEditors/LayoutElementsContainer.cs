// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Represents a container control for <see cref="LayoutElement"/>. Can contain child elements.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    [HideInEditor]
    public abstract class LayoutElementsContainer : LayoutElement
    {
        /// <summary>
        /// Helper flag that is set to true if this container is in root presenter area, otherwise it's one of child groups.
        /// It's used to collapse all child groups and open the root ones by auto.
        /// </summary>
        internal bool isRootGroup = true;

        /// <summary>
        /// Parent container who created this one.
        /// </summary>
        internal LayoutElementsContainer _parent;

        /// <summary>
        /// The children.
        /// </summary>
        public readonly List<LayoutElement> Children = new List<LayoutElement>();

        /// <summary>
        /// The child custom editors.
        /// </summary>
        public readonly List<CustomEditor> Editors = new List<CustomEditor>();

        /// <summary>
        /// Gets the control represented by this element.
        /// </summary>
        public abstract ContainerControl ContainerControl { get; }

        /// <summary>
        /// Gets the Custom Editors layout presenter.
        /// </summary>
        internal CustomEditorPresenter Presenter
        {
            get
            {
                CustomEditorPresenter result;
                var container = this;
                do
                {
                    result = container as CustomEditorPresenter;
                    container = container._parent;
                } while (container != null);
                return result;
            }
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="linkedEditor">The custom editor to be linked for a group. Used to provide more utility functions for a drop panel UI via context menu.</param>
        /// <param name="useTransparentHeader">True if use drop down icon and transparent group header, otherwise use normal style.</param>
        /// <returns>The created element.</returns>
        public GroupElement Group(string title, CustomEditor linkedEditor, bool useTransparentHeader = false)
        {
            var element = Group(title, useTransparentHeader);
            element.Panel.Tag = linkedEditor;
            element.Panel.MouseButtonRightClicked += (panel, location) => OnGroupPanelMouseButtonRightClicked(element, panel, location);
            return element;
        }

        private void OnGroupPanelMouseButtonRightClicked(GroupElement element, DropPanel groupPanel, Float2 location)
        {
            var linkedEditor = (CustomEditor)groupPanel.Tag;
            var menu = new ContextMenu();

            var revertToPrefab = menu.AddButton("Revert to Prefab", linkedEditor.RevertToReferenceValue);
            revertToPrefab.Enabled = linkedEditor.CanRevertReferenceValue;
            var resetToDefault = menu.AddButton("Reset to default", linkedEditor.RevertToDefaultValue);
            resetToDefault.Enabled = linkedEditor.CanRevertDefaultValue;
            menu.AddSeparator();
            menu.AddButton("Copy", linkedEditor.Copy);
            var paste = menu.AddButton("Paste", linkedEditor.Paste);
            paste.Enabled = linkedEditor.CanPaste;
            element.SetupContextMenu?.Invoke(menu, groupPanel);

            menu.Show(groupPanel, location);
        }

        internal string GetLayoutCachePath(string name)
        {
            // Build group identifier (made of path from group titles)
            var expandPath = name;
            var container = this;
            while (container != null && !(container is CustomEditorPresenter))
            {
                if (container.ContainerControl is DropPanel dropPanel)
                    expandPath = dropPanel.HeaderText + "/" + expandPath;
                container = container._parent;
            }
            return expandPath;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="useTransparentHeader">True if use drop down icon and transparent group header, otherwise use normal style.</param>
        /// <returns>The created element.</returns>
        public GroupElement Group(string title, bool useTransparentHeader = false)
        {
            var element = new GroupElement();
            var presenter = Presenter;
            var isSubGroup = !isRootGroup;
            if (isSubGroup)
                element.Panel.Close();
            if (presenter != null && (presenter.Features & FeatureFlags.CacheExpandedGroups) != 0)
            {
                // Build group identifier (made of path from group titles)
                var expandPath = GetLayoutCachePath(title);

                // Caching/restoring expanded groups (non-root groups cache expanded state so invert boolean expression)
                if (Editor.Instance.ProjectCache.IsGroupToggled(expandPath) ^ isSubGroup)
                    element.Panel.Close();
                else
                    element.Panel.Open();
                element.Panel.IsClosedChanged += panel => Editor.Instance.ProjectCache.SetGroupToggle(expandPath, panel.IsClosed ^ isSubGroup);
            }
            element.isRootGroup = false;
            element._parent = this;
            element.Panel.HeaderText = title;
            if (useTransparentHeader)
            {
                element.Panel.EnableDropDownIcon = true;
                element.Panel.EnableContainmentLines = false;
                element.Panel.HeaderColor = element.Panel.HeaderColorMouseOver = Color.Transparent;
            }
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new horizontal panel element.
        /// </summary>
        /// <returns>The created element.</returns>
        public HorizontalPanelElement HorizontalPanel()
        {
            var element = new HorizontalPanelElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new vertical panel element.
        /// </summary>
        /// <returns>The created element.</returns>
        public VerticalPanelElement VerticalPanel()
        {
            var element = new VerticalPanelElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new button element.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="tooltip">The tooltip text.</param>
        /// <returns>The created element.</returns>
        public ButtonElement Button(string text, string tooltip = null)
        {
            var element = new ButtonElement();
            element.Button.Text = text;
            element.Button.TooltipText = tooltip;
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new button element with custom color.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="color">The color.</param>
        /// <param name="tooltip">The tooltip text.</param>
        /// <returns>The created element.</returns>
        public ButtonElement Button(string text, Color color, string tooltip = null)
        {
            ButtonElement element = new ButtonElement();
            element.Button.Text = text;
            element.Button.TooltipText = tooltip;
            element.Button.SetColors(color);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new custom element.
        /// </summary>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public CustomElement<T> Custom<T>()
        where T : Control, new()
        {
            var element = new CustomElement<T>();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new custom element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomElement<T> Custom<T>(string name, string tooltip = null)
        where T : Control, new()
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Custom<T>();
        }

        /// <summary>
        /// Adds new custom elements container.
        /// </summary>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public CustomElementsContainer<T> CustomContainer<T>()
        where T : ContainerControl, new()
        {
            var element = new CustomElementsContainer<T>();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new custom elements container with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomElementsContainer<T> CustomContainer<T>(string name, string tooltip = null)
        where T : ContainerControl, new()
        {
            var property = AddPropertyItem(name);
            return property.CustomContainer<T>();
        }

        /// <summary>
        /// Adds new space.
        /// </summary>
        /// <param name="height">The space height.</param>
        /// <returns>The created element.</returns>
        public SpaceElement Space(float height)
        {
            var element = new SpaceElement();
            element.Init(height);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds sprite image to the layout.
        /// </summary>
        /// <param name="sprite">The sprite.</param>
        /// <returns>The created element.</returns>
        public ImageElement Image(SpriteHandle sprite)
        {
            var element = new ImageElement();
            element.Image.Brush = new SpriteBrush(sprite);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds texture image to the layout.
        /// </summary>
        /// <param name="texture">The texture.</param>
        /// <returns>The created element.</returns>
        public ImageElement Image(Texture texture)
        {
            var element = new ImageElement();
            element.Image.Brush = new TextureBrush(texture);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds GPU texture image to the layout.
        /// </summary>
        /// <param name="texture">The GPU texture.</param>
        /// <returns>The created element.</returns>
        public ImageElement Image(GPUTexture texture)
        {
            var element = new ImageElement();
            element.Image.Brush = new GPUTextureBrush(texture);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new header control.
        /// </summary>
        /// <param name="text">The header text.</param>
        /// <returns>The created element.</returns>
        public LabelElement Header(string text)
        {
            var element = Label(text);
            element.Label.Font = new FontReference(Style.Current.FontLarge);
            return element;
        }

        internal LabelElement Header(HeaderAttribute header)
        {
            var element = Header(header.Text);
            if (header.FontSize > 0)
                element.Label.Font = new FontReference(element.Label.Font.Font, header.FontSize);
            if (header.Color > 0)
                element.Label.TextColor = Color.FromRGB(header.Color);
            var size = element.Label.Font.GetFont().MeasureText(header.Text);
            element.Label.Height = size.Y;
            return element;
        }

        /// <summary>
        /// Adds new text box element.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <returns>The created element.</returns>
        public TextBoxElement TextBox(bool isMultiline = false)
        {
            var element = new TextBoxElement(isMultiline);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new check box element.
        /// </summary>
        /// <returns>The created element.</returns>
        public CheckBoxElement Checkbox()
        {
            var element = new CheckBoxElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new check box element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CheckBoxElement Checkbox(string name, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Checkbox();
        }

        /// <summary>
        /// Adds new tree element.
        /// </summary>
        /// <returns>The created element.</returns>
        public TreeElement Tree()
        {
            var element = new TreeElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="horizontalAlignment">The label text horizontal alignment.</param>
        /// <returns>The created element.</returns>
        public LabelElement Label(string text, TextAlignment horizontalAlignment = TextAlignment.Near)
        {
            var element = new LabelElement();
            element.Label.Text = text;
            element.Label.HorizontalAlignment = horizontalAlignment;
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public LabelElement Label(string name, string text, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Label(text);
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="horizontalAlignment">The label text horizontal alignment.</param>
        /// <returns>The created element.</returns>
        public CustomElement<ClickableLabel> ClickableLabel(string text, TextAlignment horizontalAlignment = TextAlignment.Near)
        {
            var element = new CustomElement<ClickableLabel>();
            element.CustomControl.Height = 18.0f;
            element.CustomControl.Text = text;
            element.CustomControl.HorizontalAlignment = horizontalAlignment;
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomElement<ClickableLabel> ClickableLabel(string name, string text, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.ClickableLabel(text);
        }

        /// <summary>
        /// Adds new float value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public FloatValueElement FloatValue()
        {
            var element = new FloatValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new float value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public FloatValueElement FloatValue(string name, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.FloatValue();
        }

        /// <summary>
        /// Adds new double value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public DoubleValueElement DoubleValue()
        {
            var element = new DoubleValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new double value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public DoubleValueElement DoubleValue(string name, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.DoubleValue();
        }

        /// <summary>
        /// Adds new slider element.
        /// </summary>
        /// <returns>The created element.</returns>
        public SliderElement Slider()
        {
            var element = new SliderElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new slider element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public SliderElement Slider(string name, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Slider();
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public SignedIntegerValueElement SignedIntegerValue()
        {
            var element = new SignedIntegerValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public UnsignedIntegerValueElement UnsignedIntegerValue()
        {
            var element = new UnsignedIntegerValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new integer value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public IntegerValueElement IntegerValue()
        {
            var element = new IntegerValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new integer value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public IntegerValueElement IntegerValue(string name, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.IntegerValue();
        }

        /// <summary>
        /// Adds new combobox element.
        /// </summary>
        /// <returns>The created element.</returns>
        public ComboBoxElement ComboBox()
        {
            var element = new ComboBoxElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new combobox element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public ComboBoxElement ComboBox(string name, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.ComboBox();
        }

        /// <summary>
        /// Adds new enum value element.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="formatMode">The formatting mode.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate = null, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode.Default)
        {
            var element = new EnumElement(type, customBuildEntriesDelegate, formatMode);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="formatMode">The formatting mode.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate = null, string tooltip = null, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode.Default)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Enum(type, customBuildEntriesDelegate, formatMode);
        }

        /// <summary>
        /// Adds object(s) editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(ValueContainer values, CustomEditor overrideEditor = null)
        {
            if (values == null)
                throw new ArgumentNullException();

            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);

            OnAddEditor(editor);
            editor.Initialize(CustomEditor.CurrentCustomEditor.Presenter, this, values);

            return editor;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(string name, ValueContainer values, CustomEditor overrideEditor = null, string tooltip = null)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Object(values, overrideEditor);
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor = null, string tooltip = null)
        {
            var property = AddPropertyItem(label, tooltip);
            return property.Object(values, overrideEditor);
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor = null, string tooltip = null)
        {
            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            var style = editor.Style;

            if (style == DisplayStyle.InlineIntoParent || name == EditorDisplayAttribute.InlineStyle)
            {
                return Object(values, editor);
            }

            if (style == DisplayStyle.Group)
            {
                var group = Group(name, editor, true);
                group.Panel.TooltipText = tooltip;
                return group.Object(values, editor);
            }

            var property = AddPropertyItem(name, tooltip);
            int start = property.Properties.Children.Count;
            var result = property.Object(values, editor);

            // Special case when properties list is nested into another properties list (eg. array of structures or LocalizedString editor)
            if (this is PropertiesListElement thisPropertiesList &&
                editor.ParentEditor != null &&
                editor.ParentEditor.LinkedLabel != null &&
                editor.ParentEditor.LinkedLabel.FirstChildControlContainer == null)
            {
                editor.ParentEditor.LinkedLabel.FirstChildControlIndex = start;
                editor.ParentEditor.LinkedLabel.FirstChildControlContainer = property.Properties;
            }

            return result;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor = null, string tooltip = null)
        {
            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            var style = editor.Style;

            if (style == DisplayStyle.InlineIntoParent)
            {
                return Object(values, editor);
            }

            if (style == DisplayStyle.Group)
            {
                var group = Group(label.Text, editor, true);
                group.Panel.TooltipText = tooltip;
                return group.Object(values, editor);
            }

            var property = AddPropertyItem(label, tooltip);
            return property.Object(values, editor);
        }

        private PropertiesListElement AddPropertyItem()
        {
            // Try to reuse previous control
            PropertiesListElement element;
            if (Children.Count > 0 && Children[Children.Count - 1] is PropertiesListElement propertiesListElement)
            {
                element = propertiesListElement;
            }
            else
            {
                element = new PropertiesListElement();
                OnAddElement(element);
            }
            return element;
        }

        /// <summary>
        /// Adds the <see cref="PropertiesListElement"/> to the current layout or reuses the previous one. Used to inject properties.
        /// </summary>
        /// <param name="name">The property label name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The element.</returns>
        public PropertiesListElement AddPropertyItem(string name, string tooltip = null)
        {
            var element = AddPropertyItem();
            element.OnAddProperty(name, tooltip);
            return element;
        }

        /// <summary>
        /// Adds the <see cref="PropertiesListElement"/> to the current layout or reuses the previous one. Used to inject properties.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The element.</returns>
        public PropertiesListElement AddPropertyItem(PropertyNameLabel label, string tooltip = null)
        {
            if (label == null)
                throw new ArgumentNullException();

            var element = AddPropertyItem();
            element.OnAddProperty(label, tooltip);
            return element;
        }

        /// <summary>
        /// Adds custom element to the layout.
        /// </summary>
        /// <param name="element">The element.</param>
        public void AddElement(LayoutElement element)
        {
            if (element == null)
                throw new ArgumentNullException();
            OnAddElement(element);
        }

        /// <summary>
        /// Called when element is added to the layout.
        /// </summary>
        /// <param name="element">The element.</param>
        protected virtual void OnAddElement(LayoutElement element)
        {
            element.Control.Parent = ContainerControl;
            Children.Add(element);
        }

        /// <summary>
        /// Called when editor is added.
        /// </summary>
        /// <param name="editor">The editor.</param>
        protected virtual void OnAddEditor(CustomEditor editor)
        {
            // This could be passed by the calling code but it's easier to hide it from the user
            // Note: we need that custom editor to link generated editor into the parent
            var customEditor = CustomEditor.CurrentCustomEditor;
            Assert.IsNotNull(customEditor);
            customEditor.OnChildCreated(editor);
            Editors.Add(editor);
        }

        /// <summary>
        /// Clears the layout.
        /// </summary>
        public virtual void ClearLayout()
        {
            Children.Clear();
            Editors.Clear();
        }

        /// <inheritdoc />
        public override Control Control => ContainerControl;
    }
}
